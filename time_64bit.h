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
 * @brief Modified and explicitly 64 bit versions of gm_time_r() and mktime() as well as a basic daylight savings correction function
 *
 * This interface exists because to workaround the problem of `sizeof(time_t) == 4`
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct tm_64_bit_t
{
    int32_t tm_usec;
    int32_t tm_sec;
    int32_t tm_min;
    int32_t tm_hour;
    int32_t tm_mday;
    int32_t tm_mon;
    int64_t tm_year;
    int32_t tm_wday;
    int32_t tm_yday;
} tm_64_bit_t;

/**
 * Convert seconds since 1970-01-01 to broken down time
 *
 * @param t Seconds since 1970-01-01
 * @param tm Pointer to output structure
 *
 * @returns True on success, False on failure
 */
bool gmtime_r_64bit(const int64_t t, tm_64_bit_t* const tm);

/**
 * Convert broken down time to seconds since 1970-01-01
 *
 * @param tm Broken down time to convert
 *
 * @returns Seconds since 1970-01-01
 */
int64_t mktime_64bit(const tm_64_bit_t* const tm);

/**
 * Convert microseconds since 1970-01-01 to broken down time
 *
 * @param t Microseconds since 1970-01-01
 * @param tm Pointer to output structure
 *
 * @returns True on success, False on failure
 */
static inline bool gmtime_r_64bit_us(const int64_t t, tm_64_bit_t* const tm)
{
    if (!gmtime_r_64bit(t / 1000000ll, tm))
        return false;
    tm->tm_usec = t % 1000000ll;
    if (tm->tm_usec < 0)
        tm->tm_usec += 1000000ll;
    return true;
}

/**
 * Convert broken down time to seconds since 1970-01-01
 *
 * @param tm Broken down time to convert
 *
 * @returns Microseconds since 1970-01-01
 */
static inline int64_t mktime_64bit_us(const tm_64_bit_t* const tm) { return mktime_64bit(tm) * 1000000ll + tm->tm_usec; }

/**
 * Time-zone shift a broken down time structure
 *
 * Uses the United States DST algorithm
 *
 * @param tm_in Broken down time to shift (May be the same as tm_out, however results are undefined on failure)
 * @param tm_out Pointer to output structure (May be the same as tm_in, however results are undefined on failure)
 * @param offset_st Timezone offset (in seconds) during standard time
 * @param offset_dt Timezone offset (in seconds) during daylight savings time
 *
 * @returns True on success, False on failure
 */
bool get_tz_corrected_tm_64_bit(const tm_64_bit_t* const tm_in, tm_64_bit_t* const tm_out, const int32_t offset_st, const int32_t offset_dt);
