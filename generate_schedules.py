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

def dtrange(start : datetime.datetime,
           end: datetime.datetime
           ) -> list[datetime.datetime]:
    cur = start
    l = []
    while (cur <= end):
        l.append(cur)
        cur = cur + datetime.timedelta(days=1)
    return l

schedule_start = d(2026, 8, 7)
schedule_end = d(2027, 5, 21)

schedule_true_start: datetime.datetime = schedule_start - datetime.timedelta(weeks=16)
schedule_true_end: datetime.datetime = schedule_end + datetime.timedelta(weeks=16)

schedule_exceptions = [
    *dtrange(schedule_true_start, schedule_start - datetime.timedelta(days=1)), # Schedule header (Summer break)
    # 08 - August

    # 09 - September
    d(2026,  9,  7), # Labor Day
    d(2026,  9, 14), # AK Reads act

    # 10 - October

    # 11 - November
    d(2026, 11, 11), # Veterans Day
    d(2026, 11, 26), # Thanksgiving Day
    d(2026, 11, 27), # Day After Thanksgiving

    # 12 - December
    *drange(2026, 12, 21,  # Winter break
            2026, 12, 31), # Winter break

    # 01 - January
    d(2027,  1,  1), # New Years
    d(2027,  1, 18), # MLK Day

    # 02 - February

    # 03 - March
    *drange(2027,  3,  8,  # Spring break
            2027,  3, 12), # Spring break

    # 04 - April

    # 05 - May

    *dtrange(schedule_end + datetime.timedelta(days=1), schedule_true_end), # Schedule footer (Summer break)
]

def t(h: int, m: int) -> datetime.timedelta:
    return datetime.timedelta(hours=h, minutes=m)

# Used on schedule_exceptions days, regardless of level
time_on_exercise = [
    [        ], # Sunday
    [        ], # Monday
    [t(7,  5)], # Tuesday
    [        ], # Wednesday
    [        ], # Thursday
    [        ], # Friday
    [        ]  # Saturday
]

# Used on schedule_exceptions days, regardless of level
time_off_exercise = [
    [t(25, 0)], # Sunday
    [t(25, 0)], # Monday
    [t(25, 0),t(7, 10)], # Tuesday
    [t(25, 0)], # Wednesday
    [t(25, 0)], # Thursday
    [t(25, 0)], # Friday
    [t(25, 0)]  # Saturday
]

time_on_level_2 = [
    [        ], # Sunday
    [t(7, 55)], # Monday
    [t(7, 55)], # Tuesday
    [t(7, 55)], # Wednesday
    [t(7, 55)], # Thursday
    [t(7, 55)], # Friday
    [        ]  # Saturday
]

time_off_soft_level_2 = [
    [        ], # Sunday
    [t(16, 0)], # Monday
    [t(16, 0)], # Tuesday
    [t(16, 0)], # Wednesday
    [t(16, 0)], # Thursday
    [t(16, 0)], # Friday
    [        ]  # Saturday
]

time_off_level_2 = [
    [t(25, 0)], # Sunday
    [t(25, 0)], # Monday
    [t(25, 0)], # Tuesday
    [t(25, 0)], # Wednesday
    [t(25, 0)], # Thursday
    [t(25, 0)], # Friday
    [t(25, 0)]  # Saturday
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

time_off_soft_level_1 = [
    [        ], # Sunday
    [t(17, 0)], # Monday
    [t(17, 0)], # Tuesday
    [t(17, 0)], # Wednesday
    [t(17, 0)], # Thursday
    [t(17, 0)], # Friday
    [        ]  # Saturday
]

time_off_level_1 = [
    [t(25, 0)], # Sunday
    [t(25, 0)], # Monday
    [t(25, 0)], # Tuesday
    [t(25, 0)], # Wednesday
    [t(25, 0)], # Thursday
    [t(25, 0)], # Friday
    [t(25, 0)]  # Saturday
]

def generate_schedule(
        time_on: list[list[datetime.timedelta]],
        time_off_soft: list[list[datetime.timedelta]],
        time_off: list[list[datetime.timedelta]]
        ) -> list[tuple[int, bool, datetime.datetime, str]]:
    cur = schedule_true_start
    out = []
    while(cur <= schedule_true_end):
        day_of_week = (cur.weekday() + 1) % 7
        cur_str = cur.strftime("%Y-%m-%d %a")
        if(cur not in schedule_exceptions):
            for i in time_on[day_of_week]:
                out.append((
                    int((cur + i).timestamp()),
                    1, # On State
                    1, # Allow resume
                    cur + i,
                    f"Regular from {cur_str}"
                ))
            for i in time_off_soft[day_of_week]:
                out.append((
                    int((cur + i).timestamp()),
                    0, # On State
                    0, # Allow resume
                    cur + i,
                    f"Regular from {cur_str}"
                ))
            for i in time_off[day_of_week]:
                out.append((
                    int((cur + i).timestamp()),
                    0, # On State
                    1, # Allow resume
                    cur + i,
                    f"Regular from {cur_str}"
                ))
        else:
            for i in time_on_exercise[day_of_week]:
                out.append((
                    int((cur + i).timestamp()),
                    1, # On State
                    0, # Allow resume
                    cur + i,
                    f"Exception from {cur_str}"
                ))
            for i in time_off_exercise[day_of_week]:
                out.append((
                    int((cur + i).timestamp()),
                    0, # On State
                    0, # Allow resume
                    cur + i,
                    f"Exception from {cur_str}"
                ))

        cur = cur + datetime.timedelta(days=1)

    return sorted(out, key=lambda x: x[0])

def write_schedule_header(
        name: str,
        sched: list[tuple[int, bool, datetime.datetime, str]]
        ) -> None:
    with open(f"{name}.h", 'w') as fd:
        epoch = sched[0][0]
        fd.write("/* clang-format off */\n")
        fd.write(f"static const schedule_t {name} =" " { " f"{sched[0][0]}ull, {len(sched)},\n")
        fd.write("    {\n")
        for i in sched:
            fd.write("        {% 9d, %d, %d }, // %s; %s\n" % (i[0] - epoch, i[1], i[2], i[3].strftime("%Y-%m-%d %H:%M:%S %:z %a"), i[4]))
        fd.write("    } };\n")
        fd.write("/* clang-format on */\n")

if __name__ == '__main__':
    write_schedule_header("schedule_level_1", generate_schedule(time_on_level_1, time_off_soft_level_1, time_off_level_1))
    write_schedule_header("schedule_level_2", generate_schedule(time_on_level_2, time_off_soft_level_2, time_off_level_2))
