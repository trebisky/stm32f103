/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2012 LeafLabs, LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @file libmaple/stm32f1/include/series/pwr.h
 * @author Marti Bolivar <mbolivar@leaflabs.com>
 * @brief STM32F1 Power control (PWR) support.
 */

#ifndef _LIBMAPLE_STM32F1_PWR_H_
#define _LIBMAPLE_STM32F1_PWR_H_

/*
 * Register bit definitions
 */

/* Control register */

/* PVD level selection */
#define PWR_CR_PLS_2_2V (0x0 << 5)
#define PWR_CR_PLS_2_3V (0x1 << 5)
#define PWR_CR_PLS_2_4V (0x2 << 5)
#define PWR_CR_PLS_2_5V (0x3 << 5)
#define PWR_CR_PLS_2_6V (0x4 << 5)
#define PWR_CR_PLS_2_7V (0x5 << 5)
#define PWR_CR_PLS_2_8V (0x6 << 5)
#define PWR_CR_PLS_2_9V (0x7 << 5)

#endif
