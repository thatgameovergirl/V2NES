# V2NES Standalone Makefile - No 3ds_rules dependency
#---------------------------------------------------------------------------------
.SUFFIXES:

# Always use local devkitPro
override DEVKITPRO := /c/Projects/V2NES/devkitPro
override DEVKITARM := $(DEVKITPRO)/devkitARM
override CTRULIB   := $(DEVKITPRO)/libctru

TOPDIR   := $(CURDIR)
BUILDDIR := $(TOPDIR)/build

PREFIX   := arm-none-eabi-
CC       := $(PREFIX)gcc
CXX      := $(PREFIX)g++
AS       := $(PREFIX)as
LD       := $(PREFIX)g++
AR       := $(PREFIX)gcc-ar
OBJCOPY  := $(PREFIX)objcopy

SHELL    := /usr/bin/bash
export SHELL

#---------------------------------------------------------------------------------
TARGET          := V2NES
APP_TITLE       := V2NES
APP_DESCRIPTION := Dual-Screen NES Emulator
APP_AUTHOR      := HristValkyrja

OUTPUT   := $(TOPDIR)/$(TARGET)
DEPSDIR  := $(BUILDDIR)

# Source directories
SRC     := source
PLAT    := platform
CORE    := cores/virtuanes
CORE3DS := cores/virtuanes/3ds
NESDIR  := cores/virtuanes/NES
APUDIR  := cores/virtuanes/NES/apuex
EMUDIR  := cores/virtuanes/NES/apuex/emu2413

VPATH := $(TOPDIR)/$(SRC) $(TOPDIR)/$(PLAT) $(TOPDIR)/$(CORE) \
         $(TOPDIR)/$(CORE3DS) $(TOPDIR)/$(NESDIR) $(TOPDIR)/$(APUDIR) \
         $(TOPDIR)/$(EMUDIR) $(TOPDIR)/$(NESDIR)/mapper $(TOPDIR)/$(NESDIR)/padex

INCLUDES := -I$(TOPDIR)/include -I$(TOPDIR)/$(SRC) -I$(TOPDIR)/$(PLAT) \
            -I$(TOPDIR)/$(CORE) -I$(TOPDIR)/$(CORE3DS) -I$(TOPDIR)/$(NESDIR) \
            -I$(TOPDIR)/$(APUDIR) -I$(TOPDIR)/$(EMUDIR) \
            -I$(TOPDIR)/$(NESDIR)/mapper -I$(TOPDIR)/$(NESDIR)/padex \
            -I$(CTRULIB)/include

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS   := -g -Wall -O2 -mword-relocations -ffunction-sections -fpermissive $(ARCH)
CFLAGS   += $(INCLUDES) -D__3DS__ -DARM11 -D_3DS
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
LDFLAGS  := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS     := -lcitro2d -lcitro3d -lctru -lm
LIBPATHS := -L$(CTRULIB)/lib

