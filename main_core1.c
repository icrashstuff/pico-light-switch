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

#include <stdio.h>

#include "pico/time.h"

#define LOG(fmt, ...) printf("Core 1: " fmt, ##__VA_ARGS__)

typedef struct
{
    bool on;
    bool in_region;
} schedule_current_state_t;

static schedule_current_state_t schedule_get_state(const schedule_t* const schedule)
{
    const microseconds_t epoch_time = (get_unix_time() / 1000000ull) - schedule->epoch;

    schedule_current_state_t r = { 0 };

    for (uint32_t i = 0; i < schedule->num_entries; i++)
    {
        if (schedule->entries[i].timestamp < epoch_time)
        {
            r.on = schedule->entries[i].on;
            if (epoch_time < (microseconds_t)(schedule->entries[i].timestamp + SCHEDULE_TRIGGER_REGION_LENGTH))
                r.in_region = 1;
        }
    }

    return r;
}

/* Definition in main.c */
extern loop_measure_t core1_loop_measure;

void main_core1()
{
    LOG("Started\n");
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
        sleep_ms(1);
    }

    LOG("Waiting for actuators to retract\n");
    while (actuator_in_cycle(&level_1_on) || actuator_in_cycle(&level_1_off) || actuator_in_cycle(&level_2_on) || actuator_in_cycle(&level_2_off))
    {
        actuator_poll(&level_1_on);
        actuator_poll(&level_1_off);
        actuator_poll(&level_2_on);
        actuator_poll(&level_2_off);
        sleep_ms(1);
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
        state_level_1 = schedule_get_state(&schedule_level_1);
        state_level_2 = schedule_get_state(&schedule_level_2);

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

        actuator_poll(&level_1_on);
        actuator_poll(&level_1_off);
        actuator_poll(&level_2_on);
        actuator_poll(&level_2_off);
        loop_measure_end_loop(&core1_loop_measure);
    }
}
