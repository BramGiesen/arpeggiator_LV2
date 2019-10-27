all:
	$(MAKE) -C arpeggiator/source
	$(MAKE) -C midi-pattern/source

clean:
	$(MAKE) clean -C arpeggiator/source
	$(MAKE) clean -C midi-pattern/source
