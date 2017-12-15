#include <mgos.h>
#include <mgos_i2c.h>
#include <mgos_mqtt.h>

#include "i2c_vl53l0x.h"
#include "i2c_tmp102.h"

#include "system.h"
#include "communication.h"
#include "timer.h"

#define LOG_INIT_STATE(SUCCESS, WHAT) if (SUCCESS) { \
		LOG(LL_INFO, ("%s: successfully initialized", WHAT)); \
	} else { \
		LOG(LL_ERROR, ("%s: failed to initialize", WHAT)); \
	}

enum mgos_app_init_result mgos_app_init(void)
{
	LOG(LL_DEBUG, ("init spark app"));

	bool success = (MGOS_INIT_OK == mgos_gpio_init());
	LOG_INIT_STATE(success, "GPIO interface");

	if (success)
	{
		success = (MGOS_INIT_OK == mgos_timers_init());
		LOG_INIT_STATE(success, "System timers");
	}

	if (success)
	{
		success = mgos_i2c_init();
		LOG_INIT_STATE(success, "I2C interface");
	}

	if (success)
	{
		success = mgos_mqtt_init();
		LOG_INIT_STATE(success, "MQTT interface");
	}

//	if (success)
//	{
//		success = mgos_wifi_init();
//		LOG_INIT_STATE(success, "Wifi interface");
//	}

	if (success)
	{
		success = i2c_tmp102_init();
		LOG_INIT_STATE(success, "Temperature sensor");
	}

	if (success)
	{
		success = i2c_vl53l0x_init();
		LOG_INIT_STATE(success, "Range sensor");
	}

	if (success)
	{
		success = com_init();
		LOG_INIT_STATE(success, "Communication");
	}

	if (success)
	{
		success = timer_init();
		LOG_INIT_STATE(success, "App timers");
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
