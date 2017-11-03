/*
 * i2c_vl53l0x.c
 *
 *  Created on: 19.09.2017
 *      Author: denis
 */

#include "i2c_vl53l0x.h"

#include <mgos_i2c.h>
#include <vl53l0x_platform.h>
#include <vl53l0x_i2c_platform.h>
#include <vl53l0x_api.h>
#include <assert.h>

const uint8_t I2C_VL53L0X_DEVICE_ADDR = 0x52;
const uint8_t I2C_VL53L0X_DEVICE_ADDR_INVALID = 0;

static VL53L0X_Dev_t device_comm_params;
static VL53L0X_DeviceInfo_t device_info;

#define LOG_ON_ERR(STR, ERR) if ((ERR) != VL53L0X_ERROR_NONE){ \
	char str[64]; \
	if (VL53L0X_get_pal_error_string((ERR), str) == VL53L0X_ERROR_NONE) { \
		LOG(LL_ERROR, (STR " (%s)", str)); \
	}}

static void i2c_vl53l0x_interrupt_handler(int pin, void *arg)
{
	(void) arg;
	(void) pin;
}

static bool i2c_vl53l0x_is_device_initialized()
{
	return (device_comm_params.I2cDevAddr != I2C_VL53L0X_DEVICE_ADDR_INVALID);
}

bool i2c_vl53l0x_check_device()
{
	bool success = false;
	VL53L0X_DeviceError device_err = VL53L0X_DEVICEERROR_NONE;
	VL53L0X_Error rv = VL53L0X_GetDeviceErrorStatus(&device_comm_params,
			&device_err);

	if (rv == VL53L0X_ERROR_NONE)
	{
		if (device_err == VL53L0X_DEVICEERROR_NONE)
		{
			success = true;
		}
		else
		{
			char str_err[512];
			str_err[0] = '\0';
			rv = VL53L0X_GetDeviceErrorString(device_err, (char*) &str_err);
			LOG_ON_ERR("Failed to get device error string", rv);
		}
	}
	return success;
}

bool i2c_vl53l0x_enable_device(bool enable)
{
	const int gpio_xshut = get_cfg()->spark.gpio_vl53l0x_xshut;
	const bool enabled = mgos_gpio_read(gpio_xshut);
	bool success = true;

	if (enabled != enable)
	{
		mgos_gpio_write(gpio_xshut, enable);

		if (enable)
		{
			// Wait until device has booted
			if (success)
			{
				const VL53L0X_Error rv = VL53L0X_WaitDeviceBooted(
						&device_comm_params);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to wait for device boot", rv);
			}

			if (success)
			{
				const VL53L0X_Error rv = VL53L0X_DataInit(&device_comm_params);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to init device data", rv);
			}

			if (success)
			{
				const VL53L0X_Error rv = VL53L0X_StaticInit(
						&device_comm_params);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to static init device", rv);
			}

			if (success)
			{
				// set power mode
				const VL53L0X_Error rv = VL53L0X_SetPowerMode(
						&device_comm_params,
						VL53L0X_POWERMODE_STANDBY_LEVEL1);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to set device power mode", rv);
			}

			if (success)
			{
				const VL53L0X_Error rv = VL53L0X_SetDeviceMode(
						&device_comm_params,
						VL53L0X_DEVICEMODE_SINGLE_RANGING);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to set device mode", rv);
			}
		}
	}
	else
	{
		LOG(LL_WARN, ("Device already %s", (enable ? "enabled" : "disabled")));
	}
	return success;
}

