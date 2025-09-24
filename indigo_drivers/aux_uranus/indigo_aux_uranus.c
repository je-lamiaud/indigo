// Copyright (c) 2025 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 1.0 by J-E Lamiaud

/** INDIGO Uranus driver
 \file indigo_aux_uranus.c
 */

#define DRIVER_VERSION 0x00001
#define DRIVER_NAME "indigo_aux_uranus"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <termios.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_io.h>
#include <indigo/indigo_gps_driver.h>

#include "indigo_aux_uranus.h"

#define PRIVATE_DATA                                    ((uranus_private_data *)device->private_data)

#define AUX_WEATHER_GROUP                        "Weather"

#define AUX_WEATHER_PROPERTY                     (PRIVATE_DATA->weather_property)
#define AUX_WEATHER_TEMPERATURE_ITEM             (AUX_WEATHER_PROPERTY->items + 0)
#define AUX_WEATHER_SKY_TEMPERATURE_ITEM         (AUX_WEATHER_PROPERTY->items + 1)
#define AUX_WEATHER_DEWPOINT_ITEM                (AUX_WEATHER_PROPERTY->items + 2)
#define AUX_WEATHER_HUMIDITY_ITEM                (AUX_WEATHER_PROPERTY->items + 3)
#define AUX_WEATHER_PRESSURE_ITEM                (AUX_WEATHER_PROPERTY->items + 4)
#define AUX_WEATHER_CLOUD_COVER_ITEM             (AUX_WEATHER_PROPERTY->items + 5)
#define AUX_WEATHER_SKY_BRIGHTNESS_ITEM          (AUX_WEATHER_PROPERTY->items + 6)
#define AUX_WEATHER_SKY_BORTLE_CLASS_ITEM        (AUX_WEATHER_PROPERTY->items + 7)
#define X_AUX_WEATHER_NELM_ITEM                  (AUX_WEATHER_PROPERTY->items + 8)

#define AUX_CLOUD_THRESHOLDS_PROPERTY            (PRIVATE_DATA->cloud_condition_thresholds_property)
#define AUX_CLOUD_CLEAR_THRESHOLD_ITEM      	    (AUX_CLOUD_THRESHOLDS_PROPERTY->items + 0)
#define AUX_CLOUD_CLOUDY_THRESHOLD_ITEM     	    (AUX_CLOUD_THRESHOLDS_PROPERTY->items + 1)

#define AUX_CLOUD_PROPERTY                  	    (PRIVATE_DATA->cloud_condition_property)
#define AUX_CLOUD_CLEAR_ITEM                	    (AUX_CLOUD_PROPERTY->items + 0)
#define AUX_CLOUD_CLOUDY_ITEM               	    (AUX_CLOUD_PROPERTY->items + 1)
#define AUX_CLOUD_OVERCAST_ITEM             	    (AUX_CLOUD_PROPERTY->items + 2)

#define X_AUX_URANUS_HEALTH_PROPERTY             (PRIVATE_DATA->health_property)
#define X_AUX_URANUS_HEALTH_ITEM                 (X_AUX_URANUS_HEALTH_PROPERTY->items + 0)
#define X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY	 (PRIVATE_DATA->battery_voltage_property)
#define X_AUX_URANUS_BATTERY_VOLTAGE_ITEM        (X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY->items + 0)
#define X_AUX_SENSOR_READINGS_PROPERTY           (PRIVATE_DATA->sensor_readings_property)
#define X_AUX_INFRARED_SENSOR_TEMPERATURE_ITEM   (X_AUX_SENSOR_READINGS_PROPERTY->items + 0)
#define X_AUX_FULL_SPETRUM_RAW_VALUE_ITEM        (X_AUX_SENSOR_READINGS_PROPERTY->items + 1)
#define X_AUX_INFRARED_RAW_VALUE_ITEM            (X_AUX_SENSOR_READINGS_PROPERTY->items + 2)
#define X_AUX_VISUAL_RAW_VALUE_ITEM              (X_AUX_SENSOR_READINGS_PROPERTY->items + 3)
#define X_AUX_URANUS_RESET_PROPERTY              (PRIVATE_DATA->reset_property)
#define X_AUX_URANUS_RESET_ITEM                  (X_AUX_URANUS_RESET_PROPERTY->items + 0)

