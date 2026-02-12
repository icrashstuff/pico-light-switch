/**
 * pico-light-switch - TODO
 *
 * @file
 * @copyright
 * @parblock
 * SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025 Ian Hangartner <icrashstuff at outlook dot com>
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
 * Originally copied from pico-sunrise
 *
 * @brief Average loop time/Loops per second structure
 */
#include "loop_measurer.h"
#include "config.h"

#include "hardware/timer.h"

loop_measure_t loop_measure_init()
{
    loop_measure_t r = {};
    r.last_push = ~0;
    return r;
}

void loop_measure_end_loop(loop_measure_t* obj)
{
    uint64_t cur_time = time_us_64();
    if (obj->last_push == ~0ull)
        obj->last_push = cur_time;

    obj->loop_times[obj->loop_times_pos++] = cur_time - obj->last_push;
    obj->loop_times_pos %= LOOP_AVERAGE_SAMPLE_COUNT;
    obj->last_push = cur_time;

    microseconds_t new_average_loop_time = 0;
    for (uint32_t i = 0; i < LOOP_AVERAGE_SAMPLE_COUNT; i++)
        new_average_loop_time += obj->loop_times[i];
    new_average_loop_time /= (microseconds_t)(LOOP_AVERAGE_SAMPLE_COUNT);

    obj->average_loop_time = new_average_loop_time;
    obj->loops_per_second = ((double)(MICROSECONDS_PER_SECOND)) / ((double)(obj->average_loop_time));
}
