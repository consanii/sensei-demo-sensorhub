/*
 * ----------------------------------------------------------------------
 *
 * File: scd41_sensor.c
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

#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include "config.h"
#include "i2c_helpers.h"
#include "scd41_sensor.h"

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

#define GPIO_NODE_scd41_pwr DT_NODELABEL(gpio_scd41_pwr)
#define GPIO_NODE_i2c_scd41_en DT_NODELABEL(gpio_ext_i2c_scd41_en)

static const struct gpio_dt_spec gpio_SCD41_pwr = GPIO_DT_SPEC_GET(GPIO_NODE_scd41_pwr, gpios);
static const struct gpio_dt_spec gpio_I2C_SCD41_EN = GPIO_DT_SPEC_GET(GPIO_NODE_i2c_scd41_en, gpios);

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

void test_scd41() {
  LOG_INF("Testing SCD41 (CO2 Sensor)" SPACES);

  int16_t error = NO_ERROR;

  error = scd4x_stop_periodic_measurement();
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d stopping periodic measurement", error);
  }

  // error = scd4x_reinit();
  // if (error != NO_ERROR) {
  //   LOG_ERR(" * Error %d reinitializing SCD41", error);
  // }

  uint16_t serial_number[3] = {0};
  error = scd4x_get_serial_number(serial_number, 3);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d getting serial number", error);
  } else {
    uint64_t serial_as_int = 0;
    sensirion_common_to_integer((uint8_t *)serial_number, (uint8_t *)&serial_as_int, LONG_INTEGER, 6);

    LOG_INF(" - Serial Number                       : %llu" SPACES, serial_as_int);
  }

  gpio_pin_toggle_dt(&gpio_debug_1);
  error = scd4x_measure_single_shot();
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d starting periodic measurement", error);
  }

  // Wait until data is ready
  bool data_ready = false;
  uint32_t time = k_uptime_get_32();
  do {
    error = scd4x_get_data_ready_status(&data_ready);
    if (error != NO_ERROR) {
      LOG_ERR(" * Error %d getting data ready status", error);
      return;
    }

    if (!data_ready) {
      k_usleep(100);

      if ((k_uptime_get_32() - time) > 10 * 1000) {
        LOG_ERR(" * SCD41 Timeout waiting for data ready status");
        break;
      }
    }
  } while (!data_ready);
  LOG_INF(" > Data ready after %u ms", k_uptime_get_32() - time);

  uint16_t co2_concentration;
  uint32_t temperature;
  uint32_t relative_humidity;
  error = scd4x_read_measurement(&co2_concentration, &temperature, &relative_humidity);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d reading measurement", error);
  } else {
    LOG_INF(" - CO2                                 : %u ppm" SPACES, co2_concentration);
    LOG_INF(" - Temperature                         : %.2f °C" SPACES, (float)(temperature / 1000.0f));
    LOG_INF(" - Humidity                            : %.2f %% RH" SPACES, (float)(relative_humidity / 1000.0f));
  }
  gpio_pin_toggle_dt(&gpio_debug_1);
}

int poweron_scd41() {
  LOG_INF("Power On SCD41 (CO2 Sensor)" SPACES);

  int16_t error_i16 = NO_ERROR;
  int32_t error_i32 = NO_ERROR;

  // Power up SCD41
  error_i32 = gpio_pin_set_dt(&gpio_SCD41_pwr, 1);
  if (error_i32 != NO_ERROR) {
    LOG_ERR("Error %d, SSCD41 EN GPIO configuration error", error_i32);
    k_msleep(1000);
    return -1;
  }

  // Connect I2C bus to SCD41
  error_i32 = gpio_pin_set_dt(&gpio_I2C_SCD41_EN, 1);
  if (error_i32 != NO_ERROR) {
    LOG_ERR("Error %d, SCD41 I2C EN GPIO init error", error_i32);
    k_msleep(1000);
    return -1;
  }

  // Wait for I2C bus to be ready
  k_msleep(100);

  // Initialize SCD41 driver
  scd4x_init(0x62);

  // Wait for SCD41 to be ready
  error_i16 = scd4x_wake_up();
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d waking up SCD41", error_i16);
  }

  return 0;
}

int poweroff_scd41() {
  int16_t error = NO_ERROR;

  LOG_INF("Power Off SCD41 (CO2 Sensor)" SPACES);

  error = scd4x_power_down();
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d powering down SCD41", error);
    k_msleep(1000);
    return -1;
  }

  // Power down SCD41
  if (gpio_pin_set_dt(&gpio_SCD41_pwr, 0) < 0) {
    LOG_ERR("SCD41 EN GPIO configuration error");
    k_msleep(1000);
    return -1;
  }

  if (gpio_pin_set_dt(&gpio_I2C_SCD41_EN, 0) < 0) {
    LOG_ERR("SCD41 I2C EN GPIO configuration error");
    k_msleep(1000);
    return -1;
  }

  return 0;
}
