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
FetchContent_Declare(
    tinygl
    GIT_REPOSITORY https://github.com/meadiode/tinygl_1b
    GIT_TAG main
)
FetchContent_MakeAvailable(tinygl)
target_include_directories(pico_oled PRIVATE ${tinygl_SOURCE_DIR}/include)
file(GLOB TINYGL_SOURCES ${tinygl_SOURCE_DIR}/src/*.c)
target_sources(pico_oled PRIVATE ${TINYGL_SOURCES})

# Dependency - U8G2
FetchContent_Declare(
    u8g2
    GIT_REPOSITORY https://github.com/olikraus/u8g2.git
    GIT_TAG master
)
FetchContent_MakeAvailable(u8g2)
# This is a bit odd, as U8G2 is a CMake project, but if you just link it in `target_link_libraries`, the linker will complain with:
#  > pico_oled.elf section `.rodata' will not fit in region `FLASH'
#  > region `FLASH' overflowed by 11419420 bytes
# i.e. the resulting binary is too big. And no, LTO doesn't help.
# The solution is to add the U8G2 sources directly into the project, and not link it as a library.
target_include_directories(pico_oled PRIVATE ${u8g2_SOURCE_DIR}/csrc)
file(GLOB U8G2_SOURCES ${u8g2_SOURCE_DIR}/csrc/*.c)
target_sources(pico_oled PRIVATE ${U8G2_SOURCES})

# Definitions
#  - Enable bit-render in tinygl
target_compile_definitions(pico_oled PRIVATE TGL_FEATURE_RENDER_BITS=1)

target_link_libraries(pico_oled pico_stdlib pico_stdio_rtt hardware_spi)