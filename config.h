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
 * @brief Program configuration
 */
#pragma once

/******************************************************
 *                  ACTUATOR CONFIG                   *
 ******************************************************/

/* GPIO pin that signals actuator extension for "LEVEL 1 ON" actuators */
#define ACTUATOR_GPIO_LEVEL_1_ON_EXTEND 10
/* GPIO pin that signals actuator retraction for "LEVEL 1 ON" actuators */
#define ACTUATOR_GPIO_LEVEL_1_ON_RETRACT 11

/* GPIO pin that signals actuator extension for "LEVEL 1 OFF" actuators */
#define ACTUATOR_GPIO_LEVEL_1_OFF_EXTEND 12
/* GPIO pin that signals actuator retraction for "LEVEL 1 OFF" actuators */
#define ACTUATOR_GPIO_LEVEL_1_OFF_RETRACT 13

/* GPIO pin that signals actuator extension for "LEVEL 2 ON" actuators */
#define ACTUATOR_GPIO_LEVEL_2_ON_EXTEND 18
/* GPIO pin that signals actuator retraction for "LEVEL 2 ON" actuators */
#define ACTUATOR_GPIO_LEVEL_2_ON_RETRACT 19

/* GPIO pin that signals actuator extension for "LEVEL 2 OFF" actuators */
#define ACTUATOR_GPIO_LEVEL_2_OFF_EXTEND 20
/* GPIO pin that signals actuator retraction for "LEVEL 2 OFF" actuators */
#define ACTUATOR_GPIO_LEVEL_2_OFF_RETRACT 21

/** Time (in microseconds) for full actuator travel */
#define ACTUATOR_TRAVEL_TIME (10ull * 1000ull * 1000ull)

/** Time (in microseconds) to rest between switching directions */
#define ACTUATOR_REST_TIME (500ull * 1000ull)

/** Logic level that must be put to "extend" gpio pin for actuator retraction to occur */
#define ACTUATOR_ACTIVE_LOGIC_LEVEL_EXTEND 0

/** Logic level that must be put to "retract" gpio pin for actuator retraction to occur */
#define ACTUATOR_ACTIVE_LOGIC_LEVEL_RETRACT 0

/******************************************************
 *                  SCHEDULE CONFIG                   *
 ******************************************************/

/**
 * Length of time (in seconds) to repeatedly cycle the actuator when the schedule calls for an update
 */
#define SCHEDULE_TRIGGER_REGION_LENGTH (60ull)

/**
 * When reloading schedule, trigger an "ON" actuation if the last status was "ON"
 */
#define SCHEDULE_TRIGGER_REGION_ON_RESET_IF_IN_ON_REGION 1

/**
 * When reloading schedule, trigger an "OFF" actuation if the last status was "OFF"
 */
#define SCHEDULE_TRIGGER_REGION_ON_RESET_IF_IN_OFF_REGION 0

/******************************************************
 *                    MISC CONFIG                     *
 ******************************************************/

/**
 * Minimum number of microseconds between each successive printing of program status
 */
#define STATUS_PRINT_INTERVAL (1000ull * 1000ull)

/**
 * Maximum uptime permitted before hard reset
 */
#define REBOOT_INTERVAL (24ull * 60ull * 60ull * 1000ull * 1000ull)

/**
 * Minimum uptime before led switches from 2 Hz blink to 0.5 Hz blink
 */
#define SLOW_BLINK_UPTIME (1ull * 60ull * 60ull * 1000ull * 1000ull)
