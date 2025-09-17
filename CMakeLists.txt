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
set(PICO_FLASH_SIZE_BYTES "16 * 1024 * 1024")


# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(FatFS_second C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


#add lvgl from userhome
add_subdirectory(${USERHOME}/lvgl lvgl)

# Add executable. Default name is the project name, version 0.1

add_executable(FatFS_second 
FatFS_second.c
dev/lv_port.c
dev/st7789.c
fatfs/ff.c
fatfs/diskio.c
#fatfs/ffsystem.c
fatfs/ffunicode.c
#fatfs/usb_descriptors.c
msc/usb_descriptors.c
msc/Tiny_usb_msc_callback.c
)

pico_set_program_name(FatFS_second "FatFS_second")
pico_set_program_version(FatFS_second "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(FatFS_second 0)
pico_enable_stdio_usb(FatFS_second 0)

# Add the standard library to the build
target_link_libraries(FatFS_second
        pico_stdlib
        hardware_spi
        hardware_dma
        hardware_flash
        #hardware_uart
        tinyusb_device
        tinyusb_board
        #dev_library
        lvgl
        lvgl::demos
        lvgl::examples
        )

# Add the standard include files to the build
target_include_directories(FatFS_second PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}  
        ${CMAKE_CURRENT_LIST_DIR}/dev  
        ${CMAKE_CURRENT_LIST_DIR}/fatfs  
        ${CMAKE_CURRENT_LIST_DIR}/msc  
)

pico_add_extra_outputs(FatFS_second)

