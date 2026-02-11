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
 * @ref config.c
 */
#include "config.h"

#include <stdio.h>

#include "hardware/watchdog.h"
#include "lwip/apps/sntp.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

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

extern void main_core1(void);

int main()
{
    stdio_init_all();

    init_unix_time();

    multicore_launch_core1(main_core1);

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

    cyw43_arch_lwip_begin();
    sntp_setservername(0, SNTP_SERVER_ADDRESS_0);
    sntp_setservername(1, SNTP_SERVER_ADDRESS_1);
    sntp_setservername(2, SNTP_SERVER_ADDRESS_2);
    sntp_setservername(3, SNTP_SERVER_ADDRESS_3);
    sntp_init();
    cyw43_arch_lwip_end();

    while (1)
    {
        setup_status();
        const microseconds_t us_up = time_us_64();
        const microseconds_t us_cur = get_unix_time();
        const microseconds_t us_sync = unix_time_get_last_sync();
        const microseconds_t us_since_last_sync = us_cur - us_sync;
        if (us_up > SLOW_BLINK_UPTIME)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (time_us_64() / 1000000) & 1);
        else
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (time_us_64() / 500000) & 1);
        status("Uptime:          %llu.%06llus\n", us_up / 1000000, us_up % 1000000);
        status("Last clock sync: %llu.%06llu (%llus ago)\n", us_sync / 1000000, us_sync % 1000000, us_since_last_sync / 1000000);
        status("Current:         %llu.%06llu\n", us_cur / 1000000, us_cur % 1000000);

        cyw43_arch_poll();

        if (time_us_64() > REBOOT_INTERVAL)
            die();
    }

    cyw43_arch_lwip_begin();
    sntp_stop();
    cyw43_arch_lwip_end();
    cyw43_arch_deinit();
}
