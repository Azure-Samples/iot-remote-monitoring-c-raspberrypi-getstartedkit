// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "iothubtransportmqtt.h"
#include "schemalib.h"
#include "iothub_client.h"
#include "serializer_devicetwin.h"
#include "schemaserializer.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"

#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "bme280.h"
#include "locking.h"

static const char* deviceId = "[Device Id]";
static const char* connectionString = "HostName=[IoTHub Name].azure-devices.net;DeviceId=[Device Id];SharedAccessKey=[Device Key]";

static const char* deviceInfo = "{ \"ObjectType\": \"DeviceInfo\","
"\"IsSimulatedDevice\": 0,"
"\"Version\" : '1.0',"
"\"DeviceProperties\" :"
"{\"DeviceID\": \"%s\", \"TelemetryInterval\" : 1, \"HubEnabledState\" : true},"
"\"Telemetry\" : ["
"{\"Name\": \"Temperature\", \"DisplayName\" : \"Temperature\", \"Type\" : \"double\"},"
"{ \"Name\": \"Humidity\", \"DisplayName\" : \"Humidity\", \"Type\" : \"double\" }] }";

static const char* telemetryData = "{"
"\"DeviceID\": \"%s\","
"\"Temperature\" : %f,"
"\"Humidity\" : %f } ";

static IOTHUB_CLIENT_HANDLE g_iotHubClientHandle = NULL;

static const int Spi_channel = 0;
static const int Spi_clock = 1000000L;

static const int Grn_led_pin = 7;

static int Lock_fd;

/*json of supported methods*/
static char* supportedMethod = "{ \"LightBlink\": \"light blink\", \"ChangeLightStatus--LightStatusValue-int\""
": \"Change light status, on and off\" }";

// Define the Model
BEGIN_NAMESPACE(Contoso);

/* Reported properties */
DECLARE_STRUCT(SystemProperties,
ascii_char_ptr, FirmwareVersion
);

DECLARE_MODEL(ConfigProperties,
WITH_REPORTED_PROPERTY(uint8_t, TelemetryInterval)
);

DECLARE_DEVICETWIN_MODEL(Thermostat,
/* Device twin properties */
WITH_REPORTED_PROPERTY(ConfigProperties, Config),
WITH_REPORTED_PROPERTY(SystemProperties, System),

WITH_DESIRED_PROPERTY(uint8_t, TelemetryInterval, onDesiredTelemetryInterval),

/* Direct methods implemented by the device */
WITH_METHOD(LightBlink),
WITH_METHOD(ChangeLightStatus, int, LightStatusValue),

/* Register direct methods with solution portal */
WITH_REPORTED_PROPERTY(ascii_char_ptr_no_quotes, SupportedMethods)
);

END_NAMESPACE(Contoso);

/* Callback after sending reported properties */
void deviceTwinCallback(int status_code, void* userContextCallback)
{
	(void)(userContextCallback);
	printf("IoTHub: reported properties delivered with status_code = %u\n", status_code);
}

void onDesiredTelemetryInterval(void* argument)
{
	/* By convention 'argument' is of the type of the MODEL */
	Thermostat* thermostat = argument;
	printf("Received a new desired_TelemetryInterval = %d\r\n", thermostat->TelemetryInterval);
	thermostat->Config.TelemetryInterval = thermostat->TelemetryInterval;
	if (IoTHubDeviceTwin_SendReportedStateThermostat(thermostat, deviceTwinCallback, NULL) != IOTHUB_CLIENT_OK)
	{
		printf("Report Config.TelemetryInterval property failed");
	}
	else
	{
		printf("Report new value of Config.TelemetryInterval property: %d\r\n", thermostat->Config.TelemetryInterval);
	}
}

/*change light status on Raspberry Pi to received value*/
METHODRETURN_HANDLE ChangeLightStatus(Thermostat* thermostat, int lightstatus)
{
	printf("Raspberry Pi light status change\n");
	pinMode(Grn_led_pin, OUTPUT);
	printf("LED value\n %d", lightstatus);
	digitalWrite(Grn_led_pin, lightstatus);
	return MethodReturn_Create(201, "\"light status changed\"");
}

METHODRETURN_HANDLE LightBlink(Thermostat* thermostat)
{
	int blinkCount = 2;
	printf("Raspberry Pi light blink\n");
	while (blinkCount--)
	{
		pinMode(Grn_led_pin, OUTPUT);
		printf("light on\n");
		digitalWrite(Grn_led_pin, 1);
		ThreadAPI_Sleep(1000);
		printf("light off\n");
		digitalWrite(Grn_led_pin, 0);
		ThreadAPI_Sleep(1000);
	}
	return MethodReturn_Create(201, "\"light blink success\"");
}

/* Send data to IoT Hub */
static void sendMessage(IOTHUB_CLIENT_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
	if (messageHandle == NULL)
	{
		printf("unable to create a new IoTHubMessage\r\n");
	}
	else
	{
		if (IoTHubClient_SendEventAsync(iotHubClientHandle, messageHandle, NULL, NULL) != IOTHUB_CLIENT_OK)
		{
			printf("failed to hand over the message to IoTHubClient");
		}
		else
		{
			printf("IoTHubClient accepted the message for delivery\r\n");
		}

		IoTHubMessage_Destroy(messageHandle);
	}
	free((void*)buffer);
}

