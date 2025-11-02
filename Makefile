# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Adrian "asie" Siekierka, 2023

WONDERFUL_TOOLCHAIN ?= /opt/wonderful
TARGET = wswan/medium
include $(WONDERFUL_TOOLCHAIN)/target/$(TARGET)/makedefs.mk

PYTHON3		?= python3
UV		?= uv

VERSION		?= $(shell git rev-parse --short=8 HEAD)

# Metadata
# --------

NAME		:= menu

# Source code paths
# -----------------

INCLUDEDIRS	:= src/menu src/shared
SOURCEDIRS	:= src/menu src/shared
ASSETDIRS	:= assets/menu
CBINDIRS	:= data/menu

# Defines passed to all files
# ---------------------------

DEFINES		:= -DVERSION=\"$(VERSION)\" -DLIBWS_USE_EXTBANK

# Libraries
# ---------

export LIBNILE_PATH	:= vendor/libnile
export ATHENAOS_PATH	:= vendor/AthenaOS
LIBS		:= -lnilefs -lnile -lwsx -lws
LIBDIRS		:= $(WF_ARCH_LIBDIRS) \
		   $(LIBNILE_PATH)/dist/$(TARGET)

# Build artifacts
# ---------------

BUILDDIR	:= build/menu
ELF		:= build/$(NAME).elf
ELF_STAGE1	:= build/$(NAME)_stage1.elf
ROM		:= dist/NILESWAN/MENU.WS

# Verbose flag
# ------------

ifeq ($(V),1)
_V		:=
else
_V		:= @
endif

# Source files
# ------------

ifneq ($(ASSETDIRS),)
    SOURCES_WFPROCESS	:= $(shell find -L $(ASSETDIRS) -name "*.lua")
    INCLUDEDIRS		+= $(addprefix $(BUILDDIR)/,$(ASSETDIRS))
endif
ifneq ($(CBINDIRS),)
    SOURCES_CBIN	:= $(shell find -L $(CBINDIRS) -name "*.bin")
    INCLUDEDIRS		+= $(addprefix $(BUILDDIR)/,$(CBINDIRS))
endif
SOURCES_S	:= $(shell find -L $(SOURCEDIRS) -name "*.s")
SOURCES_C	:= $(shell find -L $(SOURCEDIRS) -name "*.c")
SOURCES_LANG	:= $(shell find -L lang)

SOURCES_CBIN += build/bootstub.bin

# Compiler and linker flags
# -------------------------

WARNFLAGS	:= -Wall

INCLUDEFLAGS	:= $(foreach path,$(INCLUDEDIRS),-I$(path)) \
		   $(foreach path,$(LIBDIRS),-isystem $(path)/include)

LIBDIRSFLAGS	:= $(foreach path,$(LIBDIRS),-L$(path)/lib)

ASFLAGS		:= -x assembler-with-cpp $(DEFINES) $(WF_ARCH_CFLAGS) \
		   $(INCLUDEFLAGS) -ffunction-sections -fdata-sections -fno-common

CFLAGS		:= -std=gnu11 $(WARNFLAGS) $(DEFINES) $(WF_ARCH_CFLAGS) \
		   $(INCLUDEFLAGS) -ffunction-sections -fdata-sections -fno-common -O2

LDFLAGS		:= -T$(WF_LDSCRIPT) $(LIBDIRSFLAGS) \
		   $(WF_ARCH_LDFLAGS) $(LIBS)

BUILDROMFLAGS	:= -c src/menu/wfconfig.toml --trim

# Intermediate build files
# ------------------------

OBJS_ASSETS	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_CBIN))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_WFPROCESS))) \
		   $(BUILDDIR)/assets/menu/lang_gen.o

OBJS_SOURCES	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_S))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_C)))

OBJS		:= $(OBJS_ASSETS) $(OBJS_SOURCES)

DEPS		:= $(OBJS:.o=.d)

# Targets
# -------

.PHONY: all clean dist distclean fonts athenaos-compatible athenaos-native libnile-bootfriend libnile-medium usage usage-symbols

all: $(ROM) compile_commands.json

dist: all athenaos-compatible athenaos-native
	@echo "  DIST"
	@cp $(ATHENAOS_PATH)/dist/AthenaBIOS-*-ww.raw dist/NILESWAN/BIOSATHC.RAW
	@cp $(ATHENAOS_PATH)/dist/AthenaBIOS-*-nileswan.raw dist/NILESWAN/BIOSATHN.RAW
	@cp $(ATHENAOS_PATH)/dist/AthenaOS-*-nileswan.raw dist/NILESWAN/ATHENAFX.RAW
	@rm -r dist/NILESWAN/LICENSE || true
	@cp -R docs/license dist/NILESWAN/LICENSE

distclean: clean

fonts: assets/menu/fonts/ark-pixel-12px-proportional-ja.bdf

