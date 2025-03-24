/*
 * ----------------------------------------------------------------------
 *
 * File: pwr_bsp.c
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

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "bsp/pwr_bsp.h"
#include "pwr/pwr.h"
#include "pwr/pwr_common.h"

#include "max77654.h"

// ======== Defines/Variables ======================================================================

LOG_MODULE_REGISTER(pwr_bsp, LOG_LEVEL_INF);

static const struct device *pwr_i2c = DEVICE_DT_GET(DT_ALIAS(i2ca));

/* The devicetree node identifier */
#define GPIO_NODE_scd41_pwr DT_NODELABEL(gpio_scd41_pwr)
#define GPIO_NODE_sgp41_pwr DT_NODELABEL(gpio_sgp41_pwr)

#define GPIO_NODE_i2c_sgp41_en DT_NODELABEL(gpio_ext_i2c_sgp41_en)
#define GPIO_NODE_i2c_as7331_en DT_NODELABEL(gpio_ext_i2c_as7331_en)
#define GPIO_NODE_i2c_scd41_en DT_NODELABEL(gpio_ext_i2c_scd41_en)
#define GPIO_NODE_hm0360_clk_en DT_NODELABEL(gpio_ext_hm0360_clk_en)

#define GPIO_NODE_gap9_i2c_ctrl DT_NODELABEL(gpio_gap9_i2c_ctrl)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec gpio_SCD41_pwr = GPIO_DT_SPEC_GET(GPIO_NODE_scd41_pwr, gpios);
static const struct gpio_dt_spec gpio_SGP41_pwr = GPIO_DT_SPEC_GET(GPIO_NODE_sgp41_pwr, gpios);

static const struct gpio_dt_spec gpio_I2C_SGP41_EN = GPIO_DT_SPEC_GET(GPIO_NODE_i2c_sgp41_en, gpios);
static const struct gpio_dt_spec gpio_I2C_AS7331_EN = GPIO_DT_SPEC_GET(GPIO_NODE_i2c_as7331_en, gpios);
static const struct gpio_dt_spec gpio_I2C_SCD41_EN = GPIO_DT_SPEC_GET(GPIO_NODE_i2c_scd41_en, gpios);
static const struct gpio_dt_spec gpio_HM0360_CLK_EN = GPIO_DT_SPEC_GET(GPIO_NODE_hm0360_clk_en, gpios);

static const struct gpio_dt_spec gpio_p0_6_gap9_i2c_ctrl = GPIO_DT_SPEC_GET(GPIO_NODE_gap9_i2c_ctrl, gpios);

// ======== Functions ==============================================================================

int pwr_bsp_init() {
  // Configure LED power gating pin

  // Configure power gating pins on the sensor shield
  if (gpio_pin_configure_dt(&gpio_SCD41_pwr, GPIO_OUTPUT_INACTIVE) < 0) {
    LOG_ERR("SCD41 EN GPIO init error");
    k_msleep(1000);
    return -1;
  }
  if (gpio_pin_configure_dt(&gpio_SGP41_pwr, GPIO_OUTPUT_INACTIVE) < 0) {
    LOG_ERR("SGP41 EN GPIO init error");
    k_msleep(1000);
    return -1;
  }

  // Configure I2C power gating pins on the sensor shield
  if (gpio_pin_configure_dt(&gpio_I2C_SGP41_EN, GPIO_OUTPUT_INACTIVE) < 0) {
    LOG_ERR("SGP41 I2C EN GPIO init error");
    k_msleep(1000);
    return -1;
  }
  if (gpio_pin_configure_dt(&gpio_I2C_AS7331_EN, GPIO_OUTPUT_INACTIVE) < 0) {
    LOG_ERR("AS7331 I2C EN GPIO init error");
    k_msleep(1000);
    return -1;
  }
  if (gpio_pin_configure_dt(&gpio_I2C_SCD41_EN, GPIO_OUTPUT_INACTIVE) < 0) {
    LOG_ERR("SCD41 I2C EN GPIO init error");
    k_msleep(1000);
    return -1;
  }

  // Configure HM0360 clock gating pin on the sensor shield
  if (gpio_pin_configure_dt(&gpio_HM0360_CLK_EN, GPIO_OUTPUT_INACTIVE) < 0) {
    LOG_ERR("HM0360 I2C EN GPIO init error");
    k_msleep(1000);
    return -1;
  }

  // Configure GAP9 I2C bus control pin
  if (gpio_pin_configure_dt(&gpio_p0_6_gap9_i2c_ctrl, GPIO_OUTPUT_INACTIVE) < 0) {
    LOG_ERR("GAP9 I2C GPIO init error");
    return 0;
  }

  if (!device_is_ready(pwr_i2c)) {
    LOG_ERR("PWR I2C not ready!");
    k_msleep(1000);
    return -1;
  }

  return 0;
}

int pwr_bsp_start() {
  // Configure PMIC
  struct max77654_conf *pmic_conf = &pmic_h.conf;

  pmic_conf->sbb_conf[0].mode = MAX77654_SBB_MODE_BUCKBOOST;
  pmic_conf->sbb_conf[0].peak_current = MAX77654_SBB_PEAK_CURRENT_1A;
  pmic_conf->sbb_conf[0].active_discharge = false;
  pmic_conf->sbb_conf[0].en = MAX77654_REG_ON;
  pmic_conf->sbb_conf[0].output_voltage_mV = 3300;

  pmic_conf->sbb_conf[1].mode = MAX77654_SBB_MODE_BUCKBOOST;
  pmic_conf->sbb_conf[1].peak_current = MAX77654_SBB_PEAK_CURRENT_1A;
  pmic_conf->sbb_conf[1].active_discharge = false;
  pmic_conf->sbb_conf[1].en = MAX77654_REG_ON;
  pmic_conf->sbb_conf[1].output_voltage_mV = 2800;

  pmic_conf->sbb_conf[2].mode = MAX77654_SBB_MODE_BUCKBOOST;
  pmic_conf->sbb_conf[2].peak_current = MAX77654_SBB_PEAK_CURRENT_1A;
  pmic_conf->sbb_conf[2].active_discharge = false;
  pmic_conf->sbb_conf[2].en = MAX77654_REG_ON;
  pmic_conf->sbb_conf[2].output_voltage_mV = 1200;

  pmic_conf->ldo_conf[0].mode = MAX77654_LDO_MODE_LDO;
  pmic_conf->ldo_conf[0].active_discharge = false;
  pmic_conf->ldo_conf[0].en = MAX77654_REG_ON;
  pmic_conf->ldo_conf[0].output_voltage_mV = 3300;

  max77654_config(&pmic_h);

  LOG_INF("PMIC configured for UT SensorShield");

  return 0;
}
