all: editor game res_viewer convert lobby_server \
	 convert.exe game.exe editor.exe res_viewer.exe

BUILD=build

_dummy := $(shell [ -d $(BUILD) ] || mkdir -p $(BUILD))
_dummy := $(shell [ -d $(BUILD)/gfx ] || mkdir -p $(BUILD)/gfx)
_dummy := $(shell [ -d $(BUILD)/ui ] || mkdir -p $(BUILD)/ui)
_dummy := $(shell [ -d $(BUILD)/sys ] || mkdir -p $(BUILD)/sys)
_dummy := $(shell [ -d $(BUILD)/net ] || mkdir -p $(BUILD)/net)
_dummy := $(shell [ -d $(BUILD)/audio ] || mkdir -p $(BUILD)/audio)
_dummy := $(shell [ -d $(BUILD)/io ] || mkdir -p $(BUILD)/io)
_dummy := $(shell [ -d $(BUILD)/game ] || mkdir -p $(BUILD)/game)
_dummy := $(shell [ -d $(BUILD)/game/orders ] || mkdir -p $(BUILD)/game/orders)
_dummy := $(shell [ -d $(BUILD)/hud ] || mkdir -p $(BUILD)/hud)
_dummy := $(shell [ -d $(BUILD)/editor ] || mkdir -p $(BUILD)/editor)
_dummy := $(shell [ -d $(BUILD)/lz4 ] || mkdir -p $(BUILD)/lz4)

SHARED_SRC=\
	gfx/texture_format gfx/texture gfx/texture_bmp gfx/texture_tga gfx/texture_png gfx/font gfx/opengl \
	gfx/texture_cache gfx/device gfx/device_texture gfx/drawing gfx/scene_renderer gfx/packed_texture \
	sys/frame_allocator sys/memory sys/profiler sys/platform sys/xml sys/config sys/data_sheet \
	net/socket net/lobby net/chunk net/host net/server net/client \
	occluder_map base base_math navi_map navi_heightmap grid grid_intersect \
	game/tile game/sprite game/sprites game/sprite_legacy game/tile_map game/tile_map_legacy game/entity_map \
	game/world game/entity game/entity_world_proxy game/container game/level game/visibility game/path \
	game/door game/projectile game/impact game/item game/weapon game/armour game/inventory game/base game/entities \
	game/actor game/actor_ai game/actor_proto game/orders game/proto game/trigger game/character \
	game/orders/attack game/orders/change_stance game/orders/die game/orders/idle game/orders/interact \
	game/orders/inventory game/orders/move game/orders/look_at game/orders/track game/orders/get_hit \
	hud/base hud/layer hud/widget hud/char_icon hud/stance hud/weapon hud/inventory hud/hud hud/options \
	ui/window ui/button ui/tile_list ui/progress_bar ui/list_box ui/text_box ui/message_box \
	ui/file_dialog ui/edit_box ui/combo_box ui/image_button \
	io/controller io/console io/main_menu_loop io/single_player_loop io/multi_player_loop io/server_loop \
	editor/tile_selector editor/tiles_editor editor/entities_editor editor/group_editor\
	editor/tiles_pad editor/group_pad editor/tile_group editor/view editor/entities_pad \
	audio/device audio/sound audio/device_music audio/mp3_decoder

LIBS_SRC=lz4/lz4 lz4/lz4hc

PROGRAM_SRC=editor game res_viewer convert lobby_server

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

LIBS=-lglfw -lbaselib -lpng -lz -lmpg123
LINUX_LIBS=$(LIBS) -lopenal -lGL -lGLU -lrt -fopenmp 
MINGW_LIBS=$(LIBS) -lOpenAL32 -ldsound -lole32 -lwinmm -lglu32 -lopengl32 -lws2_32

LIBS_convert=-lzip 

INCLUDES=-Isrc/

NICE_FLAGS=-Woverloaded-virtual -Wnon-virtual-dtor -Werror=return-type -Wno-reorder -Wno-uninitialized \
		   -Wno-unused-but-set-variable -Wno-unused-variable -Wparentheses #-Werror

FLAGS=-std=c++0x -O0 -ggdb -Wall $(NICE_FLAGS) $(INCLUDES)
LIB_FLAGS=-O2

LINUX_FLAGS=$(FLAGS) -rdynamic -fopenmp
MINGW_FLAGS=$(FLAGS) -mno-ms-bitfields #mms-bitfields triggers attribute packed bug on gcc4.7

CXX=g++
CC =gcc

MGW_CXX=i686-w64-mingw32-g++
MGW_CC =i686-w64-mingw32-gcc 


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
	    $(CXX) -o $@ $^ -rdynamic $(LIBS_$@) $(LINUX_LIBS)

$(MINGW_PROGRAMS): %.exe: $(MINGW_SHARED_OBJECTS) $(BUILD)/%_.o $(BUILD)/sys/platform_windows_.o
	$(MGW_CXX) -o $@ $^  $(LIBS_$*) $(MINGW_LIBS)

clean:
	-rm -f $(LINUX_OBJECTS) $(LINUX_LIB_OBJECTS) $(MINGW_LIB_OBJECTS) $(MINGW_OBJECTS) $(LINUX_PROGRAMS) \
			$(MINGW_PROGRAMS) $(DEPS) $(BUILD)/.depend
	-rmdir $(BUILD)/game/orders $(BUILD)/lz4 $(BUILD)/net $(BUILD)/io
	-rmdir $(BUILD)/gfx $(BUILD)/sys $(BUILD)/ui $(BUILD)/game $(BUILD)/hud $(BUILD)/editor
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

