/*
 * ----------------------------------------------------------------------
 *
 * File: max77654_sensor.c
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

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include "config.h"
#include "i2c_helpers.h"
#include "max77654_sensor.h"

#include "pwr/pwr_common.h"

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_INF);

void test_max77654() {
  LOG_INF("Testing MAX77654 (PMIC)" SPACES);

  k_mutex_lock(&pwr_mutex, K_FOREVER);

  int value;

  // Array with name, unit and index
  const struct {
    const char *name;
    const char *unit;
    int index;
  } value_names[] = {
      {"AGND Voltage                        ", "mV", MAX77654_AGND},
      {"VSYS Voltage                        ", "mV", MAX77654_VSYS},
      {"CHGIN Voltage                       ", "mV", MAX77654_CHGIN_V},
      {"CHGIN Current                       ", "mA", MAX77654_CHGIN_I},
      {"Battery Voltage                     ", "mV", MAX77654_BATT_V},
      {"Battery Current                     ", "%", MAX77654_BATT_I_CHG},
      {"Battery Discharge Current           ", "mA", MAX77654_BATT_I_8MA2},
      {"Thermistor Voltage                  ", "mV", MAX77654_THM},
      {"Thermistor Bias                     ", "mV", MAX77654_TBIAS},
      // {"Battery Discharge Current (  8.2 mA)", "mA", MAX77654_BATT_I_8MA2},
      // {"Battery Discharge Current ( 40.5 mA)", "mA", MAX77654_BATT_I_40MA5},
      // {"Battery Discharge Current ( 72.3 mA)", "mA", MAX77654_BATT_I_72MA3},
      // {"Battery Discharge Current (103.4 mA)", "mA", MAX77654_BATT_I_103MA4},
      // {"Battery Discharge Current (134.1 mA)", "mA", MAX77654_BATT_I_134MA1},
      // {"Battery Discharge Current (164.1 mA)", "mA", MAX77654_BATT_I_164MA1},
      // {"Battery Discharge Current (193.7 mA)", "mA", MAX77654_BATT_I_193MA7},
      // {"Battery Discharge Current (222.7 mA)", "mA", MAX77654_BATT_I_222MA7},
      // {"Battery Discharge Current (251.2 mA)", "mA", MAX77654_BATT_I_251MA2},
      // {"Battery Discharge Current (279.3 mA)", "mA", MAX77654_BATT_I_279MA3},
      // {"Battery Discharge Current (300.0 mA)", "mA", MAX77654_BATT_I_300MA},
  };

  // Iterate over all max77654_measure_t types:
  for (uint32_t i = 0; i < sizeof(value_names) / sizeof(value_names[0]); i++) {
    if (max77654_measure(&pmic_h, value_names[i].index, &value) != E_MAX77654_SUCCESS) {
      LOG_ERR(" * PMIC measure failed!");
      return;
    }
    LOG_INF(" - %s: %i %s" SPACES, value_names[i].name, value, value_names[i].unit);
  }

  k_mutex_unlock(&pwr_mutex);
}