assets/menu/fonts/ark-pixel-12px-proportional-ja.bdf:
	@echo "  UV      $@"
	$(_V)$(UV) --directory vendor/modified-ark-pixel-font sync
	$(_V)$(UV) --directory vendor/modified-ark-pixel-font run -m tools.cli --cleanup --font-sizes 12 --width-modes proportional --font-formats bdf --attachments release
	$(_V)cp vendor/modified-ark-pixel-font/build/outputs/$(@F) $@

athenaos-compatible:
	@$(MAKE) -C $(ATHENAOS_PATH) bios

athenaos-native:
	@$(MAKE) -C $(ATHENAOS_PATH) CONFIG=config/config.nileswan.mk

libnile-bootfriend:
	@$(MAKE) -C $(LIBNILE_PATH) TARGET=wswan/bootfriend install

libnile-medium:
	@$(MAKE) -C $(LIBNILE_PATH) TARGET=wswan/medium install

build/bootstub.bin: libnile-bootfriend
	$(_V)$(MAKE) -f Makefile.bootstub --no-print-directory

$(ROM) $(ELF): $(ELF_STAGE1)
	@echo "  ROM     $@"
	@$(MKDIR) -p $(@D)
	$(_V)$(BUILDROM) -o $(ROM) --output-elf $(ELF) $(BUILDROMFLAGS) $<

$(ELF_STAGE1): $(OBJS)
	@echo "  LD      $@"
	@$(MKDIR) -p $(@D)
	$(_V)$(CC) -r -o $(ELF_STAGE1) $(OBJS) $(WF_CRT0) $(LDFLAGS)

clean:
	@echo "  CLEAN"
	$(_V)$(MAKE) -f Makefile.bootstub clean --no-print-directory
	$(_V)$(RM) $(ELF_STAGE1) $(ELF) dist/ $(BUILDDIR) compile_commands.json || true
	$(_V)cd $(LIBNILE_PATH) && $(MAKE) TARGET=wswan/bootfriend clean
	$(_V)cd $(LIBNILE_PATH) && $(MAKE) TARGET=wswan/medium clean
	$(_V)cd $(ATHENAOS_PATH) && $(MAKE) clean
	$(_V)cd $(ATHENAOS_PATH) && $(MAKE) CONFIG=config/config.nileswan.mk clean
	$(_V)rm assets/menu/fonts/ark-pixel-12px-proportional-ja.bdf || true

usage-symbols: $(ELF)
	$(_V)$(ROMUSAGE) $< -g -C -d 0 --symbol-top 128

usage: $(ELF)
	$(_V)$(ROMUSAGE) $< -g -C

compile_commands.json: $(OBJS) | Makefile
	@echo "  MERGE   compile_commands.json"
	$(_V)$(WF)/bin/wf-compile-commands-merge $@ $(patsubst %.o,%.cc.json,$^)

# Rules
# -----

$(BUILDDIR)/%.s.o : %.s | $(OBJS_ASSETS) libnile-medium
	@echo "  AS      $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(CC) $(ASFLAGS) -MMD -MP -MJ $(patsubst %.o,%.cc.json,$@) -c -o $@ $<

$(BUILDDIR)/%.c.o : %.c | $(OBJS_ASSETS) libnile-medium
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(CC) $(CFLAGS) -MMD -MP -MJ $(patsubst %.o,%.cc.json,$@) -c -o $@ $<

# FIXME
# $(_V)$(WF)/bin/wf-bin2s --address-space __far --section ".rom0_ff.bootstub" $(@D) $<
$(BUILDDIR)/%.bin.o $(BUILDDIR)/%_bin.h : %.bin
	@echo "  BIN2S   $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(WF)/bin/wf-bin2s --address-space __far --section ".rom0.bootstub" $(@D) $<
	$(_V)$(CC) $(CFLAGS) -MMD -MP -c -o $(BUILDDIR)/$*.bin.o $(BUILDDIR)/$*_bin.s

$(BUILDDIR)/assets/menu/lang_gen.o $(BUILDDIR)/assets/menu/lang_gen.h : $(SOURCES_LANG)
	@echo "  LANG"
	@$(MKDIR) -p $(@D)
	$(_V)$(PYTHON3) tools/gen_strings.py lang $(BUILDDIR)/assets/menu/lang_gen.c $(BUILDDIR)/assets/menu/lang_gen.h
	$(_V)$(CC) $(CFLAGS) -MMD -MP -c -o $(BUILDDIR)/assets/menu/lang_gen.o $(BUILDDIR)/assets/menu/lang_gen.c

$(BUILDDIR)/%.lua.o : %.lua | fonts
	@echo "  PROCESS $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(WF)/bin/wf-process -o $(BUILDDIR)/$*.s -t $(TARGET) --depfile $(BUILDDIR)/$*.lua.d --depfile-target $(BUILDDIR)/$*.lua.o $<
	$(_V)$(CC) $(ASFLAGS) -c -o $(BUILDDIR)/$*.lua.o $(BUILDDIR)/$*.s

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)
