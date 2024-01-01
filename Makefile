.PHONY: all clean install

SUBDIR = release

all clean install uninstall:
	@mkdir -p $(SUBDIR)
	@cp -f scripts/Makefile $(SUBDIR)/
	$(MAKE) -C $(SUBDIR) $@
