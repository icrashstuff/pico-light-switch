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
 * @brief Entry point of pico-light-switch
 */
/**
 * @mainpage
 * TODO
 *
 * @par Parts required
 * - Raspberry Pi Pico 2 W\n
 * - A linear actuator
 *
 * @par Configuration
 * @ref main.c
 */
#include <stdio.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "actuator.h"

int main()
{
    stdio_init_all();

    struct actuator_t actuator0;
    {
        struct actuator_config_t cinfo = {};
        cinfo.time_travel = 10 * 1000 * 1000;
        cinfo.time_rest = 500 * 1000;
        cinfo.gpio_extend = 18;
        cinfo.gpio_retract = 19;
        cinfo.logic_active_level_extend = 0;
        cinfo.logic_active_level_retract = 0;
        actuator_init(&actuator0, &cinfo);
    }

    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    while (actuator_in_cycle(&actuator0))
        actuator_poll(&actuator0);

    actuator_trigger(&actuator0);

    while (1)
        actuator_poll(&actuator0);

    cyw43_arch_deinit();
    return 0;
}
