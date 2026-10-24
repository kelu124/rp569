cmake_minimum_required(VERSION 3.13)

# Pico SDK 2.x (RP2350 support). PICO_SDK_PATH from the environment, else the VS Code extension's copy.
if(NOT DEFINED ENV{PICO_SDK_PATH} AND EXISTS $ENV{HOME}/.pico-sdk/sdk/2.2.0)
    set(ENV{PICO_SDK_PATH} $ENV{HOME}/.pico-sdk/sdk/2.2.0)
endif()
set(PICO_BOARD rp569)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)

include(pico_sdk_import.cmake)
project(rp569_rp2350 C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(rp569_rp2350 src/main.c)
target_link_libraries(rp569_rp2350 pico_stdlib hardware_adc hardware_uart)
target_include_directories(rp569_rp2350 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

# CLI on the USB serial port, mirrored on UART0 (RP EXT header pins 2/3)
pico_enable_stdio_usb(rp569_rp2350 1)
pico_enable_stdio_uart(rp569_rp2350 1)
pico_add_extra_outputs(rp569_rp2350)   # .uf2 for drag-and-drop in BOOTSEL / 'dfu' mode
