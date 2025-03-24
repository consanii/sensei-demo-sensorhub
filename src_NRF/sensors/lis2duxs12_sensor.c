/*
 * ----------------------------------------------------------------------
 *
 * File: lis2duxs12_sensor.c
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
#include "lis2duxs12_sensor.h"

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

static const struct device *const i2c_a = DEVICE_DT_GET(DT_ALIAS(i2ca));

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

void test_lis2duxs12() {
  LOG_INF("Testing LIS2DUXS12 (Accelerometer)" SPACES);

  int16_t error = NO_ERROR;

  i2c_ctx_t i2c_ctx;
  i2c_ctx.i2c_handle = i2c_a;
  i2c_ctx.i2c_addr = 0x19;

  stmdev_ctx_t lis2duxs12_ctx;
  lis2duxs12_ctx.write_reg = i2c_write_reg;
  lis2duxs12_ctx.read_reg = i2c_read_reg;
  lis2duxs12_ctx.handle = &i2c_ctx;

  error = lis2duxs12_exit_deep_power_down(&lis2duxs12_ctx);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d exiting deep power down", error);
  }

  uint8_t lis2duxs12_id;
  error = lis2duxs12_device_id_get(&lis2duxs12_ctx, &lis2duxs12_id);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d getting ID", error);
  } else {
    LOG_INF(" - ID                                  : 0x%02X" SPACES, lis2duxs12_id);
  }

  /* Restore default configuration */
  error = lis2duxs12_init_set(&lis2duxs12_ctx, LIS2DUXS12_RESET);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d during reset", error);
    return;
  }

  lis2duxs12_status_t status;
  do {
    lis2duxs12_status_get(&lis2duxs12_ctx, &status);
  } while (status.sw_reset);

  gpio_pin_toggle_dt(&gpio_debug_1);

  /* Set bdu and if_inc recommended for driver usage */
  error = lis2duxs12_init_set(&lis2duxs12_ctx, LIS2DUXS12_SENSOR_ONLY_ON);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d during init", error);
  }

  /* Set Output Data Rate */
  lis2duxs12_md_t md;
  md.fs = LIS2DUXS12_2g;
  md.bw = LIS2DUXS12_ODR_div_16;
  md.odr = LIS2DUXS12_1Hz6_ULP;
  error = lis2duxs12_mode_set(&lis2duxs12_ctx, &md);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d setting mode", error);
  }

  lis2duxs12_xl_data_t data_xl;
  error = lis2duxs12_xl_data_get(&lis2duxs12_ctx, &md, &data_xl);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d getting data", error);
  } else {
    LOG_INF(" - Acceleration X                      : % 7.2f mg" SPACES, data_xl.mg[0]);
    LOG_INF(" - Acceleration Y                      : % 7.2f mg" SPACES, data_xl.mg[1]);
    LOG_INF(" - Acceleration Z                      : % 7.2f mg" SPACES, data_xl.mg[2]);
  }

  lis2duxs12_outt_data_t data_temp;
  error = lis2duxs12_outt_data_get(&lis2duxs12_ctx, &md, &data_temp);
  if (error != NO_ERROR) {
    LOG_ERR(" * Error %d getting temperature", error);
  } else {
    LOG_INF(" - Temperature                         : %3.2f °C" SPACES, data_temp.heat.deg_c);
  }
  gpio_pin_toggle_dt(&gpio_debug_1);
}