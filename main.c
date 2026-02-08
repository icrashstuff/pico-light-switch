/**
 * pico-light-switch - TODO
 *
 * @file
 * @copyright
 * @parblock
 * SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: Copyright (c) 2026 Ian Hangartner <icrashstuff at outlook dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * @endparblock
 *
 * @brief Entry point of pico-light-switch
 */
/**
 * @mainpage
 * TODO
 *
 * @par Parts required
 * - Raspberry Pi Pico 2 W\n
 * - A linear actuator
 *
 * @par Configuration
 * @ref main.c
 */
#include "config.h"

#include <stdio.h>

#include "hardware/watchdog.h"
#include "lwip/apps/sntp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "actuator.h"
#include "unix_time.h"

[[noreturn]] void die(void)
{
    watchdog_enable(0, false);
    while (1)
        tight_loop_contents();
}

static int status_impl_dummy_func(const char* fmt, va_list va)
{
    (void)fmt;
    (void)va;
    return 0;
}

static int (*status_impl)(const char* fmt, va_list vlist) = vprintf;

#if defined(__GNUC__) || defined(__clang__)
#define formatting_attribute(fmtargnumber) __attribute__((format(__printf__, fmtargnumber, fmtargnumber + 1)))
#endif
static formatting_attribute(1) int status(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int r = status_impl(fmt, args);
    va_end(args);
    return r;
}

static void setup_status()
{
    static uint64_t last_status_time = 0;
    const uint64_t loop_start_time = time_us_64();
    if (loop_start_time / STATUS_PRINT_INTERVAL != last_status_time / STATUS_PRINT_INTERVAL)
    {
        status_impl = vprintf;
        last_status_time = loop_start_time;
    }
    else
        status_impl = status_impl_dummy_func;
}

int main()
{
    stdio_init_all();

    init_unix_time();

    struct actuator_t actuator0;
    {
        struct actuator_config_t cinfo = {};
        cinfo.time_travel = 10 * 1000 * 1000;
        cinfo.time_rest = 500 * 1000;
        cinfo.gpio_extend = 18;
        cinfo.gpio_retract = 19;
        cinfo.logic_active_level_extend = 0;
        cinfo.logic_active_level_retract = 0;
        actuator_init(&actuator0, &cinfo);
    }

    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        die();
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, WIFI_AUTH_MODE, 30000))
    {
        printf("failed to connect\n");
        die();
    }

    watchdog_enable(8192, true);

    sntp_init();
    watchdog_update();

    while (actuator_in_cycle(&actuator0))
    {
        watchdog_update();
        setup_status();
        status("%llu\n", get_unix_time() / 1000000);
        actuator_poll(&actuator0);

        if (time_us_64() > REBOOT_INTERVAL)
            die();
    }

    watchdog_update();
    actuator_trigger(&actuator0);

    while (1)
    {
        watchdog_update();
        setup_status();
        const microseconds_t us = get_unix_time();
        status("%llu.%06llu\n", us / 1000000, us % 1000000);
        actuator_poll(&actuator0);

        if (time_us_64() > REBOOT_INTERVAL)
            die();
    }

    sntp_stop();
    cyw43_arch_deinit();
}
