SUBDIRS = src

SUBCLEAN = $(addsuffix .cln, $(SUBDIRS))

.PHONY: clean install tests subdirs $(SUBDIRS) $(SUBCLEAN) $(SUBTESTS)

subdirs : $(SUBDIRS)

$(SUBDIRS) :
	$(MAKE) -C $@ --no-print-directory

test : src

clean : $(SUBCLEAN)

$(SUBCLEAN) :
	$(MAKE) clean -C $(basename $@) --no-print-directory

install : src
	$(MAKE) install -C src --no-print-directory

tests : test
	src/build/lilith test/test_builtins.llth test/test_stdlib.llth
