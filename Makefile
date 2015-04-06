BUILD_DIR=build
MINGW_PREFIX=i686-w64-mingw32.static-

-include Makefile.local


BUILD_DIR=build

_dummy := $(shell [ -d $(BUILD_DIR) ] || mkdir -p $(BUILD_DIR))
_dummy := $(shell [ -d $(BUILD_DIR)/gfx ] || mkdir -p $(BUILD_DIR)/gfx)
_dummy := $(shell [ -d $(BUILD_DIR)/ui ] || mkdir -p $(BUILD_DIR)/ui)
_dummy := $(shell [ -d $(BUILD_DIR)/sys ] || mkdir -p $(BUILD_DIR)/sys)
_dummy := $(shell [ -d $(BUILD_DIR)/net ] || mkdir -p $(BUILD_DIR)/net)
_dummy := $(shell [ -d $(BUILD_DIR)/audio ] || mkdir -p $(BUILD_DIR)/audio)
_dummy := $(shell [ -d $(BUILD_DIR)/io ] || mkdir -p $(BUILD_DIR)/io)
_dummy := $(shell [ -d $(BUILD_DIR)/game ] || mkdir -p $(BUILD_DIR)/game)
_dummy := $(shell [ -d $(BUILD_DIR)/game/orders ] || mkdir -p $(BUILD_DIR)/game/orders)
_dummy := $(shell [ -d $(BUILD_DIR)/hud ] || mkdir -p $(BUILD_DIR)/hud)
_dummy := $(shell [ -d $(BUILD_DIR)/editor ] || mkdir -p $(BUILD_DIR)/editor)

SHARED_SRC=\
	gfx/texture_cache gfx/drawing gfx/scene_renderer gfx/packed_texture \
	sys/frame_allocator sys/config sys/data_sheet \
	net/socket net/base net/chunk net/host net/server net/client \
	occluder_map base navi_map navi_heightmap grid grid_intersect \
	game/tile game/sprite game/sprites game/sprite_legacy game/tile_map game/tile_map_legacy game/entity_map \
	game/world game/entity game/entity_world_proxy game/container game/level game/visibility game/path \
	game/door game/projectile game/impact game/item game/weapon game/armour game/ammo game/inventory game/base \
	game/entities game/actor game/brain game/actor_proto game/orders game/proto game/trigger game/character \
	game/orders/attack game/orders/change_stance game/orders/die game/orders/idle game/orders/interact \
	game/orders/inventory game/orders/move game/orders/look_at game/orders/track game/orders/get_hit \
	game/thinking_entity game/game_mode game/death_match game/single_player_mode game/pc_controller game/turret \
	hud/base hud/widget hud/layer hud/button hud/edit_box hud/console hud/grid hud/target_info \
	hud/char_icon hud/weapon hud/inventory hud/hud hud/main_panel hud/options hud/class hud/character hud/stats \
	hud/multi_player_menu \
	ui/window ui/button ui/tile_list ui/progress_bar ui/list_box ui/text_box ui/message_box \
	ui/file_dialog ui/edit_box ui/combo_box ui/image_button \
	io/controller io/loop io/main_menu_loop io/game_loop \
	editor/tile_selector editor/tiles_editor editor/entities_editor editor/group_editor\
	editor/tiles_pad editor/group_pad editor/tile_group editor/view editor/entities_pad \
	audio/device audio/sound audio/device_music audio/mp3_decoder

PROGRAM_SRC=editor game res_viewer convert lobby_server

ALL_SRC=$(PROGRAM_SRC) $(SHARED_SRC)

DEPS:=$(ALL_SRC:%=$(BUILD_DIR)/%.dep)

LINUX_OBJECTS:=$(ALL_SRC:%=$(BUILD_DIR)/%.o)
MINGW_OBJECTS:=$(ALL_SRC:%=$(BUILD_DIR)/%_.o)

LINUX_SHARED_OBJECTS:=$(SHARED_SRC:%=$(BUILD_DIR)/%.o)  $(LIBS_SRC:%=$(BUILD_DIR)/%.o)
MINGW_SHARED_OBJECTS:=$(SHARED_SRC:%=$(BUILD_DIR)/%_.o) $(LIBS_SRC:%=$(BUILD_DIR)/%_.o)

