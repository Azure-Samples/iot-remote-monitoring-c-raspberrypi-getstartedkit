#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

project(azure-remote-monitoring-raspberry-pi-c)

option(use_amqp_kit "use samples provided in the kit" ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../azure-iot-sdk-c ${CMAKE_CURRENT_BINARY_DIR}/azure-iot-sdk-c)

add_subdirectory(remote_monitoring)