#define X_AUX_WEATHER_NELM_ITEM_NAME            "X_AUX_WEATHER_NELM"
#define X_AUX_URANUS_HEALTH_PROPERTY_NAME        "X_AUX_URANUS_HEALTH"
#define X_AUX_URANUS_HEALTH_ITEM_NAME            "HEALTH"
#define X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY_NAME "X_AUX_URANUS_BATTERY_VOLTAGE"
#define X_AUX_URANUS_BATTERY_VOLTAGE_ITEM_NAME   "BATTERY_VOLTAGE"
#define X_AUX_SENSOR_READINGS_PROPERTY_NAME      "X_AUX_URANUS_SENSOR_READINGS"
#define X_AUX_INFRARED_SENSOR_TEMPERATURE_ITEM_NAME "INFRARED_SENSOR_TEMPERATURE"
#define X_AUX_FULL_SPETRUM_RAW_VALUE_ITEM_NAME   "FULL_SPECTRUM_RAW_VALUE"
#define X_AUX_INFRARED_RAW_VALUE_ITEM_NAME       "INFRARED_RAW_VALUE"
#define X_AUX_VISUAL_RAW_VALUE_ITEM_NAME         "VISUAL_RAW_VALUE"
#define X_AUX_URANUS_RESET_PROPERTY_NAME         "X_AUX_URANUS_RESET"
#define X_AUX_URANUS_RESET_ITEM_NAME             "RESET"

#define RESPONSE_LENGTH 120

typedef struct {
	int handle;
	int device_count;
	indigo_property *health_property;
	indigo_property *battery_voltage_property;
	indigo_property *sensor_readings_property;
	indigo_property *reset_property;
	indigo_property *weather_property;
	indigo_property *cloud_condition_thresholds_property;
	indigo_property *cloud_condition_property;
	indigo_timer *aux_timer_callback;
	indigo_timer *gps_timer_callback;
	bool start_measure;
	int altitude;            // Altitude is provided by the MA command which is used by the weather driver
	bool altitude_available; // The GPS driver reuses it if available, otherwise it uses the MA command
	time_t gps_time;
	int gps_timeout;
	pthread_mutex_t mutex;
} uranus_private_data;


// -------------------------------------------------------------------------------- serial interface

static bool uranus_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_open_serial_with_speed(DEVICE_PORT_ITEM->text.value, 115200);
	if (PRIVATE_DATA->handle < 0) {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to connect to %s", DEVICE_PORT_ITEM->text.value);
		return false;
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Connected to %s", DEVICE_PORT_ITEM->text.value);
	return true;
}

static bool uranus_command(indigo_device *device, const char *command, char *response, int max) {
	bool result = false;
	pthread_mutex_lock(&PRIVATE_DATA->mutex);
	tcflush(PRIVATE_DATA->handle, TCIOFLUSH);
	result = indigo_write(PRIVATE_DATA->handle, command, strlen(command));
	if (result)
		result = indigo_write(PRIVATE_DATA->handle, "\n", 1);
	if (result && response != NULL) {
		if (indigo_read_line(PRIVATE_DATA->handle, response, max) == -1) {
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> no response", command);
			result = false;
		}
	}
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Command %s -> %s", command, (result && response != NULL) ? response : "");
	pthread_mutex_unlock(&PRIVATE_DATA->mutex);
	return result;
}

static void uranus_close(indigo_device *device) {
	if (PRIVATE_DATA->handle >= 0) {
		close(PRIVATE_DATA->handle);
		PRIVATE_DATA->handle = -1;
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Disconnected");
	}
}

static bool uranus_connect(indigo_device *device) {
	if (!uranus_open(device))
		return false;

	char response[16] = { 0 };
	if (uranus_command(device, "M#", response, 16) && strncmp(response, "MS_", 3) == 0) {
		strncpy(X_AUX_URANUS_HEALTH_ITEM->text.value, &response[3], INDIGO_VALUE_SIZE);
		X_AUX_URANUS_HEALTH_PROPERTY->state = strncmp(X_AUX_URANUS_HEALTH_ITEM->text.value, "OK", sizeof(response)) == 0 ? INDIGO_OK_STATE : INDIGO_ALERT_STATE;
		indigo_update_property(device, X_AUX_URANUS_HEALTH_PROPERTY, NULL);
		return true;
	}

	INDIGO_DRIVER_ERROR(DRIVER_NAME, "Handshake failed");
	uranus_close(device);
	return false;
}

// -------------------------------------------------------------------------------- async handlers

