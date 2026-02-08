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
 * @brief Actuator interface (Implementation)
 */

#include "actuator.h"

#include <string.h> /* memset(), memcpy() */

#include "hardware/gpio.h" /* gpio_init(), gpio_set_dir(), gpio_put() */
#include "pico/time.h" /* time_us_64() */

void actuator_init(struct actuator_t* const a, const struct actuator_config_t* const conf)
{
    memset(a, 0, sizeof(*a));
    memcpy(&a->conf, conf, sizeof(*conf));
    gpio_init(a->conf.gpio_retract);
    gpio_init(a->conf.gpio_extend);
    gpio_set_dir(a->conf.gpio_retract, GPIO_OUT);
    gpio_set_dir(a->conf.gpio_extend, GPIO_OUT);
    gpio_put(a->conf.gpio_retract, !a->conf.logic_active_level_retract);
    gpio_put(a->conf.gpio_extend, !a->conf.logic_active_level_extend);

    a->timestamp_start_retract = time_us_64();
    a->timestamp_end_retract = a->timestamp_start_retract + a->conf.time_travel;
    actuator_poll(a);
}

bool actuator_in_cycle(const struct actuator_t* const a)
{
    const uint64_t cur = time_us_64();
    return cur < a->timestamp_end_retract || cur < a->timestamp_end_extend;
}

void actuator_poll(struct actuator_t* const a)
{
    const uint64_t cur = time_us_64();
    if (a->timestamp_start_retract < cur && cur < a->timestamp_end_retract)
    {
        gpio_put(a->conf.gpio_retract, a->conf.logic_active_level_retract);
        gpio_put(a->conf.gpio_extend, !a->conf.logic_active_level_extend);
    }
    else if (a->timestamp_start_extend < cur && cur < a->timestamp_end_extend)
    {
        gpio_put(a->conf.gpio_retract, !a->conf.logic_active_level_retract);
        gpio_put(a->conf.gpio_extend, a->conf.logic_active_level_extend);
    }
    else
    {
        gpio_put(a->conf.gpio_retract, !a->conf.logic_active_level_retract);
        gpio_put(a->conf.gpio_extend, !a->conf.logic_active_level_extend);
    }
}

void actuator_trigger(struct actuator_t* const a)
{
    if (actuator_in_cycle(a))
        return;
    a->timestamp_start_extend = time_us_64() + a->conf.time_rest;
    a->timestamp_end_extend = a->timestamp_start_extend + a->conf.time_travel;
    a->timestamp_start_retract = a->timestamp_end_extend + a->conf.time_rest;
    a->timestamp_end_retract = a->timestamp_start_retract + a->conf.time_travel;
    actuator_poll(a);
}
