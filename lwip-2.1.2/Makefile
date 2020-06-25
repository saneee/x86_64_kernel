# Default make target
.PHONY: all
all: out/liblwip.a

X64 = 1

BITS = 64
XFLAGS = -DDEBUG -std=gnu11 -m64 -DX64 -mcmodel=kernel -mtls-direct-seg-refs \
	-mno-red-zone  -fno-delete-null-pointer-checks  -fno-omit-frame-pointer \
	-fno-optimize-sibling-calls -fno-var-tracking-assignments -fconserve-stack \
	-fno-strict-overflow 	-Wno-pointer-sign  -falign-jumps=1 -falign-loops=1 \
	--param=allow-store-data-races=0   -fno-common  -mno-80387 \
	-mskip-rax-setup -mtune=generic \
	-fno-asynchronous-unwind-tables  -mno-mmx  -mno-3dnow \
	-mno-avx
LDFLAGS = -m elf_x86_64  -L ../libs/  -nodefaultlibs

FSGSBASE=$(shell cat /proc/cpuinfo|grep fsgsbase)
ifneq ($(strip $(FSGSBASE)),)
   XFLAGS+= -D__FSGSBASE__
endif
OUT = out

HOST_CC ?= gcc

OPT ?= -O2

LIBS_SOURCES = $(shell find src -name "*.c")
LIBS_OBJECTS = $(patsubst %.c, %.o, $(LIBS_SOURCES))

# Cross-compiling (e.g., on Mac OS X)
CROSS_COMPILE ?=

# If the makefile can't find QEMU, specify its path here
QEMU ?= qemu-system-x86_64

CC = $(CROSS_COMPILE)gcc  
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
cc-option = $(shell if $(CC) $(1) -S -o /dev/null -xc /dev/null \
	> /dev/null 2>&1; then echo "$(1)"; else echo "$(2)"; fi ;)

CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -Werror
CFLAGS += -g -Wall -MD -D__KERNEL__ 
CFLAGS += -ffreestanding -fno-common -nostdlib -I arch/x86_64/include -Iinclude \
	-Iinclude/api -Iinclude/api/x64 -Isrc/include -gdwarf-2 $(XFLAGS) $(OPT) 
#CFLAGS += $(call cc-option, -fno-stack-protector, "")
#CFLAGS += $(call cc-option, -fno-stack-protector-all, "")
ASFLAGS = -gdwarf-2 -Wa,-divide -D__ASSEMBLY__ -Iinclude -I arch/x86_64/include $(XFLAGS)
CC += $(CFLAGS)
$(OUT)/liblwip.a: $(LIBS_OBJECTS)
	ar -rs $(OUT)/liblwip.a $(LIBS_OBJECTS)
	/bin/cp $(OUT)/liblwip.a ../libs/ 
.o:
	 $(CC) $(CFLAGS) -c $< -o $@

.c.o:
	$(CC) $(MODULEC_FLAGS) -c $< -o $@
.S.o:
	$(CC) $(ASFLAGS)  -c -o $@ $<

clean: 
	rm -rf $(LIBS_OBJECTS)


