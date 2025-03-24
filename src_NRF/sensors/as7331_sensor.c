/*
 * ----------------------------------------------------------------------
 *
 * File: as7331_sensor.c
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

#include "as7331_reg.h"
#include "as7331_sensor.h"
#include "config.h"
#include "i2c_helpers.h"

#define GPIO_NODE_i2c_as7331_en DT_NODELABEL(gpio_ext_i2c_as7331_en)
static const struct gpio_dt_spec gpio_I2C_AS7331_EN = GPIO_DT_SPEC_GET(GPIO_NODE_i2c_as7331_en, gpios);

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

static const struct device *const i2c_b = DEVICE_DT_GET(DT_ALIAS(i2cb));

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

i2c_ctx_t as7331_i2c_ctx;
as7331_t as7331_ctx;

as7331_reg_osrstat_t print_as7331_status(as7331_t *as7331_ctx) {
  as7331_reg_osrstat_t status = {0};

  int error = as7331_get_status(as7331_ctx, &status);
  if (error) {
    LOG_ERR(" * Error getting status");
    return status;
  } else {
    LOG_INF(" - Status                              : 0x%04X", status.word);
    LOG_INF("   - Device Operating State            : %d", status.osr.dos);
    LOG_INF("   - Software Reset                    : %d", status.osr.sw_res);
    LOG_INF("   - Power Down Enabled                : %d", status.osr.pd);
    LOG_INF("   - Start State                       : %d", status.osr.ss);
    LOG_INF("   - Power State                       : %d", status.powerstate);
    LOG_INF("   - Standby State                     : %d", status.standbystate);
    LOG_INF("   - Not Ready                         : %d", status.notready);
    LOG_INF("   - Data Ready                        : %d", status.ndata);
    LOG_INF("   - Data Overwrite                    : %d", status.ldata);
    LOG_INF("   - ADC Overflow                      : %d", status.adcof);
    LOG_INF("   - MRES Overflow                     : %d", status.mresof);
    LOG_INF("   - Outconv Overflow                  : %d", status.outconvof);
  }

  return status;
}

void test_as7331() {
  LOG_INF("Testing AS7331 (UV Sensor)" SPACES);
  int error = NO_ERROR;

  // Specify sensor parameters //
  MMODE as7331_mmode = AS7331_CMD_MODE; // choices are modes are CONT, CMD, SYNS, SYND
  CCLK as7331_cclk = AS7331_1024;       // choices are 1.024, 2.048, 4.096, or 8.192 MHz
  uint8_t as7331_sb = 0x00;             // standby enabled 0x01 (to save power), standby disabled 0x00
  uint8_t as7331_breakTime = 40; // sample time == 8 us x breakTime (0 - 255, or 0 - 2040 us range), CONT or SYNX modes

  uint8_t as7331_gain = 10; // ADCGain = 2^(11-gain), by 2s, 1 - 2048 range,  0 < gain = 11 max, default 10
  uint8_t as7331_time = 12; // 2^time in ms, so 0x07 is 2^6 = 64 ms, 0 < time = 15 max, default  6

  // print_as7331_status(&sensor);

  LOG_DBG(" * Resetting AS7331");
  error = as7331_reset(&as7331_ctx);
  if (error) {
    LOG_ERR(" * Error resetting AS7331");
    return;
  }

  gpio_pin_toggle_dt(&gpio_debug_1);
  LOG_DBG(" * Powering up AS7331");
  error = as7331_power_up(&as7331_ctx);
  if (error) {
    LOG_ERR(" * Error powering up AS7331");
    return;
  }

  // Set configuration mode
  LOG_DBG(" * Setting configuration mode");
  error = as7331_set_configuration_mode(&as7331_ctx);
  if (error) {
    LOG_ERR(" * Error setting configuration mode");
  }

  // Get ID
  LOG_DBG(" * Getting ID");
  uint8_t id;
  error = as7331_get_chip_id(&as7331_ctx, &id);
  if (error) {
    LOG_ERR(" * Error getting ID");
  } else {
    LOG_INF(" - ID                                  : 0x%02X" SPACES, id);
  }

  LOG_DBG(" * Initializing AS7331");
  error = as7331_init(&as7331_ctx, as7331_mmode, as7331_cclk, as7331_sb, as7331_breakTime, as7331_gain, as7331_time);
  if (error) {
    LOG_ERR(" * Error initializing AS7331");
  }

  // Set measurement mode
  LOG_DBG(" * Setting measurement mode");
  error = as7331_set_measurement_mode(&as7331_ctx);
  if (error) {
    LOG_ERR(" * Error setting measurement mode");
  }

  // One shot
  LOG_DBG(" * Starting one shot");
  error = as7331_start_measurement(&as7331_ctx);
  if (error) {
    LOG_ERR(" * Error starting one shot");
  }

  bool data_ready = false;
  uint32_t time = k_uptime_get_32();
  as7331_reg_osrstat_t status;
  do {
    error = as7331_get_status(&as7331_ctx, &status);
    if (error) {
      LOG_ERR(" * Error getting status");
      return;
    } else {
      LOG_DBG(" - Status                              : 0x%04X", status.word);
      LOG_DBG("   - Device Operating State            : %d", status.osr.dos);
      LOG_DBG("   - Software Reset                    : %d", status.osr.sw_res);
      LOG_DBG("   - Power Down Enabled                : %d", status.osr.pd);
      LOG_DBG("   - Start State                       : %d", status.osr.ss);
      LOG_DBG("");
      LOG_DBG("   - Power State                       : %d", status.powerstate);
      LOG_DBG("   - Standby State                     : %d", status.standbystate);
      LOG_DBG("   - Not Ready                         : %d", status.notready);
      LOG_DBG("   - Data Ready                        : %d", status.ndata);
      LOG_DBG("   - Data Overwrite                    : %d", status.ldata);
      LOG_DBG("   - ADC Overflow                      : %d", status.adcof);
      LOG_DBG("   - MRES Overflow                     : %d", status.mresof);
      LOG_DBG("   - Outconv Overflow                  : %d", status.outconvof);
    }
    data_ready = status.ndata;

    if (!data_ready) {
      k_usleep(100);

      if ((k_uptime_get_32() - time) > 10 * 1000) {
        LOG_ERR(" * BH1730FVC Timeout waiting for data ready status");
        break;
      }
    }
  } while (!data_ready);
  LOG_INF(" > Data ready after %u ms", k_uptime_get_32() - time);

  // Read all
  struct {
    uint16_t temp;
    uint16_t uva;
    uint16_t uvb;
    uint16_t uvc;
  } all;
  error = as7331_read_all(&as7331_ctx, (uint16_t *)&all);
  if (error) {
    LOG_ERR(" * Error reading all");
  } else {
    float temp = all.temp * 0.05f - 66.9f;

    LOG_INF(" - Temp                                : %.2f °C" SPACES, temp);
    LOG_INF(" - UVA                                 : %u" SPACES, all.uva);
    LOG_INF(" - UVB                                 : %u" SPACES, all.uvb);
    LOG_INF(" - UVC                                 : %u" SPACES, all.uvc);
  }

  gpio_pin_toggle_dt(&gpio_debug_1);
}

int poweron_as7331() {
  LOG_INF("Power On AS7331 (UV Sensor)" SPACES);
  int error = NO_ERROR;

  as7331_i2c_ctx.i2c_handle = i2c_b;
  as7331_i2c_ctx.i2c_addr = AS7331_I2C_ADD;

  as7331_ctx.ctx.read_reg = i2c_read_reg;
  as7331_ctx.ctx.write_reg = i2c_write_reg;
  as7331_ctx.ctx.handle = &as7331_i2c_ctx;

  // Power up AS7331
  if (gpio_pin_set_dt(&gpio_I2C_AS7331_EN, 1) < 0) {
    LOG_ERR("AS7331 I2C EN GPIO configuration error");
    k_msleep(1000);
    return -1;
  }

  // Wait for I2C bus to be ready
  k_msleep(100);

  error = as7331_power_up(&as7331_ctx);
  if (error) {
    LOG_ERR(" * Error powering up AS7331");
    k_msleep(1000);
    return -1;
  }

  return 0;
}

int poweroff_as7331() {
  LOG_INF("Power Off AS7331 (UV Sensor)" SPACES);

  int error = NO_ERROR;

  // Power down
  error = as7331_power_down(&as7331_ctx);
  if (error) {
    LOG_ERR(" * Error powering down AS7331");
    k_msleep(1000);
    return -1;
  }

  // Disconnect sensor from I2C bus
  if (gpio_pin_set_dt(&gpio_I2C_AS7331_EN, 0) < 0) {
    LOG_ERR("AS7331 I2C EN GPIO configuration error");
    k_msleep(1000);
    return -1;
  }
  return 0;
}