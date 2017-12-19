/*
 * communication.c
 *
 *  Created on: 03.12.2017
 *      Author: denis
 */

#include "communication.h"

#include "system.h"
#include "i2c_tmp102.h"
#include "i2c_vl53l0x.h"

#include <common/cs_time.h>
#include <mgos_mqtt.h>

static i2c_vl53l0x_range_measurement_data s_current_range_measurement_data;
static bool s_message_sent = false;

static uint16_t diff(uint16_t a, uint16_t b)
{
	return (a > b) ? (a - b) : (b - a);
}

bool com_init()
{
	s_message_sent = false;
	memset(&s_current_range_measurement_data, 0, sizeof(s_current_range_measurement_data));
	return true;
}

static size_t com_create_message(char * buf, size_t size)
{
	const double now = cs_time();
	const size_t written = snprintf(buf, size, "{ "
			"\"time\": %06.3f,"
			"\"device-id\": \"%s\","
			"\"measurement\": {"
			"\"temperature\": %2.1f,"
			"\"range\": %d,"
			"\"max-range\": %d,"
			"\"range-status\": %d"
			"}}", now,
			mgos_sys_config_get_device_id(),
			sys_get_temperature(),
			s_current_range_measurement_data.range_mm,
			s_current_range_measurement_data.max_range_mm,
			s_current_range_measurement_data.status
			);
	return written;
}

static bool com_send_last_data()
{
	char message[1048];

	const size_t len = com_create_message(message, sizeof(message));
	bool success = (len < sizeof(message));

	if (success)
	{
		const char* topic = mgos_sys_config_get_spark_mqtt_topic();
		assert(topic != NULL);
		LOG(LL_DEBUG, ("Sending new range data: %s:%s", topic, message));
		success = mgos_mqtt_pub(topic, message, len, 0, false);
	}


	if (!success)
	{
		LOG(LL_ERROR, ("Failed to send range data"));
	}
	return success;
}

static void com_process_new_data(i2c_vl53l0x_range_measurement_data * range_data)
{
	const uint16_t distance_diff = diff(s_current_range_measurement_data.range_mm,
			range_data->range_mm);

	// if data has been changed
	if (distance_diff >= (uint16_t)mgos_sys_config_get_spark_threshold_dist())
	{
		s_current_range_measurement_data = *range_data;
		s_message_sent = com_send_last_data();
	}
}

void com_range_measurement()
{
	sys_temperature temp;
	bool success = i2c_tmp102_get_temperature(&temp);

	if (success)
	{
		sys_set_temperature(temp);

		i2c_vl53l0x_range_measurement_data data;
		bool success = i2c_vl53l0x_get_new_range(&data);

		if (success)
		{
			com_process_new_data(&data);
		}
		else
		{
			LOG(LL_ERROR, ("Failed to get new range value"));
		}
	}
	else
	{
		LOG(LL_ERROR, ("Failed to get new temperature"));
	}
}

void com_alive()
{
	if (!s_message_sent)
	{
		const bool success = com_send_last_data();
		if (success)
		{
			s_message_sent = false;
		}
		else
		{
			LOG(LL_ERROR, ("Failed to send alive signal"));
		}
	}
}
