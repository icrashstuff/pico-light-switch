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
 * - Linear actuator(s)\n
 * - Relay boards\n
 *
 * @par Parts I personally used
 * - Raspberry Pi Pico 2 W\n
 * - FD17 Linear actuator (FD17-12-15-110.160-60)\n
 * - Waveshare Pico-Relay-B (https://www.waveshare.com/pico-relay-b.htm)\n
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

#include "loop_measurer.h"
#include "unix_time.h"

#define LOG(fmt, ...) printf("Core 0: " fmt, ##__VA_ARGS__)

[[noreturn]] void die(void)
{
    const microseconds_t us_up = time_us_64();
    LOG("die() called @ %llu.%06llus\n", us_up / 1000000, us_up % 1000000);
    fflush(stdout);
    watchdog_enable(0, false);
    while (1)
        tight_loop_contents();
}

/* Defined in void main_core1.c */
extern uint32_t core0_connection_attempt;
extern bool core0_connected;
extern void main_core1(void);

loop_measure_t core0_loop_measure;
loop_measure_t core1_loop_measure;

#define arraysize(X) (sizeof(X) / sizeof(*(X)))

static bool connect_to_network()
{
    const char* ssids[] = { WIFI_PRIMARY_SSID, WIFI_FALLBACK_SSID };
    const char* passwords[] = { WIFI_PRIMARY_PASSWORD, WIFI_FALLBACK_PASSWORD };
    const uint32_t authmodes[] = { WIFI_PRIMARY_AUTH_MODE, WIFI_FALLBACK_AUTH_MODE };

    const uint64_t connection_timeouts[] = { 7500, 15000, 30000 };

    for (const uint64_t* conn_timeout = connection_timeouts; conn_timeout < connection_timeouts + arraysize(connection_timeouts); conn_timeout++)
    {
        for (size_t j = 0; j < arraysize(ssids); j++)
        {
            if (*ssids[j] == 0)
                continue;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN));
            LOG("Attempting to connecting to SSID: '%s', timeout=%llums, authmode=0x%08x\n", ssids[j], *conn_timeout, authmodes[j]);
            if (cyw43_arch_wifi_connect_timeout_ms(ssids[j], *passwords[j] != '\0' ? passwords[j] : NULL, authmodes[j], *conn_timeout))
                LOG("Failed to connect to SSID: '%s'!\n", ssids[j]);
            else
                return true;
            core0_connection_attempt++;
        }
    }

    return false;
}

int main()
{
    stdio_init_all();
    while (!stdio_usb_connected() && time_us_64() < MAX_WAIT_USB_STDIO)
        sleep_ms(5);

    printf("\n\nBegin boot\npico-light-switch %s %s\n\n", __DATE__, __TIME__);

    core0_loop_measure = loop_measure_init();
    core1_loop_measure = loop_measure_init();

    LOG("Initializing unix time\n");
    init_unix_time();

    LOG("Launching core 1\n");
    multicore_launch_core1(main_core1);

    LOG("Initializing cyw43_arch for country %s\n", WIFI_COUNTRY_CODE_STR);
    if (cyw43_arch_init_with_country(WIFI_COUNTRY_CODE))
    {
        LOG("Failed to initialise cyw43_arch!\n");
        die();
    }
    /* Activate status led after cyw43 init so there is some indication the pico is online */
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    cyw43_arch_enable_sta_mode();

    if (!connect_to_network())
    {
        LOG("Failed to connect to a network, resetting!\n");
        die();
    }
    core0_connected = 1;

    LOG("Initializing SNTP\n");
    cyw43_arch_lwip_begin();
    sntp_setservername(0, SNTP_SERVER_ADDRESS_0);
    sntp_setservername(1, SNTP_SERVER_ADDRESS_1);
    sntp_setservername(2, SNTP_SERVER_ADDRESS_2);
    sntp_setservername(3, SNTP_SERVER_ADDRESS_3);
    sntp_init();
    cyw43_arch_lwip_end();

    LOG("Setup done, beginning loop\n");
    while (1)
    {
        /* Fast blink the status led while waiting for clock sync */
        if (unix_time_get_last_sync() != 0)
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (time_us_64() / 750000) & 1);
        else
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, (time_us_64() / 250000) & 1);

        cyw43_arch_poll();

        if (time_us_64() > REBOOT_INTERVAL)
            die();
        loop_measure_end_loop(&core0_loop_measure);
    }

    cyw43_arch_lwip_begin();
    sntp_stop();
    cyw43_arch_lwip_end();
    cyw43_arch_deinit();
}
