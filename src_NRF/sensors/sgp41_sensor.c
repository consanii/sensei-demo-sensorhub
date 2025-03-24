/*
 * ----------------------------------------------------------------------
 *
 * File: sgp41_sensor.c
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

#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include "config.h"
#include "i2c_helpers.h"
#include "sgp41_sensor.h"

#define GPIO_NODE_i2c_sgp41_en DT_NODELABEL(gpio_ext_i2c_sgp41_en)
#define GPIO_NODE_sgp41_pwr DT_NODELABEL(gpio_sgp41_pwr)

static const struct gpio_dt_spec gpio_I2C_SGP41_EN = GPIO_DT_SPEC_GET(GPIO_NODE_i2c_sgp41_en, gpios);
static const struct gpio_dt_spec gpio_SGP41_pwr = GPIO_DT_SPEC_GET(GPIO_NODE_sgp41_pwr, gpios);

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

void test_sgp41() {
  LOG_INF("Testing SGP41 (VOC Sensor)" SPACES);

  int16_t error = NO_ERROR;
  uint16_t serial_number[3] = {0};
  error = sgp41_get_serial_number(serial_number);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d getting serial number", error);
  } else {
    uint64_t serial_as_int = 0;
    sensirion_common_to_integer((uint8_t *)serial_number, (uint8_t *)&serial_as_int, LONG_INTEGER, 6);
    LOG_INF(" - Serial Number                       : %llu" SPACES, serial_as_int);
  }

  // Self Test
  uint16_t test_result;
  error = sgp41_execute_self_test(&test_result);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d self testing", error);
  } else {
    LOG_INF(" - Self Test                           : 0x%04X" SPACES, test_result);
  }

  uint16_t default_rh = 0x8000;
  uint16_t default_t = 0x6666;
  uint16_t sraw_voc;
  uint16_t sraw_nox;

  gpio_pin_toggle_dt(&gpio_debug_1);

  // WIESEP: Skip conditioning phase
  // error = sgp41_execute_conditioning(default_rh, default_t, &sraw_voc);
  // if (error != NO_ERROR) {
  //   LOG_ERR(" * Error %d measuring raw", error);
  // } else {
  //   LOG_INF(" - SRAW VOC (Conditioning)             : %u" SPACES, sraw_voc);
  // }
  // LOG_INF(" - Start conditioning for 5s");

  error = sgp41_measure_raw_signals(default_rh, default_t, &sraw_voc, &sraw_nox);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d reading signals", error);
    return;
  } else {
    LOG_INF(" - SRAW VOC                            : %u" SPACES, sraw_voc);
    LOG_INF(" - SRAW NOX                            : %u" SPACES, sraw_nox);
  }
  gpio_pin_toggle_dt(&gpio_debug_1);
}

int poweron_sgp41() {
  LOG_INF("Power On SGP41 (VOC Sensor)" SPACES);

  // Power up SGP41
  if (gpio_pin_set_dt(&gpio_SGP41_pwr, 1) < 0) {
    LOG_ERR("SGP41 EN GPIO configuration error");
    k_msleep(1000);
    return -1;
  }

  if (gpio_pin_set_dt(&gpio_I2C_SGP41_EN, 1) < 0) {
    LOG_ERR("SGP41 I2C EN GPIO configuration error");
    k_msleep(1000);
    return -1;
  }
  // Wait for I2C bus to be ready
  k_msleep(100);

  return 0;
}

int poweroff_sgp41() {
  LOG_INF("Power Off SGP41 (VOC Sensor)" SPACES);

  // Power down SGP41
  if (gpio_pin_set_dt(&gpio_I2C_SGP41_EN, 0) < 0) {
    LOG_ERR("SGP41 I2C EN GPIO configuration error");
    k_msleep(1000);
    return -1;
  }

  if (gpio_pin_set_dt(&gpio_SGP41_pwr, 0) < 0) {
    LOG_ERR("SGP41 EN GPIO configuration error");
    k_msleep(1000);
    return -1;
  }

  return 0;
}