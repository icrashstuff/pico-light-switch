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
 * @brief Timestamp/Timedelta formatting interface (implementation)
 */
#include "ftime.h"

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "time_64bit.h"

char* fdelta_us(const uint64_t us, char* const buffer, size_t buf_size)
{
    uint64_t s = us / 1000000ull;
    uint64_t m = s / 60ull;
    uint64_t h = m / 60ull;
    uint64_t d = h / 24ull;

    snprintf(buffer, buf_size, "%02llu:%02llu:%02llu:%02llu.%06llu", d, h % 24ull, m % 60ull, s % 60ull, us % 1000000ull);
    return buffer;
}

char* fdelta(const uint64_t s, char* const buffer, size_t buf_size)
{
    uint64_t m = s / 60ull;
    uint64_t h = m / 60ull;
    uint64_t d = h / 24ull;

    snprintf(buffer, buf_size, "%02llu:%02llu:%02llu:%02llu", d, h % 24ull, m % 60ull, s % 60ull);
    return buffer;
}

#define DAYS_IN_LEAP_CYCLE (365ull * 303ull + 366ull * 97ull)
#define DAYS_BY_SUB_LEAP_CYCLE_1_END (365ull * 75ull + 366ull * 25ull)
#define DAYS_BY_SUB_LEAP_CYCLE_2_END (365ull * 76ull + 366ull * 24ull + DAYS_BY_SUB_LEAP_CYCLE_1_END)
#define DAYS_BY_SUB_LEAP_CYCLE_3_END (365ull * 76ull + 366ull * 24ull + DAYS_BY_SUB_LEAP_CYCLE_2_END)
#define DAYS_BY_SUB_LEAP_CYCLE_4_END (365ull * 76ull + 366ull * 24ull + DAYS_BY_SUB_LEAP_CYCLE_3_END)

static_assert(DAYS_IN_LEAP_CYCLE == DAYS_BY_SUB_LEAP_CYCLE_4_END, "");

char* ftime_us(const uint64_t us, char* const buffer, size_t buf_size)
{
    tm_64_bit_t tm = {};
    gmtime_r_64bit_us(us, &tm);
    get_tz_corrected_tm_64_bit(&tm, &tm, TIMEZONE_OFFSET_ST, TIMEZONE_OFFSET_DT);

    snprintf(buffer, buf_size, "%04lld-%02d-%02d %02d:%02d:%02d.%06d", //
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_usec);
    return buffer;
}

char* ftime(const uint64_t s, char* const buffer, size_t buf_size)
{
    tm_64_bit_t tm = {};
    gmtime_r_64bit(s, &tm);
    get_tz_corrected_tm_64_bit(&tm, &tm, TIMEZONE_OFFSET_ST, TIMEZONE_OFFSET_DT);

    snprintf(buffer, buf_size, "%04lld-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return buffer;
}
