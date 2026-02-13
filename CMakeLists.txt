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
    loop_measurer.c
)
target_include_directories(pico-light-switch PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(pico-light-switch PUBLIC
    PICO_INCLUDE_RTC_DATETIME=0
    PICO_DEBUG_MALLOC=1
)


set(WIFI_PRIMARY_SSID     "" CACHE STRING "Primary Wifi network SSID")
set(WIFI_PRIMARY_PASSWORD "" CACHE STRING "Primary Wifi network password")
set(WIFI_PRIMARY_AUTH_MODE CYW43_AUTH_WPA2_AES_PSK CACHE STRING "Primary Wifi network authorization type (A CYW43_AUTH_* value)")

# The reason for a fallback wifi network is for the following use case:
# Wifi network in production (primary) is different from the wifi network in testing (fallback)
set(WIFI_FALLBACK_SSID     "" CACHE STRING "Fallback network Wifi SSID")
set(WIFI_FALLBACK_PASSWORD "" CACHE STRING "Fallback network Wifi password")
set(WIFI_FALLBACK_AUTH_MODE CYW43_AUTH_WPA2_AES_PSK CACHE STRING "Fallback Wifi network authorization type (A CYW43_AUTH_* value)")

set(WIFI_COUNTRY_CODE "CYW43_COUNTRY_WORLDWIDE" CACHE STRING
    "Country code macro to use for cyw43 wifi (see $PICO_SDK_PATH/lib/cyw43-driver/src/cyw43_country.h)"
)

if("${WIFI_COUNTRY_CODE}" STREQUAL "CYW43_COUNTRY_WORLDWIDE")
    message(WARNING "Wifi country code set to worldwide")
endif()


target_compile_definitions(pico-light-switch PRIVATE
    WIFI_PRIMARY_SSID=\"${WIFI_PRIMARY_SSID}\"
    WIFI_PRIMARY_PASSWORD=\"${WIFI_PRIMARY_PASSWORD}\"
    WIFI_PRIMARY_AUTH_MODE=${WIFI_PRIMARY_AUTH_MODE}
    WIFI_FALLBACK_SSID=\"${WIFI_FALLBACK_SSID}\"
    WIFI_FALLBACK_PASSWORD=\"${WIFI_FALLBACK_PASSWORD}\"
    WIFI_FALLBACK_AUTH_MODE=${WIFI_FALLBACK_AUTH_MODE}
    WIFI_COUNTRY_CODE=${WIFI_COUNTRY_CODE}
    WIFI_COUNTRY_CODE_STR=\"${WIFI_COUNTRY_CODE}\"
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
