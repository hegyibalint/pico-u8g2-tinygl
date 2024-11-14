cmake_minimum_required(VERSION 3.30)
set(CMAKE_BUILD_TYPE Debug)

# Modules used
include(ExternalProject)
include(FetchContent)

# Enable colors on Ninja
if(CMAKE_GENERATOR MATCHES "Ninja")
    add_compile_options(-fdiagnostics-color=always)
endif()

# Settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Including and initializing the Raspberry Pico SDK
include($ENV{PICO_SDK}/pico_sdk_init.cmake)
pico_sdk_init()

# Base setup for the project
project(pico_oled)
file(GLOB SOURCES src/*.c)
add_executable(pico_oled ${SOURCES})

# Dependency - TinyGL
target_include_directories(pico_oled PRIVATE ${CMAKE_SOURCE_DIR}/deps/tinygl_1b/include)
file(GLOB TINYGL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/tinygl_1b/src/*.c)
target_sources(pico_oled PRIVATE ${TINYGL_SOURCES})

# This is a bit odd, as U8G2 is a CMake project, but if you just link it in `target_link_libraries`, the linker will complain with:
#  > pico_oled.elf section `.rodata' will not fit in region `FLASH'
#  > region `FLASH' overflowed by 11419420 bytes
# i.e. the resulting binary is too big. And no, LTO doesn't help.
# The solution is to add the U8G2 sources directly into the project, and not link it as a library.
target_include_directories(pico_oled PRIVATE ${CMAKE_SOURCE_DIR}/deps/u8g2/csrc)
file(GLOB U8G2_SOURCES ${CMAKE_SOURCE_DIR}/deps/u8g2/csrc/*.c)
target_sources(pico_oled PRIVATE ${U8G2_SOURCES})

# Definitions
#  - Enable bit-render in tinygl
target_compile_definitions(pico_oled PRIVATE TGL_FEATURE_RENDER_BITS=1)

target_link_libraries(pico_oled pico_stdlib pico_stdio_rtt hardware_spi)