SUBDIRS = lib/collections src

SUBCLEAN = $(addsuffix .cln, $(SUBDIRS))

.PHONY: clean install tests subdirs $(SUBDIRS) $(SUBCLEAN) $(SUBTESTS)

subdirs : $(SUBDIRS)

src : lib/collections

$(SUBDIRS) :
	$(MAKE) -C $@ --no-print-directory

clean : $(SUBCLEAN)

$(SUBCLEAN) :
	$(MAKE) clean -C $(basename $@) --no-print-directory

install : src
	$(MAKE) install -C src --no-print-directory

tests : src
	src/build/lilith test/test_builtins.llth test/test_stdlib.llth
