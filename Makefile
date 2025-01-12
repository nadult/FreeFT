all: programs

FWK_DIR        = libfwk/
MODE          ?= debug
FWK_MODE      ?= devel
CFLAGS         = -Isrc/ -Ibuild/ -fopenmp
PCH_SOURCE    := src/freeft_pch.h
LDFLAGS_linux := -lz -lmpg123 -lzip -Wl,--export-dynamic
LDFLAGS_mingw := -lz -lmpg123 -lzip -lbcrypt
BUILD_DIR      = build/$(if $(findstring linux,$(PLATFORM)),,$(PLATFORM)_)$(MODE)

include $(FWK_DIR)Makefile-shared

PACKAGER      := $(FWK_DIR)tools/packager

# --- Creating necessary sub-directories ----------------------------------------------------------

SUBDIRS        = build
BUILD_SUBDIRS  = gfx ui sys net audio io game game/orders hud editor

ifndef JUNK_GATHERING
_dummy := $(shell mkdir -p $(SUBDIRS))
_dummy := $(shell mkdir -p $(addprefix $(BUILD_DIR)/,$(BUILD_SUBDIRS)))
endif

# --- Lists of source files -----------------------------------------------------------------------

SHARED_SRC := \
	gfx/texture_cache gfx/drawing gfx/scene_renderer gfx/packed_texture \
	sys/config sys/data_sheet \
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
	ui/file_dialog ui/edit_box ui/combo_box ui/image_button ui/loading_bar \
	io/controller io/loop io/main_menu_loop io/game_loop \
	audio/device audio/device_music audio/mp3_decoder \
	editor/tile_selector editor/tiles_editor editor/entities_editor editor/group_editor\
	editor/tiles_pad editor/group_pad editor/tile_group editor/view editor/entities_pad \
	res_manager

DEPRECATED_SRC:= temp/frame_allocator

PROGRAM_SRC    := editor game res_viewer convert lobby_server

ALL_SRC        := $(PROGRAM_SRC) $(SHARED_SRC)

OBJECTS        := $(ALL_SRC:%=$(BUILD_DIR)/%.o)
SHARED_OBJECTS := $(SHARED_SRC:%=$(BUILD_DIR)/%.o)  $(LIBS_SRC:%=$(BUILD_DIR)/%.o)

PROGRAMS       := $(PROGRAM_SRC:%=%$(PROGRAM_SUFFIX))
programs: $(PROGRAMS)


# --- Build targets -------------------------------------------------------------------------------

$(OBJECTS): $(BUILD_DIR)/%.o:  src/%.cpp $(PCH_TARGET)
	$(COMPILER) -MMD $(CFLAGS) $(PCH_CFLAGS) -c src/$*.cpp -o $@

$(PROGRAMS): %$(PROGRAM_SUFFIX): $(SHARED_OBJECTS) $(BUILD_DIR)/%.o $(FWK_LIB_FILE)
	$(LINKER) -o $@ $^ $(LDFLAGS)

build/res_embedded.cpp: data_embed/fonts/* data_embed/* make_embedded.sh $(PACKAGER)
	./make_embedded.sh
src/res_manager.cpp: build/res_embedded.cpp

$(PACKAGER): $(FWK_LIB_FILE) .FORCE
	$(MAKE) $(FWK_MAKE_ARGS) tools/packager

DEPS:=$(ALL_SRC:%=$(BUILD_DIR)/%.d) $(PCH_TEMP).d

JUNK_FILES    := $(OBJECTS) $(PROGRAMS) $(DEPS) build/res_embedded.cpp
JUNK_DIRS     := $(SUBDIRS)

# --- Other stuff ---------------------------------------------------------------------------------

# Recreates dependency files, in case they got outdated
depends: $(PCH_FILE_MAIN)
	@echo $(ALL_SRC) | tr '\n' ' ' | xargs -P16 -t -d' ' -I '{}' $(COMPILER) $(CFLAGS) $(PCH_CFLAGS) \
		src/'{}'.cpp -MM -MF $(BUILD_DIR)/'{}'.d -MT $(BUILD_DIR)/'{}'.o -E > /dev/null

ifndef JUNK_GATHERING
-include $(DEPS)
endif
