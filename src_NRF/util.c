/*
 * ----------------------------------------------------------------------
 *
 * File: util.c
 *
 * Last edited: 30.10.2025
 *
 * Copyright (c) 2024 ETH Zurich and University of Bologna
 *
 * Authors:
 * - Philip Wiese (wiesep@iis.ee.ethz.ch), ETH Zurich
 *
 * ----------------------------------------------------------------------
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>

#include "util.h"

#define GPIO_NODE_debug_signal_2 DT_NODELABEL(gpio_debug_signal_2)
static const struct gpio_dt_spec gpio_debug_2 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_2, gpios);

/**
 * @brief Sends a sync signal to the debug signal GPIO pin.
 *
 */
void sync() {
  gpio_pin_set_dt(&gpio_debug_2, 1);
  k_msleep(1);
  gpio_pin_set_dt(&gpio_debug_2, 0);
}