static void aux_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	char response[RESPONSE_LENGTH] = { 0 }, *pnt;

	if (PRIVATE_DATA->start_measure) {
		// Start a sky brightness measurement
		if (uranus_command(device, "SQ:1", response, RESPONSE_LENGTH)) {
			if (strncmp(response, "SQ:MSR", 6) == 0) {
				AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
				PRIVATE_DATA->start_measure = false;
			} else {
				AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		} else {
			AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	} else {
		// Retrieve sky brightness measure
		if (uranus_command(device, "SQ", response, RESPONSE_LENGTH)) {
			char *tok = strtok_r(response, ":", &pnt);
			if (tok == NULL || strncmp(tok, "SQ", 2) != 0) {
				AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
				X_AUX_SENSOR_READINGS_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				AUX_WEATHER_SKY_BRIGHTNESS_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				X_AUX_WEATHER_NELM_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				X_AUX_FULL_SPETRUM_RAW_VALUE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				X_AUX_VISUAL_RAW_VALUE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				X_AUX_INFRARED_RAW_VALUE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				AUX_WEATHER_SKY_BORTLE_CLASS_ITEM->number.value = indigo_aux_sky_bortle(AUX_WEATHER_SKY_BRIGHTNESS_ITEM->number.value);
				AUX_WEATHER_PROPERTY->state = INDIGO_OK_STATE;
				X_AUX_SENSOR_READINGS_PROPERTY->state = INDIGO_OK_STATE;
			}
		} else {
			AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
			X_AUX_SENSOR_READINGS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		PRIVATE_DATA->start_measure = true;

		PRIVATE_DATA->altitude_available = false;
		if (uranus_command(device, "MA", response, RESPONSE_LENGTH)) {
			char *tok = strtok_r(response, ":", &pnt);
			if (tok == NULL || strncmp(tok, "MS_OK", 5) != 0) {
				AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
				X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY->state = INDIGO_ALERT_STATE;
				X_AUX_SENSOR_READINGS_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				AUX_WEATHER_TEMPERATURE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				AUX_WEATHER_HUMIDITY_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				AUX_WEATHER_DEWPOINT_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				AUX_WEATHER_PRESSURE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				strtok_r(NULL, ":", &pnt); // MSL pressure
				PRIVATE_DATA->altitude = strtol(strtok_r(NULL, ":", &pnt), NULL, 10);
				PRIVATE_DATA->altitude_available = true;
				AUX_WEATHER_SKY_TEMPERATURE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				X_AUX_INFRARED_SENSOR_TEMPERATURE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				strtok_r(NULL, ":", &pnt); // Battery usage indcator
				X_AUX_URANUS_BATTERY_VOLTAGE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY->state = INDIGO_OK_STATE;
			}
		} else {
			AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
			X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY->state = INDIGO_ALERT_STATE;
			X_AUX_SENSOR_READINGS_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		if (uranus_command(device, "CI", response, RESPONSE_LENGTH)) {
			char *tok = strtok_r(response, ":", &pnt);
			if (tok == NULL || strncmp(tok, "CI", 2) != 0) {
				AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
				AUX_CLOUD_PROPERTY->state = INDIGO_ALERT_STATE;
			} else {
				indigo_atod(strtok_r(NULL, ":", &pnt));
				AUX_WEATHER_CLOUD_COVER_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				if (AUX_WEATHER_CLOUD_COVER_ITEM->number.value <= AUX_CLOUD_CLEAR_THRESHOLD_ITEM->number.value) {
					AUX_CLOUD_PROPERTY->state = INDIGO_OK_STATE;
					indigo_set_switch(AUX_CLOUD_PROPERTY, AUX_CLOUD_CLEAR_ITEM, true);
				} else if (AUX_WEATHER_CLOUD_COVER_ITEM->number.value <= AUX_CLOUD_CLOUDY_THRESHOLD_ITEM->number.value) {
					AUX_CLOUD_PROPERTY->state = INDIGO_OK_STATE;
					indigo_set_switch(AUX_CLOUD_PROPERTY, AUX_CLOUD_CLOUDY_ITEM, true);
				} else {
					AUX_CLOUD_PROPERTY->state = INDIGO_OK_STATE;
					indigo_set_switch(AUX_CLOUD_PROPERTY, AUX_CLOUD_OVERCAST_ITEM, true);
				}
			}
		} else {
			AUX_WEATHER_PROPERTY->state = INDIGO_ALERT_STATE;
			AUX_CLOUD_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		indigo_update_property(device, X_AUX_URANUS_HEALTH_PROPERTY, NULL);
		indigo_update_property(device, X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY, NULL);
		indigo_update_property(device, X_AUX_SENSOR_READINGS_PROPERTY, NULL);
		indigo_update_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_update_property(device, AUX_CLOUD_PROPERTY, NULL);
	}

	indigo_reschedule_timer(device, 5, &PRIVATE_DATA->aux_timer_callback);
}

static void aux_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->device_count ==0) {
			if (!uranus_connect(device))
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			char response[RESPONSE_LENGTH] = { 0 }, *pnt;

			PRIVATE_DATA->device_count++;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_define_property(device, X_AUX_URANUS_HEALTH_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_SENSOR_READINGS_PROPERTY, NULL);
			indigo_define_property(device, X_AUX_URANUS_RESET_PROPERTY, NULL);
			indigo_define_property(device, AUX_WEATHER_PROPERTY, NULL);
			indigo_define_property(device, AUX_CLOUD_PROPERTY, NULL);

			if (uranus_command(device, "MV", response, RESPONSE_LENGTH)) {
				char *tok = strtok_r(response, ":", &pnt);
				if (tok != NULL && strncmp(tok, "MV", 2) == 0) {
					strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, strtok_r(NULL, ":", &pnt), INDIGO_VALUE_SIZE);
				} else {
					strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "---", INDIGO_VALUE_SIZE);
				}
			} else {
				strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "---", INDIGO_VALUE_SIZE);
			}
			if (uranus_command(device, "SR", response, RESPONSE_LENGTH)) {
				char *tok = strtok_r(response, ":", &pnt);
				if (tok != NULL && strncmp(tok, "SR", 2) == 0) {
					strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, strtok_r(NULL, ":", &pnt), INDIGO_VALUE_SIZE);
				} else {
					strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "---", INDIGO_VALUE_SIZE);
				}
			} else {
				strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "---", INDIGO_VALUE_SIZE);
			}
			indigo_update_property(device, INFO_PROPERTY, NULL);
			PRIVATE_DATA->start_measure = true;
			indigo_set_timer(device, 0, aux_timer_callback, &PRIVATE_DATA->aux_timer_callback);
		} else {
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->aux_timer_callback);
		PRIVATE_DATA->altitude_available = false;
		PRIVATE_DATA->device_count--;
		if (PRIVATE_DATA->device_count == 0)
			uranus_close(device);
		indigo_delete_property(device, X_AUX_URANUS_HEALTH_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_SENSOR_READINGS_PROPERTY, NULL);
		indigo_delete_property(device, X_AUX_URANUS_RESET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_WEATHER_PROPERTY, NULL);
		indigo_delete_property(device, AUX_CLOUD_PROPERTY, NULL);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

