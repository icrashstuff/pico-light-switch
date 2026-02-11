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
#
# This is a script of middling quality to run the actuator on a regular raspberry pi
import generate_schedules
import datetime
import gpiozero
import asyncio


async def a_retract(extend: gpiozero.OutputDevice,
                    retract: gpiozero.OutputDevice,
                    time_rest: float,
                    time_travel: float):
    print(f"Retracting, E={extend.pin} R={retract.pin}")

    extend.off()
    retract.off()
    await asyncio.sleep(time_rest)

    retract.on()
    await asyncio.sleep(time_travel)

    extend.off()
    retract.off()


async def a_extend(extend: gpiozero.OutputDevice,
                   retract: gpiozero.OutputDevice,
                   time_rest: float,
                   time_travel: float):
    print(f"Extending,  E={extend.pin} R={retract.pin}")
    extend.off()
    retract.off()
    await asyncio.sleep(time_rest)

    extend.on()
    await asyncio.sleep(time_travel)

    extend.off()
    retract.off()


async def a_cycle(extend: gpiozero.OutputDevice,
                  retract: gpiozero.OutputDevice,
                  time_rest: float,
                  time_travel: float):
    await a_extend(extend, retract, time_rest, time_travel)
    await a_retract(extend, retract, time_rest, time_travel)


async def a_update(on: tuple[gpiozero.OutputDevice, gpiozero.OutputDevice],
                   off: tuple[gpiozero.OutputDevice, gpiozero.OutputDevice],
                   time_rest: float,
                   time_travel: float,
                   state: bool):
    print(f"Updating actuators to {state}")
    # Retract actuators
    await asyncio.gather(
        a_retract(*off, time_rest, time_travel),
        a_retract(*on, time_rest, time_travel)
    )

    if (state):
        await a_cycle(*on, time_rest, time_travel)
    else:
        await a_cycle(*off, time_rest, time_travel)

if __name__ == "__main__":
    import argparse
    import signal
    import time
    import sys
    parser = argparse.ArgumentParser(
        description="Script of middling quality to run the actuator on a regular raspberry pi"
    )

    schedules = {
        "level_1": (generate_schedules.time_on_level_1, generate_schedules.time_off_level_1),
        "level_2": (generate_schedules.time_on_level_2, generate_schedules.time_off_level_2),
    }

    parser.add_argument("--pin_on_extend",   default=17, metavar="GPIO_PIN", help="Default: 17", type=int)
    parser.add_argument("--pin_on_retract",  default=27, metavar="GPIO_PIN", help="Default: 27", type=int)
    parser.add_argument("--pin_off_extend",  default=10, metavar="GPIO_PIN", help="Default: 10", type=int)
    parser.add_argument("--pin_off_retract", default=9,  metavar="GPIO_PIN", help="Default: 9",  type=int)
    parser.add_argument("--rest_time",
                        default=0.5,
                        metavar="SECONDS",
                        help="Rest time (in seconds) between switching actuator directions (default 0.5)",
                        type=float)
    parser.add_argument("--travel_time",
                        default=10,
                        metavar="SECONDS",
                        help="Time (in seconds) that the actuator takes to undergo a full travel (default 10)",
                        type=float)
    parser.add_argument("--region-length",
                        default=75,
                        metavar="SECONDS",
                        help="Time (in seconds) that the actuator will be continuously cycled (default 75)",
                        type=float)
    parser.add_argument("schedule_name", choices=schedules.keys())

    args = parser.parse_args()

    time_on = schedules[args.schedule_name][0]
    time_off = schedules[args.schedule_name][1]

    on_extend = gpiozero.OutputDevice(17, active_high=False, initial_value=False)
    on_retract = gpiozero.OutputDevice(27, active_high=False, initial_value=False)
    off_extend = gpiozero.OutputDevice(10, active_high=False, initial_value=False)
    off_retract = gpiozero.OutputDevice(9, active_high=False, initial_value=False)

    def SIGINT_handler(signum, frame):
        print("Caught SIGINIT, stopping!")
        on_extend.off()
        on_retract.off()
        off_extend.off()
        off_retract.off()
        sys.exit(0)
    signal.signal(signal.SIGINT, SIGINT_handler)

    sched = generate_schedules.generate_schedule(time_on, time_off)

    unix_time = datetime.datetime.now(datetime.timezone.utc).timestamp()
    state = False
    for i in sched:
        if (unix_time > i[0]):
            state = i[1]
        else:
            break

    print(f"Prior state is {state}")
    if (state):
        print("Resuming state")
        asyncio.run(
            a_update(
                (on_extend, on_retract),
                (off_extend, off_retract),
                args.rest_time,
                args.travel_time,
                state
            )
        )
    else:
        async def r():
            print("Retracting both actuators")
            await asyncio.gather(
                        a_retract(on_extend, on_retract,
                                  args.rest_time, args.travel_time),
                        a_retract(off_extend, off_retract,
                                  args.rest_time, args.travel_time)
                        )
        asyncio.run(r())
    while (1):
        unix_time = datetime.datetime.now(datetime.timezone.utc).timestamp()
        for i in sched:
            region_s = i[0]
            region_e = i[0] + args.region_length
            if (region_s < unix_time and unix_time < region_e):
                state = i[1]
                asyncio.run(
                    a_update(
                        (on_extend, on_retract),
                        (off_extend, off_retract),
                        args.rest_time,
                        args.travel_time,
                        state
                    )
                )
                break

        time.sleep(1)
