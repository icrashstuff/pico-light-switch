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
#include "lcd.h"
#include "loop_measurer.h"
#include "schedules.h"
#include "unix_time.h"

#include "config.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "pico/time.h"

#include "ftime.h"

#include "ws2812.pio.h"

#define LOG(fmt, ...) printf("Core %u: " fmt, get_core_num(), ##__VA_ARGS__)

typedef struct
{
    bool on;
    bool allow_resume;
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
            r.allow_resume = schedule->entries[i].allow_resume;
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

static bool status_can_print = 0;
static uint8_t status_lcd_page = 0;

#if defined(__GNUC__) || defined(__clang__)
#define formatting_attribute(fmtargnumber) __attribute__((format(__printf__, fmtargnumber, fmtargnumber + 1)))
#endif
static formatting_attribute(1) int status(const char* fmt, ...)
{
    if (!status_can_print)
        return 0;
    va_list args;
    va_start(args, fmt);
    int r = vprintf(fmt, args);
    va_end(args);
    return r;
}

#define LCD_WIDTH 20

static formatting_attribute(4) void status_lcd(uint8_t page, uint8_t line, bool center, const char* fmt, ...)
{
    if (!status_can_print || page != status_lcd_page)
        return;
    char buf[LCD_WIDTH * 2 + 4];
    char* s = buf + LCD_WIDTH;
    memset(buf, ' ', sizeof(buf));

    va_list args;
    va_start(args, fmt);
    vsnprintf(s, LCD_WIDTH, fmt, args);
    va_end(args);

    size_t len = strlen(s);

    if (center && len < LCD_WIDTH)
    {
        size_t shift = (LCD_WIDTH - len) / 2;
        s -= shift;
    }

    for (size_t i = 0; i < LCD_WIDTH; i++)
        if (s[i] == 0)
            s[i] = ' ';
    lcd_set_cursor(line, 0);
    s[20] = 0;
    lcd_string(s);
}

static void setup_status_lcd(uint8_t num_pages) { status_lcd_page = (time_us_64() / (STATUS_PRINT_INTERVAL * STATUS_LCD_INTERVALS_PER_PAGE)) % num_pages; }

static void setup_status()
{
    static uint64_t last_status_time = 0;
    const uint64_t loop_start_time = time_us_64();
    if (loop_start_time / STATUS_PRINT_INTERVAL != last_status_time / STATUS_PRINT_INTERVAL)
    {
        status_can_print = 1;
        last_status_time = loop_start_time;
    }
    else
    {
        status_can_print = 0;
    }
}

/* Definition in main.c */
extern loop_measure_t core0_loop_measure;
extern loop_measure_t core1_loop_measure;
[[noreturn]] extern void die(void);

uint32_t core0_connection_attempt = 0;
bool core0_connected = false;

#ifdef WS2812_STATUS_GPIO
static PIO led_pio = {};
static uint led_sm = {};
static uint led_offset = {};
#endif

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

#ifdef WAVESHARE_RP2350_RELAY_6CH_W
    c.bytes[3] = r * 255;
    c.bytes[2] = g * 255;
#else
    c.bytes[2] = r * 255;
    c.bytes[3] = g * 255;
#endif
    c.bytes[1] = b * 255;

    /* Ensure WS2812 reset interval has passed */
    static uint64_t last_us_up = 0;
    uint64_t us_since_last = us_up - last_us_up;
    if (us_since_last < 500)
        sleep_us(500 - us_since_last);
    last_us_up = us_up;

#ifdef WS2812_STATUS_GPIO
    pio_sm_put_blocking(led_pio, led_sm, c.word);
#endif
}

void main_core1()
{
    LOG("Started\n");
    gpio_pull_up(SCHEDULE_SELECT_PIN);

#ifdef WS2812_STATUS_GPIO
    LOG("Initializing ws2812 status led\n");
    pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &led_pio, &led_sm, &led_offset, WS2812_STATUS_GPIO, 1, true);
    ws2812_program_init(led_pio, led_sm, led_offset, WS2812_STATUS_GPIO, 800000, false);
    pio_sm_put_blocking(led_pio, led_sm, 0);
