all:
	$(MAKE) -C arpeggiator/source
	$(MAKE) -C midi-pattern/source

clean:
	$(MAKE) clean -C arpeggiator/source
	$(MAKE) clean -C midi-pattern/source

install:
	mkdir bundles
	cp -r arpeggiator/source/arpeggiator.lv2 bundles/
	cp -r midi-pattern/source/midi-pattern.lv2 bundles/

