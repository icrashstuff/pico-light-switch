/**
 * pico-light-switch - TODO
 *
 * @file
 * @copyright
 * @parblock
 * SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Ian Hangartner <icrashstuff at outlook dot com>
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
 * @brief Basic daylight savings correction function
 *
 * Originally copied from pico-sunrise
 */
#include "time_64bit.h"
#include <stdbool.h>
#include <stdint.h>

// Python script to generate this table
// #!/bin/python3
// import sys
// from datetime import date
// s = ""
// for year in range(2000, 2400):
//     for day in range(8, 15):
//         if(date(year, 3, day).weekday() == 6):
//             s = "".join((s, f"{day:x}"))
//             break
//
// # Dump source
// with open(sys.modules[__name__].__file__, "r") as fd:
//     print("// Python script to generate this table")
//     for i in fd:
//         print(f"// {i}", end="")
//
// # Dump array
// print("static const uint8_t dst_start_days[] = {")
// for line  in [s[i:i+16]   for i in range(0, len(s),   16)]:
//     print("    ", end="")
//     for c in [line[i:i+2] for i in range(0, len(line), 2)]:
//         print(f"0x{c}, ", end="")
//     print()
// print("};")
//

/** The idea behind this lookup table comes from https://cs.uwaterloo.ca/~alopez-o/math-faq/node73.html */
static const uint8_t dst_start_days[] = { 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98,
    0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed,
    0xcb, 0x98, 0xed, 0xba, 0x98, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc,
    0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba,
    0x98, 0xdc, 0xba, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9,
    0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e,
    0xdc, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed,
    0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e, 0xcb, 0xa9, 0xed, 0xcb, 0x98, 0xed, 0xba, 0x98, 0xdc, 0xba, 0x8e, 0xdc, 0xa9, 0x8e };

bool get_tz_corrected_tm_64_bit(const tm_64_bit_t* const tm_in, tm_64_bit_t* const tm_out, const int32_t offset_st, const int32_t offset_dt)
{
    int64_t leap_cycle_year = (tm_in->tm_year + 1900ll) % 400ll;
    if (leap_cycle_year < 0)
        leap_cycle_year += 400ll;

    uint8_t dst_start_day = dst_start_days[leap_cycle_year >> 1];

    if ((leap_cycle_year & 1) == 0)
        dst_start_day >>= 4;
    dst_start_day &= 0xF;

    // In the US, daylight saving time starts on the second Sunday
    // in March and ends on the first Sunday in November, with the
    // time changes taking place at 2:00 a.m. local time.
    // - Retrieved on 2025-12-10 from:
    //   https://en.wikipedia.org/wiki/Daylight_saving_time_in_the_United_States

    tm_64_bit_t gmt_dt_start = {}; // Second sunday of March of tm_in->tm_year at 2 am + offset_st
    gmt_dt_start.tm_year = tm_in->tm_year;
    gmt_dt_start.tm_mon = 3 - 1;
    gmt_dt_start.tm_mday = dst_start_day;
    gmt_dt_start.tm_hour = 2;
    gmt_dt_start.tm_sec = offset_st;

    tm_64_bit_t gmt_dt_end = {}; // First sunday of November of tm_in->tm_year at 2 am + offset_dt
    gmt_dt_end.tm_year = tm_in->tm_year;
    gmt_dt_end.tm_mon = 11 - 1;
    gmt_dt_end.tm_mday = dst_start_day - 7;
    gmt_dt_end.tm_hour = 2;
    gmt_dt_end.tm_sec = offset_dt;

    int64_t ts_gmt_dt_start = mktime_64bit_us(&gmt_dt_start);
    int64_t ts_gmt_dt_end = mktime_64bit_us(&gmt_dt_end);
    int64_t ts_cur = mktime_64bit_us(tm_in);

    if (ts_gmt_dt_start <= ts_cur && ts_cur < ts_gmt_dt_end)
        ts_cur += ((int64_t)offset_dt) * 1000000ll;
    else
        ts_cur += ((int64_t)offset_st) * 1000000ll;

    return gmtime_r_64bit_us(ts_cur, tm_out);
}
