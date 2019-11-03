all:
	$(MAKE) -C arpeggiator/source
	$(MAKE) -C midi-pattern/source

install:
	cp -r arpeggiator/source/bg-arpeggiator.lv2 /usr/lib/lv2/
	cp -r midi-pattern/source/bg-midi-pattern.lv2 /usr/lib/lv2/
clean:
	$(MAKE) clean -C arpeggiator/source
	$(MAKE) clean -C midi-pattern/source
