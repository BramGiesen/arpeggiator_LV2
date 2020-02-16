PREFIX  := /usr
LIBDIR  := $(PREFIX)/lib
DESTDIR :=

all:
	$(MAKE) -C arpeggiator/source
	$(MAKE) -C midi-pattern/source

install:
	install -d $(DESTDIR)$(LIBDIR)/lv2/
	cp -r arpeggiator/source/*.lv2/  $(DESTDIR)$(LIBDIR)/lv2/
	cp -r midi-pattern/source/*.lv2/ $(DESTDIR)$(LIBDIR)/lv2/

clean:
	$(MAKE) clean -C arpeggiator/source
	$(MAKE) clean -C midi-pattern/source
