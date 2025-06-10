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
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(RP2040_LVGL_ST7789_2.8 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add the LVGL library
add_subdirectory(lvgl)
add_subdirectory(ui)
# Add executable. Default name is the project name, version 0.1

add_executable(RP2040_LVGL_ST7789_2.8 
RP2040_LVGL_ST7789_2.8.c 
src/st7789.c
src/xpt2046.c
src/lv_port_disp.c
src/lv_port_indev.c
src/main_screen.c
)

pico_set_program_name(RP2040_LVGL_ST7789_2.8 "RP2040_LVGL_ST7789_2.8")
pico_set_program_version(RP2040_LVGL_ST7789_2.8 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(RP2040_LVGL_ST7789_2.8 0)
pico_enable_stdio_usb(RP2040_LVGL_ST7789_2.8 1)

# Add the standard library to the build
target_link_libraries(RP2040_LVGL_ST7789_2.8
        pico_stdlib
        pico_multicore                        
        hardware_spi
        hardware_gpio
        hardware_dma
        hardware_uart
        hardware_sync       
        lvgl::lvgl
        lvgl::demos
        lvgl::examples
        ui::ui
        )

# Add the standard include files to the build
target_include_directories(RP2040_LVGL_ST7789_2.8 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/inc
)

pico_add_extra_outputs(RP2040_LVGL_ST7789_2.8)

