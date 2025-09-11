# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(LVGL9.3_BASICS C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add subderectoruies
add_subdirectory(lvgl)

# Add executable. Default name is the project name, version 0.1

add_executable(LVGL9.3_BASICS 
LVGL9.3_BASICS.c 
dev/lv_port.c
dev/st7789.c
Example/ex1.c
)

pico_set_program_name(LVGL9.3_BASICS "LVGL9.3_BASICS")
pico_set_program_version(LVGL9.3_BASICS "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(LVGL9.3_BASICS 0)
pico_enable_stdio_usb(LVGL9.3_BASICS 1)

# Add the standard library to the build
target_link_libraries(LVGL9.3_BASICS
        pico_stdlib
        hardware_spi
        hardware_dma
        lvgl
        )

# Add the standard include files to the build
target_include_directories(LVGL9.3_BASICS PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/dev
        ${CMAKE_CURRENT_LIST_DIR}/Example
)

pico_add_extra_outputs(LVGL9.3_BASICS)