static void aux_uranus_reset_handler(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}
	char response[RESPONSE_LENGTH] = { 0 };

	if (uranus_command(device, "MQ:", NULL, 0)) {
		X_AUX_URANUS_RESET_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		X_AUX_URANUS_RESET_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	X_AUX_URANUS_RESET_ITEM->sw.value = false;
	indigo_update_property(device, X_AUX_URANUS_RESET_PROPERTY, NULL);
	// Device reboots, we need to disconnect
	indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
	aux_connection_handler(device);
}

// -------------------------------------------------------------------------------- INDIGO aux device implementation

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_SQM|INDIGO_INTERFACE_AUX_WEATHER) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- HEALTH
		X_AUX_URANUS_HEALTH_PROPERTY = indigo_init_text_property(NULL, device->name, X_AUX_URANUS_HEALTH_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Health status", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_AUX_URANUS_HEALTH_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_text_item(X_AUX_URANUS_HEALTH_ITEM, X_AUX_URANUS_HEALTH_ITEM_NAME, "Health status", "---");
		// -------------------------------------------------------------------------------- VOLTAGE
		X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY = indigo_init_number_property(NULL, device->name, X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Battery voltage", INDIGO_OK_STATE, INDIGO_RO_PERM, 1);
		if (X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_AUX_URANUS_BATTERY_VOLTAGE_ITEM, X_AUX_URANUS_BATTERY_VOLTAGE_ITEM_NAME, "Voltage [V]", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- SENSOR READINGS
		X_AUX_SENSOR_READINGS_PROPERTY = indigo_init_number_property(NULL, device->name, X_AUX_SENSOR_READINGS_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Sensor readings", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (X_AUX_SENSOR_READINGS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_AUX_INFRARED_SENSOR_TEMPERATURE_ITEM, X_AUX_INFRARED_SENSOR_TEMPERATURE_ITEM_NAME, "Infrared sensor temperature [\u00B0C]", -40, 60, 0, 0);
		indigo_init_number_item(X_AUX_FULL_SPETRUM_RAW_VALUE_ITEM, X_AUX_FULL_SPETRUM_RAW_VALUE_ITEM_NAME, "Full spectrum raw value", 0, 65535, 0, 0);
		indigo_init_number_item(X_AUX_INFRARED_RAW_VALUE_ITEM, X_AUX_INFRARED_RAW_VALUE_ITEM_NAME, "Infrared raw value", 0, 65535, 0, 0);
		indigo_init_number_item(X_AUX_VISUAL_RAW_VALUE_ITEM, X_AUX_VISUAL_RAW_VALUE_ITEM_NAME, "Visual raw value", 0, 65535, 0, 0);
		// -------------------------------------------------------------------------------- RESET
		X_AUX_URANUS_RESET_PROPERTY = indigo_init_switch_property(NULL, device->name, X_AUX_URANUS_RESET_PROPERTY_NAME, AUX_ADVANCED_GROUP, "Reset", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		indigo_init_switch_item(X_AUX_URANUS_RESET_ITEM, X_AUX_URANUS_RESET_ITEM_NAME, "Reset Uranus", false);
		// -------------------------------------------------------------------------------- WEATHER
		AUX_WEATHER_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_WEATHER_PROPERTY_NAME, AUX_WEATHER_GROUP, "Weather conditions", INDIGO_OK_STATE, INDIGO_RO_PERM, 9);
		if (AUX_WEATHER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_WEATHER_TEMPERATURE_ITEM, AUX_WEATHER_TEMPERATURE_ITEM_NAME, "Ambient temperature [\u00B0C]", -40, 60, 0, 0);
		indigo_init_number_item(AUX_WEATHER_HUMIDITY_ITEM, AUX_WEATHER_HUMIDITY_ITEM_NAME, "Humidity [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_DEWPOINT_ITEM, AUX_WEATHER_DEWPOINT_ITEM_NAME, "Dewpoint [\u00B0C]", -40, 60, 0, 0);
		indigo_init_number_item(AUX_WEATHER_PRESSURE_ITEM, AUX_WEATHER_PRESSURE_ITEM_NAME, "Atmospheric pressure (hPa)", 300, 1250, 0, 0);
		indigo_init_number_item(AUX_WEATHER_CLOUD_COVER_ITEM, "X_AUX_WEATHER_COULD_COVER", "Cloud cover [%]", 0, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_SKY_BRIGHTNESS_ITEM, AUX_WEATHER_SKY_BRIGHTNESS_ITEM_NAME, "Sky brightness [m/arcsec\u00B2]", 0, 30, 0, 0);
		indigo_init_number_item(AUX_WEATHER_SKY_TEMPERATURE_ITEM, AUX_WEATHER_SKY_TEMPERATURE_ITEM_NAME, "Sky temperature [\u00B0C]", -100, 100, 0, 0);
		indigo_init_number_item(AUX_WEATHER_SKY_BORTLE_CLASS_ITEM, AUX_WEATHER_SKY_BORTLE_CLASS_ITEM_NAME, "Sky Bortle class", 1, 9, 0, 0);
		indigo_init_number_item(X_AUX_WEATHER_NELM_ITEM, X_AUX_WEATHER_NELM_ITEM_NAME, "Naked eye limiting magnitude", 0, 10, 0, 0);
		// -------------------------------------------------------------------------------- AUX_CLOUD_THRESHOLDS
		AUX_CLOUD_THRESHOLDS_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_CLOUD_THRESHOLDS_PROPERTY_NAME, "Settings", "Cloud thresholds [%]", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (AUX_CLOUD_THRESHOLDS_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(AUX_CLOUD_CLEAR_THRESHOLD_ITEM, AUX_CLOUD_CLEAR_ITEM_NAME, "Clear (less than)", 0, 100, 0, 10);
		indigo_init_number_item(AUX_CLOUD_CLOUDY_THRESHOLD_ITEM, AUX_CLOUD_CLOUDY_ITEM_NAME, "Cloudy (less than)", 0, 100, 0, 80);
		// -------------------------------------------------------------------------------- AUX_CLOUD
		AUX_CLOUD_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_CLOUD_PROPERTY_NAME, AUX_WEATHER_GROUP, "Cloud conditions", INDIGO_OK_STATE, INDIGO_RO_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (AUX_CLOUD_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(AUX_CLOUD_CLEAR_ITEM, AUX_CLOUD_CLEAR_ITEM_NAME, "Clear", false);
		indigo_init_switch_item(AUX_CLOUD_CLOUDY_ITEM, AUX_CLOUD_CLOUDY_ITEM_NAME, "Cloudy", false);
		indigo_init_switch_item(AUX_CLOUD_OVERCAST_ITEM, AUX_CLOUD_OVERCAST_ITEM_NAME, "Overcast", false);
		// -------------------------------------------------------------------------------- INFO
		INFO_PROPERTY->count = 8;
		strncpy(INFO_DEVICE_MODEL_ITEM->text.value, "Uranus Meteo Sensor", INDIGO_VALUE_SIZE);
		strncpy(INFO_DEVICE_FW_REVISION_ITEM->text.value, "---", INDIGO_VALUE_SIZE);
		strncpy(INFO_DEVICE_HW_REVISION_ITEM->text.value, "---", INDIGO_VALUE_SIZE);
		strncpy(INFO_DEVICE_SERIAL_NUM_ITEM->text.value, "---", INDIGO_VALUE_SIZE);
		// -------------------------------------------------------------------------------- DEVICE_PORT, DEVICE_PORTS
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
#ifdef INDIGO_MACOS
		for (int i = 0; i < DEVICE_PORTS_PROPERTY->count; i++) {
			if (!strncmp(DEVICE_PORTS_PROPERTY->items[i].name, "/dev/cu.usbmodem", 16)) {
				indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[i].name);
				break;
			}
		}
#endif
#ifdef INDIGO_LINUX
		if (DEVICE_PORTS_PROPERTY->count > 1) {
			/* 0 is refresh button */
			indigo_copy_value(DEVICE_PORT_ITEM->text.value, DEVICE_PORTS_PROPERTY->items[1].name);
		} else {
			strcpy(DEVICE_PORT_ITEM->text.value, "/dev/ttyUSB0");
		}
#endif
		// --------------------------------------------------------------------------------
		ADDITIONAL_INSTANCES_PROPERTY->hidden = DEVICE_CONTEXT->base_device != NULL;
		pthread_mutex_init(&PRIVATE_DATA->mutex, NULL);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		indigo_define_matching_property(X_AUX_URANUS_HEALTH_PROPERTY);
		indigo_define_matching_property(X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY);
		indigo_define_matching_property(X_AUX_SENSOR_READINGS_PROPERTY);
		indigo_define_matching_property(X_AUX_URANUS_RESET_PROPERTY);
		indigo_define_matching_property(AUX_WEATHER_PROPERTY);
		indigo_define_matching_property(AUX_CLOUD_PROPERTY);
	}
	indigo_define_matching_property(AUX_CLOUD_THRESHOLDS_PROPERTY);
	return indigo_aux_enumerate_properties(device, NULL, NULL);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_connection_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(X_AUX_URANUS_RESET_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- RESET
		indigo_property_copy_values(X_AUX_URANUS_RESET_PROPERTY, property, false);
		X_AUX_URANUS_RESET_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_AUX_URANUS_RESET_PROPERTY, NULL);
		indigo_set_timer(device, 0, aux_uranus_reset_handler, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_CLOUD_THRESHOLDS_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- AUX_CLOUD_THRESHOLDS
		indigo_property_copy_values(AUX_CLOUD_THRESHOLDS_PROPERTY, property, false);
		AUX_CLOUD_THRESHOLDS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, AUX_CLOUD_THRESHOLDS_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_CLOUD_THRESHOLDS_PROPERTY);
		}
	// --------------------------------------------------------------------------------
	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(X_AUX_URANUS_HEALTH_PROPERTY);
	indigo_release_property(X_AUX_URANUS_BATTERY_VOLTAGE_PROPERTY);
	indigo_release_property(X_AUX_SENSOR_READINGS_PROPERTY);
	indigo_release_property(X_AUX_URANUS_RESET_PROPERTY);
	indigo_release_property(AUX_WEATHER_PROPERTY);
	indigo_release_property(AUX_CLOUD_PROPERTY);
	indigo_delete_property(device, AUX_CLOUD_THRESHOLDS_PROPERTY, NULL);
	indigo_release_property(AUX_CLOUD_THRESHOLDS_PROPERTY);
	pthread_mutex_destroy(&PRIVATE_DATA->mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

// -------------------------------------------------------------------------------- GPS device

// -------------------------------------------------------------------------------- async handlers
static void gps_timer_callback(indigo_device *device) {
	if (!IS_CONNECTED) {
		return;
	}

	if (PRIVATE_DATA->gps_timeout <= 0) {
		PRIVATE_DATA->gps_timeout = 9;

		char response[RESPONSE_LENGTH] = { 0 }, *pnt;

		// Get time and location
		if (uranus_command(device, "GP", response, RESPONSE_LENGTH)) {
			char *tok = strtok_r(response, ":", &pnt);
			if (tok == NULL || strncmp(tok, "GP", 2) != 0) {
				GPS_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
				GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				GPS_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
			} else {
				const char *fix = strtok_r(NULL, ":", &pnt);
				if (!strncmp(fix, "0", 1)) {
					if (GPS_STATUS_PROPERTY->state != INDIGO_OK_STATE
						|| GPS_STATUS_NO_FIX_ITEM->light.value != INDIGO_ALERT_STATE) {
						GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_ALERT_STATE;
						GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
						GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
						GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
					}
					if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE)
						GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE)
						GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
				} else if (!strncmp(fix, "2", 1)) {
					if (GPS_STATUS_PROPERTY->state != INDIGO_OK_STATE
						|| GPS_STATUS_2D_FIX_ITEM->light.value != INDIGO_BUSY_STATE) {
						GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
						GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_BUSY_STATE;
						GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
						GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
					}
					if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_BUSY_STATE)
						GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_BUSY_STATE;
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE)
						GPS_UTC_TIME_PROPERTY->state = INDIGO_BUSY_STATE;
				} else if (!strncmp(fix, "3", 1)) {
					if (GPS_STATUS_PROPERTY->state != INDIGO_OK_STATE
						|| GPS_STATUS_3D_FIX_ITEM->light.value != INDIGO_OK_STATE) {
						GPS_STATUS_NO_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
						GPS_STATUS_2D_FIX_ITEM->light.value = INDIGO_IDLE_STATE;
						GPS_STATUS_3D_FIX_ITEM->light.value = INDIGO_OK_STATE;
						GPS_STATUS_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
					}
					if (GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state != INDIGO_OK_STATE)
						GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_OK_STATE;
					if (GPS_UTC_TIME_PROPERTY->state != INDIGO_OK_STATE)
						GPS_UTC_TIME_PROPERTY->state = INDIGO_OK_STATE;
				} else {
					GPS_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
					GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
					GPS_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
					GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
				}
				struct tm utc;
				PRIVATE_DATA->gps_time = strtol(strtok_r(NULL, ":", &pnt), NULL, 10);
				if (gmtime_r(&PRIVATE_DATA->gps_time, &utc) != NULL) {
					sprintf(GPS_UTC_ITEM->text.value, "%04d-%02d-%02dT%02d:%02d:%02d",
							utc.tm_year + 1900, utc.tm_mon, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec);
				} else {
					sprintf(GPS_UTC_ITEM->text.value, "0000-00-00T00:00:00.00");
					GPS_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
				}
				GPS_UTC_OFFEST_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = indigo_atod(strtok_r(NULL, ":", &pnt));
				GPS_ADVANCED_STATUS_SVS_IN_USE_ITEM->number.value = strtol(strtok_r(NULL, ":", &pnt), NULL, 10);
				GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_OK_STATE;
			}
		} else {
			GPS_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
			GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			GPS_UTC_TIME_PROPERTY->state = INDIGO_ALERT_STATE;
			GPS_ADVANCED_STATUS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, GPS_STATUS_PROPERTY, NULL);
		}

		if (PRIVATE_DATA->altitude_available) {
			GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = PRIVATE_DATA->altitude;
			PRIVATE_DATA->altitude_available = false;
		} else {
			// Interrogate altitude
			if (uranus_command(device, "MA", response, RESPONSE_LENGTH)) {
				char *tok = strtok_r(response, ":", &pnt);
				if (tok == NULL || strncmp(tok, "MS_OK", 5) != 0) {
					GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
				} else {
					strtok_r(NULL, ":", &pnt);
					strtok_r(NULL, ":", &pnt);
					strtok_r(NULL, ":", &pnt);
					strtok_r(NULL, ":", &pnt);
					strtok_r(NULL, ":", &pnt);
					GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = strtol(strtok_r(NULL, ":", &pnt), NULL, 10);
				}
			} else {
				GPS_GEOGRAPHIC_COORDINATES_PROPERTY->state = INDIGO_ALERT_STATE;
			}
		}

		indigo_update_property(device, GPS_GEOGRAPHIC_COORDINATES_PROPERTY, NULL);
		indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
		indigo_update_property(device, GPS_ADVANCED_STATUS_PROPERTY, NULL);
	} else {
		PRIVATE_DATA->gps_timeout--;
		// Extrapolate the time in between interrogations
		struct tm utc;
		PRIVATE_DATA->gps_time++;
		if (gmtime_r(&PRIVATE_DATA->gps_time, &utc) != NULL) {
			sprintf(GPS_UTC_ITEM->text.value, "%04d-%02d-%02dT%02d:%02d:%02d",
					utc.tm_year + 1900, utc.tm_mon, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec);
			indigo_update_property(device, GPS_UTC_TIME_PROPERTY, NULL);
		}
	}

	indigo_reschedule_timer(device, 1, &PRIVATE_DATA->gps_timer_callback);
}

