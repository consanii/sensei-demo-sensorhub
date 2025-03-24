/*
 * ----------------------------------------------------------------------
 *
 * File: as7331_sensor.h
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

#ifndef AS7331_SENSOR_H
#define AS7331_SENSOR_H

#include "as7331_reg.h"

void test_as7331();
int poweroff_as7331();
int poweron_as7331();

as7331_reg_osrstat_t print_as7331_status(as7331_t *sensor);

#endif // AS7331_SENSOR_H