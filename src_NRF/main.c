/*
 * ----------------------------------------------------------------------
 *
 * File: main.c
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
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "pwr/pwr.h"
#include "pwr/pwr_common.h"
#include "pwr/thread_pwr.h"

#include "config.h"
#include "i2c_helpers.h"
#include "test.h"
#include "util.h"

#include "as7331_sensor.h"
#include "bh1730fvc_sensor.h"
#include "bme688_sensor.h"
#include "ilps28qsw_sensor.h"
#include "ism330dhcx_sensor.h"
#include "lis2duxs12_sensor.h"
#include "max77654_sensor.h"
#include "max_m10s_sensor.h"
#include "scd41_sensor.h"
#include "sgp41_sensor.h"

static const struct device *const bme_dev = DEVICE_DT_GET_ONE(bosch_bme680);
static const struct device *const uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

#define GPIO_NODE_debug_signal_2 DT_NODELABEL(gpio_debug_signal_2)
static const struct gpio_dt_spec gpio_debug_2 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_2, gpios);

#define GPIO_NODE_ext_as7331_ready DT_NODELABEL(gpio_ext_as7331_ready)
static const struct gpio_dt_spec gpio_ext_as7331_ready = GPIO_DT_SPEC_GET(GPIO_NODE_ext_as7331_ready, gpios);

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// Imported from ilps28qsw_sensor.c
extern stmdev_ctx_t ilps28qsw_ctx;
extern ilps28qsw_md_t ilps28qsw_md;

// Imported from bh1730_sensor.c
extern bh1730_t bh1730_ctx;

// Imported from as7331_sensor.c
extern i2c_ctx_t as7331_i2c_ctx;
extern as7331_t as7331_ctx;

typedef struct sensor_values {
  uint32_t timestamp;
  uint16_t scd41_co2;
  float scd41_temperature;
  float scd41_humidity;
  uint16_t sgp41_voc;
  uint16_t sgp41_nox;
  float ilps28qsw_pressure;
  float ilps28qsw_temperature;
  float bme688_temperature;
  float bme688_pressure;
  float bme688_humidity;
  float bme688_gas_resistance;
  uint16_t bh1730_visible;
  uint16_t bh1730_ir;
  uint32_t bh1730_lux;
  float as7331_temp;
  uint16_t as7331_uva;
  uint16_t as7331_uvb;
  uint16_t as7331_uvc;
} __attribute__((aligned(4))) sensor_values_t;

int main(void) {
  int16_t error_i16 = NO_ERROR;
  int32_t error_i32 = NO_ERROR;

  uint16_t default_rh = 0x8000;
  uint16_t default_t = 0x6666;

  LOG_INIT();

  // Clear screen
  LOG_INF("Sensor Shield Scan Test on %s", CONFIG_BOARD);

  gpio_pin_set_dt(&gpio_debug_1, 0);
  gpio_pin_set_dt(&gpio_debug_2, 0);

  // Set gpio_ext_as7331_ready as active high input
  gpio_pin_configure_dt(&gpio_ext_as7331_ready, GPIO_INPUT);

  // Initialize and start power management
  pwr_init();
  pwr_start();

  k_msleep(100);

  if (!device_is_ready(uart_dev)) {
    LOG_ERR("CDC ACM device not ready");
    k_msleep(1000);
    return -1;
  }

  error_i32 = usb_enable(NULL);
  if (error_i32 != NO_ERROR) {
    k_msleep(1000);
    return -1;
  }
  LOG_INF("USB enabled");

  if (!device_is_ready(bme_dev)) {
    LOG_ERR("BME688 not not ready.");
    k_msleep(1000);
    return -1;
  }
  LOG_INF("Device %p name is %s", bme_dev, bme_dev->name);

  k_msleep(5000);

  // ------------------- Sensor Tests ----------------------------------------------------------------------------------
  LOG_INF("===== Testing all sensors ======");
  test_sensors();

  // ------------------- Sensor Data Collection ------------------------------------------------------------------------
  LOG_INF("===== Gathering Data ======");
  sensor_values_t sensor_values = {0};

  // ----------------- SCD41 (CO2 Sensor) ------------------------------------------------------------------------------
  LOG_INF("Preparing SCD41");
  LOG_INF(" - Turn on SCD41");
  poweron_scd41();
  error_i16 = scd4x_start_periodic_measurement();
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d starting periodic measurement", error_i16);
    k_msleep(1000);
    return -1;
  }

  // -----------------  SGP41 (VOC Sensor) -----------------------------------------------------------------------------
  LOG_INF("Preparing SGP41");
  LOG_INF(" - Turn on SGP41");
  poweron_sgp41();

  LOG_INF(" - Start conditioning for 10s");
  // Perform conditioning on SGP41 for 10s
  error_i16 = sgp41_execute_conditioning(default_rh, default_t, &sensor_values.sgp41_voc);
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d starting conditioning", error_i16);
    k_msleep(1000);
    return -1;
  }
  k_msleep(10000);
  LOG_INF(" - SRAW VOC (Conditioning)             : %u", sensor_values.sgp41_voc);

  LOG_INF(" - Start measuring raw signals");
  sgp41_measure_raw_signals(default_rh, default_t, &sensor_values.sgp41_voc, &sensor_values.sgp41_nox);
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d reading signals", error_i16);
    k_msleep(1000);
    return -1;
  } else {
    LOG_INF(" - SRAW VOC                            : %u", sensor_values.sgp41_voc);
    LOG_INF(" - SRAW NOX                            : %u", sensor_values.sgp41_nox);
  }

  // ----------------- ILPS28QSW (Pressure Sensor) ---------------------------------------------------------------------
  // No power on needed.

  // ----------------- BME688 (Environmental Sensor) -------------------------------------------------------------------
  // No power on needed.

  // ----------------- BH1730FVC (Ambient Light Sensor) ----------------------------------------------------------------
  LOG_INF("Preparing BH1730FVC");
  LOG_INF(" - Turn on BH1730FVC");
  error_i16 = poweron_bh1730();
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d powering on BH1730FVC", error_i16);
    k_msleep(1000);
    return -1;
  }

  LOG_INF(" - Configuring BH1730FVC");
  error_i32 = bh1730_init(&bh1730_ctx, BH1730_GAIN_X64, BH1730_INT_50MS);
  if (error_i32) {
    LOG_ERR(" * Error %d initializing BH1730FVC", error_i32);
    k_msleep(1000);
    return -1;
  } else {
    LOG_INF(" - Integration Time                    : %.2f ms" SPACES, (bh1730_ctx.integration_time_us / 1000.f));
    LOG_INF(" - Gain                                : x%d" SPACES, bh1730_ctx.gain);
  }

  // ----------------- AS7331 (UV Sensor) ----------------------------------------------------------------------------
  LOG_INF("Preparing AS7331");
  LOG_INF(" - Turn on AS7331");
  error_i16 = poweron_as7331();
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d powering on AS7331", error_i16);
    k_msleep(1000);
    return -1;
  }

  error_i32 = as7331_reset(&as7331_ctx);
  if (error_i32) {
    LOG_ERR(" * Error %d resetting AS7331", error_i32);
    k_msleep(1000);
    return -1;
  }

  error_i16 = poweron_as7331();
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d powering on AS7331", error_i16);
    k_msleep(1000);
    return -1;
  }

  // Specify sensor parameters //
  MMODE AS7331_mmode = AS7331_CMD_MODE; // choices are modes are CONT, CMD, SYNS, SYND
  CCLK AS7331_cclk = AS7331_1024;       // choices are 1.024, 2.048, 4.096, or 8.192 MHz
  uint8_t AS7331_sb = 0x00;             // standby enabled 0x01 (to save power), standby disabled 0x00
  uint8_t AS7331_breakTime = 255; // sample time == 8 us x breakTime (0 - 255, or 0 - 2040 us range), CONT or SYNX modes

  uint8_t AS7331_gain = 10; // ADCGain = 2^(11-gain), by 2s, 1 - 2048 range,  0 < gain = 11 max, default 10
  uint8_t AS7331_time = 11; // 2^time in ms, so 0x07 is 2^6 = 64 ms, 0 < time = 15 max, default  6

  LOG_INF(" - Configuring AS7331");
  error_i32 = as7331_set_configuration_mode(&as7331_ctx);
  if (error_i32) {
    LOG_ERR(" * Error %d setting configuration mode", error_i32);
    k_msleep(1000);
    return -1;
  }
  error_i32 =
      as7331_init(&as7331_ctx, AS7331_mmode, AS7331_cclk, AS7331_sb, AS7331_breakTime, AS7331_gain, AS7331_time);
  if (error_i32) {
    LOG_ERR(" * Error %d initializing sensor", error_i32);
    k_msleep(1000);
    return -1;
  }

  LOG_INF(" - Starting continuous measurement");
  error_i32 = as7331_set_measurement_mode(&as7331_ctx);
  if (error_i32) {
    LOG_ERR(" * Error %d setting measurement mode", error_i32);
    k_msleep(1000);
    return -1;
  }

  // Already start first  measurement
  error_i32 = as7331_start_measurement(&as7331_ctx);
  if (error_i32) {
    LOG_ERR(" * AS7331 Error %d starting one-shot measurement", error_i32);
    k_msleep(1000);
    return -1;
  }

  // ----------------- CSV Header --------------------------------------------------------------------------------------
  // Print CSV header for sensor_values
  printf("Timestamp,"
         "SCD41_CO2,"
         "SCD41_Temperature,"
         "SCD41_Humidity,"
         "SGP41_VOC,"
         "SGP41_NOX,"
         "ILPS28QSW_Pressure,"
         "ILPS28QSW_Temperature,"
         "BME688_Temperature,"
         "BME688_Pressure,"
         "BME688_Humidity,"
         "BME688_Gas_Resistance,"
         "BH1730FVC_Visible,"
         "BH1730FVC_IR,"
         "BH1730FVC_Lux,"
         "AS7331_Temperature,"
         "AS7331_UVA,"
         "AS7331_UVB,"
         "AS7331_UVC\n");

  // ----------------- Main Loop --------------------------------------------------------------------------------------
  bool data_ready;
  uint32_t time;

  int32_t loop_time = k_uptime_get_32();

  while (1) {
    gpio_pin_set_dt(&gpio_debug_1, 1);
    sync();

    // ----------------- SCD41 (CO2 Sensor) ----------------------------------------------------------------------------
    // Wait until data is ready
    data_ready = false;
    time = k_uptime_get_32();
    do {
      error_i16 = scd4x_get_data_ready_status(&data_ready);
      if (error_i16 != NO_ERROR) {
        LOG_ERR(" * SCD41 Error %d getting data ready status", error_i16);
        break;
      }

      if (!data_ready) {
        k_usleep(100);

        if ((k_uptime_get_32() - time) > 10 * 1000) {
          LOG_ERR(" * SCD41 Timeout waiting for data ready status");
          break;
        }
      }
    } while (!data_ready);
    LOG_DBG("SCD41 Data ready after %u ms", k_uptime_get_32() - time);

    int32_t scd41_temperature, scd41_humidity;
    error_i16 = scd4x_read_measurement(&sensor_values.scd41_co2, &scd41_temperature, &scd41_humidity);

    sensor_values.scd41_temperature = scd41_temperature / 1000.0f;
    sensor_values.scd41_humidity = scd41_humidity / 1000.0f;

    if (error_i16 != NO_ERROR) {
      LOG_ERR(" * SCD41 Error %d reading measurement", error_i16);
      break;
    }
    sync();

    // -----------------  SGP41 (VOC Sensor) ---------------------------------------------------------------------------
    error_i16 = sgp41_measure_raw_signals(default_rh, default_t, &sensor_values.sgp41_voc, &sensor_values.sgp41_nox);
    if (error_i16 != NO_ERROR) {
      LOG_ERR(" * SGP41 Error %d reading signals", error_i16);
      break;
    }
    sync();

    // ----------------- ILPS28QSW (Pressure Sensor) -------------------------------------------------------------------
    ilps28qsw_data_t data;
    error_i32 = ilps28qsw_data_get(&ilps28qsw_ctx, &ilps28qsw_md, &data);
    if (error_i32) {
      LOG_ERR(" * ILPS28QSW Error %d getting data", error_i32);
      break;
    }
    sensor_values.ilps28qsw_pressure = data.pressure.hpa;
    sensor_values.ilps28qsw_temperature = data.heat.deg_c;
    sync();

    // ----------------- BME688 (Environmental Sensor) -----------------------------------------------------------------
    struct sensor_value temp, press, humidity, gas_res;
    sensor_sample_fetch(bme_dev);
    sensor_channel_get(bme_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    sensor_channel_get(bme_dev, SENSOR_CHAN_PRESS, &press);
    sensor_channel_get(bme_dev, SENSOR_CHAN_HUMIDITY, &humidity);
    sensor_channel_get(bme_dev, SENSOR_CHAN_GAS_RES, &gas_res);

    sensor_values.bme688_temperature = temp.val1 + (temp.val2 / 1000000.0);
    sensor_values.bme688_pressure = press.val1 + (press.val2 / 1000000.0);
    sensor_values.bme688_humidity = humidity.val1 + (humidity.val2 / 1000000.0);
    sensor_values.bme688_gas_resistance = gas_res.val1 + (gas_res.val2 / 1000000.0);
    sync();

    // ----------------- BH1730FVC (Ambient Light Sensor) --------------------------------------------------------------
    uint8_t data_ready = false;
    time = k_uptime_get_32();
    do {
      error_i32 = bh1730_valid(&bh1730_ctx, &data_ready);
      if (error_i32) {
        LOG_ERR(" * BH1730FVC Error %d reading valid status", error_i32);
        break;
      }

      if (!data_ready) {
        k_usleep(100);

        if ((k_uptime_get_32() - time) > 10 * 1000) {
          LOG_ERR(" * BH1730FVC Timeout waiting for data ready status");
          break;
        }
      }
    } while (!data_ready);
    LOG_DBG("BH1730FVC Data ready after %u ms", k_uptime_get_32() - time);

    error_i32 = bh1730_read_visible(&bh1730_ctx, &sensor_values.bh1730_visible);
    if (error_i32) {
      LOG_ERR(" * BH1730FVC Error %d reading visible light", error_i32);
      break;
    }

    error_i32 = bh1730_read_ir(&bh1730_ctx, &sensor_values.bh1730_ir);
    if (error_i32) {
      LOG_ERR(" * BH1730FVC Error %d reading IR light", error_i32);
      break;
    }

    error_i32 = bh1730_read_lux(&bh1730_ctx, &sensor_values.bh1730_lux);
    if (error_i32) {
      LOG_ERR(" * BH1730FVC Error %d reading lux", error_i32);
      break;
    }
    sync();

    // ----------------- AS7331 (UV Sensor) ----------------------------------------------------------------------------
    data_ready = false;
    // as7331_reg_osrstat_t status;
    time = k_uptime_get_32();
    do {
      // error_i32 = as7331_get_status(&as7331_ctx, &status);
      // if (error_i32) {
      //   LOG_ERR(" * AS7331 Error %d getting status", error_i32);
      //   break;
      // }
      // data_ready = status.ndata;

      // Read gpio_ext_as7331_ready to determine if sensor is read
      int ret = gpio_pin_get_dt(&gpio_ext_as7331_ready);
      data_ready = (ret == 1);

      if (!data_ready) {
        k_usleep(100);

        if ((k_uptime_get_32() - time) > 10 * 1000) {
          LOG_ERR(" * AS7331 Timeout waiting for data ready status");
          break;
        }
      }
    } while (!data_ready);
    LOG_DBG("AS7331 Data ready after %d ms", k_uptime_get_32() - time);

    // Read all
    struct {
      uint16_t temp;
      uint16_t uva;
      uint16_t uvb;
      uint16_t uvc;
    } as7331_all;
    error_i32 = as7331_read_all(&as7331_ctx, (uint16_t *)&as7331_all);
    if (error_i32) {
      LOG_ERR(" * AS7331 Error %d reading all values", error_i32);
      break;
    }

    sensor_values.as7331_temp = as7331_all.temp * 0.05f - 66.9f;
    sensor_values.as7331_uva = as7331_all.uva;
    sensor_values.as7331_uvb = as7331_all.uvb;
    sensor_values.as7331_uvc = as7331_all.uvc;

    // Already start next measurement
    error_i32 = as7331_start_measurement(&as7331_ctx);
    if (error_i32) {
      LOG_ERR(" * AS7331 Error %d starting one-shot measurement", error_i32);
      break;
    }
    sync();

    // ----------------- CSV Output ------------------------------------------------------------------------------------
    gpio_pin_set_dt(&gpio_debug_1, 0);

    sensor_values.timestamp = k_uptime_get_32();

    // Print all elements in sensor_values as CSV formatted string
    printf("%u,%u,%f,%f,%u,%u,%f,%f,%f,%f,%f,%f,%u,%u,%u,%f,%u,%u,%u\n", sensor_values.timestamp,
           sensor_values.scd41_co2, sensor_values.scd41_temperature, sensor_values.scd41_humidity,
           sensor_values.sgp41_voc, sensor_values.sgp41_nox, sensor_values.ilps28qsw_pressure,
           sensor_values.ilps28qsw_temperature, sensor_values.bme688_temperature, sensor_values.bme688_pressure,
           sensor_values.bme688_humidity, sensor_values.bme688_gas_resistance, sensor_values.bh1730_visible,
           sensor_values.bh1730_ir, sensor_values.bh1730_lux, sensor_values.as7331_temp, sensor_values.as7331_uva,
           sensor_values.as7331_uvb, sensor_values.as7331_uvc);

    int32_t loop_duration = k_uptime_get_32() - loop_time;
    loop_time = k_uptime_get_32();
    LOG_DBG("Loop duration: %d ms", loop_duration);

    // Sleep to get 5s intervals
    if (loop_duration < SAMPLING_TIME) {
      int32_t sleep_duration = SAMPLING_TIME - loop_duration;
      LOG_DBG("Sleeping for %d ms", sleep_duration);
      k_msleep(sleep_duration);
    } else {
      LOG_WRN("Loop duration too long: %d ms", loop_duration);
    }
  }

  // ----------------- Power off sensors -------------------------------------------------------------------------------
  LOG_INF("===== Powering off sensors ======");

  // ----------------- SCD41 (CO2 Sensor) ------------------------------------------------------------------------------
  LOG_INF(" - Stop periodic measurement of SCD41");
  error_i16 = scd4x_stop_periodic_measurement();
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d stopping periodic measurement", error_i16);
    k_msleep(1000);
    return -1;
  }
  LOG_INF(" - Power off SCD41");
  poweroff_scd41();

  // ----------------- SGP41 (VOC Sensor) ------------------------------------------------------------------------------
  LOG_INF(" - Turn off heater of SGP41");
  error_i16 = sgp41_turn_heater_off();
  if (error_i16 != NO_ERROR) {
    LOG_ERR(" * Error %d turning heater off", error_i16);
    k_msleep(1000);
    return -1;
  }
  LOG_INF(" - Power off SGP41");
  poweroff_sgp41();

  // ----------------- ILPS28QSW (Pressure Sensor) ---------------------------------------------------------------------
  // No power off needed.

  // ----------------- BME688 (Environmental Sensor) -------------------------------------------------------------------
  // No power off needed.

  // ----------------- BH1730FVC (Ambient Light Sensor) ----------------------------------------------------------------
  LOG_INF(" - Power off BH1730FVC");
  poweroff_bh1730();

  // ----------------- AS7331 (UV Sensor) ------------------------------------------------------------------------------
  LOG_INF(" - Power off AS7331");
  poweroff_as7331();

  return 0;
}