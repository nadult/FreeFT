all: game

BUILD_DIR=/tmp/build

_dummy := $(shell [ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR))
_dummy := $(shell [ -d $(BUILD_DIR)/gfx ] || mkdir -p $(BUILD_DIR)/gfx)
_dummy := $(shell [ -d $(BUILD_DIR)/io ] || mkdir -p $(BUILD_DIR)/io)
_dummy := $(shell [ -d $(BUILD_DIR)/script ] || mkdir -p $(BUILD_DIR)/script)
_dummy := $(shell [ -d $(BUILD_DIR)/sys ] || mkdir -p $(BUILD_DIR)/sys)

FILES=gfx/texture_format gfx/texture gfx/texture_bmp gfx/texture_tga gfx/texture_png gfx/font \
	  gfx/device gfx/device_texture gfx/drawing gfx/texture_zar gfx/sprite gfx/tile \
	script/parser script/script script/sparser \
	sys/error_logger sys/frame_allocator sys/memory sys/profiler \
	game base tile_map

MAIN_FILES=game
SHARED_FILES=$(filter-out $(MAIN_FILES), $(FILES))

#WX_LIBS=`/usr/local/bin/wx-config --libs`
#GTK_LIBS=`pkg-config --libs`
LIBS=-L/usr/lib -L/usr/local/lib64 -lglfw -lbaselib -lpng -lz

LINUX_LIBS=$(LIBS) -lXrandr -lGL -lGLU
MINGW_LIBS=$(LIBS) -lglu32 -lopengl32 -lpthreadGC2
#WX_INCLUDES=`/usr/local/bin/wx-config --cflags`
#GTK_INCLUDES=`pkg-config gtkmm-2.4 --cflags`
INCLUDES=-I./

NICE_FLAGS=-Woverloaded-virtual -Wnon-virtual-dtor -Werror=return-type
FLAGS=-rdynamic -std=gnu++0x -O0 -ggdb -ffast-math -msse2 -Wall -DOPENGL_VERSION=0x20 $(NICE_FLAGS)

include Makefile.local

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

game: $(SHARED_OBJECTS) $(BUILD_DIR)/game.o
	$(CXX) $(FLAGS) $(INCLUDES) -o $@ $^ $(LINUX_LIBS) -pthread

clean:
	-rm -f $(OBJECTS) $(MGW_OBJECTS) $(DEPS) $(BUILD_DIR)/.depend game
	-rmdir $(BUILD_DIR)/gfx $(BUILD_DIR)/script $(BUILD_DIR)/io $(BUILD_DIR)/sys
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

