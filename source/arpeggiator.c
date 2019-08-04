#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

#ifndef DEBUG
#define DEBUG 0
#endif
#define debug_print(...) \
    ((void)((DEBUG) ? fprintf(stderr, __VA_ARGS__) : 0))


#ifndef M_PI
#    define M_PI 3.14159265
#endif
#define PLUGIN_URI "http://bramgiesen.com/arpeggiator"


// Struct for a 3 byte MIDI event
typedef struct {
    LV2_Atom_Event event;
    uint8_t        msg[3];
} LV2_Atom_MIDI;


typedef enum {
    MIDI_IN        = 0,
    MIDI_OUT       = 1,
    BPM_PORT       = 2,
    DIVISIONS_PORT = 3,
    SYNC_PORT      = 4,
    CONTROL_PORT   = 5
} PortIndex;


typedef struct {
    LV2_URID atom_Blank;
    LV2_URID atom_Float;
    LV2_URID atom_Object;
    LV2_URID atom_Path;
    LV2_URID atom_Resource;
    LV2_URID atom_Sequence;
    LV2_URID time_Position;
    LV2_URID time_barBeat;
    LV2_URID time_beatsPerMinute;
    LV2_URID time_speed;
} ClockURIs;

typedef struct {

    LV2_URID_Map*          map; // URID map feature
    LV2_Log_Log* 	       log;
    LV2_Log_Logger      logger; // Logger API
    ClockURIs             uris; // Cache of mapped URIDs

    // URIDs
    LV2_URID urid_midiEvent;

    const LV2_Atom_Sequence* MIDI_in;
    LV2_Atom_Sequence*       MIDI_out;

    float*    changeBpm;
    float*    changedDiv;
    int*   	  sync;
    float     divisions;
    double    samplerate;
    int       prevSync;
    LV2_Atom_Sequence* control;
    // Variables to keep track of the tempo information sent by the host
    float     bpm; // Beats per minute (tempo)
    uint32_t  pos;
    uint32_t  period;
    uint32_t  h_wavelength;

    bool      printed;
    float     speed; // Transport speed (usually 0=stop, 1=play)
    float     prevSpeed;
    float     beatInMeasure;

    float 	  elapsed_len; // Frames since the start of the last click
    uint32_t  wave_offset; // Current play offset in the wave

    // Envelope parameters
    uint32_t  attack_len;
    uint32_t  decay_len;
} Arpeggiator;



static LV2_Atom_MIDI
createMidiEvent(Arpeggiator* self, uint8_t status, uint8_t note, uint8_t velocity)
{
    LV2_Atom_MIDI msg;
    memset(&msg, 0, sizeof(LV2_Atom_MIDI));

    msg.event.body.size = 3;
    msg.event.body.type = self->urid_midiEvent;

    msg.msg[0] = status;
    msg.msg[1] = note;
    msg.msg[2] = velocity;

    return msg;
}



static void
connect_port(LV2_Handle instance,
        uint32_t   port,
        void*      data)
{
    Arpeggiator* self = (Arpeggiator*)instance;

    switch ((PortIndex)port) {
        case MIDI_IN:
            self->MIDI_in    = (const LV2_Atom_Sequence*)data;
            break;
        case MIDI_OUT:
            self->MIDI_out   = (LV2_Atom_Sequence*)data;
            break;
        case BPM_PORT:
            self->changeBpm = (float*)data;
            break;
        case DIVISIONS_PORT:
            self->changedDiv = (float*)data;
            break;
        case SYNC_PORT:
            self->sync = (int*)data;
            break;
        case CONTROL_PORT:
            self->control = (LV2_Atom_Sequence*)data;
            break;
    }
}

static void
activate(LV2_Handle instance)
{
    Arpeggiator* self = (Arpeggiator*)instance;

    self->bpm = *self->changeBpm;
    self->divisions =*self->changedDiv;
    self->pos = 0;
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
        double                    rate,
        const char*               bundle_path,
        const LV2_Feature* const* features)
{
    Arpeggiator* self = (Arpeggiator*)calloc(1, sizeof(Arpeggiator));
    if (!self)
    {
        return NULL;
    }

    for (uint32_t i=0; features[i]; ++i)
    {
        if (!strcmp (features[i]->URI, LV2_URID__map))
        {
            self->map = (LV2_URID_Map*)features[i]->data;
        }
        else if (!strcmp (features[i]->URI, LV2_LOG__log))
        {
            self->log = (LV2_Log_Log*)features[i]->data;
        }
    }

    lv2_log_logger_init (&self->logger, self->map, self->log);

    if (!self->map) {
        lv2_log_error (&self->logger, "StepSeq.lv2 error: Host does not support urid:map\n");
        free (self);
        return NULL;
    }


    // Map URIS
    ClockURIs* const    uris  = &self->uris;
    LV2_URID_Map* const map   = self->map;
    self->urid_midiEvent      = map->map(map->handle, LV2_MIDI__MidiEvent);
    uris->atom_Blank          = map->map(map->handle, LV2_ATOM__Blank);
    uris->atom_Float          = map->map(map->handle, LV2_ATOM__Float);
    uris->atom_Object         = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_Path           = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_Resource       = map->map(map->handle, LV2_ATOM__Resource);
    uris->atom_Sequence       = map->map(map->handle, LV2_ATOM__Sequence);
    uris->time_Position       = map->map(map->handle, LV2_TIME__Position);
    uris->time_barBeat        = map->map(map->handle, LV2_TIME__barBeat);
    uris->time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
    uris->time_speed          = map->map(map->handle, LV2_TIME__speed);

    debug_print("DEBUGING");
    self->samplerate = rate;
    self->prevSync   = 0; 
    self->beatInMeasure = 0;
    self->printed = false;
    self->prevSpeed = 0;
    return (LV2_Handle)self;
}



