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
#include "schedules.h"
#include "unix_time.h"

#include "config.h"

#include <stdio.h>

#define LOG(fmt, ...) printf("Core 1: " fmt, ##__VA_ARGS__)

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

    LOG("Waiting for actuators to retract\n");
    while (actuator_in_cycle(&level_1_on) || actuator_in_cycle(&level_1_off) || actuator_in_cycle(&level_2_on) || actuator_in_cycle(&level_2_off))
    {
        actuator_poll(&level_1_on);
        actuator_poll(&level_1_off);
        actuator_poll(&level_2_on);
        actuator_poll(&level_2_off);
    }

    LOG("Halting!\n");
}
