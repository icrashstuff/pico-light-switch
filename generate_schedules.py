#!/bin/python3
# SPDX-License-Identifier: MIT
#
# SPDX-FileCopyrightText: Copyright (c) 2026 Ian Hangartner <icrashstuff at outlook dot com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
import datetime
import zoneinfo
import struct


tz = zoneinfo.ZoneInfo("America/Anchorage")

def d(y: int, m: int, d: int) -> datetime.datetime:
    return datetime.datetime(y, m, d, tzinfo=tz)

def drange(y1: int, m1: int, d1: int,
           y2: int, m2: int, d2: int
           ) -> list[datetime.datetime]:
    cur = d(y1, m1, d1)
    end = d(y2, m2, d2)
    l = []
    while (cur <= end):
        l.append(cur)
        cur = cur + datetime.timedelta(days=1)
    return l

schedule_start = d(2025, 8, 14)
schedule_end = d(2026, 5, 22)

schedule_exceptions = [
    d(2025,  9,  1), # Labor day
    d(2025,  9, 15), # Reads Act
    d(2025, 11,  3), # PC
    d(2025, 11, 10), # PL
    d(2025, 11, 11), # Veterans day
    d(2025, 11, 27), # Thanksgiving
    d(2025, 11, 28), # Day after thanksgiving
    *drange(2025, 12, 19, 2026, 1, 2), # Winter break
    d(2026,  1, 19), # MLK
    d(2026,  2,  9), # PC
    d(2026,  2, 16), # PC
    *drange(2026, 3, 6, 2026, 3, 13), # Spring break
    d(2026,  5,  1), # PL
]

def t(h: int, m: int) -> datetime.timedelta:
    return datetime.timedelta(hours=h, minutes=m)

time_on_level_2 = [
    [        ], # Sunday
    [t(9, 55)], # Monday
    [t(8, 55)], # Tuesday
    [t(8, 55)], # Wednesday
    [t(8, 55)], # Thursday
    [t(8, 55)], # Friday
    [        ]  # Saturday
]

time_off_level_2 = [
    [                  ], # Sunday
    [t(16, 0), t(25, 0)], # Monday
    [t(16, 0), t(25, 0)], # Tuesday
    [t(16, 0), t(25, 0)], # Wednesday
    [t(16, 0), t(25, 0)], # Thursday
    [t(16, 0), t(25, 0)], # Friday
    [                  ]  # Saturday
]

time_on_level_1 = [
    [        ], # Sunday
    [t(7, 55)], # Monday
    [t(7, 55)], # Tuesday
    [t(7, 55)], # Wednesday
    [t(7, 55)], # Thursday
    [t(7, 55)], # Friday
    [        ]  # Saturday
]

time_off_level_1 = [
    [                  ], # Sunday
    [t(17, 0), t(25, 0)], # Monday
    [t(17, 0), t(25, 0)], # Tuesday
    [t(17, 0), t(25, 0)], # Wednesday
    [t(17, 0), t(25, 0)], # Thursday
    [t(17, 0), t(25, 0)], # Friday
    [                  ]  # Saturday
]

def generate_schedule(time_on:  list[list[datetime.timedelta]],
                      time_off: list[list[datetime.timedelta]]
                      ) -> list[tuple[int, bool]]:
    cur = schedule_start
    out = []
    while(cur <= schedule_end):
        if(cur not in schedule_exceptions):
            day_of_week = (cur.weekday() + 1) % 7
            for i in time_on[day_of_week]:
                out.append((int((cur + i).timestamp()), 1))
            for i in time_off[day_of_week]:
                out.append((int((cur + i).timestamp()), 0))

        cur = cur + datetime.timedelta(days=1)
    return out

def write_schedule_header(name: str, sched: list[tuple[int, bool]]) -> None:
    with open(f"{name}.h", 'w') as fd:
        epoch = sched[0][0]
        fd.write(f"static const schedule_t {name} =" " { " f"{sched[0][0]}ull, {len(sched)},\n")
        fd.write("    {\n")
        for i in sched:
            fd.write("        { %d, %d },\n" % (i[0] - epoch, i[1]))
        fd.write("    } };\n")

if __name__ == '__main__':
    write_schedule_header("schedule_level_1", generate_schedule(time_on_level_1, time_off_level_1))
    write_schedule_header("schedule_level_2", generate_schedule(time_on_level_2, time_off_level_2))
