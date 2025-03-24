/*
 * ----------------------------------------------------------------------
 *
 * File: bh1730fvc_sensor.c
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

#include "bh1730fvc_sensor.h"
#include "config.h"
#include "i2c_helpers.h"

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

static const struct device *const i2c_b = DEVICE_DT_GET(DT_ALIAS(i2cb));

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

bh1730_t bh1730_ctx;
i2c_ctx_t bh1730_i2c_ctx;

void test_bh1730fvc() {
  LOG_INF("Testing BH1730FVC (Light Sensor)" SPACES);

  int error;

  gpio_pin_toggle_dt(&gpio_debug_1);
  error = bh1730_init(&bh1730_ctx, BH1730_GAIN_X64, BH1730_INT_50MS);
  if (error) {
    LOG_ERR(" * Error initializing BH1730FVC");
    return;
  } else {
    LOG_INF(" - Integration Time                    : %.2f ms" SPACES, (bh1730_ctx.integration_time_us / 1000.f));
    LOG_INF(" - Gain                                : x%d" SPACES, bh1730_ctx.gain);
  }

  uint8_t data_ready = false;
  uint32_t time = k_uptime_get_32();
  do {
    error = bh1730_valid(&bh1730_ctx, &data_ready);
    if (error) {
      LOG_ERR(" * Error reading valid status");
      return;
    }

    if (!data_ready) {
      k_usleep(100);

      if ((k_uptime_get_32() - time) > 10 * 1000) {
        LOG_ERR(" * BH1730FVC Timeout waiting for data ready status");
        break;
      }
    }
  } while (!data_ready);
  LOG_INF(" > Data ready after %u ms", k_uptime_get_32() - time);

  uint16_t visible;
  error = bh1730_read_visible(&bh1730_ctx, &visible);
  if (error) {
    LOG_ERR(" * Error reading visible light");
  } else {
    LOG_INF(" - Visible                             : %u" SPACES, visible);
  }

  uint16_t ir;
  error = bh1730_read_ir(&bh1730_ctx, &ir);
  if (error) {
    LOG_ERR(" * Error reading IR light");
  } else {
    LOG_INF(" - IR                                  : %u" SPACES, ir);
  }

  uint32_t lux;
  error = bh1730_read_lux(&bh1730_ctx, &lux);
  if (error) {
    LOG_ERR(" * Error reading lux");
  } else {
    LOG_INF(" - LUX                                 : %u" SPACES, lux);
  }
  gpio_pin_toggle_dt(&gpio_debug_1);
}

int poweron_bh1730() {
  LOG_INF("Power On BH1730FVC (Light Sensor)" SPACES);
  int error = NO_ERROR;

  bh1730_i2c_ctx.i2c_handle = i2c_b;
  bh1730_i2c_ctx.i2c_addr = BH1730_I2C_ADD;

  bh1730_ctx.ctx.read_reg = i2c_read_reg;
  bh1730_ctx.ctx.write_reg = i2c_write_reg;
  bh1730_ctx.ctx.handle = &bh1730_i2c_ctx;

  error = bh1730_power_on(&bh1730_ctx);
  if (error) {
    LOG_ERR(" * Error powering on BH1730FVC");
    k_msleep(1000);
    return -1;
  }
  return 0;
}

int poweroff_bh1730() {
  LOG_INF("Power Off BH1730FVC (Light Sensor)" SPACES);
  int error = NO_ERROR;

  error = bh1730_power_down(&bh1730_ctx);
  if (error) {
    LOG_ERR(" * Error powering down BH1730FVC");
    k_msleep(1000);
    return -1;
  }

  return 0;
}