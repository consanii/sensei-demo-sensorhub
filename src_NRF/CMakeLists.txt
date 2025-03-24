# Copyright (c) 2024 ETH Zurich and University of Bologna
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Enable shields
set(SHIELD UT_SensorShield_v1)

# Setup SENSEI SDK
set(SENSEI_SDK_ROOT $ENV{SENSEI_SDK_ROOT})
if(NOT SENSEI_SDK_ROOT)
    message(FATAL_ERROR "SENSEI_SDK_ROOT is not defined")
endif()

# Append UBXLIB as a Zephyr module
get_filename_component(UBXLIB_BASE "${SENSEI_SDK_ROOT}/NRF/system/third_party/max-m10s/ubxlib" ABSOLUTE)

list(APPEND EXTRA_ZEPHYR_MODULES
    ${SENSEI_SDK_ROOT}/NRF
    ${UBXLIB_BASE}
)

# Do not use the default configuration
set(UBXLIB_NO_DEF_CONF)

# Use STDC drivers (USE_STDC_ILPS28QSW=y)
# set(CONFIG_USE_STDC_ILPS28QSW y)
# set(CONFIG_USE_STDC_ISM330DHCX y)

# Configure the project
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pmic_test)

target_sources(app PRIVATE
    main.c
    util.c
    test.c
    i2c_helpers.c
    bsp/pwr_bsp.c
    sensors/as7331_sensor.c
    sensors/bh1730fvc_sensor.c
    sensors/bme688_sensor.c
    sensors/ilps28qsw_sensor.c
    sensors/ism330dhcx_sensor.c
    sensors/lis2duxs12_sensor.c
    sensors/max_m10s_sensor.c
    sensors/max77654_sensor.c
    sensors/scd41_sensor.c
    sensors/sgp41_sensor.c
)
target_include_directories(app PRIVATE
    .
    sensors
)

# target_compile_definitions(app PRIVATE
#     U_LOCATION_TIMEOUT_SECONDS=10
# )
