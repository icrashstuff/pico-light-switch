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
 * - An RP2350 based board\n
 * - Linear actuator(s)\n
 * - Relay boards\n
 * - 20x4 LCD display driven by HD4478U interfaced to a PCF8574 i2c backpack\n
 *
 * @par Parts I personally used
 * - FD17 Linear actuator (FD17-12-15-110.160-60)\n
 * - Waveshare RP2350-Relay-6CH (https://www.waveshare.com/rp2350-relay-6ch.htm?sku=32576)\n
 * - Black on Green 20x4 LCD with PCF8574 backpack
 *
 * @par Configuration
 * @ref config.c
 */
#include "config.h"

#include "license_texts.h"

#include <stdio.h>

#include "hardware/i2c.h"
#include "hardware/watchdog.h"
#include "lwip/apps/sntp.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "ftime.h"
#include "loop_measurer.h"
#include "unix_time.h"

#define LOG(fmt, ...) printf("Core %u: " fmt, get_core_num(), ##__VA_ARGS__)

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

void dump_program_info()
{
    char fbuf[128];
#define FBUF(X) fbuf##X, sizeof(fbuf##X)
    LOG("===> Program license\n%s\n", license_program);
    LOG("===> Pico SDK license\n%s\n", license_pico_SDK);
    LOG("===> CYW43 license\n%s\n", license_CYW43);
    LOG("===> Program info\n");
    LOG("Program name:  pico-light-switch\n");
    LOG("Compile date:  %s %s\n", __DATE__, __TIME__);
    LOG("RELAY_BOARD:   %s\n", CMAKE_RELAY_BOARD);
    LOG("PICO_BOARD:    %s\n", CMAKE_PICO_BOARD);
    LOG("PICO_PLATFORM: %s\n", CMAKE_PICO_PLATFORM);
    putc('\n', stdout);
    LOG("===> USB config\n");
    LOG("Manufacturer: '%s'\n", USBD_MANUFACTURER);
    LOG("Product:      '%s'\n", USBD_PRODUCT);
    putc('\n', stdout);
    LOG("===> WiFi config\n");
    LOG("Primary SSID:  '%s'\n", WIFI_PRIMARY_SSID);
    LOG("Primary PASS:  '%s'\n", WIFI_PRIMARY_PASSWORD);
    LOG("Primary AUTH:  0x%08x (%s)\n", WIFI_PRIMARY_AUTH_MODE, WIFI_PRIMARY_AUTH_MODE_STR);
    LOG("Fallback SSID: '%s'\n", WIFI_FALLBACK_SSID);
    LOG("Fallback PASS: '%s'\n", WIFI_FALLBACK_PASSWORD);
    LOG("Fallback AUTH: 0x%08x (%s)\n", WIFI_FALLBACK_AUTH_MODE, WIFI_FALLBACK_AUTH_MODE_STR);
    LOG("Country:       %s\n", WIFI_COUNTRY_CODE_STR);
    putc('\n', stdout);
    LOG("===> Actuator config\n");
    LOG("Travel time: %s\n", fdelta_us(ACTUATOR_TRAVEL_TIME, FBUF()));
    LOG("Rest time:   %s\n", fdelta_us(ACTUATOR_REST_TIME, FBUF()));
    LOG("GPIO 'ON'  Extend:  %d\n", ACTUATOR_GPIO_ACT_ON_EXTEND);
    LOG("GPIO 'ON'  Retract: %d\n", ACTUATOR_GPIO_ACT_ON_RETRACT);
    LOG("GPIO 'OFF' Extend:  %d\n", ACTUATOR_GPIO_ACT_OFF_EXTEND);
    LOG("GPIO 'OFF' Retract: %d\n", ACTUATOR_GPIO_ACT_OFF_RETRACT);
    LOG("Active logic level extend:  %s\n", (ACTUATOR_ACTIVE_LOGIC_LEVEL_EXTEND) ? "HIGH" : "LOW");
    LOG("Active logic level retract: %s\n", (ACTUATOR_ACTIVE_LOGIC_LEVEL_RETRACT) ? "HIGH" : "LOW");
    putc('\n', stdout);
    LOG("===> Schedule config\n");
    LOG("Select pin: %d\n", SCHEDULE_SELECT_PIN);
    LOG("Region trigger duration: %s\n", fdelta(SCHEDULE_TRIGGER_REGION_LENGTH, FBUF()));
    LOG("Trigger on reset if in 'ON'  region: %d\n", SCHEDULE_TRIGGER_REGION_ON_RESET_IF_IN_ON_REGION);
    LOG("Trigger on reset if in 'OFF' region: %d\n", SCHEDULE_TRIGGER_REGION_ON_RESET_IF_IN_OFF_REGION);
    putc('\n', stdout);
    LOG("===> Timezone config\n");
    LOG("Offset Daylight Time: %s\n", fdelta(TIMEZONE_OFFSET_DT, FBUF()));
    LOG("Offset Standard Time: %s\n", fdelta(TIMEZONE_OFFSET_ST, FBUF()));
    putc('\n', stdout);
    LOG("===> LCD config\n");
    LOG("i2c instance:  %d\n", (STATUS_LCD_I2C_INSTANCE == i2c0) ? 0 : 1);
    LOG("i2c address:   %02x (%d)\n", STATUS_LCD_I2C_ADDRESS, STATUS_LCD_I2C_ADDRESS);
    LOG("i2c SDL GPIO:  %d\n", STATUS_LCD_I2C_SDA_PIN);
    LOG("i2c SCL GPIO:  %d\n", STATUS_LCD_I2C_SCL_PIN);
    LOG("Page interval: %s\n", fdelta_us(((uint64_t)STATUS_LCD_INTERVALS_PER_PAGE) * STATUS_PRINT_INTERVAL, FBUF()));
    putc('\n', stdout);
    LOG("===> Miscellaneous config\n");
#ifdef WS2812_STATUS_GPIO
    LOG("WS2812 Status GPIO: %d\n", WS2812_STATUS_GPIO);
#else
    LOG("WS2812 Status GPIO: n/a\n");
#endif
    LOG("WS2812 Status Heartbeat Period: %d\n", WS2812_STATUS_HEARTBEAT_PERIOD);
    LOG("USB STDIO wait time: %s\n", fdelta_us(MAX_WAIT_USB_STDIO, FBUF()));
    LOG("Loop averaging sample count: %d\n", LOOP_AVERAGE_SAMPLE_COUNT);
    LOG("Automatic reboot interval: %s\n", fdelta(AUTOMATIC_REBOOT_INTERVAL, FBUF()));
    LOG("Automatic reboot minimum distance to region: %s\n", fdelta(AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION, FBUF()));
}

int main()
{
    gpio_init(SCHEDULE_SELECT_PIN);
    gpio_set_dir(SCHEDULE_SELECT_PIN, GPIO_IN);
    gpio_pull_up(SCHEDULE_SELECT_PIN);

    gpio_init(I2C_POWER_INTERRUPT_GPIO);
    gpio_set_dir(I2C_POWER_INTERRUPT_GPIO, GPIO_OUT);
    gpio_put(I2C_POWER_INTERRUPT_GPIO, I2C_POWER_INTERRUPT_ACTIVE_LOGIC_LEVEL);

    stdio_init_all();
    absolute_time_t time_usb_connect_timeout = make_timeout_time_us(time_us_64() + MAX_WAIT_USB_STDIO);

    gpio_put(I2C_POWER_INTERRUPT_GPIO, I2C_POWER_INTERRUPT_ACTIVE_LOGIC_LEVEL);
    sleep_us(I2C_POWER_WAIT_TIME);
    gpio_put(I2C_POWER_INTERRUPT_GPIO, !I2C_POWER_INTERRUPT_ACTIVE_LOGIC_LEVEL);
    sleep_us(I2C_POWER_WAIT_TIME);

    while (!stdio_usb_connected() && !time_reached(time_usb_connect_timeout))
        sleep_ms(5);

    printf("\n\n");
    LOG("Begin boot\n");
    dump_program_info();
    printf("\n\n");

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

        loop_measure_end_loop(&core0_loop_measure);
    }

    cyw43_arch_lwip_begin();
    sntp_stop();
    cyw43_arch_lwip_end();
    cyw43_arch_deinit();
}
