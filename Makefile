<<<<<<< HEAD
PREFIX  := /usr
LIBDIR  := $(PREFIX)/lib
DESTDIR :=
=======
PREFIX ?= /usr
libdir ?= $(PREFIX)/lib
>>>>>>> 1eb55ff21d3e428f1708d1fc1354bb1b0d8a0d6b

all:
	$(MAKE) -C arpeggiator/source
	$(MAKE) -C midi-pattern/source

install:
<<<<<<< HEAD
	install -d $(DESTDIR)$(LIBDIR)/lv2/
	cp -r arpeggiator/source/*.lv2/  $(DESTDIR)$(LIBDIR)/lv2/
	cp -r midi-pattern/source/*.lv2/ $(DESTDIR)$(LIBDIR)/lv2/
=======
	cp -r arpeggiator/source/bg-arpeggiator.lv2/* $(DESTDIR)$(libdir)/lv2/bg-arpeggiator.lv2
	cp -r midi-pattern/source/bg-midi-pattern.lv2/* $(DESTDIR)$(libdir)/lv2/bg-midi-pattern.lv2

>>>>>>> 1eb55ff21d3e428f1708d1fc1354bb1b0d8a0d6b
clean:
	$(MAKE) clean -C arpeggiator/source
	$(MAKE) clean -C midi-pattern/source
