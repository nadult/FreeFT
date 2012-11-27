all: editor game res_viewer

BUILD_DIR=/tmp/build

_dummy := $(shell [ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR))
_dummy := $(shell [ -d $(BUILD_DIR)/gfx ] || mkdir -p $(BUILD_DIR)/gfx)
_dummy := $(shell [ -d $(BUILD_DIR)/ui ] || mkdir -p $(BUILD_DIR)/ui)
_dummy := $(shell [ -d $(BUILD_DIR)/sys ] || mkdir -p $(BUILD_DIR)/sys)

FILES=gfx/texture_format gfx/texture gfx/texture_bmp gfx/texture_tga gfx/texture_png gfx/font \
	  gfx/device gfx/device_texture gfx/drawing gfx/texture_zar gfx/sprite gfx/tile gfx/scene_renderer \
	sys/frame_allocator sys/memory sys/profiler sys/platform sys/platform_linux \
	editor game res_viewer base tile_map tile_group actor entity navigation_map \
	ui/window ui/button ui/tile_list ui/progress_bar ui/list_box ui/text_box ui/message_box ui/file_dialog ui/edit_box \
	ui/tile_selector ui/tile_map_editor ui/tile_group_editor

MAIN_FILES=editor game res_viewer
SHARED_FILES=$(filter-out $(MAIN_FILES), $(FILES))

LIBS=-lglfw -lbaselib -lpng -lz

LINUX_LIBS=$(LIBS) -lGL -lGLU
MINGW_LIBS=$(LIBS) -lglu32 -lopengl32 -lpthreadGC2
INCLUDES=-I./ -Irapidxml-1.13/

NICE_FLAGS=-Woverloaded-virtual -Wnon-virtual-dtor -Werror=return-type -Wno-reorder -Wno-uninitialized
FLAGS=-rdynamic -std=gnu++0x -O0 -ggdb -ffast-math -msse2 -Wall $(NICE_FLAGS)

-include Makefile.local

MGW_CXX=i686-mingw32-g++

DEPS:=$(FILES:%=$(BUILD_DIR)/%.dep)
SOURCES:=$(FILES:%=%.cpp)
OBJECTS:=$(FILES:%=$(BUILD_DIR)/%.o)
MGW_OBJECTS:=$(FILES:%=$(BUILD_DIR)/%_.o)

SHARED_OBJECTS:=$(SHARED_FILES:%=$(BUILD_DIR)/%.o)
MGW_SHARED_OBJECTS:=$(SHARED_FILES:%=$(BUILD_DIR)/%_.o)

$(DEPS): $(BUILD_DIR)/%.dep: %.cpp
	$(CXX) $(FLAGS) $(INCLUDES) -MM $< -MT $(BUILD_DIR)/$*.o > $@

#pch.h.gch: pch.h base.h base_math.h
#	$(CXX) $(FLAGS) $(INCLUDES) pch.h -o $@

$(OBJECTS): $(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(FLAGS) $(INCLUDES) -c $*.cpp -o $@

$(MGW_OBJECTS): $(BUILD_DIR)/%_.o: %.cpp
	$(MGW_CXX) $(FLAGS) $(INCLUDES) -c $*.cpp -o $@

editor: $(SHARED_OBJECTS) $(BUILD_DIR)/editor.o
	$(CXX) $(FLAGS) $(INCLUDES) -o $@ $^ $(LINUX_LIBS) -pthread

game: $(SHARED_OBJECTS) $(BUILD_DIR)/game.o
	$(CXX) $(FLAGS) $(INCLUDES) -o $@ $^ $(LINUX_LIBS) -pthread

res_viewer: $(SHARED_OBJECTS) $(BUILD_DIR)/res_viewer.o
	$(CXX) $(FLAGS) $(INCLUDES) -o $@ $^ $(LINUX_LIBS) -pthread

clean:
	-rm -f $(OBJECTS) $(MGW_OBJECTS) $(DEPS) $(BUILD_DIR)/.depend editor game res_viewer
	-rmdir $(BUILD_DIR)/gfx $(BUILD_DIR)/script $(BUILD_DIR)/io $(BUILD_DIR)/sys $(BUILD_DIR)/ui
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

