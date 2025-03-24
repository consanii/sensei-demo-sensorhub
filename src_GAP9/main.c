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

/***Include****/
#include "pmsis.h"
#include <stdint.h>
#include <stdio.h>

#include "pmsis/drivers/i2c_slave.h"

#define I2C_SLAVE_ADDR0 ((0x0A << 1))

#define BUFF_SIZE (16)

#define APP_DELAY_US 1000000 // 1s delay

#define I2C_SLAVE_INTERFACE (1)

#define PAD_GPIO_46 (PI_PAD_046)
#define PAD_GPIO_47 (PI_PAD_047)
#define PAD_GPIO_48 (PI_PAD_048)
#define PAD_GPIO_49 (PI_PAD_049)
#define PAD_GPIO_50 (PI_PAD_050)
#define PAD_GPIO_51 (PI_PAD_051)

#define HM0360_XSHUTDOWN_PAD (PAD_GPIO_49)
#define HM0360_XSLEEP_PAD (PAD_GPIO_50)

// buffer to hold rx on slave side
// BUFF SIZE + addr + size + an extra byte for matcher
uint8_t rx_buffer[((BUFF_SIZE + 2) * 4) + 1];
uint32_t g_l2_buff[BUFF_SIZE];

extern void pi_gpio_print(void);

void data_init(int nb) {
  // set the whole addr field to some ducky value
  // allows to check if transfer makes sense
  rx_buffer[0] = 0x6F;
  rx_buffer[1] = 0x7F;
  rx_buffer[2] = 0x8F;
  rx_buffer[3] = 0x9F;
  rx_buffer[4] = 0xAF;

  for (int i = 0; i < nb; i++) {
    g_l2_buff[i] = 0xdeadbeef;
  }
}

void i2c_memory_emu_rx_callback(pi_i2c_slave_args_t *arg) {
  uint32_t l2_addr = *(uint32_t *)arg->l2_buffer;
  uint32_t size = *(uint32_t *)(arg->l2_buffer + 4);

  // ignoring addr
  if (arg->nb_bytes > 8) {
    // single transfer --> just reset buffer
    memcpy(g_l2_buff, arg->l2_buffer + 8, size);
    // for(int i = 0; i < BUFF_SIZE; i++)
    // {
    //     printf("g_l2_buff[%i] = %x\n",i,g_l2_buff[i]);
    // }
  }

  pi_i2c_slave_stop_rx(arg->handle);
  pi_i2c_slave_stop_tx(arg->handle);
  pi_i2c_slave_unlock(arg->handle, 1);
  pi_i2c_slave_set_tx(arg->handle, g_l2_buff, (BUFF_SIZE * 4));
  pi_i2c_slave_set_rx(arg->handle, rx_buffer, ((BUFF_SIZE + 2) * 4) + 1);

  arg->ret = 0;
}

void i2c_memory_emu_tx_callback(pi_i2c_slave_args_t *arg) {
  if (arg->nb_bytes == 0) {
    pi_i2c_slave_unlock(arg->handle, 0);
    arg->ret = 0;
    return;
  }
  // nothing to do for this emulation except unlocking and reloading buf
  pi_i2c_slave_stop_tx(arg->handle);
  pi_i2c_slave_unlock(arg->handle, 0);
  pi_i2c_slave_set_tx(arg->handle, g_l2_buff, BUFF_SIZE * 4);
  arg->ret = 0;
}

int i2c_slave_setup(pi_device_t *slave_dev) {
  struct pi_i2c_slave_conf *conf = pi_l2_malloc(sizeof(struct pi_i2c_slave_conf));
  pi_i2c_slave_conf_init(conf);
  conf->itf = I2C_SLAVE_INTERFACE;
  pi_i2c_slave_conf_set_addr0(conf, I2C_SLAVE_ADDR0, 0x1F, 0, 1, 0);

  conf->rx_callback = i2c_memory_emu_rx_callback;
  conf->tx_callback = i2c_memory_emu_tx_callback;

  pi_open_from_conf(slave_dev, conf);
  int ret = pi_i2c_slave_open(slave_dev);
  if (!ret) {
    pi_i2c_slave_set_rx(slave_dev->data, rx_buffer, ((BUFF_SIZE + 2) * 4) + 1);
    pi_i2c_slave_set_tx(slave_dev->data, g_l2_buff, BUFF_SIZE * 4);
  } else {
    printf("slave open failed\n");
  }

  return ret;
}

int main(void) {
  pi_pad_function_set(HM0360_XSHUTDOWN_PAD, PI_PAD_FUNC1);
  pi_gpio_pin_configure(HM0360_XSHUTDOWN_PAD, PI_GPIO_OUTPUT | PI_GPIO_PULL_DISABLE | PI_GPIO_DRIVE_STRENGTH_LOW);
  pi_pad_function_set(HM0360_XSLEEP_PAD, PI_PAD_FUNC1);
  pi_gpio_pin_configure(HM0360_XSLEEP_PAD, PI_GPIO_OUTPUT | PI_GPIO_PULL_DISABLE | PI_GPIO_DRIVE_STRENGTH_LOW);
  pi_time_wait_us(10000);

  pi_gpio_pin_write(HM0360_XSHUTDOWN_PAD, 1);
  pi_time_wait_us(10000);
  pi_gpio_pin_write(HM0360_XSLEEP_PAD, 1);
  pi_time_wait_us(10000);

  pi_pad_function_set(PI_PAD_042, PI_PAD_FUNC0); // SDA
  pi_pad_function_set(PI_PAD_043, PI_PAD_FUNC0); // SCL

  data_init(BUFF_SIZE);

  pi_device_t *slave_dev = pi_l2_malloc(sizeof(pi_device_t));

  i2c_slave_setup(slave_dev);

  while (1) {
    pi_time_wait_us(1000000);
  }
  return 0;
}
