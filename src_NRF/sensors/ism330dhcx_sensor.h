/*
 * ----------------------------------------------------------------------
 *
 * File: ism330dhcx_sensor.h
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

#ifndef ISM330DHCX_SENSOR_H
#define ISM330DHCX_SENSOR_H

#include "ism330dhcx_reg.h"

#define ISM330DHCX_ACC_SENSITIVITY_FS_2G 0.061f
#define ISM330DHCX_ACC_SENSITIVITY_FS_4G 0.122f
#define ISM330DHCX_ACC_SENSITIVITY_FS_8G 0.244f
#define ISM330DHCX_ACC_SENSITIVITY_FS_16G 0.488f

#define ISM330DHCX_GYRO_SENSITIVITY_FS_125DPS 4.375f
#define ISM330DHCX_GYRO_SENSITIVITY_FS_250DPS 8.750f
#define ISM330DHCX_GYRO_SENSITIVITY_FS_500DPS 17.500f
#define ISM330DHCX_GYRO_SENSITIVITY_FS_1000DPS 35.000f
#define ISM330DHCX_GYRO_SENSITIVITY_FS_2000DPS 70.000f
#define ISM330DHCX_GYRO_SENSITIVITY_FS_4000DPS 140.000f

int32_t ism330dhcx_xl_sensitivity(const stmdev_ctx_t *ctx, float *sensitivity);
int32_t ism330dhcx_gy_sensitivity(const stmdev_ctx_t *ctx, float *sensitivity);

void test_ism330dhcx();

#endif // ISM330DHCX_SENSOR_H