#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for remote_monitoring sample

compileAsC99()

include_directories(${SERIALIZER_INC_FOLDER} ${SHARED_UTIL_INC_FOLDER} ${PLATFORM_INC_FOLDER})

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
target_link_libraries(remote_monitoring serializer iothub_client iothub_client_mqtt_transport wiringPi)