// Update the current position based on a host message.  This is called by
// run() when a time:Position is received.

static void
update_position(Arpeggiator* self, const LV2_Atom_Object* obj)
{
    const ClockURIs* uris = &self->uris;

    // Received new transport position/speed
    LV2_Atom *beat = NULL, *bpm = NULL, *speed = NULL;
    lv2_atom_object_get(obj,
            uris->time_barBeat, &beat,
            uris->time_beatsPerMinute, &bpm,
            uris->time_speed, &speed,
            NULL);
    if (bpm && bpm->type == uris->atom_Float)
    {
        // Tempo changed, update BPM
        self->bpm = ((LV2_Atom_Float*)bpm)->body;
    }
    if (speed && speed->type == uris->atom_Float)
    {
        // Speed changed, e.g. 0 (stop) to 1 (play)
        self->speed = ((LV2_Atom_Float*)speed)->body;
    }
    if (beat && beat->type == uris->atom_Float)
    {
        // Received a beat position, synchronise
        // This hard sync may cause clicks, a real plugin would be more graceful
        const float frames_per_beat = (self->samplerate * (60.0f / (self->bpm * self->divisions)));
        const float bar_beats       = (((LV2_Atom_Float*)beat)->body * self->divisions);
        const float beat_beats      = bar_beats - floorf(bar_beats);
        self->beatInMeasure         = ((LV2_Atom_Float*)beat)->body; 
        self->elapsed_len           = beat_beats * frames_per_beat;
    }
}

static uint32_t 
resetPhase(Arpeggiator* self)
{
    uint32_t pos = (uint32_t)fmod(self->samplerate * (60.0f / self->bpm) * self->beatInMeasure, (self->samplerate * (60.0f / (self->bpm * (self->divisions / 2.0f)))));

    return pos;
}


static void
run(LV2_Handle instance, uint32_t n_samples)
{
    Arpeggiator* self = (Arpeggiator*)instance;

    const ClockURIs* uris = &self->uris;
    const LV2_Atom_Sequence* in = self->control;

    for (const LV2_Atom_Event* ev = lv2_atom_sequence_begin(&in->body);
            !lv2_atom_sequence_is_end(&in->body, in->atom.size, ev);
            ev = lv2_atom_sequence_next(ev)) {

        if (ev->body.type == uris->atom_Object ||
                ev->body.type == uris->atom_Blank) {
            const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == uris->time_Position) {
                update_position(self, obj);
            }
        }
    }


    const uint32_t out_capacity = self->MIDI_out->atom.size;

    // Write an empty Sequence header to the output
    lv2_atom_sequence_clear(self->MIDI_out);

    // Read incoming events
    LV2_ATOM_SEQUENCE_FOREACH(self->MIDI_in, ev)
    {
        if (ev->body.type == self->urid_midiEvent)
        {
            const uint8_t* const msg = (const uint8_t*)(ev + 1);

            const uint8_t channel = msg[0] & 0x0F;
            const uint8_t status  = msg[0] & 0xF0;

            switch (status)
            {
            case LV2_MIDI_MSG_NOTE_ON:
                //append notes to list
                break;
            case LV2_MIDI_MSG_NOTE_OFF:
                //remove notes from list
                break;
            default:
                break;
            }
        }
        lv2_atom_sequence_append_event(self->MIDI_out,
                out_capacity, ev);
    }

    for(uint32_t i = 0; i < n_samples; i ++) {
        //map bpm to host or to bpm parameter
        if (!*self->sync) {
            self->bpm = *self->changeBpm;
        } else {
            self->bpm = self->bpm;
        }
        //reset phase when playing starts or stops
        if (self->speed != self->prevSpeed) {
            self->pos = resetPhase(self);
            self->prevSpeed = self->speed;
        }
        //reset phase when sync is turned on
        if (*self->sync != self->prevSync) {
            self->pos = resetPhase(self);
            self->prevSync = *self->sync;
        }
        //reset phase when there is a new division
        if (self->divisions != *self->changedDiv) {
            self->divisions = *self->changedDiv;
            self->pos = resetPhase(self);
        }

        self->period = (uint32_t)(self->samplerate * (60.0f / (self->bpm * (self->divisions / 2.0f))));
        self->h_wavelength = (self->period/2.0f);

        if(self->pos >= self->period && i < n_samples) {
            self->pos = 0;
        } else {
            if(self->pos < self->h_wavelength) {
                //trigger MIDI message
            } else {
                //set gate
            }
        }
        self->pos += 1;
    }
}



static void
deactivate(LV2_Handle instance)
{
}

static void
cleanup(LV2_Handle instance)
{
    free(instance);
}

static const void*
extension_data(const char* uri)
{
    return NULL;
}

static const LV2_Descriptor descriptor = {
    PLUGIN_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};

LV2_SYMBOL_EXPORT
    const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
    switch (index) {
        case 0:  return &descriptor;
        default: return NULL;
    }
}

