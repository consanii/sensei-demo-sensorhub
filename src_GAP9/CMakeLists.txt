# Copyright (c) 2024 ETH Zurich and University of Bologna
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.19)

###############################################################################
# Panel Control
###############################################################################
set(TARGET_NAME "main")
set(TARGET_SRCS main.c)


###############################################################################
# CMake pre initialization
###############################################################################
set(GAP_CUSTOM_BSP $ENV{SENSEI_SDK_ROOT}/GAP9)

set(CONFIG_GAP_SDK_HOME $ENV{GAP_SDK_HOME})
include($ENV{GAP_SDK_HOME}/utils/cmake/setup.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${TARGET_NAME} C ASM)
add_executable(${TARGET_NAME} ${TARGET_SRCS})

# add_compile_definitions("TRACE_GPIO=1" "TRACE_PAD=1" "PI_LOG_LOCAL_LEVEL=5")

target_compile_options(${TARGET_NAME} PUBLIC ${TARGET_COMPILE_OPTIONS})

###############################################################################
# CMake post initialization
###############################################################################
setupos(${TARGET_NAME})

