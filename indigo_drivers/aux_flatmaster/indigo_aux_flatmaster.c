// Copyright (c) 2019-2026 Rumen G. Bogdanovski
// All rights reserved.

// You may use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).

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

// This file generated from indigo_aux_flatmaster.driver

#pragma mark - Includes

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_aux_driver.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_aux_flatmaster.h"

#pragma mark - Common definitions

#define DRIVER_VERSION       0x0300000B
#define DRIVER_NAME          "indigo_aux_flatmaster"
#define DRIVER_LABEL         "PegasusAstro FlatMaster"
#define AUX_DEVICE_NAME      "PegasusAstro FlatMaster"
#define PRIVATE_DATA         ((flatmaster_private_data *)device->private_data)

//+ define

#define AUX_LIGHT_ADD_PRESET_PROPERTY_NAME "X_FLM_ADD_PRESET"
#define AUX_LIGHT_ADD_PRESET_NAME_ITEM_NAME "NAME"

#define AUX_LIGHT_PRESETS_INTENSITY_PROPERTY_NAME "X_FLM_PRESETS_INTENSITY"

#define AUX_LIGHT_REMOVE_PRESET_PROPERTY_NAME "X_FLM_REMOVE_PRESET"
#define AUX_LIGHT_REMOVE_PRESET_NAME_ITEM_NAME "NAME"

#define AUX_LIGHT_PRESETS_PROPERTY_NAME "X_FLM_PRESETS"

#define INTENSITY(val)       ((int)floor((100 - (int)(val) - 0) * (220 - 20) / (100 - 0) + 20))
/* make sure switch value is provided as 0 / 1 */
#define SWITCH_VALUE(item)   ((item)->sw.value ? '1' : '0')

//- define

#pragma mark - Property definitions

#define AUX_LIGHT_SWITCH_PROPERTY      (PRIVATE_DATA->aux_light_switch_property)
#define AUX_LIGHT_SWITCH_ON_ITEM       (AUX_LIGHT_SWITCH_PROPERTY->items + 0)
#define AUX_LIGHT_SWITCH_OFF_ITEM      (AUX_LIGHT_SWITCH_PROPERTY->items + 1)

#define AUX_LIGHT_INTENSITY_PROPERTY   (PRIVATE_DATA->aux_light_intensity_property)
#define AUX_LIGHT_INTENSITY_ITEM       (AUX_LIGHT_INTENSITY_PROPERTY->items + 0)

#define AUX_LIGHT_ADD_PRESET_PROPERTY  (PRIVATE_DATA->aux_light_add_preset_property)
#define AUX_LIGHT_ADD_PRESET_NAME_ITEM (AUX_LIGHT_ADD_PRESET_PROPERTY->items + 0)

#define AUX_LIGHT_PRESETS_INTENSITY_PROPERTY (PRIVATE_DATA->aux_light_presets_intensity_property)

#define AUX_LIGHT_REMOVE_PRESET_PROPERTY  (PRIVATE_DATA->aux_light_remove_preset_property)
#define AUX_LIGHT_REMOVE_PRESET_NAME_ITEM (AUX_LIGHT_REMOVE_PRESET_PROPERTY->items + 0)

#define AUX_LIGHT_PRESETS_PROPERTY     (PRIVATE_DATA->aux_light_presets_property)

#pragma mark - Private data definition

typedef struct {
	indigo_uni_handle *handle;
	indigo_property *aux_light_switch_property;
	indigo_property *aux_light_intensity_property;
	indigo_property *aux_light_add_preset_property;
	indigo_property *aux_light_presets_intensity_property;
	indigo_property *aux_light_remove_preset_property;
	indigo_property *aux_light_presets_property;
	//+ data
	char response[12];
	//- data
} flatmaster_private_data;

#pragma mark - Low level code

//+ code

static int index_of_preset(indigo_device *device, const char *name) {
	int i;
	for (i = 0; i < AUX_LIGHT_PRESETS_PROPERTY->count; i++) {
		if (!strncasecmp((AUX_LIGHT_PRESETS_PROPERTY->items+i)->label, name, INDIGO_NAME_SIZE))
			return i;
	}

	// Not found
	return -1;
}

