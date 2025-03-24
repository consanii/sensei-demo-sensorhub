/*
 * ----------------------------------------------------------------------
 *
 * File: ilps28qsw_sensor.c
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
#include "ilps28qsw_sensor.h"

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

static const struct device *const i2c_b = DEVICE_DT_GET(DT_ALIAS(i2cb));

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

stmdev_ctx_t ilps28qsw_ctx;
i2c_ctx_t ilps28qsw_i2c_ctx;
ilps28qsw_md_t ilps28qsw_md;

void test_ilpS28qsw() {
  int32_t error = NO_ERROR;

  LOG_INF("Testing ILPS28QSW (Pressure Sensor)" SPACES);

  ilps28qsw_i2c_ctx.i2c_handle = i2c_b;
  ilps28qsw_i2c_ctx.i2c_addr = 0x5C;

  ilps28qsw_ctx.write_reg = i2c_write_reg;
  ilps28qsw_ctx.read_reg = i2c_read_reg;
  ilps28qsw_ctx.handle = &ilps28qsw_i2c_ctx;

  uint8_t ilps28qsw_id;
  error = ilps28qsw_id_get(&ilps28qsw_ctx, (ilps28qsw_id_t *)&ilps28qsw_id);
  if (error) {
    LOG_ERR(" * Error %d getting device ID", error);
  } else {
    LOG_INF(" - ID                                  : 0x%02X" SPACES, (ilps28qsw_id));
  }

  /* Restore default configuration */
  error = ilps28qsw_init_set(&ilps28qsw_ctx, ILPS28QSW_RESET);
  if (error) {
    LOG_ERR(" * Error %d during reset", error);
    return;
  }

  /* Check if device is ready */
  ilps28qsw_stat_t status;
  do {
    ilps28qsw_status_get(&ilps28qsw_ctx, &status);
  } while (status.sw_reset);

  /* Disable AH/QVAR to save power consumption */
  error = ilps28qsw_ah_qvar_en_set(&ilps28qsw_ctx, PROPERTY_DISABLE);
  if (error) {
    LOG_ERR(" * Error %d enabling AH/QVAR", error);
  }

  gpio_pin_toggle_dt(&gpio_debug_1);
  /* Set bdu and if_inc recommended for driver usage */
  error = ilps28qsw_init_set(&ilps28qsw_ctx, ILPS28QSW_DRV_RDY);
  if (error) {
    LOG_ERR(" * Error %d during init", error);
  }

  /* Select bus interface */
  ilps28qsw_bus_mode_t bus_mode;
  bus_mode.filter = ILPS28QSW_AUTO;
  error = ilps28qsw_bus_mode_set(&ilps28qsw_ctx, &bus_mode);
  if (error) {
    LOG_ERR(" * Error %d setting bus mode", error);
  }

  /* Set Output Data Rate */
  ilps28qsw_md.odr = ILPS28QSW_4Hz;
  ilps28qsw_md.avg = ILPS28QSW_16_AVG;
  ilps28qsw_md.lpf = ILPS28QSW_LPF_ODR_DIV_4;
  ilps28qsw_md.fs = ILPS28QSW_1260hPa;
  error = ilps28qsw_mode_set(&ilps28qsw_ctx, &ilps28qsw_md);
  if (error) {
    LOG_ERR(" * Error %d setting mode", error);
  }

  /* Read output only if new values are available */
  ilps28qsw_all_sources_t all_sources;
  error = ilps28qsw_all_sources_get(&ilps28qsw_ctx, &all_sources);
  if (error) {
    LOG_ERR(" * Error %d getting all sources", error);
    return;
  }

  /* Read pressure and temperature */
  ilps28qsw_data_t data;
  error = ilps28qsw_data_get(&ilps28qsw_ctx, &ilps28qsw_md, &data);
  if (error) {
    LOG_ERR(" * Error %d getting data", error);
  } else {
    LOG_INF(" - Pressure                            : %4.2f kPa" SPACES, data.pressure.hpa / 10.0);
    LOG_INF(" - Temperature                         : %4.2f °C" SPACES, data.heat.deg_c);
  }
  gpio_pin_toggle_dt(&gpio_debug_1);
}