void SendDeviceInfo(IOTHUB_CLIENT_HANDLE iotHubClientHandle)
{
	char* buffer = malloc(sizeof(char) * 512);
	sprintf(buffer, deviceInfo, deviceId);
	printf("send device info: %s %d\r\n", buffer, strlen(buffer));
	sendMessage(iotHubClientHandle, buffer, strlen(buffer));
}

void SendTelemetryData(IOTHUB_CLIENT_HANDLE iotHubClientHandle)
{
	float tempC = -300.0;
	float pressurePa = -300;
	float humidityPct = -300;

	int sensorResult = bme280_read_sensors(&tempC, &pressurePa, &humidityPct);

	if (sensorResult == 1)
	{
		printf("Read Sensor Data: Humidity = %.1f%% Temperature = %.1f*C \n",
			humidityPct, tempC);
	}
	else
	{
		printf("Read Sensor Data Failed, send simulated data Humidity = %.1f%% Temperature = %.1f*C \n", humidityPct, tempC);
	}

	char* buffer = malloc(sizeof(char) * 256);
	sprintf(buffer, telemetryData, deviceId, tempC , humidityPct);
	printf("Sending sensor value: %s %d\r\n", buffer, strlen(buffer));
	sendMessage(iotHubClientHandle, buffer, strlen(buffer));
}

void remote_monitoring_run(void)
{
	if (platform_init() != 0)
	{
		printf("Failed to initialize the platform.\n");
	}
	else
	{
		if (SERIALIZER_REGISTER_NAMESPACE(Contoso) == NULL)
		{
			printf("Unable to SERIALIZER_REGISTER_NAMESPACE\n");
		}
		else
		{
			IOTHUB_CLIENT_HANDLE iotHubClientHandle = IoTHubClient_CreateFromConnectionString(connectionString, MQTT_Protocol);
			g_iotHubClientHandle = iotHubClientHandle;
			if (iotHubClientHandle == NULL)
			{
				printf("Failure in IoTHubClient_CreateFromConnectionString\n");
			}
			else
			{
#ifdef MBED_BUILD_TIMESTAMP
				// For mbed add the certificate information
				if (IoTHubClient_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
				{
					printf("Failed to set option \"TrustedCerts\"\n");
				}
#endif // MBED_BUILD_TIMESTAMP
				Thermostat* thermostat = IoTHubDeviceTwin_CreateThermostat(iotHubClientHandle);
				if (thermostat == NULL)
				{
					printf("Failure in IoTHubDeviceTwin_CreateThermostat\n");
				}
				else
				{
					/* Set values for reported properties */
					thermostat->Config.TelemetryInterval = 3;
					thermostat->System.FirmwareVersion = "1.0";
					/* Specify the signatures of the supported direct methods */
					thermostat->SupportedMethods = supportedMethod;

					/* Send reported properties to IoT Hub */
					if (IoTHubDeviceTwin_SendReportedStateThermostat(thermostat, deviceTwinCallback, NULL) != IOTHUB_CLIENT_OK)
					{
						printf("Failed sending serialized reported state\n");
					}
					else
					{
						printf("Send DeviceInfo object to IoT Hub at startup\n");

						SendDeviceInfo(iotHubClientHandle);

						/* set default telemetry interval */
						thermostat->TelemetryInterval = 3;

						while (1)
						{
							SendTelemetryData(iotHubClientHandle);

							ThreadAPI_Sleep(thermostat->TelemetryInterval * 1000);
						}

						IoTHubDeviceTwin_DestroyThermostat(thermostat);
					}
				}
				IoTHubClient_Destroy(iotHubClientHandle);
			}
			serializer_deinit();
		}
	}
	platform_deinit();
}

int remote_monitoring_init(void)
{
	int result;

	Lock_fd = open_lockfile(LOCKFILE);

	if (setuid(getuid()) < 0)
	{
		perror("Dropping privileges failed. (did you use sudo?)n");
		result = EXIT_FAILURE;
	}
	else
	{
		result = wiringPiSetup();
		if (result != 0)
		{
			perror("Wiring Pi setup failed.");
		}
		else
		{
			result = wiringPiSPISetup(Spi_channel, Spi_clock);
			if (result < 0)
			{
				printf("Can't setup SPI, error %i calling wiringPiSPISetup(%i, %i)  %sn",
					result, Spi_channel, Spi_clock, strerror(result));
			}
			else
			{
				int sensorResult = bme280_init(Spi_channel);
				if (sensorResult != 1)
				{
					printf("It appears that no BMP280 module on Chip Enable %i is attached. Aborting.\n", Spi_channel);
					result = 1;
				}
				else
				{
					// Read the Temp & Pressure module.
					float tempC = -300.0;
					float pressurePa = -300;
					float humidityPct = -300;
					sensorResult = bme280_read_sensors(&tempC, &pressurePa, &humidityPct);
					if (sensorResult == 1)
					{
						printf("Temperature = %.1f *C  Pressure = %.1f Pa  Humidity = %1f %%\n",
							tempC, pressurePa, humidityPct);
						result = 0;
					}
					else
					{
						printf("Unable to read BME280 on pin %i. Aborting.\n", Spi_channel);
						result = 1;
					}
				}
			}
		}
	}
	return result;
}

int main(void)
{
	int result = remote_monitoring_init();
	if (result == 0)
	{
		remote_monitoring_run();
	}
	return result;
}