static bool flatmaster_command(indigo_device *device, char *command, ...) {
	long result = indigo_uni_discard(PRIVATE_DATA->handle);
	if (result >= 0) {
		va_list args;
		va_start(args, command);
		result = indigo_uni_vtprintf(PRIVATE_DATA->handle, command, args, "\n");
		va_end(args);
		if (result > 0) {
			result = indigo_uni_read_section(PRIVATE_DATA->handle, PRIVATE_DATA->response, sizeof(PRIVATE_DATA->response) - 1, "\n", "\r\n", INDIGO_DELAY(1));
		}
	}
	return result > 0;
}

static bool flatmaster_open(indigo_device *device) {
	PRIVATE_DATA->handle = indigo_uni_open_serial(DEVICE_PORT_ITEM->text.value, INDIGO_LOG_DEBUG);
	if (PRIVATE_DATA->handle != NULL) {
		if (flatmaster_command(device, "#") && !strcmp("OK_FM", PRIVATE_DATA->response)) {
			if (flatmaster_command(device, "V")) {
				INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, DRIVER_LABEL);
				INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, PRIVATE_DATA->response);
				indigo_update_property(device, INFO_PROPERTY, NULL);
				return true;
			}
		}
		indigo_uni_close(&PRIVATE_DATA->handle);
	}
	return false;
}

static void flatmaster_close(indigo_device *device) {
	flatmaster_command(device, "E:0");
	INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
	INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
	indigo_update_property(device, INFO_PROPERTY, NULL);
	indigo_uni_close(&PRIVATE_DATA->handle);
}

//- code

#pragma mark - High level code (aux)

static void aux_connection_handler(indigo_device *device) {
	if (CONNECTION_CONNECTED_ITEM->sw.value) {
		bool connection_result = true;
		connection_result = flatmaster_open(device);
		if (connection_result) {
			//+ aux.on_connect
			/* Switch the panel according to the property state */
			if (flatmaster_command(device, "E:%c", SWITCH_VALUE(AUX_LIGHT_SWITCH_ON_ITEM))) {
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_ALERT_STATE;
			      }
			/* If, On, set the intensity according to the property */
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
			if (AUX_LIGHT_SWITCH_ON_ITEM->sw.value) {
				/* bring 220-20 in range of 0-100 */
				if (!flatmaster_command(device, "L:%d", INTENSITY(AUX_LIGHT_INTENSITY_ITEM->number.value))) {
					AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
				}
			} else if (AUX_LIGHT_SWITCH_OFF_ITEM->sw.value) {
				/* Set the intensity to 0 for an off panel,
				 * this will make pre an post V2 firmware behave the same */
				if (!flatmaster_command(device, "L:%d", INTENSITY(0))) {
					AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
					indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
				}
			}
			//- aux.on_connect
		}
		if (connection_result) {
			indigo_define_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_ADD_PRESET_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_REMOVE_PRESET_PROPERTY, NULL);
			indigo_define_property(device, AUX_LIGHT_PRESETS_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_send_message(device, OK_PROPERTY, "Connected to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
		} else {
			indigo_send_message(device, ALERT_PROPERTY, "Failed to connect to %s on %s", AUX_DEVICE_NAME, DEVICE_PORT_ITEM->text.value);
			CONNECTION_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		}
	} else {
		indigo_cancel_pending_handlers(device);
		//+ aux.on_disconnect
		flatmaster_command(device, "L:%d", INTENSITY(0));
		flatmaster_command(device, "E:0");
		//- aux.on_disconnect
		// Cancelled change handlers must not leave properties BUSY: a new session starts in a clean state.
		indigo_property *cancelled_properties[] = {
			AUX_LIGHT_SWITCH_PROPERTY,
			AUX_LIGHT_INTENSITY_PROPERTY,
		};
		for (unsigned i = 0; i < sizeof(cancelled_properties) / sizeof(cancelled_properties[0]); i++) {
			if (cancelled_properties[i] != NULL && cancelled_properties[i]->state == INDIGO_BUSY_STATE) {
				cancelled_properties[i]->state = INDIGO_OK_STATE;
			}
		}
		indigo_delete_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_ADD_PRESET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_REMOVE_PRESET_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_PRESETS_PROPERTY, NULL);
		flatmaster_close(device);
		indigo_send_message(device, OK_PROPERTY, "Disconnected from %s", device->name);
		CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	}
	indigo_aux_change_property(device, NULL, CONNECTION_PROPERTY);
}

