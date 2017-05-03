#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)
#this is CMakeLists.txt for the BME280 and a protected file access for Raspberry PI.

compileAsC99()

set(platform_c_files
  ./src/bme280.c
  ./src/locking.c
)

set(platform_h_files
  ./inc/bme280.h
  ./inc/locking.h
)

set(PLATFORM_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using serializer lib" FORCE)
include_directories(${PLATFORM_INC_FOLDER})

add_library(
  aziotplatform ${platform_c_files} ${platform_h_files}
)

install (TARGETS aziotplatform DESTINATION lib)
install (FILES ${platform_h_files} DESTINATION include/azureiot/platform_specific)
