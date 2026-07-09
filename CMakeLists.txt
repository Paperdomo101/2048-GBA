cmake_minimum_required(VERSION 3.20...4.3 FATAL_ERROR)

# enforce GBA compilation
set(DKP_GBA_PLATFORM_LIBRARY "libtonc")
set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/cmake/GBA.cmake")

project(2048 VERSION 1.2 LANGUAGES C ASM)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type is set to ${CMAKE_BUILD_TYPE}.")

# exe
add_executable(2048
	source/first.c
	source/game.c
	source/global.h
	source/main.c
	source/over.c
	source/save.c
	source/state.c
	source/storage.c
	source/title.c
	source/win.c)
target_compile_features(2048 PRIVATE c_std_11)
target_compile_options(2048 PRIVATE
	-Wall -Wextra
	$<$<CONFIG:Release,RelWithDebInfo>:-mlong-calls -fomit-frame-pointer -ffast-math>)

# gfx
grit_add_binary_target(sprites graphics/sprites.png NO_MAP OPTIONS -Mh4 -Mw4)
grit_add_binary_target(score graphics/score.png NO_PALETTE NO_MAP)

grit_add_binary_target(bg0 graphics/bg0.png OPTIONS -mR4)
grit_add_binary_target(titlebg0 graphics/titlebg0.png DEPTH 8 OPTIONS -mLs -mR8)

grit_add_binary_target(gameover graphics/gameover.png NO_PALETTE NO_MAP OPTIONS -Mh4 -Mw8)
grit_add_binary_target(savescreen graphics/savescreen.png NO_PALETTE OPTIONS -mLs -mR4)
grit_add_binary_target(youwin graphics/youwin.png NO_PALETTE NO_MAP OPTIONS -Mh4 -Mw8)

dkp_add_embedded_binary_library(2048-graphics
	bg0
	gameover
	savescreen
	score
	sprites
	titlebg0
	youwin)

# sfx
mm_add_soundbank_target(soundbank
	HEADER "soundbank.h"
	INPUTS
		audio/big_merge.wav
		audio/lose.wav
		audio/merge.wav
		audio/save.wav
		audio/slide.wav
		audio/small_merge.wav
		audio/spawn.wav
		audio/start.wav
		audio/win.wav)
target_include_directories(2048 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
dkp_add_embedded_binary_library(2048-sounds soundbank)

# libs
add_library(maxmod STATIC IMPORTED)
set_target_properties(maxmod PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "$ENV{DEVKITPRO}/libgba/include"
	IMPORTED_LOCATION "$ENV{DEVKITPRO}/libgba/lib/libmm.a")
target_link_libraries(2048 PRIVATE
	2048-graphics 2048-sounds maxmod)

# debug
dkp_target_generate_symbol_list(2048)

# rom
gba_create_rom(2048
	TITLE "2048 GBA"
	GAMECODE "2048"
	MAKERCODE "BT")

# source code formatting
if(NOT ASTYLE_FOUND)
	find_program(ASTYLE_EXECUTABLE astyle)
	if(ASTYLE_EXECUTABLE)
		message(STATUS "Found source code formatter: astyle")
		set(ASTYLE_FOUND 1 CACHE INTERNAL "Astyle has been found")
	endif()
endif()
if(ASTYLE_EXECUTABLE)
	separate_arguments(ASTYLE_OPTIONS UNIX_COMMAND
		"--options=none --project=none --suffix=none --style=attach
		--indent=tab=4 --indent-switches --indent-preproc-block
		--min-conditional-indent=0 --squeeze-lines=1 --max-code-length=100
		--pad-oper --pad-comma --pad-header --unpad-paren
		--align-pointer=type --align-reference=name")
	add_custom_target(source-format
		COMMAND ${ASTYLE_EXECUTABLE} ${ASTYLE_OPTIONS}
			"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
			"${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
		COMMENT "Applying source code format..." VERBATIM)
endif()