#---------------------------------------------------------------------------------
# Collect source files (flat names - no subdirectory conflicts possible)
# MapperFactory.cpp includes all mapper .cpp files via #include
# PAD.cpp includes all padex .cpp files
#---------------------------------------------------------------------------------
# Phase 1: Only compile NES core + V2NES source, not platform files
# Platform files have libctru v1→v2 API incompatibilities to fix in Phase 2
# Phase 2: Collect source files
# NES core + V2NES source + vmport platform C files that compile
# Platform CPP files are replaced with clean nes_core bridge
CPP_SRC := $(notdir $(wildcard $(SRC)/*.cpp))           # V2NES sources + nes_core
CPP_SRC += $(notdir $(wildcard $(CORE)/*.cpp))           # virtuanes root (App, Config, etc.)
CPP_SRC += $(notdir $(wildcard $(CORE3DS)/*.cpp))        # 3DS bridge (3dsimpl, palette)
CPP_SRC += $(notdir $(wildcard $(NESDIR)/*.cpp))         # NES engine
CPP_SRC += $(notdir $(wildcard $(APUDIR)/*.cpp))         # APU extensions
CPP_SRC += $(notdir $(wildcard $(EMUDIR)/*.cpp))         # VRC7 emulator
CPP_SRC += 3dslodepng.cpp                                # PNG loader (for loading screen logo)

C_SRC := $(notdir $(wildcard $(SRC)/*.c))
C_SRC += $(notdir $(wildcard $(PLAT)/*.c))               # 3dshack etc. (only C files that compile)

OBJ_CPP := $(addprefix $(BUILDDIR)/, $(CPP_SRC:.cpp=.o))
OBJ_C   := $(addprefix $(BUILDDIR)/, $(C_SRC:.c=.o))

# Shader objects (compiled separately)
# Shader objects (Phase 2 - need platform GPU layer first)
# SHADER_OBJS := $(BUILDDIR)/shaderfast2.shbin.o ...
SHADER_OBJS :=

# Embedded binary assets compiled into the .3dsx via tex3ds + bin2s
DATA_OBJS := $(BUILDDIR)/logo_png.o $(BUILDDIR)/hud_icons_t3x.o \
             $(BUILDDIR)/aquamarine_glow_top_png.o \
             $(BUILDDIR)/aquamarine_glow_bottom_png.o \
             $(BUILDDIR)/aquamarine_glow_back_png.o \
             $(BUILDDIR)/aquamarine_glow_4wrd_png.o \
             $(BUILDDIR)/aquamarine_glow_maps_png.o \
             $(BUILDDIR)/aquamarine_glow_walk_png.o \
             $(BUILDDIR)/aquamarine_glow_play_png.o \
             $(BUILDDIR)/aquamarine_glow_menu_png.o

OBJS := $(OBJ_CPP) $(OBJ_C) $(SHADER_OBJS) $(DATA_OBJS)

#---------------------------------------------------------------------------------
# Targets
#---------------------------------------------------------------------------------
.PHONY: all clean

all: $(OUTPUT).3dsx

$(BUILDDIR):
	@mkdir -p $@

# Compilation rules
$(BUILDDIR)/%.o: %.cpp | $(BUILDDIR)
	@echo "  CC $(notdir $<)"
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d -c $< -o $@ $(CXXFLAGS)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	@echo "  CC $(notdir $<)"
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -c $< -o $@ $(CFLAGS)

# Embed data/logo.png as a linkable object. bin2s exposes:
#   extern const u8 logo_png[];
#   extern const u8 logo_png_end[];
#   extern const u32 logo_png_size;
$(BUILDDIR)/logo_png.o: data/logo.png | $(BUILDDIR)
	@echo "  DATA logo.png"
	@cp $< $(BUILDDIR)/logo.png
	cd $(BUILDDIR) && bin2s -a 4 logo.png > _logo.s && $(AS) -o logo_png.o _logo.s && rm -f _logo.s logo.png

$(BUILDDIR)/aquamarine_glow_top_png.o: data/aquamarine_glow_top.png | $(BUILDDIR)
	@echo "  DATA aquamarine_glow_top.png"
	@cp $< $(BUILDDIR)/aquamarine_glow_top.png
	cd $(BUILDDIR) && bin2s -a 4 aquamarine_glow_top.png > _glow_top.s && $(AS) -o aquamarine_glow_top_png.o _glow_top.s && rm -f _glow_top.s aquamarine_glow_top.png

$(BUILDDIR)/aquamarine_glow_bottom_png.o: data/aquamarine_glow_bottom.png | $(BUILDDIR)
	@echo "  DATA aquamarine_glow_bottom.png"
	@cp $< $(BUILDDIR)/aquamarine_glow_bottom.png
	cd $(BUILDDIR) && bin2s -a 4 aquamarine_glow_bottom.png > _glow_bot.s && $(AS) -o aquamarine_glow_bottom_png.o _glow_bot.s && rm -f _glow_bot.s aquamarine_glow_bottom.png

$(BUILDDIR)/aquamarine_glow_back_png.o: data/aquamarine_glow_back.png | $(BUILDDIR)
	@echo "  DATA aquamarine_glow_back.png"
	@cp $< $(BUILDDIR)/aquamarine_glow_back.png
	cd $(BUILDDIR) && bin2s -a 4 aquamarine_glow_back.png > _glow_back.s && $(AS) -o aquamarine_glow_back_png.o _glow_back.s && rm -f _glow_back.s aquamarine_glow_back.png

$(BUILDDIR)/aquamarine_glow_4wrd_png.o: data/aquamarine_glow_4wrd.png | $(BUILDDIR)
	@echo "  DATA aquamarine_glow_4wrd.png"
	@cp $< $(BUILDDIR)/aquamarine_glow_4wrd.png
	cd $(BUILDDIR) && bin2s -a 4 aquamarine_glow_4wrd.png > _glow_fowd.s && $(AS) -o aquamarine_glow_4wrd_png.o _glow_fowd.s && rm -f _glow_fowd.s aquamarine_glow_4wrd.png

$(BUILDDIR)/aquamarine_glow_maps_png.o: data/aquamarine_glow_maps.png | $(BUILDDIR)
	@echo "  DATA aquamarine_glow_maps.png"
	@cp $< $(BUILDDIR)/aquamarine_glow_maps.png
	cd $(BUILDDIR) && bin2s -a 4 aquamarine_glow_maps.png > _glow_maps.s && $(AS) -o aquamarine_glow_maps_png.o _glow_maps.s && rm -f _glow_maps.s aquamarine_glow_maps.png

$(BUILDDIR)/aquamarine_glow_walk_png.o: data/aquamarine_glow_walk.png | $(BUILDDIR)
	@echo "  DATA aquamarine_glow_walk.png"
	@cp $< $(BUILDDIR)/aquamarine_glow_walk.png
	cd $(BUILDDIR) && bin2s -a 4 aquamarine_glow_walk.png > _glow_walk.s && $(AS) -o aquamarine_glow_walk_png.o _glow_walk.s && rm -f _glow_walk.s aquamarine_glow_walk.png

$(BUILDDIR)/aquamarine_glow_play_png.o: data/aquamarine_glow_play.png | $(BUILDDIR)
	@echo "  DATA aquamarine_glow_play.png"
	@cp $< $(BUILDDIR)/aquamarine_glow_play.png
	cd $(BUILDDIR) && bin2s -a 4 aquamarine_glow_play.png > _glow_play.s && $(AS) -o aquamarine_glow_play_png.o _glow_play.s && rm -f _glow_play.s aquamarine_glow_play.png

$(BUILDDIR)/aquamarine_glow_menu_png.o: data/aquamarine_glow_menu.png | $(BUILDDIR)
	@echo "  DATA aquamarine_glow_menu.png"
	@cp $< $(BUILDDIR)/aquamarine_glow_menu.png
	cd $(BUILDDIR) && bin2s -a 4 aquamarine_glow_menu.png > _glow_menu.s && $(AS) -o aquamarine_glow_menu_png.o _glow_menu.s && rm -f _glow_menu.s aquamarine_glow_menu.png

# Convert HUD icon sprite sheet PNG → t3x (PICA200 tiled) → linkable object.
# tex3ds handles the Morton swizzle at build time so there is zero CPU cost at runtime.
$(BUILDDIR)/hud_icons_t3x.o: data/hud_icons.png | $(BUILDDIR)
	@echo "  TEX3DS hud_icons.png"
	$(DEVKITPRO)/tools/bin/tex3ds.exe -f rgba8 -z auto $< -o $(BUILDDIR)/hud_icons.t3x
	cd $(BUILDDIR) && bin2s -a 4 hud_icons.t3x > _hud_icons.s && $(AS) -o hud_icons_t3x.o _hud_icons.s && rm -f _hud_icons.s hud_icons.t3x

# Shader compilation
$(BUILDDIR)/shaderfast2.shbin.o: $(CORE3DS)/shaderfast2.g.pica $(CORE3DS)/shaderfast2.v.pica | $(BUILDDIR)
	@echo "  SHADER fast2..."
	picasso -o $(BUILDDIR)/shaderfast2.shbin $(CORE3DS)/shaderfast2.g.pica $(CORE3DS)/shaderfast2.v.pica
	cd $(BUILDDIR) && bin2s -a shaderfast2.shbin > _shader.s && $(AS) -o shaderfast2.shbin.o _shader.s && rm -f _shader.s

$(BUILDDIR)/shaderslow.shbin.o: $(CORE3DS)/shaderslow.v.pica | $(BUILDDIR)
	@echo "  SHADER slow..."
	picasso -o $(BUILDDIR)/shaderslow.shbin $(CORE3DS)/shaderslow.v.pica
	cd $(BUILDDIR) && bin2s -a shaderslow.shbin > _shader.s && $(AS) -o shaderslow.shbin.o _shader.s && rm -f _shader.s

$(BUILDDIR)/shaderslow2.shbin.o: $(CORE3DS)/shaderslow2.v.pica | $(BUILDDIR)
	@echo "  SHADER slow2..."
	picasso -o $(BUILDDIR)/shaderslow2.shbin $(CORE3DS)/shaderslow2.v.pica
	cd $(BUILDDIR) && bin2s -a shaderslow2.shbin > _shader.s && $(AS) -o shaderslow2.shbin.o _shader.s && rm -f _shader.s

# Link
$(OUTPUT).elf: $(OBJS)
	@echo "  LINK $(notdir $@)"
	@$(LD) $(LDFLAGS) $(OBJS) $(LIBPATHS) $(LIBS) -o $@

# Pack
$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh
	@echo "  PACK $(notdir $@)"
	3dsxtool $< $@ --smdh=$(OUTPUT).smdh
	@echo "BUILD SUCCESS: $(notdir $@)  [$(words $(OBJS)) objects]"

$(OUTPUT).smdh:
	@smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(CTRULIB)/default_icon.png $@

clean:
	@echo "  CLEAN"
	@rm -rf $(BUILDDIR) $(OUTPUT).3dsx $(OUTPUT).smdh $(OUTPUT).elf $(OUTPUT).lst

-include $(DEPSDIR)/*.d
