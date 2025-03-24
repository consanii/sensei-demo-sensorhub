/*
 * ----------------------------------------------------------------------
 *
 * File: bme688_sensor.c
 *
 * Last edited: 30.10.2025
 *
 * Copyright (c) 2025 ETH Zurich and University of Bologna
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
#include <zephyr/drivers/sensor.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include "bme688_sensor.h"
#include "config.h"
#include "i2c_helpers.h"

static const struct device *const bme_dev = DEVICE_DT_GET_ONE(bosch_bme680);

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

void test_bme688() {
  LOG_INF("Testing BME680 (Environmental Sensor)");

  struct sensor_value temp, press, humidity, gas_res;

  gpio_pin_toggle_dt(&gpio_debug_1);
  sensor_sample_fetch(bme_dev);
  sensor_channel_get(bme_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
  sensor_channel_get(bme_dev, SENSOR_CHAN_PRESS, &press);
  sensor_channel_get(bme_dev, SENSOR_CHAN_HUMIDITY, &humidity);
  sensor_channel_get(bme_dev, SENSOR_CHAN_GAS_RES, &gas_res);
  gpio_pin_toggle_dt(&gpio_debug_1);

  LOG_INF(" - Temperature                         : %d.%06d °C" SPACES, temp.val1, temp.val2);
  LOG_INF(" - Pressure                            : %d.%06d kPa" SPACES, press.val1, press.val2);
  LOG_INF(" - Humidity                            : %d.%06d %%" SPACES, humidity.val1, humidity.val2);
  LOG_INF(" - Gas Resistance                      : %d.%06d ohm" SPACES, gas_res.val1, gas_res.val2);
}