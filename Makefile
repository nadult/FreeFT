all: editor game res_viewer convert game.exe editor.exe res_viewer.exe

BUILD=build

_dummy := $(shell [ -d $(BUILD) ] || mkdir -p $(BUILD))
_dummy := $(shell [ -d $(BUILD)/gfx ] || mkdir -p $(BUILD)/gfx)
_dummy := $(shell [ -d $(BUILD)/ui ] || mkdir -p $(BUILD)/ui)
_dummy := $(shell [ -d $(BUILD)/sys ] || mkdir -p $(BUILD)/sys)
_dummy := $(shell [ -d $(BUILD)/game ] || mkdir -p $(BUILD)/game)
_dummy := $(shell [ -d $(BUILD)/gameui ] || mkdir -p $(BUILD)/gameui)
_dummy := $(shell [ -d $(BUILD)/editor ] || mkdir -p $(BUILD)/editor)
_dummy := $(shell [ -d $(BUILD)/lz4 ] || mkdir -p $(BUILD)/lz4)

SHARED_SRC=\
	gfx/texture_format gfx/texture gfx/texture_bmp gfx/texture_tga gfx/texture_png gfx/font \
	gfx/texture_cache gfx/device gfx/device_texture gfx/drawing gfx/sprite gfx/sprite_legacy \
	gfx/tile gfx/scene_renderer gfx/pal_texture \
	sys/frame_allocator sys/memory sys/profiler sys/platform sys/xml sys/config \
	base tile_map tile_group navigation_map navigation_bitmap grid grid_intersect \
	game/world game/actor game/actor_orders game/entity game/container game/door game/projectile \
	game/item game/inventory game/enums \
	ui/window ui/button ui/tile_list ui/progress_bar ui/list_box ui/text_box ui/message_box \
	ui/file_dialog ui/edit_box ui/combo_box \
	editor/tile_selector editor/tiles_editor editor/entities_editor editor/group_editor \
	editor/tiles_pad editor/group_pad

LIBS_SRC=lz4/lz4 lz4/lz4hc

PROGRAM_SRC=editor game res_viewer convert

ALL_SRC=$(PROGRAM_SRC) $(SHARED_SRC) sys/platform_linux sys/platform_windows

DEPS:=$(ALL_SRC:%=$(BUILD)/%.dep)

LINUX_OBJECTS:=$(ALL_SRC:%=$(BUILD)/%.o)
MINGW_OBJECTS:=$(ALL_SRC:%=$(BUILD)/%_.o)

LINUX_LIB_OBJECTS:=$(LIBS_SRC:%=$(BUILD)/%.o)
MINGW_LIB_OBJECTS:=$(LIBS_SRC:%=$(BUILD)/%_.o)

LINUX_SHARED_OBJECTS:=$(SHARED_SRC:%=$(BUILD)/%.o)  $(LIBS_SRC:%=$(BUILD)/%.o)
MINGW_SHARED_OBJECTS:=$(SHARED_SRC:%=$(BUILD)/%_.o) $(LIBS_SRC:%=$(BUILD)/%_.o)

LINUX_PROGRAMS:=$(PROGRAM_SRC:%=%)
MINGW_PROGRAMS:=$(PROGRAM_SRC:%=%.exe)

LIBS=-lglfw -lbaselib -lpng -lz
LINUX_LIBS=$(LIBS) -lGL -lGLU -lrt
MINGW_LIBS=$(LIBS) -lglu32 -lopengl32

INCLUDES=-Isrc/

NICE_FLAGS=-Woverloaded-virtual -Wnon-virtual-dtor -Werror=return-type -Wno-reorder -Wno-uninitialized \
		   -Wno-unused-but-set-variable -Wno-unused-variable

FLAGS=-std=gnu++0x -O0 -ggdb -Wall $(NICE_FLAGS) $(INCLUDES)
LIB_FLAGS=-O2

LINUX_FLAGS=$(FLAGS) -rdynamic
MINGW_FLAGS=$(FLAGS)

CXX=g++
CC =gcc

MGW_CXX=i686-mingw32-g++
MGW_CC =i686-mingw32-gcc


-include Makefile.local

$(BUILD)/liblz4.a: libs/lz4/lz4.c libs/lz4/lz4hc.c
		$(CC) $(LINUX_FLAGS) $< 

$(DEPS): $(BUILD)/%.dep: src/%.cpp
	    $(CXX) $(LINUX_FLAGS) -MM $< -MT $(BUILD)/$*.o   > $@
	    $(CXX) $(LINUX_FLAGS) -MM $< -MT $(BUILD)/$*_.o >> $@

$(LINUX_LIB_OBJECTS): $(BUILD)/%.o:  libs/%.c
	    $(CC)  $(LIB_FLAGS) -c libs/$*.c -o $@

$(MINGW_LIB_OBJECTS): $(BUILD)/%_.o: libs/%.c
	$(MGW_CC)  $(LIB_FLAGS) -c libs/$*.c -o $@

$(LINUX_OBJECTS): $(BUILD)/%.o:  src/%.cpp
	    $(CXX) $(LINUX_FLAGS) -c src/$*.cpp -o $@

$(MINGW_OBJECTS): $(BUILD)/%_.o: src/%.cpp
	$(MGW_CXX) $(MINGW_FLAGS) -c src/$*.cpp -o $@

$(LINUX_PROGRAMS): %:     $(LINUX_SHARED_OBJECTS) $(BUILD)/%.o  $(BUILD)/sys/platform_linux.o
	    $(CXX) -o $@ $^ -rdynamic $(LINUX_LIBS)

$(MINGW_PROGRAMS): %.exe: $(MINGW_SHARED_OBJECTS) $(BUILD)/%_.o $(BUILD)/sys/platform_windows_.o
	$(MGW_CXX) -o $@ $^ $(MINGW_LIBS)

clean:
	-rm -f $(LINUX_OBJECTS) $(LINUX_LIB_OBJECTS) $(MINGW_LIB_OBJECTS) $(MINGW_OBJECTS) $(LINUX_PROGRAMS) \
			$(MINGW_PROGRAMS) $(DEPS) $(BUILD)/.depend
	-rmdir $(BUILD)/gfx $(BUILD)/sys $(BUILD)/ui $(BUILD)/game $(BUILD)/gameui $(BUILD)/editor $(BUILD)/lz4
	-rmdir $(BUILD)

$(BUILD)/.depend: $(DEPS)
	cat $(DEPS) > $(BUILD)/.depend

depend: $(BUILD)/.depend

.PHONY: clean depend

DEPEND_FILE=$(BUILD)/.depend
DEP=$(wildcard $(DEPEND_FILE))
ifneq "$(DEP)" ""
include $(DEP)
endif

