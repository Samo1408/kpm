# KPM build targeting the KPatch Next source tree.
# KP_DIR must point to the root containing kernel/include/kpmodule.h.
ifndef TARGET_COMPILE
TARGET_COMPILE ?= aarch64-none-elf-
endif

KP_DIR ?= ../third_party/KPatch-Next-EXP
CC := $(TARGET_COMPILE)gcc
LD := $(TARGET_COMPILE)ld
STRIP := $(TARGET_COMPILE)strip

CFLAGS := -O2 -fno-PIC -fno-asynchronous-unwind-tables -fno-stack-protector \
          -ffreestanding -fno-builtin -fno-common

# KPatch Next stores its Linux header subset under kernel/linux/; the extra
# KernelPatch paths keep the makefile usable with a compatible upstream tree.
INCLUDE_DIRS := \
  kernel/include \
  kernel/patch/include \
  kernel/patch/include/uapi \
  kernel/linux/include \
  kernel/linux/arch/arm64/include \
  kernel/linux/arch/arm64/include/generated \
  kernel/linux/include/generated \
  kernel/linux/tools/arch/arm64/include \
  kernel/arch/arm64/include \
  kernel/arch/arm64/include/generated \
  kernel/include/generated \
  kernel/tools/arch/arm64/include
INCLUDE_FLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(KP_DIR)/$(dir))

.PHONY: all clean check-kp
all: check-kp universal-samsung-spoof.kpm

check-kp:
	@test -f "$(KP_DIR)/kernel/include/kpmodule.h" || { echo "Missing KPM API: $(KP_DIR)/kernel/include/kpmodule.h" >&2; exit 1; }
	@test -f "$(KP_DIR)/kernel/include/compiler.h" || { echo "Missing compiler.h in KPM source tree" >&2; exit 1; }
	@test -f "$(KP_DIR)/kernel/include/hook.h" || { echo "Missing hook.h in KPM source tree" >&2; exit 1; }

universal-samsung-spoof.kpm: main.o
	$(LD) -r -o $@ $^
	$(STRIP) -g $@

main.o: main.c
	$(CC) $(CFLAGS) $(INCLUDE_FLAGS) -c -o $@ $<

clean:
	rm -f *.o *.kpm
