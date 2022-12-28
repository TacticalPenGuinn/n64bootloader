V=1
SOURCE_DIR=src
BUILD_DIR=build
include ./n64.linux.mk

PROG_NAME=linux
ROM_EXTENSION=.z64

vmlinux = vmlinux.32
mydisk = mydisk

N64_TOOLFLAGS = --header $(N64_HEADERPATH) --title $(N64_ROM_TITLE) \
		-o $(PROG_NAME)$(ROM_EXTENSION) build/$(PROG_NAME).elf.bin \
		-s 1048568B disk.size.bin \
		-s 1048572B vmlinux.size.bin \
		-s 1M $(vmlinux) \
		-s $$(util/size2bin $(vmlinux))B $(mydisk)

all: vmlinux.size.bin disk.size.bin $(PROG_NAME)$(ROM_EXTENSION).gz
.PHONY: all


$(PROG_NAME)$(ROM_EXTENSION).gz: $(PROG_NAME)$(ROM_EXTENSION)
	@gzip -kf $< 

OBJS = $(BUILD_DIR)/main.o

$(PROG_NAME)$(ROM_EXTENSION): N64_ROM_TITLE="Linux" 

vmlinux.size.bin: util/size2bin $(vmlinuz)
	@echo $(DISKOFF)
	@util/size2bin $(vmlinux) vmlinux.size.bin

disk.size.bin: util/size2bin $(mydisk)
	@util/size2bin $(mydisk) disk.size.bin

$(BUILD_DIR)/$(PROG_NAME).elf: $(OBJS)

clean:
	rm -f $(BUILD_DIR)/* *.z64 *.size.bin *.gz
.PHONY: clean

util/size2bin:
	$(MAKE) -C util

-include $(wildcard $(BUILD_DIR)/*.d)