static void gps_connection_handler(indigo_device *device) {
	indigo_lock_master_device(device);
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		if (PRIVATE_DATA->device_count ==0) {
			if (!uranus_connect(device->master_device))
				CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
		}
		
		if (CONNECTION_PROPERTY->state == INDIGO_BUSY_STATE) {
			PRIVATE_DATA->device_count++;
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			GPS_GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM->number.value = 0;
			GPS_GEOGRAPHIC_COORDINATES_LATITUDE_ITEM->number.value = 0;
			GPS_GEOGRAPHIC_COORDINATES_ELEVATION_ITEM->number.value = 0;
			sprintf(GPS_UTC_ITEM->text.value, "0000-00-00T00:00:00.00");
			PRIVATE_DATA->gps_timeout = 0;
			indigo_set_timer(device, 0, gps_timer_callback, &PRIVATE_DATA->gps_timer_callback);
		} else {
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_timer_sync(device, &PRIVATE_DATA->gps_timer_callback);
		PRIVATE_DATA->device_count--;
		if (PRIVATE_DATA->device_count == 0)
			uranus_close(device);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_gps_change_property(device, NULL, CONNECTION_PROPERTY);
	indigo_unlock_master_device(device);
}

// -------------------------------------------------------------------------------- INDIGO GPS device implementation

static indigo_result gps_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_gps_attach(device, DRIVER_NAME, DRIVER_VERSION) == INDIGO_OK) {
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->hidden = false;
		GPS_GEOGRAPHIC_COORDINATES_PROPERTY->count = 3;
		GPS_UTC_TIME_PROPERTY->hidden = false;
		GPS_UTC_TIME_PROPERTY->count = 2;
		GPS_ADVANCED_PROPERTY->hidden = false;
		GPS_ADVANCED_STATUS_PROPERTY->count = 1;

		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_gps_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result gps_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		if (indigo_ignore_connection_change(device, property))
			return INDIGO_OK;
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, CONNECTION_PROPERTY, NULL);
		indigo_set_timer(device, 0, gps_connection_handler, NULL);
		return INDIGO_OK;
	// --------------------------------------------------------------------------------
	}
	return indigo_gps_change_property(device, client, property);
}