#endif

    lcd_init();
    lcd_clear();

    int schedule_num_selected = gpio_get(SCHEDULE_SELECT_PIN) ? 1 : 2;

    struct actuator_t act_on;
    struct actuator_t act_off;
    LOG("Initializing actuators\n");
    {
        struct actuator_config_t cinfo = {};
        cinfo.time_travel = ACTUATOR_TRAVEL_TIME;
        cinfo.time_rest = ACTUATOR_REST_TIME;
        cinfo.logic_active_level_extend = ACTUATOR_ACTIVE_LOGIC_LEVEL_EXTEND;
        cinfo.logic_active_level_retract = ACTUATOR_ACTIVE_LOGIC_LEVEL_RETRACT;

        cinfo.gpio_extend = ACTUATOR_GPIO_ACT_ON_EXTEND;
        cinfo.gpio_retract = ACTUATOR_GPIO_ACT_ON_RETRACT;
        actuator_init(&act_on, &cinfo);

        cinfo.gpio_extend = ACTUATOR_GPIO_ACT_OFF_EXTEND;
        cinfo.gpio_retract = ACTUATOR_GPIO_ACT_OFF_RETRACT;
        actuator_init(&act_off, &cinfo);
    }

    LOG("Waiting for SNTP sync\n");
    while (unix_time_get_last_sync() == 0)
    {
        setup_status_lcd(1);
        status_lcd(0, 0, true, "UP: %s", fdelta(time_us_64() / 1000000ull, FBUF(0)));
        status_lcd(0, 1, true, "Waiting for");
        status_lcd(0, 2, true, "SNTP");
        status_lcd(0, 3, true, "");
        actuator_poll(&act_on);
        actuator_poll(&act_off);
        minimal_status();
        sleep_ms(1);

        if (time_us_64() / 1000000ull > AUTOMATIC_REBOOT_INTERVAL)
            die();
    }

    LOG("Waiting for actuators to retract\n");
    while (actuator_in_cycle(&act_on) || actuator_in_cycle(&act_off))
    {
        setup_status_lcd(1);
        status_lcd(0, 0, true, "UP: %s", fdelta(time_us_64() / 1000000ull, FBUF(0)));
        status_lcd(0, 1, true, "Waiting for");
        status_lcd(0, 2, true, "Actuator Retraction");
        status_lcd(0, 3, true, "");
        actuator_poll(&act_on);
        actuator_poll(&act_off);
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
        if (state_level_1.on && state_level_1.allow_resume && schedule_num_selected == 1)
        {
            LOG("Resuming state 'ON' for level 1\n");
            actuator_trigger(&act_on);
        }
        if (state_level_2.on && state_level_2.allow_resume && schedule_num_selected == 2)
        {
            LOG("Resuming state 'ON' for level 2\n");
            actuator_trigger(&act_on);
        }
    }
    if (SCHEDULE_TRIGGER_REGION_ON_RESET_IF_IN_OFF_REGION)
    {
        if (!state_level_1.on && state_level_1.allow_resume && schedule_num_selected == 1)
        {
            LOG("Resuming state 'OFF' for level 1\n");
            actuator_trigger(&act_off);
        }
        if (!state_level_2.on && state_level_2.allow_resume && schedule_num_selected == 2)
        {
            LOG("Resuming state 'OFF' for level 2\n");
            actuator_trigger(&act_off);
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
        status("Level 1 enabled:      %d\n", schedule_num_selected == 1);
        status("Level 1 state:        %d\n", state_level_1.on);
        status("Level 1 in_region:    %d\n", state_level_1.in_region);
        status("Level 1 allow_resume: %d\n", state_level_1.allow_resume);
        status("Level 1 cur start:    %s (%s ago)\n", ftime(state_level_1.timestamp_region_start, FBUF(0)),
            fdelta(unix_time - state_level_1.timestamp_region_start, FBUF(1)));
        status("Level 1 next on:      %s (in %s)\n", ftime(state_level_1.timestamp_region_next_on, FBUF(0)),
            fdelta(state_level_1.timestamp_region_next_on - unix_time, FBUF(1)));
        status("Level 1 next off:     %s (in %s)\n", ftime(state_level_1.timestamp_region_next_off, FBUF(0)),
            fdelta(state_level_1.timestamp_region_next_off - unix_time, FBUF(1)));

        status("Level 2 enabled:      %d\n", schedule_num_selected == 2);
        status("Level 2 state:        %d\n", state_level_2.on);
        status("Level 2 in_region:    %d\n", state_level_2.in_region);
        status("Level 2 allow_resume: %d\n", state_level_2.allow_resume);
        status("Level 2 cur start:    %s (%s ago)\n", ftime(state_level_2.timestamp_region_start, FBUF(0)),
            fdelta(unix_time - state_level_2.timestamp_region_start, FBUF(1)));
        status("Level 2 next on:      %s (in %s)\n", ftime(state_level_2.timestamp_region_next_on, FBUF(0)),
            fdelta(state_level_2.timestamp_region_next_on - unix_time, FBUF(1)));
        status("Level 2 next off:     %s (in %s)\n", ftime(state_level_2.timestamp_region_next_off, FBUF(0)),
            fdelta(state_level_2.timestamp_region_next_off - unix_time, FBUF(1)));

        if (state_level_1.in_region && !(actuator_in_cycle(&act_on) || actuator_in_cycle(&act_off)) && schedule_num_selected == 1)
        {
            LOG("Level 1: %d\n", state_level_1.on);
            actuator_trigger(state_level_1.on ? &act_on : &act_off);
        }

        if (state_level_2.in_region && !(actuator_in_cycle(&act_on) || actuator_in_cycle(&act_off)) && schedule_num_selected == 2)
        {
            LOG("Level 2: %d\n", state_level_2.on);
            actuator_trigger(state_level_2.on ? &act_on : &act_off);
        }

#define ACTV_IDLE(x) ((x) ? "ACTV" : "IDLE")
#define RESUME_NO(x) ((x) ? "RES-Y" : "RES-N")
#define ON_OFF(x) ((x) ? "ON " : "OFF")

        setup_status_lcd(4);
        status_lcd(0, 0, true, "%s", ftime(unix_time, FBUF(0)));
        status_lcd(0, 1, true, "UP: %s", fdelta(time_us_64() / 1000000ull, FBUF(0)));
        if (schedule_num_selected == 1)
            status_lcd(0, 2, true, "L1: %s %s %s", ACTV_IDLE(state_level_1.in_region), RESUME_NO(state_level_1.allow_resume), ON_OFF(state_level_1.on));
        else
            status_lcd(0, 2, true, "L1: Disabled");
        if (schedule_num_selected == 2)
            status_lcd(0, 3, true, "L2: %s %s %s", ACTV_IDLE(state_level_2.in_region), RESUME_NO(state_level_2.allow_resume), ON_OFF(state_level_2.on));
        else
            status_lcd(0, 3, true, "L2: Disabled");

        if (schedule_num_selected == 1)
            status_lcd(1, 0, true, "L1: %s %s %s", ACTV_IDLE(state_level_1.in_region), RESUME_NO(state_level_1.allow_resume), ON_OFF(state_level_1.on));
        else
            status_lcd(1, 0, true, "L1: Disabled");
        status_lcd(1, 1, true, "CUR:%s", ftime_compact(state_level_1.timestamp_region_start, FBUF(0)));
        status_lcd(1, 2, true, "NON:%s", ftime_compact(state_level_1.timestamp_region_next_on, FBUF(0)));
        status_lcd(1, 3, true, "NOF:%s", ftime_compact(state_level_1.timestamp_region_next_off, FBUF(0)));

        if (schedule_num_selected == 2)
            status_lcd(2, 0, true, "L2: %s %s %s", ACTV_IDLE(state_level_2.in_region), RESUME_NO(state_level_2.allow_resume), ON_OFF(state_level_2.on));
        else
            status_lcd(2, 0, true, "L2: Disabled");
        status_lcd(2, 1, true, "CUR:%s", ftime_compact(state_level_2.timestamp_region_start, FBUF(0)));
        status_lcd(2, 2, true, "NON:%s", ftime_compact(state_level_2.timestamp_region_next_on, FBUF(0)));
        status_lcd(2, 3, true, "NOF:%s", ftime_compact(state_level_2.timestamp_region_next_off, FBUF(0)));

        status_lcd(3, 0, true, "%s", ftime(unix_time, FBUF(0)));
        status_lcd(3, 1, true, "Schedule end dates");
        status_lcd(3, 2, true, "L1: %s", ftime_compact(schedule_level_2.entries[schedule_level_2.num_entries - 1].timestamp + schedule_level_2.epoch, FBUF(0)));
        status_lcd(3, 3, true, "L2: %s", ftime_compact(schedule_level_2.entries[schedule_level_2.num_entries - 1].timestamp + schedule_level_2.epoch, FBUF(0)));

        if (time_us_64() / 1000000ull > AUTOMATIC_REBOOT_INTERVAL //
            && !state_level_1.in_region //
            && !state_level_2.in_region //
            && !actuator_in_cycle(&act_on) && !actuator_in_cycle(&act_off) //
            && state_level_1.timestamp_region_next_on - unix_time > AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION
            && state_level_2.timestamp_region_next_on - unix_time > AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION
            && state_level_1.timestamp_region_next_off - unix_time > AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION
            && state_level_2.timestamp_region_next_off - unix_time > AUTOMATIC_REBOOT_MIN_DISTANCE_TO_REGION)
            die();

        actuator_poll(&act_on);
        actuator_poll(&act_off);
        loop_measure_end_loop(&core1_loop_measure);
    }
}
