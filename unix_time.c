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
 * @brief Interface to get and set unix time without disturbing the hardware clock (Implementation)
 */
#include "unix_time.h"

#include "hardware/timer.h"
#include "pico/mutex.h"

static mutex_t lock = {};

static microseconds_t offset = 0;
static microseconds_t last_sync = 0;

microseconds_t get_unix_time()
{
    microseconds_t r = 0;
    mutex_enter_blocking(&lock);
    r = time_us_64() + offset;
    mutex_exit(&lock);
    return r;
}

microseconds_t unix_time_get_last_sync()
{
    microseconds_t r = 0;
    mutex_enter_blocking(&lock);
    r = last_sync;
    mutex_exit(&lock);
    return r;
}

void set_unix_time(const microseconds_t microseconds_since_1970)
{
    mutex_enter_blocking(&lock);
    last_sync = (microseconds_t)microseconds_since_1970;
    offset = microseconds_since_1970 - (microseconds_t)time_us_64();
    mutex_exit(&lock);
}

void init_unix_time()
{
    mutex_init(&lock);
    offset = 0;
}