LINUX_PROGRAMS:=$(PROGRAM_SRC:%=%)
MINGW_PROGRAMS:=$(PROGRAM_SRC:%=%.exe)

all: $(LINUX_PROGRAMS) $(MINGW_PROGRAMS)

FWK_DIR=libfwk
include $(FWK_DIR)/Makefile.include

LIBS=
LINUX_LIBS=$(LIBS) $(LINUX_FWK_LIBS) -fopenmp 
MINGW_LIBS=$(LIBS) $(MINGW_FWK_LIBS)

SPECIAL_LIBS_convert=-lzip 

LINUX_CXX=clang++
LINUX_STRIP=strip
LINUX_PKG_CONFIG=pkg-config

MINGW_CXX=$(MINGW_PREFIX)g++
MINGW_STRIP=$(MINGW_PREFIX)strip
MINGW_PKG_CONFIG=$(MINGW_PREFIX)pkg-config

INCLUDES=-Isrc/ $(FWK_INCLUDES)

NICE_FLAGS=-std=c++11 -ggdb -Wall -Woverloaded-virtual -Wnon-virtual-dtor -Werror=return-type \
		   -Wno-reorder -Wno-uninitialized -Wno-unused-function -Wno-unused-variable -Wparentheses -Wno-overloaded-virtual
LINUX_FLAGS=$(NICE_FLAGS) $(INCLUDES) $(FLAGS) -fopenmp
MINGW_FLAGS=$(NICE_FLAGS) $(INCLUDES) $(FLAGS) `$(MINGW_PKG_CONFIG) libzip --cflags`

$(DEPS): $(BUILD_DIR)/%.dep: src/%.cpp
	$(LINUX_CXX) $(LINUX_FLAGS) -MM $< -MT $(BUILD_DIR)/$*.o   > $@
	$(MINGW_CXX) $(MINGW_FLAGS) -MM $< -MT $(BUILD_DIR)/$*_.o >> $@

$(LINUX_OBJECTS): $(BUILD_DIR)/%.o:  src/%.cpp
	$(LINUX_CXX) $(LINUX_FLAGS) -c src/$*.cpp -o $@

$(MINGW_OBJECTS): $(BUILD_DIR)/%_.o: src/%.cpp
	$(MINGW_CXX) $(MINGW_FLAGS) -c src/$*.cpp -o $@

$(LINUX_PROGRAMS): %:     $(LINUX_SHARED_OBJECTS) $(BUILD_DIR)/%.o  $(LINUX_FWK_LIB)
	$(LINUX_CXX) -o $@ $^ -Wl,--export-dynamic $(SPECIAL_LIBS_$@) $(LINUX_LIBS)

$(MINGW_PROGRAMS): %.exe: $(MINGW_SHARED_OBJECTS) $(BUILD_DIR)/%_.o $(MINGW_FWK_LIB)
	$(MINGW_CXX) -o $@ $^  $(SPECIAL_LIBS_$*) $(MINGW_LIBS)
	$(MINGW_STRIP) $@

clean:
	-rm -f $(LINUX_OBJECTS) $(LINUX_LIB_OBJECTS) $(MINGW_LIB_OBJECTS) $(MINGW_OBJECTS) $(LINUX_PROGRAMS) \
			$(MINGW_PROGRAMS) $(DEPS) $(BUILD_DIR)/.depend
	-rmdir $(BUILD_DIR)/game/orders $(BUILD_DIR)/net $(BUILD_DIR)/io
	-rmdir $(BUILD_DIR)/gfx $(BUILD_DIR)/sys $(BUILD_DIR)/ui $(BUILD_DIR)/game $(BUILD_DIR)/hud $(BUILD_DIR)/editor
	-rmdir $(BUILD_DIR)

$(BUILD_DIR)/.depend: $(DEPS)
	cat $(DEPS) > $(BUILD_DIR)/.depend

depend: $(BUILD_DIR)/.depend

.PHONY: clean depend $(LINUX_FWK_LIB) $(MINGW_FWK_LIB)

DEPEND_FILE=$(BUILD_DIR)/.depend
DEP=$(wildcard $(DEPEND_FILE))
ifneq "$(DEP)" ""
include $(DEP)
endif