bool i2c_vl53l0x_init()
{
	struct mgos_i2c * i2c = mgos_i2c_get_global();

	bool success = (i2c != NULL);

	if (success)
	{
		memset(&device_comm_params, 0, sizeof(device_comm_params));
		device_comm_params.I2cDevAddr = I2C_VL53L0X_DEVICE_ADDR;
		device_comm_params.comms_type = I2C;
		device_comm_params.comms_speed_khz = mgos_i2c_get_freq(i2c) / 1000; // Hz -> kHz

		// configure XSHUT pin.
		const int gpio_xshut = get_cfg()->spark.gpio_vl53l0x_xshut;
		success = mgos_gpio_set_mode(gpio_xshut, MGOS_GPIO_MODE_OUTPUT)
				&& mgos_gpio_set_pull(gpio_xshut, MGOS_GPIO_PULL_DOWN);
	}

	if (success)
	{
		// add interrupt handler for device signalling
		const int gpio_int = get_cfg()->spark.gpio_vl53l0x_int;
		success = mgos_gpio_set_mode(gpio_int, MGOS_GPIO_MODE_INPUT) &&
				mgos_gpio_set_pull(gpio_int, MGOS_GPIO_PULL_DOWN) &&
				mgos_gpio_set_int_handler(gpio_int, MGOS_GPIO_INT_EDGE_POS,	i2c_vl53l0x_interrupt_handler, NULL) &&
				mgos_gpio_enable_int(gpio_int);
	}

	if (success)
	{
		success = i2c_vl53l0x_enable_device(true);
	}

	if (success)
	{
		// get device information
		const VL53L0X_Error rv = VL53L0X_GetDeviceInfo(&device_comm_params,
				&device_info);
		success = (rv == VL53L0X_ERROR_NONE);
		LOG_ON_ERR("Failed to get device info", rv);
	}

	if (success)
	{
		// check current error state
		success = i2c_vl53l0x_check_device();
	}

	if (success)
	{
		LOG(LL_INFO, ("Device initialized (%s/%s/%s)", device_info.Name,
				device_info.Type, device_info.ProductId));
		// Set device to sleep again!
		success = i2c_vl53l0x_enable_device(false);
	}
	else
	{
		LOG(LL_ERROR, ("Failed to initialize I2C interface"));
		i2c_vl53l0x_final();
	}

	return success;
}

bool i2c_vl53l0x_get_new_range(i2c_vl53l0x_ranging_measurement_data * data)
{
	bool success = i2c_vl53l0x_is_device_initialized();

	if (success)
	{
		// turn on device
		success = i2c_vl53l0x_enable_device(true);
	}

	if (success)
	{
		VL53L0X_Error rv = VL53L0X_PerformSingleMeasurement(
				&device_comm_params);
		success = (rv == VL53L0X_ERROR_NONE);
		LOG_ON_ERR("Failed to perform a single measurement", rv);
	}

	VL53L0X_RangingMeasurementData_t temp_data;
	if (success)
	{
		VL53L0X_Error rv = VL53L0X_GetRangingMeasurementData(
				&device_comm_params, &temp_data);
		success = (rv == VL53L0X_ERROR_NONE);
		LOG_ON_ERR("Failed to get ranging measurement data", rv);
	}

	if (success)
	{
		data->MeasurementTimeUsec = temp_data.MeasurementTimeUsec;
		data->RangeDMaxMilliMeter = temp_data.RangeDMaxMilliMeter;
		data->RangeMilliMeter = temp_data.RangeMilliMeter;
		data->RangeStatus = temp_data.RangeStatus;
		data->TimeStamp = temp_data.TimeStamp;
	}

	success = (success && i2c_vl53l0x_enable_device(false));

	return success;
}

void i2c_vl53l0x_final()
{
	device_comm_params.I2cDevAddr = I2C_VL53L0X_DEVICE_ADDR_INVALID;
	device_comm_params.comms_type = 0;
	device_comm_params.comms_speed_khz = 0;
	memset(&device_comm_params, 0, sizeof(device_comm_params));
	i2c_vl53l0x_enable_device(false);
	// disable interrupt handler
	const int gpio_int = get_cfg()->spark.gpio_vl53l0x_int;
	mgos_gpio_enable_int(gpio_int);
}

