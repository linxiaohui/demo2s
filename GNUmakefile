TOP = .

# Cross-compiler osclass toolchain
CC	:= gcc-3.3 -pipe -m32
GCC_LIB := $(shell $(CC) -print-libgcc-file-name)
AS	:= as
AR	:= ar
LD	:= /usr/i586-suse-linux/bin/ld 
OBJCOPY	:= objcopy

# Native commands
NCC	:= gcc $(CC_VER) -pipe
TAR	:= gtar
PERL	:= perl

# Compiler flags
# Note that -O2 is required for the boot loader to fit within 512 bytes;
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
CFLAGS	:= $(CFLAGS) -O2 -fno-builtin -I$(TOP) -MD -MP -Wall

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=
CLEAN_FILES := .deps bochs.log
CLEAN_PATS := *.o *.d


# Make sure that 'all' is the first target
all:


# Include Makefrags for subdirectories
include kern/Makefrag
include boot/Makefrag
-include user/Makefrag
-include tools/mkimg/Makefrag


# Eliminate default suffix rules
.SUFFIXES:
                                                                                

# Rules for building regular object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<


# For embedding one program in another
%.b.c: %.b
	rm -f $@
	$(TOP)/tools/bintoc/bintoc $< $*_bin > $@~ && $(MV) -f $@~ $@
%.b.s: %.b
	rm -f $@
	$(TOP)/tools/bintoc/bintoc -S $< $*_bin > $@~ && $(MV) -f $@~ $@




# For cleaning the source tree
clean:
	rm -rf $(CLEAN_FILES) $(foreach dir,$(OBJDIRS), \
				$(addprefix $(dir)/,$(CLEAN_PATS)))


# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(dir)/*.d))
	@$(PERL) mergedep.pl $@ $^

-include .deps

.phony: lab%.tar.gz

