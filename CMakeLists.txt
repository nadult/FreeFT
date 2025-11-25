# Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
# This file is part of FreeFT. See license.txt for details.

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_GENERATOR MATCHES "Visual Studio")
	set(CMAKE_GENERATOR_TOOLSET "ClangCL")
endif()

project(FreeFT VERSION 0.1 LANGUAGES CXX)

set(FWK_BUILD_TESTS OFF CACHE BOOL "")
set(FWK_BUILD_TOOLS OFF CACHE BOOL "")
set(FWK_UNITY_BUILD ON CACHE BOOL "")
set(FWK_DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies" CACHE PATH "")
add_subdirectory(libfwk EXCLUDE_FROM_ALL)

set(HEADERS_freeft_base
    base.h
    grid.h
    navi_heightmap.h
    navi_map.h
    occluder_map.h
    res_manager.h
)

set(SOURCES_freeft_base
    base.cpp
    grid_intersect.cpp
    grid.cpp
    navi_heightmap.cpp
    navi_map.cpp
    occluder_map.cpp
    res_manager.cpp
)

set(HEADERS_freeft_audio
    audio/device.h
    audio/internals.h
    audio/mp3_decoder.h
)

set(SOURCES_freeft_audio
	audio/device.cpp
    audio/device_music.cpp
    audio/mp3_decoder.cpp
)

set(HEADERS_freeft_editor
    editor/entities_editor.h
    editor/entities_pad.h
    editor/group_editor.h
    editor/group_pad.h
    editor/tile_group.h
    editor/tile_selector.h
    editor/tiles_editor.h
    editor/tiles_pad.h
    editor/view.h
)

set(SOURCES_freeft_editor
    editor/entities_editor.cpp
    editor/entities_pad.cpp
    editor/group_editor.cpp
    editor/group_pad.cpp
    editor/tile_group.cpp
    editor/tile_selector.cpp
    editor/tiles_editor.cpp
    editor/tiles_pad.cpp
    editor/view.cpp
)

set(HEADERS_freeft_game
    game/actor.h
    game/all_orders.h
    game/ammo.h
    game/armour.h
    game/base.h
    game/brain.h
    game/character.h
    game/container.h
    game/death_match.h
    game/door.h
    game/entity.h
    game/entity_map.h
    game/game_mode.h
    game/impact.h
    game/inventory.h
    game/item.h
    game/level.h
    game/orders.h
    game/path.h
    game/pc_controller.h
    game/projectile.h
    game/proto.h
    game/single_player_mode.h
    game/sprite.h
    game/thinking_entity.h
    game/tile.h
    game/tile_map.h
    game/trigger.h
    game/turret.h
    game/visibility.h
    game/weapon.h
    game/world.h
)

set(SOURCES_freeft_game
    game/actor.cpp
    game/actor_proto.cpp
    game/ammo.cpp
    game/armour.cpp
    game/base.cpp
    game/brain.cpp
    game/character.cpp
    game/container.cpp
    game/death_match.cpp
    game/door.cpp
    game/entities.cpp
    game/entity.cpp
    game/entity_map.cpp
    game/entity_world_proxy.cpp
    game/game_mode.cpp
    game/impact.cpp
    game/inventory.cpp
    game/item.cpp
    game/level.cpp
    game/orders.cpp
    game/path.cpp
    game/pc_controller.cpp
    game/projectile.cpp
    game/proto.cpp
    game/single_player_mode.cpp
    game/sprite.cpp
    game/sprite_legacy.cpp
    game/sprites.cpp
    game/thinking_entity.cpp
    game/tile.cpp
    game/tile_map.cpp
    game/tile_map_legacy.cpp
    game/trigger.cpp
    game/turret.cpp
    game/visibility.cpp
    game/weapon.cpp
    game/world.cpp
)

set(HEADERS_freeft_game_orders
    game/orders/attack.h
    game/orders/change_stance.h
    game/orders/die.h
    game/orders/get_hit.h
    game/orders/idle.h
    game/orders/interact.h
    game/orders/inventory.h
    game/orders/look_at.h
    game/orders/move.h
    game/orders/track.h
)

set(SOURCES_freeft_game_orders
    game/orders/attack.cpp
    game/orders/change_stance.cpp
    game/orders/die.cpp
    game/orders/get_hit.cpp
    game/orders/idle.cpp
    game/orders/interact.cpp
    game/orders/inventory.cpp
    game/orders/look_at.cpp
    game/orders/move.cpp
    game/orders/track.cpp
)

set(HEADERS_freeft_gfx
    gfx/drawing.h
    gfx/packed_texture.h
    gfx/scene_renderer.h
    gfx/texture_cache.h
)

set(SOURCES_freeft_gfx
    gfx/drawing.cpp
    gfx/packed_texture.cpp
    gfx/scene_renderer.cpp
    gfx/texture_cache.cpp
)

set(HEADERS_freeft_hud
    hud/base.h
    hud/button.h
    hud/char_icon.h
    hud/character.h
    hud/class.h
    hud/console.h
    hud/edit_box.h
    hud/grid.h
    hud/hud.h
    hud/inventory.h
    hud/layer.h
    hud/main_panel.h
    hud/multi_player_menu.h
    hud/options.h
    hud/stats.h
    hud/target_info.h
    hud/weapon.h
    hud/widget.h
)

set(SOURCES_freeft_hud
    hud/base.cpp
    hud/button.cpp
    hud/char_icon.cpp
    hud/character.cpp
    hud/class.cpp
    hud/console.cpp
    hud/edit_box.cpp
    hud/grid.cpp
    hud/hud.cpp
    hud/inventory.cpp
    hud/layer.cpp
    hud/main_panel.cpp
    hud/multi_player_menu.cpp
    hud/options.cpp
    hud/stats.cpp
    hud/target_info.cpp
    hud/weapon.cpp
    hud/widget.cpp)

set(HEADERS_freeft_io
    io/controller.h
    io/game_loop.h
    io/loop.h
    io/main_menu_loop.h
)

set(SOURCES_freeft_io
    io/controller.cpp
    io/game_loop.cpp
    io/loop.cpp
    io/main_menu_loop.cpp
)

set(HEADERS_freeft_net
    net/base.h
    net/chunk.h
    net/client.h
    net/host.h
    net/server.h
    net/socket.h
)

set(SOURCES_freeft_net
    net/base.cpp
    net/chunk.cpp
    net/client.cpp
    net/host.cpp
    net/server.cpp
    net/socket.cpp
)

set(HEADERS_freeft_sys
    sys/aligned_allocator.h
    sys/config.h
    sys/data_sheet.h
    sys/gfx_device.h
)

set(SOURCES_freeft_sys
    sys/config.cpp
    sys/data_sheet.cpp
    sys/gfx_device.cpp
)

set(HEADERS_freeft_ui
    ui/button.h
    ui/combo_box.h
    ui/edit_box.h
    ui/file_dialog.h
    ui/image_button.h
    ui/list_box.h
    ui/loading_bar.h
    ui/message_box.h
    ui/progress_bar.h
    ui/text_box.h
    ui/tile_list.h
    ui/window.h
)

set(SOURCES_freeft_ui
    ui/button.cpp
    ui/combo_box.cpp
    ui/edit_box.cpp
    ui/file_dialog.cpp
    ui/image_button.cpp
    ui/list_box.cpp
    ui/loading_bar.cpp
    ui/message_box.cpp
    ui/progress_bar.cpp
    ui/text_box.cpp
    ui/tile_list.cpp
    ui/window.cpp
)

function(freeft_add_module TARGET_NAME MODULE_NAME)
	set(_SOURCES ${SOURCES_${MODULE_NAME}})
	list(TRANSFORM _SOURCES PREPEND "src/")
	set(_HEADERS ${HEADERS_${MODULE_NAME}})
	list(TRANSFORM _HEADERS PREPEND "src/")

    string(REGEX REPLACE "freeft_" "" GROUP_NAME ${MODULE_NAME})
	string(REGEX REPLACE "_" "/" GROUP_NAME ${GROUP_NAME})
	source_group("${GROUP_NAME}" FILES ${_SOURCES} ${_HEADERS})
	target_sources(${TARGET_NAME} PRIVATE ${_SOURCES} ${_HEADERS})
    target_link_libraries(${TARGET_NAME} PRIVATE libfwk)
	set_source_files_properties(${_SOURCES} PROPERTIES UNITY_GROUP "${MODULE_NAME}")
endfunction()

function(freeft_add_executable TARGET_NAME SOURCE_FILE)
    set(_LIBS ${LIBS_${TARGET_NAME}})
    add_executable(${TARGET_NAME} src/${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE ${_LIBS})

    set(_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${_BASE_DIR}")
    set_target_properties(${TARGET_NAME} PROPERTIES
        DEBUG_POSTFIX "_dbg" DEVELOP_POSTFIX "_dev"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${_BASE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEVELOP "${_BASE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${_BASE_DIR}"
    )
endfunction()

include_directories(src/)

add_library(freeft_base)
freeft_add_module(freeft_base freeft_base)

add_library(freeft_audio)
freeft_add_module(freeft_audio freeft_audio)

add_library(freeft_editor)
freeft_add_module(freeft_editor freeft_editor)

add_library(freeft_game)
freeft_add_module(freeft_game freeft_game)
freeft_add_module(freeft_game freeft_game_orders)

add_library(freeft_gfx)
freeft_add_module(freeft_gfx freeft_gfx)

add_library(freeft_hud)
freeft_add_module(freeft_hud freeft_hud)

add_library(freeft_io)
freeft_add_module(freeft_io freeft_io)

add_library(freeft_net)
freeft_add_module(freeft_net freeft_net)

add_library(freeft_sys)
freeft_add_module(freeft_sys freeft_sys)

add_library(freeft_ui)
freeft_add_module(freeft_ui freeft_ui)

if(WIN32)
    set(OPENAL_LIB OpenAL32)
    set(CRYPTO_LIBS )
else()
    set(OPENAL_LIB openal)
    find_package(OpenSSL REQUIRED)
    set(CRYPTO_LIBS OpenSSL::SSL OpenSSL::Crypto)
endif()

set(LIBS_freeft freeft_io freeft_net freeft_hud freeft_ui freeft_game
    freeft_gfx freeft_audio freeft_sys freeft_base libfwk
    ${OPENAL_LIB} mpg123 vorbisfile vorbis ogg)
freeft_add_executable(freeft game.cpp)

set(LIBS_convert freeft_io freeft_game freeft_gfx freeft_sys freeft_base
     zip ${CRYPTO_LIBS} libfwk)
freeft_add_executable(convert convert.cpp)

if(WIN32)
	target_link_libraries(freeft PRIVATE shlwapi ws2_32)
endif()

#ifdef FWK_PLATFORM_WINDOWS
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "mpg123.lib")
#endif
