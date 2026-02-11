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
 * @brief Actuator interface
 */
#pragma once

#include <stdbool.h> /* bool */
#include <stdint.h> /* uint64_t, uint8_t */

struct actuator_config_t
{
    uint64_t time_travel; ///< Time (in microseconds) for full actuator travel
    uint64_t time_rest; ///< Time (in microseconds) to rest between switching directions
    uint8_t gpio_retract; ///< GPIO pin that signals actuator retraction
    uint8_t gpio_extend; ///< GPIO pin that signals actuator extension

    /** Logic level that must be put to `gpio_retract` for actuator retraction to occur */
    bool logic_active_level_retract;
    /** Logic level that must be put to `gpio_extend` for actuator extension to occur */
    bool logic_active_level_extend;
};

struct actuator_t
{
    struct actuator_config_t conf;
    uint64_t timestamp_start_extend;
    uint64_t timestamp_end_extend;
    uint64_t timestamp_start_retract;
    uint64_t timestamp_end_retract;
};

/**
 * Initialize an actuator
 *
 * NOTE: This will trigger a retract event to occur
 *
 * @param a Actuator object to initialize
 * @param conf Configuration for actuator
 */
void actuator_init(struct actuator_t* const a, const struct actuator_config_t* const conf);

/**
 * Check if actuator is currently in an extend-retract cycle
 */
bool actuator_in_cycle(const struct actuator_t* const a);

/**
 * Synchronize actuator state to hardware
 *
 * @param a Actuator to update
 */
void actuator_poll(struct actuator_t* const a);

/**
 * Trigger an extend-retract cycle
 *
 * This function is a no-op if the actuator is currently in an extend-retract cycle
 *
 * @param a Actuator to trigger
 */
void actuator_trigger(struct actuator_t* const a);
