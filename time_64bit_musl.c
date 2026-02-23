/**
 * @file
 * @copyright
 * @parblock
 * SPDX-License-Identifier: MIT
 *
 * SPDX-FileCopyrightText: Copyright (c) 2005-2020 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors/contributors listed by the git log for the copied files:
 * - Alex Xu (Hello71)
 * - Daniel Sabogal
 * - Rich Felker
 *
 * @endparblock
 *
 * Originally copied from musl libc, commit #1b76ff0767d01df72f692806ee5adee13c67ef88, specifically files:
 * - https://git.musl-libc.org/cgit/musl/tree/src/time/__tm_to_secs.c
 * - https://git.musl-libc.org/cgit/musl/tree/src/time/__secs_to_tm.c
 * - https://git.musl-libc.org/cgit/musl/tree/src/time/__month_to_secs.c
 * - https://git.musl-libc.org/cgit/musl/tree/src/time/__year_to_secs.c
 *
 * Modifications by Ian Hangartner:
 * - Switch to fixed width integer types
 * - Switch from struct tm to tm_64_bit_t
 * - __tm_to_secs(): Change return type from int to bool
 * - __tm_to_secs(): Change meaning of return value (1=Success, 0=Failure)
 */
#include "time_64bit.h"
#include <stdbool.h>
#include <stdint.h>

/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400LL * (31LL + 29LL))

#define DAYS_PER_400Y ((int64_t)(365 * 400 + 97))
#define DAYS_PER_100Y ((int32_t)(365 * 100 + 24))
#define DAYS_PER_4Y ((int32_t)(365 * 4 + 1))

bool gmtime_r_64bit(const int64_t t, tm_64_bit_t* const tm)
{
    int64_t days, secs, years;
    int32_t remdays, remsecs, remyears;
    int32_t qc_cycles, c_cycles, q_cycles;
    int32_t months;
    int32_t wday, yday, leap;
    static const char days_in_month[] = { 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29 };

    /* Reject time_t values whose year would overflow int32_t */
    if (t < INT32_MIN * 31622400LL || t > INT32_MAX * 31622400LL)
        return 0;

    secs = t - LEAPOCH;
    days = secs / 86400;
    remsecs = secs % 86400;
    if (remsecs < 0)
    {
        remsecs += 86400;
        days--;
    }

    wday = (3 + days) % 7;
    if (wday < 0)
        wday += 7;

    qc_cycles = days / DAYS_PER_400Y;
    remdays = days % DAYS_PER_400Y;
    if (remdays < 0)
    {
        remdays += DAYS_PER_400Y;
        qc_cycles--;
    }

    c_cycles = remdays / DAYS_PER_100Y;
    if (c_cycles == 4)
        c_cycles--;
    remdays -= c_cycles * DAYS_PER_100Y;

    q_cycles = remdays / DAYS_PER_4Y;
    if (q_cycles == 25)
        q_cycles--;
    remdays -= q_cycles * DAYS_PER_4Y;

    remyears = remdays / 365;
    if (remyears == 4)
        remyears--;
    remdays -= remyears * 365;

    leap = !remyears && (q_cycles || !c_cycles);
    yday = remdays + 31 + 28 + leap;
    if (yday >= 365 + leap)
        yday -= 365 + leap;

    years = remyears + 4 * q_cycles + 100 * c_cycles + 400LL * qc_cycles;

    for (months = 0; days_in_month[months] <= remdays; months++)
        remdays -= days_in_month[months];

    if (months >= 10)
    {
        months -= 12;
        years++;
    }

    if (years + 100 > INT32_MAX || years + 100 < INT32_MIN)
        return 0;

    tm->tm_year = years + 100;
    tm->tm_mon = months + 2;
    tm->tm_mday = remdays + 1;
    tm->tm_wday = wday;
    tm->tm_yday = yday;

    tm->tm_hour = remsecs / 3600;
    tm->tm_min = remsecs / 60 % 60;
    tm->tm_sec = remsecs % 60;
    tm->tm_usec = 0;

    return 1;
}

static int32_t __month_to_secs(int32_t month, int32_t is_leap)
{
    static const int32_t secs_through_month[] = {
        0,
        31 * 86400,
        59 * 86400,
        90 * 86400,
        120 * 86400,
        151 * 86400,
        181 * 86400,
        212 * 86400,
        243 * 86400,
        273 * 86400,
        304 * 86400,
        334 * 86400,
    };
    int32_t t = secs_through_month[month];
    if (is_leap && month >= 2)
        t += 86400;
    return t;
}

static int64_t __year_to_secs(int64_t year, int32_t* is_leap)
{
    if (year - 2ULL <= 136)
    {
        int32_t y = year;
        int32_t leaps = (y - 68) >> 2;
        if (!((y - 68) & 3))
        {
            leaps--;
            if (is_leap)
                *is_leap = 1;
        }
        else if (is_leap)
            *is_leap = 0;
        return 31536000LL * (y - 70LL) + 86400LL * leaps;
    }

    int32_t cycles, centuries, leaps, rem, dummy;

    if (!is_leap)
        is_leap = &dummy;
    cycles = (year - 100ll) / 400ll;
    rem = (year - 100ll) % 400ll;
    if (rem < 0)
    {
        cycles--;
        rem += 400;
    }
    if (!rem)
    {
        *is_leap = 1;
        centuries = 0;
        leaps = 0;
    }
    else
    {
        if (rem >= 200)
        {
            if (rem >= 300)
                centuries = 3, rem -= 300;
            else
                centuries = 2, rem -= 200;
        }
        else
        {
            if (rem >= 100)
                centuries = 1, rem -= 100;
            else
                centuries = 0;
        }
        if (!rem)
        {
            *is_leap = 0;
            leaps = 0;
        }
        else
        {
            leaps = rem / 4U;
            rem %= 4U;
            *is_leap = !rem;
        }
    }

    leaps += 97 * cycles + 24 * centuries - *is_leap;

    return (year - 100LL) * 31536000LL + leaps * 86400LL + 946684800LL + 86400LL;
}

int64_t mktime_64bit(const tm_64_bit_t* const tm)
{
    int32_t is_leap;
    int64_t year = tm->tm_year;
    int32_t month = tm->tm_mon;
    if (month >= 12 || month < 0)
    {
        int32_t adj = month / 12;
        month %= 12;
        if (month < 0)
        {
            adj--;
            month += 12;
        }
        year += adj;
    }
    int64_t t = __year_to_secs(year, &is_leap);
    t += __month_to_secs(month, is_leap);
    t += 86400LL * (tm->tm_mday - 1);
    t += 3600LL * tm->tm_hour;
    t += 60LL * tm->tm_min;
    t += tm->tm_sec;
    return t;
}
