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
 * @brief Interface to get and set unix time without disturbing the hardware clock
 */
#pragma once

#include <stdint.h>

#define MICROSECONDS_PER_SECOND ((microseconds_t)(1000ll * 1000ll))
#define MICROSECONDS_PER_MINUTE ((microseconds_t)(60ll * MICROSECONDS_PER_SECOND))
#define MICROSECONDS_PER_HOUR ((microseconds_t)(60ll * MICROSECONDS_PER_MINUTE))
#define MICROSECONDS_PER_DAY ((microseconds_t)(24ll * MICROSECONDS_PER_HOUR))

typedef int64_t microseconds_t;

/**
 * Gets current unix time
 *
 * @returns Microseconds since 1970
 */
microseconds_t get_unix_time();

/**
 * Sets unix time
 */
void set_unix_time(const microseconds_t microseconds_since_1970);

/**
 * Initializes internal mutex
 */
void init_unix_time();
