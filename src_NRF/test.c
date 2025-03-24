/*
 * ----------------------------------------------------------------------
 *
 * File: test.c
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

#define GPIO_NODE_debug_signal_1 DT_NODELABEL(gpio_debug_signal_1)
static const struct gpio_dt_spec gpio_debug_1 = GPIO_DT_SPEC_GET(GPIO_NODE_debug_signal_1, gpios);

static const struct device *const i2c_a = DEVICE_DT_GET(DT_ALIAS(i2ca));
static const struct device *const i2c_b = DEVICE_DT_GET(DT_ALIAS(i2cb));

LOG_MODULE_REGISTER(test, LOG_LEVEL_INF);

#define GAP9_I2C_SLAVE 0
#define I2C_SLAVE_L2_TEST_ADDRESS (0x1c019000)
#define I2C_SLAVE_L2_TEST_SIZE (BUFF_SIZE * 4)
#define BUFF_SIZE (16)

#define GAP9_I2C_SLAVE_ADDR (0x0A)

/* Buffer to write in EEPROM : BUF_SIZE + 2, for the memory address. */
uint32_t write_buff[BUFF_SIZE + 2];
/* Buffer to read from EEPROM : BUF_SIZE. */
uint32_t read_buff[BUFF_SIZE];

/* BUffer holding the memory address & size for read transactions. */
uint32_t addr_buff[2];

void data_init(int nb) {
  addr_buff[0] = I2C_SLAVE_L2_TEST_ADDRESS;
  addr_buff[1] = I2C_SLAVE_L2_TEST_SIZE;

  write_buff[0] = I2C_SLAVE_L2_TEST_ADDRESS;
  write_buff[1] = I2C_SLAVE_L2_TEST_SIZE;
  for (int i = 0; i < nb; i++) {
    write_buff[i + 2] = i + 1UL;
  }
}

void test_sensors(void) {
  data_init(BUFF_SIZE);

  LOG_INF("Scanning I2C A Interface");
  i2c_scan(i2c_a);

  LOG_INF("Scanning I2C B Interface");
  i2c_scan(i2c_b);

#if GAP9_I2C_SLAVE
  int ret = 0;
  k_msleep(100);
  printf("> Testing I2C communication with GAP9 I2C slave\r\n");

  memset(read_buff, 0, BUFF_SIZE * 4);
  memset(addr_buff, 0, 2 * 4);

  ret = i2c_write(i2c_a, (uint8_t *)write_buff, (BUFF_SIZE + 2) * 4, GAP9_I2C_SLAVE_ADDR);
  ret += i2c_read(i2c_a, (uint8_t *)addr_buff, 2 * 4, GAP9_I2C_SLAVE_ADDR);
  ret += i2c_read(i2c_a, (uint8_t *)read_buff, BUFF_SIZE * 4, GAP9_I2C_SLAVE_ADDR);
  if (ret) {
    LOG_ERR("Failed to communicate with GAP9 I2C slave\r\n");
  }

  printf("  Addr buffer: 0x%08X, 0x%08X\r\n", addr_buff[0], addr_buff[1]);
  printf("  Read buffer: ");
  for (int i = 0; i < 15; i++) {
    printf("0x%02X, ", read_buff[i]);
  }
  printf("\r\n");
#endif

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  poweron_as7331();
  test_as7331();
  poweroff_as7331();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  poweron_bh1730();
  test_bh1730fvc();
  poweroff_bh1730();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  test_ism330dhcx();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  test_lis2duxs12();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  test_bme688();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  test_ilpS28qsw();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  poweron_scd41();
  test_scd41();
  poweroff_scd41();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  poweron_sgp41();
  test_sgp41();
  poweroff_sgp41();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  gpio_pin_set_dt(&gpio_debug_1, 1);
  test_max77654();
  gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
  poweroff_max_m10s();
  // gpio_pin_set_dt(&gpio_debug_1, 1);
  // test_max_m10s();
  // gpio_pin_set_dt(&gpio_debug_1, 0);

  sync();
}