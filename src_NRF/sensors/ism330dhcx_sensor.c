/*
 * ----------------------------------------------------------------------
 *
 * File: ism330dhcx_sensor.c
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
#include "ism330dhcx_sensor.h"

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

static const struct device *const i2c_a = DEVICE_DT_GET(DT_ALIAS(i2ca));

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

int32_t ism330dhcx_xl_sensitivity(const stmdev_ctx_t *ctx, float *sensitivity) {
  int32_t error = NO_ERROR;
  ism330dhcx_fs_xl_t sensitivity_raw;
  error = ism330dhcx_xl_full_scale_get(ctx, &sensitivity_raw);
  if (error) {
    LOG_ERR(" * Error %d getting accel sensitivity", error);
  }

  switch (sensitivity_raw) {
  case ISM330DHCX_2g:
    *sensitivity = ISM330DHCX_ACC_SENSITIVITY_FS_2G;
    break;
  case ISM330DHCX_4g:
    *sensitivity = ISM330DHCX_ACC_SENSITIVITY_FS_4G;
    break;
  case ISM330DHCX_8g:
    *sensitivity = ISM330DHCX_ACC_SENSITIVITY_FS_8G;
    break;
  case ISM330DHCX_16g:
    *sensitivity = ISM330DHCX_ACC_SENSITIVITY_FS_16G;
    break;
  default:
    k_msleep(1000);
    return -1;
  }

  return 0;
}

int32_t ism330dhcx_gy_sensitivity(const stmdev_ctx_t *ctx, float *sensitivity) {
  int32_t error = NO_ERROR;
  ism330dhcx_fs_g_t sensitivity_raw;
  error = ism330dhcx_gy_full_scale_get(ctx, &sensitivity_raw);
  if (error) {
    LOG_ERR(" * Error %d getting gyro sensitivity", error);
  }

  switch (sensitivity_raw) {
  case ISM330DHCX_125dps:
    *sensitivity = ISM330DHCX_GYRO_SENSITIVITY_FS_125DPS;
    break;
  case ISM330DHCX_250dps:
    *sensitivity = ISM330DHCX_GYRO_SENSITIVITY_FS_250DPS;
    break;
  case ISM330DHCX_500dps:
    *sensitivity = ISM330DHCX_GYRO_SENSITIVITY_FS_500DPS;
    break;
  case ISM330DHCX_1000dps:
    *sensitivity = ISM330DHCX_GYRO_SENSITIVITY_FS_1000DPS;
    break;
  case ISM330DHCX_2000dps:
    *sensitivity = ISM330DHCX_GYRO_SENSITIVITY_FS_2000DPS;
    break;
  case ISM330DHCX_4000dps:
    *sensitivity = ISM330DHCX_GYRO_SENSITIVITY_FS_4000DPS;
    break;
  default:
    k_msleep(1000);
    return -1;
  }

  return 0;
}

void test_ism330dhcx() {
  int32_t error = NO_ERROR;
  LOG_INF("Testing ISM330DHCX (IMU)" SPACES);

  i2c_ctx_t i2c_ctx;
  i2c_ctx.i2c_handle = i2c_a;
  i2c_ctx.i2c_addr = 0x6A;

  stmdev_ctx_t ism330dhcx_ctx;
  ism330dhcx_ctx.write_reg = i2c_write_reg;
  ism330dhcx_ctx.read_reg = i2c_read_reg;
  ism330dhcx_ctx.handle = &i2c_ctx;

  uint8_t ism330dhcx_id;
  error = ism330dhcx_device_id_get(&ism330dhcx_ctx, &ism330dhcx_id);
  if (error) {
    LOG_ERR(" * Error %d getting device ID", error);
  } else {
    LOG_INF(" - ID                                  : 0x%02X" SPACES, ism330dhcx_id);
  }

  /* SW reset */
  error = ism330dhcx_reset_set(&ism330dhcx_ctx, PROPERTY_ENABLE);
  if (error) {
    LOG_ERR(" * Error %d during SW reset", error);
  }

  /* Enable register auto-increment */
  error = ism330dhcx_auto_increment_set(&ism330dhcx_ctx, PROPERTY_ENABLE);
  if (error) {
    LOG_ERR(" * Error %d enabling auto-increment", error);
  }

  /* Enable BDU */
  error = ism330dhcx_block_data_update_set(&ism330dhcx_ctx, PROPERTY_ENABLE);
  if (error) {
    LOG_ERR(" * Error %d enabling BDU", error);
  }

  /* FIFO mode selection */
  error = ism330dhcx_fifo_mode_set(&ism330dhcx_ctx, ISM330DHCX_BYPASS_MODE);
  if (error) {
    LOG_ERR(" * Error %d setting FIFO mode", error);
  }

  /* ACCELEROMETER Output data rate */
  error = ism330dhcx_xl_data_rate_set(&ism330dhcx_ctx, ISM330DHCX_XL_ODR_12Hz5);
  if (error) {
    LOG_ERR(" * Error %d setting accel ODR", error);
  }

  /* ACCELEROMETER Full scale */
  error = ism330dhcx_xl_full_scale_set(&ism330dhcx_ctx, ISM330DHCX_2g);
  if (error) {
    LOG_ERR(" * Error %d setting accel full scale", error);
  }

  /* GYROSCOPE Output data rate */
  error = ism330dhcx_gy_data_rate_set(&ism330dhcx_ctx, ISM330DHCX_GY_ODR_12Hz5);
  if (error) {
    LOG_ERR(" * Error %d setting gyro ODR", error);
  }

  /* GYROSCOPE Full scale */
  error = ism330dhcx_gy_full_scale_set(&ism330dhcx_ctx, ISM330DHCX_2000dps);
  if (error) {
    LOG_ERR(" * Error %d setting gyro full scale", error);
  }

  float sensitivity = 0.0f;
  int16_t data_raw[3];
  float Acceleration[3];

  /* Get accelerometer sensitivity */
  error = ism330dhcx_xl_sensitivity(&ism330dhcx_ctx, &sensitivity);
  if (error) {
    LOG_ERR(" * Error %d getting accel sensitivity", error);
  }

  gpio_pin_toggle_dt(&gpio_debug_1);
  /* Read raw accelerometer data */
  error = ism330dhcx_acceleration_raw_get(&ism330dhcx_ctx, data_raw);
  if (error) {
    LOG_ERR(" * Error %d reading accel raw data", error);
  }

  /* Calculate and log acceleration data */
  Acceleration[0] = (data_raw[0] * sensitivity);
  Acceleration[1] = (data_raw[1] * sensitivity);
  Acceleration[2] = (data_raw[2] * sensitivity);

  LOG_INF(" - Acceleration X                      : % 7.2f mg" SPACES, Acceleration[0]);
  LOG_INF(" - Acceleration Y                      : % 7.2f mg" SPACES, Acceleration[1]);
  LOG_INF(" - Acceleration Z                      : % 7.2f mg" SPACES, Acceleration[2]);

  /* Get gyroscope sensitivity */
  error = ism330dhcx_gy_sensitivity(&ism330dhcx_ctx, &sensitivity);
  if (error) {
    LOG_ERR(" * Error %d getting gyro sensitivity", error);
  }

  /* Read raw gyroscope data */
  error = ism330dhcx_angular_rate_raw_get(&ism330dhcx_ctx, data_raw);
  if (error) {
    LOG_ERR(" * Error %d reading gyro raw data", error);
  }

  /* Calculate and log gyroscope data */
  float Gyroscope[3];
  Gyroscope[0] = (data_raw[0] * sensitivity);
  Gyroscope[1] = (data_raw[1] * sensitivity);
  Gyroscope[2] = (data_raw[2] * sensitivity);

  LOG_INF(" - Gyroscope X                         : % 10.2f °/s" SPACES, Gyroscope[0] / 1000.f);
  LOG_INF(" - Gyroscope Y                         : % 10.2f °/s" SPACES, Gyroscope[1] / 1000.f);
  LOG_INF(" - Gyroscope Z                         : % 10.2f °/s" SPACES, Gyroscope[2] / 1000.f);
  gpio_pin_toggle_dt(&gpio_debug_1);
}