cmake_minimum_required(VERSION 3.13)

# Pico SDK 2.x (RP2350 support). PICO_SDK_PATH from the environment, else the VS Code extension's copy.
if(NOT DEFINED ENV{PICO_SDK_PATH} AND EXISTS $ENV{HOME}/.pico-sdk/sdk/2.2.0)
    set(ENV{PICO_SDK_PATH} $ENV{HOME}/.pico-sdk/sdk/2.2.0)
endif()
set(PICO_BOARD rp569)
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards)

include(pico_sdk_import.cmake)

# Firmware version from version.yaml (major/minor by the user, patch by the pre-commit hook)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/version.yaml)
foreach(part major minor patch)
    file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/version.yaml line REGEX "^${part}:")
    string(REGEX REPLACE "^${part}: *([0-9]+).*" "\\1" FW_${part} "${line}")
endforeach()
set(FW_VERSION ${FW_major}.${FW_minor}.${FW_patch})
execute_process(COMMAND git describe --always --dirty --abbrev=7 WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                OUTPUT_VARIABLE FW_GIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT FW_GIT)
    set(FW_GIT unknown)
endif()
configure_file(src/fw_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/fw_version.h @ONLY)

project(rp569_rp2350 VERSION ${FW_VERSION} LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(rp569_rp2350 src/main.c)
target_link_libraries(rp569_rp2350 pico_stdlib pico_unique_id hardware_adc hardware_uart)
target_include_directories(rp569_rp2350 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/generated)

# USB strings: the host library finds this port by VID/PID 2e8a:0009 + product "rp569 RP2350B"
# (serial number = the chip's unique ID, as printed by 'version')
target_compile_definitions(rp569_rp2350 PRIVATE USBD_MANUFACTURER="rp569" USBD_PRODUCT="rp569 RP2350B")

# CLI on the USB serial port, mirrored on UART0 (RP EXT header pins 2/3)
pico_enable_stdio_usb(rp569_rp2350 1)
pico_enable_stdio_uart(rp569_rp2350 1)
pico_add_extra_outputs(rp569_rp2350)   # .uf2 for drag-and-drop in BOOTSEL / 'dfu' mode
