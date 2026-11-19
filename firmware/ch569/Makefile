# rp569 CH569 firmware: USB3 composite (SD card MSC + RP2350-link CDC + debug CDC)
# Forked from CH569_Composite. Debug printf on UART0 (DEBUG=0 = Debug_UART0, PB6 TX).
#
# Toolchain: xPack riscv-none-elf-gcc (see Dockerfile). Override PREFIX for a
# differently-named toolchain, e.g. `make PREFIX=riscv64-unknown-elf-`.

# VARIANT (D-025 major = firmware variant): default = variant 0 (drive/serial modes, src/, version.yaml);
# VARIANT=stream = variant 1 (HSPI receiver -> USB bulk stream, src-stream/, version-stream.yaml, D-031).
VARIANT ?=
ifeq ($(VARIANT),stream)
TARGET  := rp569_ch569_stream
BUILD   := build-stream
VERSION_FILE := version-stream.yaml
else
TARGET  := rp569_ch569
BUILD   := build
VERSION_FILE := version.yaml
endif

PREFIX  ?= riscv-none-elf-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE    := $(PREFIX)size

# rv32imac + Zicsr: the CH569 RISC-V3A core. Older GCC (<12) spells it plain
# "rv32imac"; override ARCH if yours rejects _zicsr.
ARCH    ?= -march=rv32imac_zicsr -mabi=ilp32

# Firmware version from version.yaml (major/minor by the user, patch by the pre-commit hook) + git hash
FW_VERSION := $(shell sed -n 's/^major: *//p' $(VERSION_FILE)).$(shell sed -n 's/^minor: *//p' $(VERSION_FILE)).$(shell sed -n 's/^patch: *//p' $(VERSION_FILE))
FW_GIT     := $(shell git describe --always --dirty --abbrev=7 2>/dev/null || echo unknown)

INCLUDES := -I$(BUILD)/generated \
            $(if $(filter stream,$(VARIANT)),-Isrc-stream) \
            -Isrc \
            -Isdk/Peripheral/inc \
            -Isdk/RVMSIS \
            -Isdk/USB30Lib

CFLAGS  := $(ARCH) -Os -g -std=gnu99 \
           -ffunction-sections -fdata-sections -fmessage-length=0 \
           -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-parentheses \
           -Wno-comment -Wno-misleading-indentation \
           -DDEBUG=0 -DFREQ_SYS=80000000 \
           $(INCLUDES)

ASFLAGS := $(ARCH) -x assembler-with-cpp $(INCLUDES)

LDFLAGS := $(ARCH) -nostartfiles -Xlinker --gc-sections \
           -T sdk/Ld/Link.ld \
           -Wl,-Map,$(BUILD)/$(TARGET).map \
           --specs=nano.specs --specs=nosys.specs

USB30_LIB := sdk/USB30Lib/libCH56x_USB30_device_lib.a

ifeq ($(VARIANT),stream)
C_SOURCES := \
    src-stream/main.c \
    src-stream/usb_stream.c \
    src/chip_uid.c \
    sdk/Peripheral/src/CH56x_clk.c \
    sdk/Peripheral/src/CH56x_gpio.c \
    sdk/Peripheral/src/CH56x_pwr.c \
    sdk/Peripheral/src/CH56x_sys.c \
    sdk/Peripheral/src/CH56x_uart.c \
    sdk/RVMSIS/core_riscv.c
USB30_LIB :=
else
C_SOURCES := \
    src/Main.c \
    src/CH56x_usb30.c \
    src/CH56x_usb20.c \
    src/SW_UDISK.c \
    src/SD.c \
    src/sd_storage.c \
    src/cdc_uart.c \
    src/debug_cdc.c \
    src/ownership.c \
    src/console.c \
    src/chip_uid.c \
    src/mode.c \
    src/msc_fast.c \
    sdk/Peripheral/src/CH56x_clk.c \
    sdk/Peripheral/src/CH56x_emmc.c \
    sdk/Peripheral/src/CH56x_gpio.c \
    sdk/Peripheral/src/CH56x_pwr.c \
    sdk/Peripheral/src/CH56x_sys.c \
    sdk/Peripheral/src/CH56x_timer.c \
    sdk/Peripheral/src/CH56x_uart.c \
    sdk/RVMSIS/core_riscv.c
endif

S_SOURCES := sdk/Startup/startup_CH56x.S

OBJECTS := $(addprefix $(BUILD)/,$(C_SOURCES:.c=.o)) \
           $(addprefix $(BUILD)/,$(S_SOURCES:.S=.o))

.PHONY: all clean check check-includes check-mem size FORCE

# fw_version.h is rewritten only when the version or the git hash changes (no needless rebuilds)
$(BUILD)/generated/fw_version.h: $(VERSION_FILE) FORCE
	@mkdir -p $(dir $@)
	@printf '/* Generated from version.yaml by the Makefile: do not edit. */\n#define FW_VERSION "%s"\n#define FW_GIT     "%s"\n' \
	    "$(FW_VERSION)" "$(FW_GIT)" > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

$(BUILD)/src/console.o $(BUILD)/src/Main.o $(BUILD)/src-stream/main.o: $(BUILD)/generated/fw_version.h

all: check-includes $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex size check-mem

# Always run first: catches case-mismatched #includes that a case-insensitive
# host FS (Windows/macOS) would hide but a Linux build would choke on.
check-includes:
	python3 tools/check_includes.py

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) $(USB30_LIB)
	$(CC) $(LDFLAGS) $(OBJECTS) $(USB30_LIB) -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

size: $(BUILD)/$(TARGET).elf
	$(SIZE) $<

# RAMX use vs the RAMX the flasher leaves for this image size (USER_MEM, see tools/check_mem.py).
check-mem: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin
	python3 tools/check_mem.py $^

# Static checks (no hardware needed): include-case (catches case-sensitive-FS
# build breaks) + USB-descriptor consistency.
check: check-includes check-mem
	python3 tools/check_descriptors.py src/CH56x_usb30.c src/CH56x_usb20.c

clean:
	rm -rf $(BUILD)

FORCE:

.DEFAULT_GOAL := all
