#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for remote_monitoring sample

compileAsC99()

set(PLATFORM_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/platform_specific/inc CACHE INTERNAL "this is what needs to be included if using bme280 sensor and locked file lib" FORCE)

include_directories(${SERIALIZER_INC_FOLDER} ${SHARED_UTIL_INC_FOLDER} ${PLATFORM_INC_FOLDER})

add_subdirectory(platform_specific)

set(remote_monitoring_c_files
	remote_monitoring.c
)

set(remote_monitoring_c_files ${remote_monitoring_c_files})

set(remote_monitoring_h_files
	remote_monitoring.h
)

IF(WIN32)
	#windows needs this define
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WIN32)

include_directories(. ${IOTHUB_CLIENT_INC_FOLDER})
include_directories(../../azure-iot-sdk-c/parson)

add_executable(remote_monitoring ${remote_monitoring_c_files} ${remote_monitoring_h_files})
target_link_libraries(remote_monitoring serializer iothub_client iothub_client_mqtt_transport aziotplatform wiringPi)
