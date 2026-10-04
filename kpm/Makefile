ifndef KP_DIR
KP_DIR = ../../KernelPatch
endif

TARGET_COMPILE ?= aarch64-none-elf-
CC := $(TARGET_COMPILE)gcc
LD := $(TARGET_COMPILE)ld
STRIP := $(TARGET_COMPILE)strip

CFLAGS := -O2 -fno-PIC -fno-asynchronous-unwind-tables -fno-stack-protector -ffreestanding
INCLUDE_DIRS := . include patch/include linux/include linux/arch/arm64/include linux/tools/arch/arm64/include
INCLUDE_FLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(KP_DIR)/kernel/$(dir))

all: universal-samsung-spoof.kpm

check-kp:
	@test -f $(KP_DIR)/kernel/include/kpmodule.h
	@test -f $(KP_DIR)/kernel/include/compiler.h

universal-samsung-spoof.kpm: main.o
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -r -o $@ $^
	$(STRIP) -g $@

main.o: main.c
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c -o $@ $<

clean:
	rm -f *.o *.kpm
