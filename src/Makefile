all: ../editor ../game ../res_viewer ../game.exe ../editor.exe ../res_viewer.exe

BUILD_DIR=../build

_dummy := $(shell [ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR))
_dummy := $(shell [ -d $(BUILD_DIR)/gfx ] || mkdir -p $(BUILD_DIR)/gfx)
_dummy := $(shell [ -d $(BUILD_DIR)/ui ] || mkdir -p $(BUILD_DIR)/ui)
_dummy := $(shell [ -d $(BUILD_DIR)/sys ] || mkdir -p $(BUILD_DIR)/sys)
_dummy := $(shell [ -d $(BUILD_DIR)/game ] || mkdir -p $(BUILD_DIR)/game)
_dummy := $(shell [ -d $(BUILD_DIR)/gameui ] || mkdir -p $(BUILD_DIR)/gameui)
_dummy := $(shell [ -d $(BUILD_DIR)/editor ] || mkdir -p $(BUILD_DIR)/editor)

SHARED_SRC=\
	gfx/texture_format gfx/texture gfx/texture_bmp gfx/texture_tga gfx/texture_png gfx/font gfx/texture_cache \
	gfx/device gfx/device_texture gfx/drawing gfx/texture_zar gfx/sprite gfx/tile gfx/scene_renderer \
	sys/frame_allocator sys/memory sys/profiler sys/platform sys/xml sys/config \
	base tile_map tile_group navigation_map navigation_bitmap grid grid_intersect \
	game/world game/actor game/actor_orders game/entity game/container game/door game/projectile game/item \
	game/inventory game/enums \
	ui/window ui/button ui/tile_list ui/progress_bar ui/list_box ui/text_box ui/message_box ui/file_dialog \
	ui/edit_box ui/combo_box \
	editor/tile_selector editor/tiles_editor editor/group_editor editor/tiles_pad editor/group_pad

PROGRAM_SRC=editor game res_viewer

ALL_SRC=$(PROGRAM_SRC) $(SHARED_SRC) sys/platform_linux sys/platform_windows

DEPS:=$(ALL_SRC:%=$(BUILD_DIR)/%.dep)

LINUX_OBJECTS:=$(ALL_SRC:%=$(BUILD_DIR)/%.o)
MINGW_OBJECTS:=$(ALL_SRC:%=$(BUILD_DIR)/%_.o)

LINUX_SHARED_OBJECTS:=$(SHARED_SRC:%=$(BUILD_DIR)/%.o)
MINGW_SHARED_OBJECTS:=$(SHARED_SRC:%=$(BUILD_DIR)/%_.o)

LINUX_PROGRAMS:=$(PROGRAM_SRC:%=../%)
MINGW_PROGRAMS:=$(PROGRAM_SRC:%=../%.exe)

LIBS=-lglfw -lbaselib -lpng -lz
LINUX_LIBS=$(LIBS) -lGL -lGLU -lrt
MINGW_LIBS=$(LIBS) -lglu32 -lopengl32

INCLUDES=-I./

NICE_FLAGS=-Woverloaded-virtual -Wnon-virtual-dtor -Werror=return-type -Wno-reorder -Wno-uninitialized \
		   -Wno-unused-but-set-variable -Wno-unused-variable

FLAGS=-std=gnu++0x -O0 -ggdb -Wall $(NICE_FLAGS) $(INCLUDES)
LINUX_FLAGS=$(FLAGS) -rdynamic
MINGW_FLAGS=$(FLAGS) -fpermissive #TODO: remove permissive (needed for rapidxml compilation on gcc-4.7)

CXX=g++
MGW_CXX=i686-mingw32-g++

-include Makefile.local


$(DEPS): $(BUILD_DIR)/%.dep: %.cpp
	    $(CXX) $(LINUX_FLAGS) -MM $< -MT $(BUILD_DIR)/$*.o   > $@
	    $(CXX) $(LINUX_FLAGS) -MM $< -MT $(BUILD_DIR)/$*_.o >> $@

$(LINUX_OBJECTS): $(BUILD_DIR)/%.o:  %.cpp
	    $(CXX) $(LINUX_FLAGS) -c $*.cpp -o $@

$(MINGW_OBJECTS): $(BUILD_DIR)/%_.o: %.cpp
	$(MGW_CXX) $(MINGW_FLAGS) -c $*.cpp -o $@

$(LINUX_PROGRAMS): ../%:     $(LINUX_SHARED_OBJECTS) $(BUILD_DIR)/%.o  $(BUILD_DIR)/sys/platform_linux.o
	    $(CXX) -o $@ $^ -rdynamic $(LINUX_LIBS)

$(MINGW_PROGRAMS): ../%.exe: $(MINGW_SHARED_OBJECTS) $(BUILD_DIR)/%_.o $(BUILD_DIR)/sys/platform_windows_.o
	$(MGW_CXX) -o $@ $^ $(MINGW_LIBS)

clean:
	-rm -f $(LINUX_OBJECTS) $(MINGW_OBJECTS) $(LINUX_PROGRAMS) $(MINGW_PROGRAMS) $(DEPS) $(BUILD_DIR)/.depend
	-rmdir $(BUILD_DIR)/gfx $(BUILD_DIR)/sys $(BUILD_DIR)/ui $(BUILD_DIR)/game $(BUILD_DIR)/gameui
	-rmdir $(BUILD_DIR)

$(BUILD_DIR)/.depend: $(DEPS)
	cat $(DEPS) > $(BUILD_DIR)/.depend

depend: $(BUILD_DIR)/.depend

.PHONY: clean depend

DEPEND_FILE=$(BUILD_DIR)/.depend
DEP=$(wildcard $(DEPEND_FILE))
ifneq "$(DEP)" ""
include $(DEP)
endif

