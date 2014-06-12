TOP = .

# Cross-compiler osclass toolchain
CC	:= gcc-3.3 -pipe -m32
GCC_LIB := $(shell $(CC) -print-libgcc-file-name)
AS	:= as
AR	:= ar
LD	:= /usr/i586-suse-linux/bin/ld 
OBJCOPY	:= objcopy
OBJDUMP	:= objdump

# Native commands
NCC	:= gcc $(CC_VER) -pipe
TAR	:= tar
PERL	:= perl
MV	:= mv

# Compiler flags
# Note that -O2 is required for the boot loader to fit within 512 bytes;
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
DEFS	:=
CFLAGS	:= $(CFLAGS) $(DEFS) -O2 -fno-builtin -I$(TOP) -MD -MP -Wall -ggdb

# Linker flags for user programs
ULDFLAGS := -Ttext 0x800020

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=
CLEAN_FILES := .deps bochs.log
CLEAN_PATS := *.o *.d *.asm *.lib *.b.c


# Make sure that 'all' is the first target
all: ${USERLIB}


# Include Makefrags for subdirectories
include user/Makefrag
include kern/Makefrag
include fs/Makefrag
include boot/Makefrag




# Eliminate default suffix rules
.SUFFIXES:
                                                                                

# Rules for building regular object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<



bochs: kern/bochs.img
	bochs -q

kernel.asm: kern/kernel
	$(OBJDUMP) -S --adjust-vma=0xf00ff000 kern/kernel >kernel.asm

# For cleaning the source tree
clean:
	rm -f fs/fs.img bochs.out
	rm -rf $(CLEAN_FILES) $(foreach dir,$(OBJDIRS), \
				$(addprefix $(dir)/,$(CLEAN_PATS)))

grade:
	@make clean >/dev/null 2>/dev/null
	make all
	sh grade.sh
	@make clean >/dev/null 2>/dev/null

# For test runs
run-%:
	@rm -f kern/init.o kern/bochs.img
	@make "DEFS=-DTEST=binary_user_$*_start -DTESTSIZE=binary_user_$*_size" kern/bochs.img fs/fs.img
	bochs -q
xrun-%:
	@rm -f kern/init.o kern/bochs.img
	@make "DEFS=-DTEST=binary_user_$*_start -DTESTSIZE=binary_user_$*_size" kern/bochs.img fs/fs.img
	bochs -q
# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(dir)/*.d))
	@$(PERL) mergedep.pl $@ $^

-include .deps

.phony: lab%.tar.gz