static indigo_result gps_detach(indigo_device *device) {
	assert(device != NULL);
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		gps_connection_handler(device);
	}
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_gps_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO driver implementation

indigo_result indigo_aux_uranus(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static uranus_private_data *private_data = NULL;
	static indigo_device *aux = NULL;
	static indigo_device *gps = NULL;

	static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(
		"Pegasus Uranus Meteo",
		aux_attach,
		aux_enumerate_properties,
		aux_change_property,
		NULL,
		aux_detach
		);
	static indigo_device gps_template = INDIGO_DEVICE_INITIALIZER(
		"Pegasus Uranus GPS",
		gps_attach,
		indigo_gps_enumerate_properties,
		gps_change_property,
		NULL,
		gps_detach
		);

	SET_DRIVER_INFO(info, "PegasusAstro Uranus Meteo Sensor", __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(uranus_private_data));
			aux = indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			aux->master_device = aux;
			indigo_attach_device(aux);
			gps = indigo_safe_malloc_copy(sizeof(indigo_device), &gps_template);
			gps->private_data = private_data;
			gps->master_device = aux;
			indigo_attach_device(gps);
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			VERIFY_NOT_CONNECTED(aux);
			VERIFY_NOT_CONNECTED(gps);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				free(aux);
				aux = NULL;
			}
			if (gps != NULL) {
				indigo_detach_device(gps);
				free(gps);
				gps = NULL;
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;

		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
