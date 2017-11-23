#include <mgos.h>
#include <mgos_i2c.h>

#include "i2c_vl53l0x.h"
#include "i2c_tmp102.h"
#include "system.h"

#define LOG_INIT_STATE(SUCCESS, WHAT) if (SUCCESS) { \
		LOG(LL_INFO, ("%s: successfully initialized", WHAT)); \
	} else { \
		LOG(LL_ERROR, ("%s: failed to initialize", WHAT)); \
	}

enum spark_sensor
{
	sensor_temperature = 0,
	sensor_distance,

	sensor_count
};

struct spark_timer_info_t
{
	mgos_timer_id id;
	int timeout_ms;
};

struct spark_measurement_info_t
{
	i2c_vl53l0x_ranging_measurement_data cur_distance;
	i2c_vl53l0x_ranging_measurement_data prev_distance;
};

static struct spark_measurement_info_t spark_measurement_info;

static struct spark_timer_info_t spark_timer_info[sensor_count] =
{
		{MGOS_INVALID_TIMER_ID, 0},
		{MGOS_INVALID_TIMER_ID, 0}
};

static void spark_process_data();

static void cb_timeout_temperature(void *param)
{
	(void)param;
	LOG(LL_DEBUG, ("timeout elapsed for temperature measurement"));

	temperature_t temp;
	bool success = i2c_tmp102_get_temperature(&temp);

	if (success)
	{
		sys_set_temperature(temp);
	}
}

static void cb_timeout_distance(void *param)
{
	(void)param;
	LOG(LL_DEBUG, ("timeout elapsed for distance measurement"));

	i2c_vl53l0x_ranging_measurement_data data;
	bool success = i2c_vl53l0x_get_new_range(&data);

	if (success)
	{
		spark_measurement_info.cur_distance = data;
		LOG(LL_DEBUG, ("new distance: %d (%x)", data.RangeMilliMeter, data.RangeStatus));
		spark_process_data();
	}
}

void spark_process_data()
{
	const uint16_t distance_delta =
			(spark_measurement_info.cur_distance.RangeMilliMeter > spark_measurement_info.prev_distance.RangeMilliMeter) ?
			(spark_measurement_info.cur_distance.RangeMilliMeter - spark_measurement_info.prev_distance.RangeMilliMeter) :
			(spark_measurement_info.prev_distance.RangeMilliMeter - spark_measurement_info.cur_distance.RangeMilliMeter);
	// if data has been changed
	if (distance_delta >= (uint16_t)mgos_sys_config_get_spark_threshold_dist())
	{
		spark_measurement_info.prev_distance.RangeMilliMeter = spark_measurement_info.cur_distance.RangeMilliMeter;
		LOG(LL_DEBUG, ("new distance data to send: %d mm", spark_measurement_info.cur_distance.RangeMilliMeter));
	}
}

bool spark_timers_init()
{
	spark_timer_info[sensor_temperature].timeout_ms = mgos_sys_config_get_spark_timeout_temp();
	spark_timer_info[sensor_temperature].id = mgos_set_timer(
			spark_timer_info[sensor_temperature].timeout_ms,
			MGOS_TIMER_REPEAT, cb_timeout_temperature, NULL);
	LOG(LL_DEBUG, ("Timer [%d] for temperature measurement set (%d ms)",
			spark_timer_info[sensor_temperature].timeout_ms,
			spark_timer_info[sensor_temperature].id));

	spark_timer_info[sensor_distance].timeout_ms = mgos_sys_config_get_spark_timeout_dist();
	spark_timer_info[sensor_distance].id = mgos_set_timer(
			spark_timer_info[sensor_distance].timeout_ms,
			MGOS_TIMER_REPEAT, cb_timeout_distance, NULL);
	LOG(LL_DEBUG, ("Timer [%d] for distance measurement set (%d ms)",
				spark_timer_info[sensor_distance].timeout_ms,
				spark_timer_info[sensor_distance].id));
	return true;
}

enum mgos_app_init_result mgos_app_init(void)
{
	LOG(LL_DEBUG, ("starting spark app"));

	memset(&spark_measurement_info, 0, sizeof(spark_measurement_info));

	bool success = (MGOS_INIT_OK == mgos_gpio_init());
	LOG_INIT_STATE(success, "GPIO interface");

	if (success)
	{
		success = (MGOS_INIT_OK == mgos_timers_init());
		LOG_INIT_STATE(success, "system timers");
	}

	if (success)
	{
		success = mgos_i2c_init();
		LOG_INIT_STATE(success, "I2C interface");
	}

	if (success)
	{
		success = i2c_tmp102_init();
		LOG_INIT_STATE(success, "temperature sensor");
	}

	if (success)
	{
		temperature_t temp;
		success = i2c_tmp102_get_temperature(&temp);
		LOG_INIT_STATE(success, "request temperature");
		if (success)
		{
			sys_set_temperature(temp);
		}
	}
	if (success)
	{
		success = i2c_vl53l0x_init();
		LOG_INIT_STATE(success, "distance sensor");
	}

	if (success)
	{
		success = spark_timers_init();
		LOG_INIT_STATE(success, "app timers");
	}

	if (success)
	{
		LOG(LL_INFO, ("System started"));
		return MGOS_APP_INIT_SUCCESS;
	}
	else
	{
		LOG(LL_ERROR, ("Failed to start app"));
		mgos_msleep(10);
		return MGOS_APP_INIT_ERROR;
	}
}
