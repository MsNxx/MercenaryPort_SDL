cmake_minimum_required(VERSION 3.20)
project(MercenaryPort
	VERSION 0.0.1
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 QUIET CONFIG)
set(MERCPORT_SDL2_TARGET SDL2::SDL2)
if(NOT SDL2_FOUND)
	message(STATUS "System SDL2 not found -- fetching and building from source")
	set(SDL_SHARED OFF CACHE BOOL "" FORCE)
	set(SDL_STATIC ON  CACHE BOOL "" FORCE)
	set(SDL_TEST   OFF CACHE BOOL "" FORCE)
	include(FetchContent)
	FetchContent_Declare(SDL2
		GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
		GIT_TAG        release-2.30.11
		GIT_SHALLOW    TRUE
	)
	FetchContent_MakeAvailable(SDL2)
	set(MERCPORT_SDL2_TARGET SDL2::SDL2-static)
endif()

add_executable(MercenaryPort
	src/main.cpp
	src/audio/Audio.cpp
	src/game/Game.cpp
	src/game/Combat.cpp
	src/game/Controls.cpp
	src/game/VblHandler.cpp
	src/game/Intro.cpp
	src/game/Outro.cpp
	src/game/ScriptVm.cpp
	src/game/Benson.cpp
	src/game/Camera.cpp
	src/game/CameraGeometry.cpp
	src/game/CameraFlight.cpp
	src/game/TileDetail.cpp
	src/game/Objects.cpp
	src/game/Interior.cpp
	src/game/Workspace.cpp
	src/game/SaveLoad.cpp
	src/renderer/StarField.cpp
	src/renderer/LogMath.cpp
	src/renderer/PlanetDisc.cpp
	src/renderer/Fill.cpp
	src/renderer/Projection.cpp
	src/renderer/LineDraw.cpp
	src/renderer/Roads.cpp
	src/renderer/Hud.cpp
	src/renderer/Palette.cpp
	src/data/Gen_E0Data.cpp
	src/data/Gen_E1Data.cpp
	src/data/Gen_E2Data.cpp
	src/data/Gen_E3Data.cpp
	src/data/Gen_E4Data.cpp
	src/data/Gen_LoaderData.cpp
	src/data/Gen_MData.cpp
	src/data/Gen_S0Data.cpp
	src/data/Gen_S1Data.cpp
	src/data/Gen_S2Data.cpp
	src/data/Gen_S3Data.cpp
	src/data/Gen_S4Data.cpp
	src/data/Gen_SecondLoaderData.cpp
)

target_include_directories(MercenaryPort PRIVATE
	${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(MercenaryPort PRIVATE
	${MERCPORT_SDL2_TARGET}
	SDL2::SDL2main
)

if(MSVC)
	target_compile_options(MercenaryPort PRIVATE /W4)
else()
	target_compile_options(MercenaryPort PRIVATE
		-Wall -Wextra -Wpedantic
	)
endif()

if(APPLE)
	set_target_properties(MercenaryPort PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_BUNDLE_NAME "Mercenary"
		MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
		MACOSX_BUNDLE_GUI_IDENTIFIER "org.mercenaryport.mercenary"
	)
endif()

if(WIN32)
	set_target_properties(MercenaryPort PROPERTIES
		WIN32_EXECUTABLE TRUE
	)
endif()
