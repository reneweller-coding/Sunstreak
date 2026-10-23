# Sunstreak -- an arcade racer in the spirit of Lotus III: The Ultimate Challenge, with an endless mode on
# generated tracks. C++20 on raylib (fetched at configure time), for Windows and Linux (Steam Deck).
cmake_minimum_required( VERSION 3.22 )   # 3.22: Ubuntu 22.04, the Steam Deck's build box

# raylib 5.5 and its GLFW still ask for old CMake versions; CMake 4 refuses those without this floor. The static
# runtime must reach raylib too, whose cmake_minimum_required leaves CMP0091 unset.
set( CMAKE_POLICY_VERSION_MINIMUM 3.5 )
set( CMAKE_POLICY_DEFAULT_CMP0091 NEW )
set( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )

project( Sunstreak VERSION 0.1.0 LANGUAGES C CXX )

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( SUNSTREAK_BIN_DIR "" CACHE PATH "where the game is copied after each build (bin/<preset>)" )

include( FetchContent )
set( SUNSTREAK_EXTRACT_TIMESTAMP "" )
if( CMAKE_VERSION VERSION_GREATER_EQUAL 3.24 )
    set( SUNSTREAK_EXTRACT_TIMESTAMP DOWNLOAD_EXTRACT_TIMESTAMP TRUE )
endif()
set( BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )
set( BUILD_GAMES OFF CACHE BOOL "" FORCE )
set( BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE )
FetchContent_Declare( raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    ${SUNSTREAK_EXTRACT_TIMESTAMP} )
FetchContent_MakeAvailable( raylib )
# FLAC for MY MUSIC (raylib leaves it out by default; dr_flac is in its tree).
target_compile_definitions( raylib PRIVATE SUPPORT_FILEFORMAT_FLAC=1 )

# SDL2 only for the gamepad's rumble (GLFW cannot vibrate): its game controller subsystem, no video, no sound.
set( SDL_SHARED OFF CACHE BOOL "" FORCE )
set( SDL_STATIC ON CACHE BOOL "" FORCE )
set( SDL_TEST OFF CACHE BOOL "" FORCE )
set( SDL_TESTS OFF CACHE BOOL "" FORCE )
set( SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE )
set( SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE )
set( SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE )
set( SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE )
foreach( off SDL_AUDIO SDL_VIDEO SDL_RENDER SDL_POWER SDL_LOCALE SDL_MISC SDL_FILESYSTEM )
    set( ${off} OFF CACHE BOOL "" FORCE )
endforeach()
FetchContent_Declare( SDL2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz
    ${SUNSTREAK_EXTRACT_TIMESTAMP} )
FetchContent_MakeAvailable( SDL2 )

# Everything but main() is a library, so the tests link the same code the game runs.
file( GLOB SUNSTREAK_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h )
list( FILTER SUNSTREAK_SOURCES EXCLUDE REGEX ".*/main\\.cpp$" )
add_library( sunstreak_core STATIC ${SUNSTREAK_SOURCES} )
target_include_directories( sunstreak_core PUBLIC ${CMAKE_SOURCE_DIR}/src )
target_link_libraries( sunstreak_core PUBLIC raylib SDL2::SDL2-static )
target_compile_definitions( sunstreak_core PUBLIC SUNSTREAK_VERSION="${PROJECT_VERSION}" )
if( MSVC )
    target_compile_options( sunstreak_core PUBLIC /W4 /permissive- /utf-8 /wd4244 /wd4267 /wd4305 )
    target_compile_definitions( sunstreak_core PUBLIC _CRT_SECURE_NO_WARNINGS )
else()
    target_compile_options( sunstreak_core PUBLIC -Wall -Wextra -Wno-missing-field-initializers )
    # GCC 11's SLP vectorizer miscompiles filter cascades at -O3 (seen in the VRAudio family); GCC 12 is fine.
    if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12 )
        target_compile_options( sunstreak_core PUBLIC -fno-tree-slp-vectorize )
    endif()
endif()

add_executable( Sunstreak WIN32 src/main.cpp )
target_link_libraries( Sunstreak PRIVATE sunstreak_core )
if( MSVC )
    # A window program without a console that still starts in main().
    target_link_options( Sunstreak PRIVATE /ENTRY:mainCRTStartup )
else()
    # The C++ runtime inside, so the binary runs on SteamOS and other distributions without matching libstdc++.
    target_link_options( Sunstreak PRIVATE -static-libstdc++ -static-libgcc )
endif()

if( SUNSTREAK_BIN_DIR )
    add_custom_command( TARGET Sunstreak POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SUNSTREAK_BIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Sunstreak> "${SUNSTREAK_BIN_DIR}" )
endif()

enable_testing()
add_executable( SunstreakTests tests/TestMain.cpp )
target_link_libraries( SunstreakTests PRIVATE sunstreak_core )
add_test( NAME core COMMAND SunstreakTests )
