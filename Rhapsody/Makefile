SUBDIRS = mak-linux test

all: subdirs


subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n all || exit 1; done

clean:realclean

realclean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n realclean; done
