# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: Copyright (c) 2026 Ian Hangartner <icrashstuff at outlook dot com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
cmake_minimum_required(VERSION 3.13...4.0)

include(pico_sdk_import.cmake)

project(my_project C CXX ASM)

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

pico_sdk_init()

add_executable(pico-light-switch
    main.c
    main_core1.c
    unix_time.c
    actuator.c
)
target_include_directories(pico-light-switch PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(pico-light-switch PUBLIC
    PICO_INCLUDE_RTC_DATETIME=0
    PICO_DEBUG_MALLOC=1
)


set(WIFI_SSID     "" CACHE STRING "Wifi SSID")
set(WIFI_PASSWORD "" CACHE STRING "Wifi password")
set(WIFI_AUTH_MODE CYW43_AUTH_WPA2_AES_PSK CACHE STRING "Wifi authorization type (A CYW43_AUTH_* value)")

target_compile_definitions(pico-light-switch PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    WIFI_AUTH_MODE=${WIFI_AUTH_MODE}
)

target_compile_options(pico-light-switch PUBLIC -Wall -Wextra -Wshadow)
target_compile_definitions(pico-light-switch PUBLIC -DUSBD_MANUFACTURER="Ian Hangartner")
target_compile_definitions(pico-light-switch PUBLIC -DUSBD_PRODUCT="pico-light-switch")
target_link_options(pico-light-switch PRIVATE "-Wl,--print-memory-usage")

target_link_libraries(pico-light-switch PRIVATE
    pico_cyw43_arch_lwip_poll
    pico_lwip_sntp

    pico_stdlib
    pico_malloc
    pico_multicore
    hardware_watchdog
)


if (TARGET tinyusb_device)
    pico_enable_stdio_usb(pico-light-switch 1)
    pico_enable_stdio_uart(pico-light-switch 1)
else()
   message(FATAL_ERROR "USB not enabled!")
endif()

pico_add_extra_outputs(pico-light-switch)
