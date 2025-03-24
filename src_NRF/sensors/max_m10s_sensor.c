/*
 * ----------------------------------------------------------------------
 *
 * File: max_m10s_sensor.c
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

#include <time.h>

#include <zephyr/device.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include "config.h"
#include "i2c_helpers.h"
#include "max_m10s_sensor.h"

#include "ubxlib.h"

#include "u_cfg_app_platform_specific.h"
#include "u_gnss.h"

LOG_MODULE_DECLARE(sensors, LOG_LEVEL_DBG);

static const uNetworkCfgGnss_t gNetworkCfg = {.type = U_NETWORK_TYPE_GNSS, .moduleType = U_GNSS_MODULE_TYPE_M10};

static const uDeviceCfg_t gDeviceCfg = {.deviceType = U_DEVICE_TYPE_GNSS,
                                        .transportType = U_DEVICE_TRANSPORT_TYPE_I2C,
                                        .deviceCfg = {.cfgGnss =
                                                          {
                                                              .powerOffToBackup = true,
                                                              .moduleType = U_GNSS_MODULE_TYPE_M10,
                                                              .i2cAddress = U_GNSS_I2C_ADDRESS,
                                                          }},
                                        .transportCfg = {.cfgI2c = {
                                                             .i2c = 1,     // I2C B
                                                             .pinSda = -1, // Use -1 if on Zephyr or Linux
                                                             .pinScl = -1  // Use -1 if on Zephyr or Linux
                                                         }}};

static char *locStr(int32_t loc) {
  static char str[25];
  const char *sign = "";
  if (loc < 0) {
    loc = -loc;
    sign = "-";
  }
  snprintf(str, sizeof(str), "%s%d.%07d", sign, loc / 10000000, loc % 10000000);
  return str;
}

bool keepGoingCallback(uDeviceHandle_t devHandle) {
  static int32_t firstTime = 0;
  int32_t time_now = k_uptime_get_32();

  if (firstTime == 0) {
    firstTime = time_now;
  }

  int32_t time_diff = time_now - firstTime;
  LOG_DBG("MAX-M10S keep going callback: %8.1fs", (float)time_diff / 1000);

  // Return false after 20s timeout
  if (time_diff > MAX_M10S_TIMEOUT) {
    LOG_ERR("Keep going callback: Timeout");
    return false;
  }

  return true;
}

void test_max_m10s() {
  LOG_INF("Testing MAX-M10S (GNSS Module)" SPACES);

  int32_t errorCode = uPortInit();
  if (errorCode != 0) {
    LOG_ERR(" * Failed to initiate U-Blox library: %d", errorCode);
  } else {
    LOG_DBG(" > Initalized U-Blox library");
  }

  errorCode = uPortI2cInit(); // You only need this if an I2C interface is used
  if (errorCode != 0) {
    LOG_ERR(" * Failed to initiate U-Blox I2C library: %d", errorCode);
  } else {
    LOG_DBG(" > Initalized U-Blox I2C library");
  }

  errorCode = uDeviceInit();
  if (errorCode != 0) {
    LOG_ERR(" * Failed to initiate U-Blox library: %d", errorCode);
  } else {
    LOG_DBG(" > Initalized U-Blox library");
  }

  // Initiate GNSS Module
  uDeviceHandle_t deviceHandle;
  errorCode = uDeviceOpen(&gDeviceCfg, &deviceHandle);
  if (errorCode == 0) {

    errorCode = uNetworkInterfaceUp(deviceHandle, U_NETWORK_TYPE_GNSS, &gNetworkCfg);
    if (errorCode != 0) {
      LOG_ERR(" * Failed to bring up the GNSS network interface: %d", errorCode);
    } else {
      LOG_DBG(" > GNSS network interface up");
    }

    // Get Position
    uLocation_t location;
    errorCode = uLocationGet(deviceHandle, U_LOCATION_TYPE_GNSS, NULL, NULL, &location, keepGoingCallback);

    if (errorCode == 0) {
      printf("Position: https://maps.google.com/?q=");
      printf("%s,", locStr(location.latitudeX1e7));
      printf("%s\n", locStr(location.longitudeX1e7));
      struct tm *t = gmtime(&location.timeUtc);
      LOG_INF(" - Logitude                            : %s" SPACES, locStr(location.longitudeX1e7));
      LOG_INF(" - Latitude                            : %s" SPACES, locStr(location.latitudeX1e7));
      LOG_INF(" - Radius                              : %d m" SPACES, location.radiusMillimetres / 1000);
      LOG_INF(" - UTC Time                            : %4d-%02d-%02d %02d:%02d:%02d" SPACES, t->tm_year + 1900,
              t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    } else {
      LOG_ERR(" * Failed to get location: %d", errorCode);
    }

    // Power off the GNSS module
    // errorCode = uGnssPwrOff(deviceHandle);
    // if (errorCode != 0) {
    //   LOG_ERR(" * Failed to power off the module: %d", errorCode);
    // } else {
    //   LOG_DBG(" > GNSS module powered off successfully");
    // }

    uDeviceClose(deviceHandle, true);
  } else {
    LOG_ERR(" * Failed to initiate the module: %d", errorCode);
  }

  // errorCode = uDeviceDeinit();
  // if (errorCode != 0) {
  //   LOG_ERR(" * Failed to deinit U-Blox library: %d", errorCode);
  // } else {
  //   LOG_DBG(" > Deinitialized U-Blox library");
  // }

  // uPortI2cDeinit();
  // uPortDeinit();
}

void poweroff_max_m10s() {
  LOG_INF("Power Off MAX-M10S (GNSS Module)" SPACES);

  int32_t errorCode = uPortInit();
  if (errorCode != 0) {
    LOG_ERR(" * Failed to initiate U-Blox library: %d", errorCode);
  } else {
    LOG_DBG(" > Initalized U-Blox library");
  }

  errorCode = uPortI2cInit(); // You only need this if an I2C interface is used
  if (errorCode != 0) {
    LOG_ERR(" * Failed to initiate U-Blox I2C library: %d", errorCode);
  } else {
    LOG_DBG(" > Initalized U-Blox I2C library");
  }

  errorCode = uDeviceInit();
  if (errorCode != 0) {
    LOG_ERR(" * Failed to initiate U-Blox library: %d", errorCode);
  } else {
    LOG_DBG(" > Initalized U-Blox library");
  }

  // MAX-M10S
  uDeviceHandle_t deviceHandle;
  LOG_INF("Initiating the module...");
  errorCode = uDeviceOpen(&gDeviceCfg, &deviceHandle);
  if (errorCode == 0) {

    // Init the GNSS module
    errorCode = uGnssInit(deviceHandle);
    if (errorCode == 0) {
      LOG_INF("GNSS module initiated successfully");
    } else {
      LOG_ERR("* Failed to initiate the module: %d", errorCode);
    }

    // Power off the GNSS module
    errorCode = uGnssPwrOffBackup(deviceHandle);
    if (errorCode == 0) {
      LOG_INF("GNSS module powered off successfully");
    } else {
      LOG_ERR("* Failed to power off the module: %d", errorCode);
    }

    uDeviceClose(deviceHandle, true);
  } else {
    LOG_ERR("* Failed to initiate the module: %d", errorCode);
  }

  errorCode = uDeviceDeinit();
  if (errorCode != 0) {
    LOG_ERR("Failed to deinit U-Blox library: %d", errorCode);
  } else {
    LOG_INF("Deinitialized U-Blox library");
  }

  errorCode = uDeviceDeinit();
  if (errorCode != 0) {
    LOG_ERR(" * Failed to deinit U-Blox library: %d", errorCode);
  } else {
    LOG_DBG(" > Deinitialized U-Blox library");
  }

  uPortI2cDeinit();
  uPortDeinit();
}