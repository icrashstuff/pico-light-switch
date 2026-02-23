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
 * @brief Core 1 entry point of pico-light-switch
 */

#include "actuator.h"
#include "loop_measurer.h"
#include "schedules.h"
#include "unix_time.h"

#include "config.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "pico/time.h"

#include "ftime.h"

#include "ws2812.pio.h"

#define LOG(fmt, ...) printf("Core %u: " fmt, get_core_num(), ##__VA_ARGS__)

typedef struct
{
    bool on;
    bool in_region;

    /** Starting Timestamp (in seconds since 1970-01-01) of current region */
    uint64_t timestamp_region_start;
    /** Timestamp (in seconds since 1970-01-01) of the next "OFF" region */
    uint64_t timestamp_region_next_off;
    /** Timestamp (in seconds since 1970-01-01) of the next "ON" region */
    uint64_t timestamp_region_next_on;
} schedule_current_state_t;

schedule_current_state_t schedule_get_state(const schedule_t* const schedule)
{
    const microseconds_t epoch_time = (get_unix_time() / 1000000ull) - schedule->epoch;

    schedule_current_state_t r = { 0 };
    r.timestamp_region_next_off = UINT64_MAX;
    r.timestamp_region_next_on = UINT64_MAX;

    for (uint32_t i = 0; i < schedule->num_entries; i++)
    {
        if (schedule->entries[i].timestamp < epoch_time)
        {
            r.on = schedule->entries[i].on;
            r.timestamp_region_start = (uint64_t)(schedule->entries[i].timestamp) + schedule->epoch;
            if (epoch_time < (microseconds_t)(schedule->entries[i].timestamp + SCHEDULE_TRIGGER_REGION_LENGTH))
                r.in_region = 1;
        }
        else
        {
            if (schedule->entries[i].on)
            {
                if (r.timestamp_region_next_on == UINT64_MAX)
                    r.timestamp_region_next_on = (uint64_t)(schedule->entries[i].timestamp) + schedule->epoch;
            }
            else
            {
                if (r.timestamp_region_next_off == UINT64_MAX)
                    r.timestamp_region_next_off = (uint64_t)(schedule->entries[i].timestamp) + schedule->epoch;
            }
        }
    }

    return r;
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

/* Definition in main.c */
extern loop_measure_t core0_loop_measure;
extern loop_measure_t core1_loop_measure;
[[noreturn]] extern void die(void);

uint32_t core0_connection_attempt = 0;
bool core0_connected = false;

static PIO led_pio = {};
static uint led_sm = {};
static uint led_offset = {};

static char fbuf0[128] = "";
static char fbuf1[128] = "";
#define FBUF(X) fbuf##X, sizeof(fbuf##X)

static void minimal_status()
{
    setup_status();
    const uint64_t us_up = time_us_64();
    const uint64_t us_cur = get_unix_time();
    const uint64_t us_sync = unix_time_get_last_sync();
    const uint64_t us_since_last_sync = us_cur - us_sync;

    status("\n\n\n==> Basic Status\n");

    if (!core0_connected)
        status("Connect attempt: %d\n", core0_connection_attempt);
    status("Uptime:          %s\n", fdelta_us(us_up, FBUF(0)));
    status("Last clock sync: %s (%s ago)\n", ftime_us(us_sync, FBUF(0)), fdelta_us(us_since_last_sync, FBUF(1)));
    status("Current:         %s\n", ftime_us(us_cur, FBUF(0)));
    status("loops/sec core0: %.3f\n", core0_loop_measure.loops_per_second);
    status("loops/sec core1: %.3f\n", core1_loop_measure.loops_per_second);

    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if (!core0_connected)
        r = 1.0;
    else if (us_sync == 0)
    {
        r = 0.6875;
        g = 0.6875;
    }
    else
        g = 1.0;

#define _clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))

    r = _clamp(r, 0.0, 1.0);
    g = _clamp(g, 0.0, 1.0);
    b = _clamp(b, 0.0, 1.0);

    double heartbeat_time = (double)((us_up / 1000ull) % ((uint64_t)(WS2812_STATUS_HEARTBEAT_PERIOD))) / ((double)(WS2812_STATUS_HEARTBEAT_PERIOD));
    double heartbeat = sin(6.28318530718 * heartbeat_time) * 0.3125 + 0.6875;

    r *= heartbeat;
    g *= heartbeat;
    b *= heartbeat;

    union
    {
        uint8_t bytes[4];
        uint32_t word;
    } c;

    c.bytes[2] = r * 255;
    c.bytes[3] = g * 255;
    c.bytes[1] = b * 255;

    /* Ensure WS2812 reset interval has passed */
    static uint64_t last_us_up = 0;
    uint64_t us_since_last = us_up - last_us_up;
    if (us_since_last < 500)
        sleep_us(500 - us_since_last);
    last_us_up = us_up;

    pio_sm_put_blocking(led_pio, led_sm, c.word);
}

void main_core1()
{
    LOG("Started\n");

    LOG("Initializing ws2812 status led\n");
    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &led_pio, &led_sm, &led_offset, WS2812_STATUS_GPIO, 1, true);
    ws2812_program_init(led_pio, led_sm, led_offset, WS2812_STATUS_GPIO, 800000, false);
    pio_sm_put_blocking(led_pio, led_sm, 0);

    struct actuator_t level_1_on;
    struct actuator_t level_1_off;
    struct actuator_t level_2_on;
    struct actuator_t level_2_off;
    LOG("Initializing actuators\n");
    {
        struct actuator_config_t cinfo = {};
        cinfo.time_travel = ACTUATOR_TRAVEL_TIME;
        cinfo.time_rest = ACTUATOR_REST_TIME;
        cinfo.logic_active_level_extend = ACTUATOR_ACTIVE_LOGIC_LEVEL_EXTEND;
        cinfo.logic_active_level_retract = ACTUATOR_ACTIVE_LOGIC_LEVEL_RETRACT;

        cinfo.gpio_extend = ACTUATOR_GPIO_LEVEL_1_ON_EXTEND;
        cinfo.gpio_retract = ACTUATOR_GPIO_LEVEL_1_ON_RETRACT;
        actuator_init(&level_1_on, &cinfo);

        cinfo.gpio_extend = ACTUATOR_GPIO_LEVEL_1_OFF_EXTEND;
        cinfo.gpio_retract = ACTUATOR_GPIO_LEVEL_1_OFF_RETRACT;
        actuator_init(&level_1_off, &cinfo);

        cinfo.gpio_extend = ACTUATOR_GPIO_LEVEL_2_ON_EXTEND;
        cinfo.gpio_retract = ACTUATOR_GPIO_LEVEL_2_ON_RETRACT;
        actuator_init(&level_2_on, &cinfo);

        cinfo.gpio_extend = ACTUATOR_GPIO_LEVEL_2_OFF_EXTEND;
        cinfo.gpio_retract = ACTUATOR_GPIO_LEVEL_2_OFF_RETRACT;
        actuator_init(&level_2_off, &cinfo);
    }

    LOG("Waiting for SNTP sync\n");
    while (unix_time_get_last_sync() == 0)
    {
        actuator_poll(&level_1_on);
        actuator_poll(&level_1_off);
        actuator_poll(&level_2_on);
        actuator_poll(&level_2_off);
        minimal_status();
        sleep_ms(1);

        if (time_us_64() / 1000000ull > AUTOMATIC_REBOOT_INTERVAL)
            die();
    }

    LOG("Waiting for actuators to retract\n");
    while (actuator_in_cycle(&level_1_on) || actuator_in_cycle(&level_1_off) || actuator_in_cycle(&level_2_on) || actuator_in_cycle(&level_2_off))
    {
        actuator_poll(&level_1_on);
        actuator_poll(&level_1_off);
        actuator_poll(&level_2_on);
        actuator_poll(&level_2_off);
        minimal_status();
        sleep_ms(1);

        if (time_us_64() / 1000000ull > AUTOMATIC_REBOOT_INTERVAL)
            die();
    }

    schedule_current_state_t state_level_1 = schedule_get_state(&schedule_level_1);
    schedule_current_state_t state_level_2 = schedule_get_state(&schedule_level_2);

    LOG("Commanding actuators to resume state (if so configured)\n");
    if (SCHEDULE_TRIGGER_REGION_ON_RESET_IF_IN_ON_REGION)
    {
        if (state_level_1.on)
        {
            LOG("Resuming state 'ON' for level 1\n");
            actuator_trigger(&level_1_on);
        }
        if (state_level_2.on)
        {
            LOG("Resuming state 'ON' for level 2\n");
            actuator_trigger(&level_2_on);
        }
    }
    if (SCHEDULE_TRIGGER_REGION_ON_RESET_IF_IN_OFF_REGION)
    {
        if (!state_level_1.on)
        {
            LOG("Resuming state 'OFF' for level 1\n");
            actuator_trigger(&level_1_off);
        }
        if (!state_level_2.on)
        {
            LOG("Resuming state 'OFF' for level 2\n");
            actuator_trigger(&level_2_off);
        }
    }

    LOG("Resume on reset done, beginning loop\n");
    while (1)
    {
        minimal_status();
        state_level_1 = schedule_get_state(&schedule_level_1);
        state_level_2 = schedule_get_state(&schedule_level_2);

        uint64_t unix_time = get_unix_time() / 1000000;

        status("\n==> Schedule Status\n");
        status("Level 1 state:     %d\n", state_level_1.on);
        status("Level 1 in_region: %d\n", state_level_1.in_region);
        status("Level 1 cur start: %s (%s ago)\n", ftime(state_level_1.timestamp_region_start, FBUF(0)),
            fdelta(unix_time - state_level_1.timestamp_region_start, FBUF(1)));
        status("Level 1 next on:   %s (in %s)\n", ftime(state_level_1.timestamp_region_next_on, FBUF(0)),
            fdelta(state_level_1.timestamp_region_next_on - unix_time, FBUF(1)));
        status("Level 1 next off:  %s (in %s)\n", ftime(state_level_1.timestamp_region_next_off, FBUF(0)),
            fdelta(state_level_1.timestamp_region_next_off - unix_time, FBUF(1)));

        status("Level 2 state:     %d\n", state_level_2.on);
        status("Level 2 in_region: %d\n", state_level_2.in_region);
        status("Level 2 cur start: %s (%s ago)\n", ftime(state_level_2.timestamp_region_start, FBUF(0)),
            fdelta(unix_time - state_level_2.timestamp_region_start, FBUF(1)));
        status("Level 2 next on:   %s (in %s)\n", ftime(state_level_2.timestamp_region_next_on, FBUF(0)),
            fdelta(state_level_2.timestamp_region_next_on - unix_time, FBUF(1)));
        status("Level 2 next off:  %s (in %s)\n", ftime(state_level_2.timestamp_region_next_off, FBUF(0)),
            fdelta(state_level_2.timestamp_region_next_off - unix_time, FBUF(1)));

        if (state_level_1.in_region && !(actuator_in_cycle(&level_1_on) || actuator_in_cycle(&level_1_off)))
        {
            LOG("Level 1: %d\n", state_level_1.on);
            actuator_trigger(state_level_1.on ? &level_1_on : &level_1_off);
        }

        if (state_level_2.in_region && !(actuator_in_cycle(&level_2_on) || actuator_in_cycle(&level_2_off)))
        {
            LOG("Level 2: %d\n", state_level_2.on);
            actuator_trigger(state_level_2.on ? &level_2_on : &level_2_off);
        }

        if (time_us_64() / 1000000ull > AUTOMATIC_REBOOT_INTERVAL //
            && !state_level_1.in_region //
            && !state_level_2.in_region //
            && !actuator_in_cycle(&level_1_on) && !actuator_in_cycle(&level_1_off) //
            && !actuator_in_cycle(&level_2_on) && !actuator_in_cycle(&level_2_off) //
            && state_level_1.timestamp_region_next_on - unix_time > AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION
            && state_level_2.timestamp_region_next_on - unix_time > AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION
            && state_level_1.timestamp_region_next_off - unix_time > AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION
            && state_level_2.timestamp_region_next_off - unix_time > AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION)
            die();

        actuator_poll(&level_1_on);
        actuator_poll(&level_1_off);
        actuator_poll(&level_2_on);
        actuator_poll(&level_2_off);
        loop_measure_end_loop(&core1_loop_measure);
    }
}
