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
 * @brief Timestamp/Timedelta formatting interface
 */
#pragma once

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint64_t */

/**
 * Format microseconds value as "DD:HH:MM:SS.SSSSSS"
 *
 * @param us Time-delta to format
 * @param buffer Buffer to write to
 * @param buf_size Buffer size
 *
 * @returns Buffer pointer
 */
char* fdelta_us(const uint64_t us, char* const buffer, size_t buf_size);

/**
 * Format seconds value as "DD:HH:MM:SS"
 *
 * @param s Time-delta to format
 * @param buffer Buffer to write to
 * @param buf_size Buffer size
 *
 * @returns Buffer pointer
 */
char* fdelta(const uint64_t s, char* const buffer, size_t buf_size);

/**
 * Format microseconds since 1970 timestamp value as "YYYY-MM-DD HH:MM:SS.SSSSSS" adjusted to the local timezone as configured in @sa config.h
 *
 * @param us Time-delta to format
 * @param buffer Buffer to write to
 * @param buf_size Buffer size
 *
 * @returns Buffer pointer
 */
char* ftime_us(const uint64_t us, char* const buffer, size_t buf_size);

/**
 * Format seconds since 1970 timestamp value as "YYYY-MM-DD HH:MM:SS" adjusted to the local timezone as configured in @sa config.h
 *
 * @param us Time-delta to format
 * @param buffer Buffer to write to
 * @param buf_size Buffer size
 *
 * @returns Buffer pointer
 */
char* ftime(const uint64_t s, char* const buffer, size_t buf_size);