static void aux_light_deferred_switch_handler(indigo_device *device);

static void aux_light_switch_handler(indigo_device *device) {
	indigo_cancel_pending_handler(device, aux_light_deferred_switch_handler);

	AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_SWITCH.on_change
	char command[12];
	sprintf(command, "E:%c", SWITCH_VALUE(AUX_LIGHT_SWITCH_ON_ITEM));
	if (flatmaster_command(device, command)) {
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
		if (AUX_LIGHT_SWITCH_ON_ITEM->sw.value) {
			/* When switching on, restore (or set) the intensity */
			if (flatmaster_command(device, "L:%d", INTENSITY(AUX_LIGHT_INTENSITY_ITEM->number.value))) {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		} else if (AUX_LIGHT_SWITCH_OFF_ITEM->sw.value) {
			/* When switching off, set the intensity to 0 (for pre V2 firmware) */
			if (flatmaster_command(device, "L:%d", INTENSITY(0))) {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
			} else {
				AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
			}
			indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
		}
	} else {
		AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.AUX_LIGHT_SWITCH.on_change
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
}

static void aux_light_deferred_switch_handler(indigo_device *device) {
	indigo_set_switch(AUX_LIGHT_SWITCH_PROPERTY, AUX_LIGHT_SWITCH_ON_ITEM, true);
	AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_BUSY_STATE;
	indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
	aux_light_switch_handler(device);
}

static void aux_light_intensity_handler(indigo_device *device) {
	AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_INTENSITY.on_change
	/* Deselect current preset, if any */
	bool needsUpdate = false;
	for (int i = 0; i < AUX_LIGHT_PRESETS_PROPERTY->count; i++) {
		if ((AUX_LIGHT_PRESETS_PROPERTY->items+i)->sw.value) {
			indigo_set_switch(AUX_LIGHT_PRESETS_PROPERTY, AUX_LIGHT_PRESETS_PROPERTY->items+i, false);
			needsUpdate = true;
		}
	}
	if (needsUpdate) {
		indigo_update_property(device, AUX_LIGHT_PRESETS_PROPERTY, NULL);
	}
	/* Switch on if needed (it will also apply the intensity) */
	if (AUX_LIGHT_SWITCH_OFF_ITEM->sw.value) {
		// Only after a little while, can be cancelled by an actual switch change
		indigo_execute_handler_in(device, 0.1, aux_light_deferred_switch_handler);
	} else {
		/* Already switched on, update the intensity */
		if (!flatmaster_command(device, "L:%d", INTENSITY(AUX_LIGHT_INTENSITY_ITEM->number.value))) {
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_ALERT_STATE;
		}
	}
	//- aux.AUX_LIGHT_INTENSITY.on_change
	indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
}

static void aux_light_add_preset_handler(indigo_device *device) {
	AUX_LIGHT_ADD_PRESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_ADD_PRESET.on_change
	char name[INDIGO_NAME_SIZE];

	if (index_of_preset(device, AUX_LIGHT_ADD_PRESET_NAME_ITEM->text.value) < 0) {
		indigo_delete_property(device, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_PRESETS_PROPERTY, NULL);
		AUX_LIGHT_PRESETS_INTENSITY_PROPERTY = indigo_resize_property(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->count + 1);
		snprintf(name, INDIGO_NAME_SIZE, "X_FLM_PRESET_INTENSITY_%d", AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->count);
		indigo_init_number_item(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->items+AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->count-1,
										name, AUX_LIGHT_ADD_PRESET_NAME_ITEM->text.value,
										0, 100, 1, AUX_LIGHT_INTENSITY_ITEM->number.value);
		AUX_LIGHT_PRESETS_PROPERTY = indigo_resize_property(AUX_LIGHT_PRESETS_PROPERTY, AUX_LIGHT_PRESETS_PROPERTY->count + 1);
		snprintf(name, INDIGO_NAME_SIZE, "X_FLM_PRESET_%d", AUX_LIGHT_PRESETS_PROPERTY->count);
		indigo_init_switch_item(AUX_LIGHT_PRESETS_PROPERTY->items+AUX_LIGHT_PRESETS_PROPERTY->count-1, name, AUX_LIGHT_ADD_PRESET_NAME_ITEM->text.value, false);
		indigo_define_property(device, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, NULL);
		indigo_define_property(device, AUX_LIGHT_PRESETS_PROPERTY, NULL);

		AUX_LIGHT_ADD_PRESET_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_LIGHT_ADD_PRESET_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.AUX_LIGHT_ADD_PRESET.on_change
	indigo_update_property(device, AUX_LIGHT_ADD_PRESET_PROPERTY, NULL);
}

static void aux_light_presets_intensity_handler(indigo_device *device) {
	//+ aux.AUX_LIGHT_PRESETS_INTENSITY.on_change
	AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->state = INDIGO_OK_STATE;
	//- aux.AUX_LIGHT_PRESETS_INTENSITY.on_change
	indigo_update_property(device, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, NULL);
}

static void aux_light_remove_preset_handler(indigo_device *device) {
	AUX_LIGHT_REMOVE_PRESET_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_REMOVE_PRESET.on_change
	int preset_index;
	preset_index = index_of_preset(device, AUX_LIGHT_REMOVE_PRESET_NAME_ITEM->text.value);
	if (preset_index >= 0) {
		indigo_delete_property(device, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, NULL);
		indigo_delete_property(device, AUX_LIGHT_PRESETS_PROPERTY, NULL);
		// bring down all items above
		for ( int i = preset_index; i < AUX_LIGHT_PRESETS_PROPERTY->count - 1; i++) {
			indigo_init_number_item(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->items+i,
											(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->items+i+1)->name,
											(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->items+i+1)->label,
											0, 100, 1,
											(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->items+i+1)->number.value);
			indigo_init_switch_item(AUX_LIGHT_PRESETS_PROPERTY->items+i,
											(AUX_LIGHT_PRESETS_PROPERTY->items+i+1)->name,
											(AUX_LIGHT_PRESETS_PROPERTY->items+i+1)->label,
											(AUX_LIGHT_PRESETS_PROPERTY->items+i+1)->sw.value);
		}
		AUX_LIGHT_PRESETS_INTENSITY_PROPERTY = indigo_resize_property(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->count - 1);
		AUX_LIGHT_PRESETS_PROPERTY = indigo_resize_property(AUX_LIGHT_PRESETS_PROPERTY, AUX_LIGHT_PRESETS_PROPERTY->count - 1);

		indigo_define_property(device, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, NULL);
		indigo_define_property(device, AUX_LIGHT_PRESETS_PROPERTY, NULL);

		AUX_LIGHT_REMOVE_PRESET_PROPERTY->state = INDIGO_OK_STATE;
	} else {
		AUX_LIGHT_REMOVE_PRESET_PROPERTY->state = INDIGO_ALERT_STATE;
	}
	//- aux.AUX_LIGHT_REMOVE_PRESET.on_change
	indigo_update_property(device, AUX_LIGHT_REMOVE_PRESET_PROPERTY, NULL);
}

static void aux_light_presets_handler(indigo_device *device) {
	AUX_LIGHT_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
	//+ aux.AUX_LIGHT_PRESETS.on_change
	int i;
	for (i = 0; i < AUX_LIGHT_PRESETS_PROPERTY->count; i++) {
		if ((AUX_LIGHT_PRESETS_PROPERTY->items+i)->sw.value) {
			AUX_LIGHT_INTENSITY_ITEM->number.value = (AUX_LIGHT_PRESETS_INTENSITY_PROPERTY->items+i)->number.value;
			AUX_LIGHT_INTENSITY_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, AUX_LIGHT_INTENSITY_PROPERTY, NULL);
			if (AUX_LIGHT_SWITCH_OFF_ITEM->sw.value) {
				indigo_set_switch(AUX_LIGHT_SWITCH_PROPERTY, AUX_LIGHT_SWITCH_ON_ITEM, true);
				AUX_LIGHT_SWITCH_PROPERTY->state = INDIGO_BUSY_STATE;
				indigo_update_property(device, AUX_LIGHT_SWITCH_PROPERTY, NULL);
				indigo_set_timer(device, 0.0, aux_light_switch_handler, NULL);
			} else {
				indigo_set_timer(device, 0.0, aux_light_intensity_handler, NULL);
			}
			break;
		}
	}
	AUX_LIGHT_PRESETS_PROPERTY->state = INDIGO_OK_STATE;
	//- aux.AUX_LIGHT_PRESETS.on_change
	indigo_update_property(device, AUX_LIGHT_PRESETS_PROPERTY, NULL);
}

#pragma mark - Device API (aux)

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result aux_attach(indigo_device *device) {
	if (indigo_aux_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AUX_LIGHTBOX) == INDIGO_OK) {
		ADDITIONAL_INSTANCES_PROPERTY->hidden = device->base_device != NULL;
		DEVICE_PORT_PROPERTY->hidden = false;
		DEVICE_PORTS_PROPERTY->hidden = false;
		indigo_enumerate_serial_ports(device, DEVICE_PORTS_PROPERTY);
		//+ aux.on_attach
		INFO_PROPERTY->count = 6;
		INDIGO_COPY_VALUE(INFO_DEVICE_MODEL_ITEM->text.value, "Unknown");
		INDIGO_COPY_VALUE(INFO_DEVICE_FW_REVISION_ITEM->text.value, "Unknown");
		//- aux.on_attach
		AUX_LIGHT_SWITCH_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_SWITCH_PROPERTY_NAME, AUX_MAIN_GROUP, "Light (on/off)", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 2);
		if (AUX_LIGHT_SWITCH_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(AUX_LIGHT_SWITCH_ON_ITEM, AUX_LIGHT_SWITCH_ON_ITEM_NAME, "On", false);
		indigo_init_switch_item(AUX_LIGHT_SWITCH_OFF_ITEM, AUX_LIGHT_SWITCH_OFF_ITEM_NAME, "Off", true);
		AUX_LIGHT_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Light intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_INTENSITY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(AUX_LIGHT_INTENSITY_ITEM, AUX_LIGHT_INTENSITY_ITEM_NAME, "Intensity (%)", 0, 100, 1, 50);
		strcpy(AUX_LIGHT_INTENSITY_ITEM->number.format, "%g");
		AUX_LIGHT_ADD_PRESET_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_LIGHT_ADD_PRESET_PROPERTY_NAME, AUX_MAIN_GROUP, "Add a preset", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_ADD_PRESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_LIGHT_ADD_PRESET_NAME_ITEM, AUX_LIGHT_ADD_PRESET_NAME_ITEM_NAME, "", "");
		AUX_LIGHT_PRESETS_INTENSITY_PROPERTY = indigo_init_number_property(NULL, device->name, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY_NAME, AUX_MAIN_GROUP, "Presets intensity", INDIGO_OK_STATE, INDIGO_RW_PERM, 0);
		if (AUX_LIGHT_PRESETS_INTENSITY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		AUX_LIGHT_REMOVE_PRESET_PROPERTY = indigo_init_text_property(NULL, device->name, AUX_LIGHT_REMOVE_PRESET_PROPERTY_NAME, AUX_MAIN_GROUP, "Remove a preset", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (AUX_LIGHT_REMOVE_PRESET_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(AUX_LIGHT_REMOVE_PRESET_NAME_ITEM, AUX_LIGHT_REMOVE_PRESET_NAME_ITEM_NAME, "Name", "");
		AUX_LIGHT_PRESETS_PROPERTY = indigo_init_switch_property(NULL, device->name, AUX_LIGHT_PRESETS_PROPERTY_NAME, AUX_MAIN_GROUP, "Presets", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ONE_OF_MANY_RULE, 0);
		if (AUX_LIGHT_PRESETS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return aux_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result aux_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_SWITCH_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_INTENSITY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_ADD_PRESET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_REMOVE_PRESET_PROPERTY);
		INDIGO_DEFINE_MATCHING_PROPERTY(AUX_LIGHT_PRESETS_PROPERTY);
	}
	return indigo_aux_enumerate_properties(device, client, property);
}

static indigo_result aux_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (indigo_property_match_changeable(CONNECTION_PROPERTY, property)) {
		INDIGO_PROCESS_CONNECT(aux_connection_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_SWITCH_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_SWITCH_PROPERTY, aux_light_switch_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_INTENSITY_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_INTENSITY_PROPERTY, aux_light_intensity_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_ADD_PRESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(AUX_LIGHT_ADD_PRESET_PROPERTY, aux_light_add_preset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, aux_light_presets_intensity_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_REMOVE_PRESET_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_SYNC_CHANGE(AUX_LIGHT_REMOVE_PRESET_PROPERTY, aux_light_remove_preset_handler);
		return INDIGO_OK;
	} else if (indigo_property_match_changeable(AUX_LIGHT_PRESETS_PROPERTY, property)) {
		INDIGO_COPY_VALUES_PROCESS_CHANGE(AUX_LIGHT_PRESETS_PROPERTY, aux_light_presets_handler);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, AUX_LIGHT_INTENSITY_PROPERTY);
			indigo_save_property(device, NULL, AUX_LIGHT_SWITCH_PROPERTY);
			// Store presets creation in the config (it will recreate them with null intensities)
			char presetName[INDIGO_NAME_SIZE];
			INDIGO_COPY_NAME(presetName, AUX_LIGHT_ADD_PRESET_NAME_ITEM->text.value);  // Save previous value
			for (int i = 0; i < AUX_LIGHT_PRESETS_PROPERTY->count; i++) {
				INDIGO_COPY_NAME(AUX_LIGHT_ADD_PRESET_NAME_ITEM->text.value,
					              (AUX_LIGHT_PRESETS_PROPERTY->items+i)->label);
				indigo_save_property(device, NULL, AUX_LIGHT_ADD_PRESET_PROPERTY);
			}
			INDIGO_COPY_NAME(AUX_LIGHT_ADD_PRESET_NAME_ITEM->text.value, presetName);  // Restore value
			// Now that presets are ensured to be recreated, we can save their intensities
			indigo_save_property(device, NULL, AUX_LIGHT_PRESETS_INTENSITY_PROPERTY);
		} else if (indigo_switch_match(CONFIG_LOAD_ITEM, property)) {
			// Remove existing presets, if any, before loading the configuration
			indigo_resize_property(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY, 0);
			indigo_resize_property(AUX_LIGHT_PRESETS_PROPERTY, 0);
			// It is now safe to load
		}	}
	return indigo_aux_change_property(device, client, property);
}

static indigo_result aux_detach(indigo_device *device) {
	if (IS_CONNECTED) {
		indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
		aux_connection_handler(device);
	}
	indigo_release_property(AUX_LIGHT_SWITCH_PROPERTY);
	indigo_release_property(AUX_LIGHT_INTENSITY_PROPERTY);
	indigo_release_property(AUX_LIGHT_ADD_PRESET_PROPERTY);
	indigo_release_property(AUX_LIGHT_PRESETS_INTENSITY_PROPERTY);
	indigo_release_property(AUX_LIGHT_REMOVE_PRESET_PROPERTY);
	indigo_release_property(AUX_LIGHT_PRESETS_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_aux_detach(device);
}

#pragma mark - Device templates

static indigo_device aux_template = INDIGO_DEVICE_INITIALIZER(AUX_DEVICE_NAME, aux_attach, aux_enumerate_properties, aux_change_property, NULL, aux_detach);

#pragma mark - Main code

indigo_result indigo_aux_flatmaster(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	static flatmaster_private_data *private_data = NULL;
	static indigo_device *aux = NULL;

	SET_DRIVER_INFO(info, DRIVER_LABEL, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action) {
		return INDIGO_OK;
	}

	switch (action) {
		case INDIGO_DRIVER_INIT: {
			last_action = action;
			static indigo_device_match_pattern patterns[1] = { 0 };
			patterns[0].vendor_id = 0x0403;
			patterns[0].product_id = 0x6015;
			INDIGO_REGISER_MATCH_PATTERNS(aux_template, patterns, 1);
			private_data = (flatmaster_private_data *)indigo_safe_malloc(sizeof(flatmaster_private_data));
			aux = (indigo_device *)indigo_safe_malloc_copy(sizeof(indigo_device), &aux_template);
			aux->private_data = private_data;
			indigo_attach_device(aux);
			break;

		}
		case INDIGO_DRIVER_SHUTDOWN: {
			VERIFY_NOT_CONNECTED(aux);
			last_action = action;
			if (aux != NULL) {
				indigo_detach_device(aux);
				indigo_safe_free(aux);
				aux = NULL;
			}
			if (private_data != NULL) {
				indigo_safe_free(private_data);
				private_data = NULL;
			}
			break;

		}
		case INDIGO_DRIVER_INFO:
			break;
	}

	return INDIGO_OK;
}
