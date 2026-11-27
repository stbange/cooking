cmake_minimum_required(VERSION 3.22)
project(FatDuck VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(FATDUCK_COVERAGE "Enable gcov/lcov code coverage on fatduck_core" OFF)
option(FATDUCK_BUILD_APP "Build the SDL3 + OpenGL executable" ON)
option(FATDUCK_BUILD_TESTS "Build unit, science, and automated game tests" ON)

# 1. Vendored ANSI C Lua 5.4 library (identical across Linux, Windows, macOS, Web)
file(GLOB LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua/*.c")
add_library(lua54 STATIC ${LUA_SOURCES})
target_include_directories(lua54 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua")
if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    target_compile_definitions(lua54 PRIVATE LUA_USE_POSIX)
endif()

# 2. Vendored ANSI C Nuklear v4.13.4 immediate-mode GUI library (MIT / Public Domain)
add_library(nuklear STATIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nuklear/nuklear_impl.c")
target_include_directories(nuklear PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nuklear")
if(UNIX OR EMSCRIPTEN)
    target_link_libraries(nuklear PUBLIC m)
endif()

# 3. Core 3D Culinary Engine + Lua Host + VFS + 20-Language i18n + Nuklear UI (100% platform-independent)
set(FATDUCK_CORE_SOURCES
    src/core/fd_vfs.c
    src/core/fd_lua.c
    src/core/fd_i18n.c
    src/core/fd_geometry.c
    src/core/fd_physics.c
    src/core/fd_chemistry.c
    src/core/fd_biology.c
    src/core/fd_nutrition.c
    src/core/fd_sim.c
    src/core/fd_solver.c
    src/core/fd_ui.c
    src/core/fd_app.c
)

add_library(fatduck_core STATIC ${FATDUCK_CORE_SOURCES})
target_include_directories(fatduck_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/core")
target_link_libraries(fatduck_core PUBLIC lua54 nuklear)
if(UNIX OR EMSCRIPTEN)
    target_link_libraries(fatduck_core PUBLIC m)
endif()

if(MSVC)
    target_compile_options(fatduck_core PRIVATE /W4)
else()
    target_compile_options(fatduck_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(FATDUCK_COVERAGE AND NOT EMSCRIPTEN)
    target_compile_options(fatduck_core PUBLIC --coverage -O0 -g)
    target_link_options(fatduck_core PUBLIC --coverage)
endif()

# 4. SDL3 + OpenGL 3.3 Core / WebGL2 Application
if(FATDUCK_BUILD_APP)
    add_executable(fatduck
        src/platform/main.c
        src/platform/gl_renderer.c
        src/platform/audio_sdl.c
    )
    target_include_directories(fatduck PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb")
    target_link_libraries(fatduck PRIVATE fatduck_core)

    if(EMSCRIPTEN)
        set_target_properties(fatduck PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
        target_compile_options(fatduck PRIVATE "-sUSE_SDL=3")
        target_link_options(fatduck PRIVATE
            "-sUSE_SDL=3"
            "-sMIN_WEBGL_VERSION=2"
            "-sMAX_WEBGL_VERSION=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sSTACK_SIZE=1048576"
            "-sINITIAL_MEMORY=67108864"
            "-sEXPORTED_FUNCTIONS=['_main','_malloc','_free','_fd_web_push_asset','_fd_web_finish_boot','_fd_web_set_lang','_fd_web_get_state_json']"
            "-sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToNewUTF8','HEAPU8']"
            "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
        )
    else()
        find_package(SDL3 QUIET)
        if(NOT SDL3_FOUND)
            include(FetchContent)
            set(SDL_SHARED OFF CACHE BOOL "" FORCE)
            set(SDL_STATIC ON CACHE BOOL "" FORCE)
            set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
            set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
            FetchContent_Declare(
                SDL3
                GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
                GIT_TAG        release-3.4.20
                GIT_SHALLOW    TRUE
            )
            FetchContent_MakeAvailable(SDL3)
        endif()
        target_link_libraries(fatduck PRIVATE SDL3::SDL3)

        if(WIN32)
            target_link_libraries(fatduck PRIVATE opengl32)
        elseif(APPLE)
            find_library(OPENGL_LIBRARY OpenGL REQUIRED)
            target_link_libraries(fatduck PRIVATE ${OPENGL_LIBRARY})
        else()
            find_package(OpenGL REQUIRED)
            target_link_libraries(fatduck PRIVATE OpenGL::GL)
        endif()
    endif()

    # Stage unbaked ./assets/ folder beside the built executable / index.html
    add_custom_command(TARGET fatduck POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:fatduck>/assets"
        COMMENT "Staging external ./assets/ directory beside binary"
    )
endif()

# 5. Unit, Scientific Verification & Automated Game Tests
if(FATDUCK_BUILD_TESTS AND NOT EMSCRIPTEN)
    enable_testing()

    add_executable(test_core_unit tests/unit/test_core_unit.c)
    target_link_libraries(test_core_unit PRIVATE fatduck_core)
    add_test(NAME core_unit_tests COMMAND test_core_unit WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    add_executable(test_first_principles tests/science/test_first_principles.c)
    target_link_libraries(test_first_principles PRIVATE fatduck_core)
    add_test(NAME science_3d_benchmarks COMMAND test_first_principles WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    add_executable(test_game_replay tests/game/test_game_replay.c)
    target_link_libraries(test_game_replay PRIVATE fatduck_core)
    add_test(NAME headless_game_replay COMMAND test_game_replay WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
