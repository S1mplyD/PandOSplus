# Cross toolchain variables
# If these are not in your path, you can make them absolute.
XT_PRG_PREFIX = mipsel-linux-gnu-
#XT_PRG_PREFIX = ~/x-tools/mipsel-unknown-linux-gnu/bin/mipsel-unknown-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld

ifneq ($(wildcard /usr/bin/umps3),)
        UMPS3_DIR_PREFIX = /usr
else
        UMPS3_DIR_PREFIX = /usr/local
endif

# uMPS3-related paths
UMPS3_DIR = $(UMPS3_DIR_PREFIX)/include/umps3
UMPS3_DIR_2 = $(UMPS3_DIR_PREFIX)/include
UMPS3_DATA_DIR = $(UMPS3_DIR_PREFIX)/share/umps3/

#include path
INCLUDE_DIR = ./include

# Compiler options
CFLAGS_LANG = -ffreestanding
CFLAGS_MIPS = -mips1 -mabi=32 -mno-gpopt -G 0 -mno-abicalls -fno-pic -mfp32
CFLAGS = $(CFLAGS_LANG) $(CFLAGS_MIPS) -I${UMPS3_DIR} -I$(UMPS3_DIR_2) -I$(INCLUDE_DIR) -Wall -O0

# Linker options
LDFLAGS = -G 0  -T $(UMPS3_DATA_DIR)/umpscore.ldscript

# Add the location of crt*.S to the search path
VPATH = $(UMPS3_DATA_DIR)

.PHONY : all clean

all : kernel.core.umps

kernel.core.umps : kernel
	umps3-elf2umps -k $<

kernel : src/pcb.o src/asl.o src/main.o src/exception.o src/syscalls.o src/interrupts.o src/scheduler.o src/klog.o tests/p2test.o src/utility.o crtso.o libumps.o 
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	-rm -f *.o src/*.o kernel kernel.*.umps tests/*.o

# Pattern rule for assembly modules
%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<
