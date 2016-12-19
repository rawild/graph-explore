# Default optimization level
O ?= 2

all: graph-explore

-include build/rules.mk

LIBS = -lpthread -lm

%.o: %.c $(BUILDSTAMP)
	$(call run,$(CC) $(CPPFLAGS) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -c,COMPILE,$<)

graph-explore: graph-explore.o connect.o parser.o timer.o murmur-hash.o
	$(call run,$(CC) $(CFLAGS) $(O) -o $@ $^ $(LDFLAGS) $(LIBS),LINK $@)

clean: clean-main
clean-main:
	$(call run,rm -f graph-explore *.o *~ *.bak core *.core,CLEAN)
	$(call run,rm -rf $(DEPSDIR) out *.dSYM)

distclean: clean

.PRECIOUS: %.o
.PHONY: all clean clean-main clean-hook distclean check check-% prepare-check