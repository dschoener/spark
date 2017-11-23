/*
 * i2c_vl53l0x.c
 *
 *  Created on: 19.09.2017
 *      Author: denis
 */

#include "i2c_vl53l0x.h"
#include "system.h"

#include <mgos_i2c.h>
#include <vl53l0x_platform.h>
#include <vl53l0x_i2c_platform.h>
#include <vl53l0x_api.h>
#include <assert.h>

const uint8_t I2C_VL53L0X_DEVICE_ADDR = 0x29;
const uint8_t I2C_VL53L0X_DEVICE_ADDR_INVALID = 0;

static VL53L0X_Dev_t device_config;
static VL53L0X_DeviceInfo_t device_info;
static i2c_vl53l0x_calibration_info device_calibration =
{ true, 0, 0, true, 0, 0 };

#define LOG_ON_ERR(STR, ERR) if ((ERR) != VL53L0X_ERROR_NONE){ \
	char str[64]; \
	if (VL53L0X_get_pal_error_string((ERR), str) == VL53L0X_ERROR_NONE) { \
		LOG(LL_ERROR, (STR " (%s)", str)); \
	}}

/// Fixpoint 16.16 factor
static const float FIXPOINT1616_FACT = ((float)(1 << 16));

/**
 * Converts a FixPoint1616_t value to a float value.
 * @param fp1616 FixPoint1616_t value to be converted.
 * @return According float value.
 */
inline static float fixpoint1616_to_float(FixPoint1616_t fp1616)
{
	return (float)(fp1616 / FIXPOINT1616_FACT);
}

/**
 * Converts a float value to a FixPoint1616_t value.
 * @param value Float value to be converted.
 * @return According FixPoint1616_t value.
 */
inline static FixPoint1616_t float_to_fixpoint1616(float value)
{
	return (FixPoint1616_t)(value * FIXPOINT1616_FACT);
}

//static void i2c_vl53l0x_interrupt_handler(int pin, void *arg)
//{
//	(void) arg;
//	(void) pin;
//}

static bool i2c_vl53l0x_is_device_initialized()
{
	return (device_config.I2cDevAddr != I2C_VL53L0X_DEVICE_ADDR_INVALID);
}

bool i2c_vl53l0x_check_device()
{
	VL53L0X_DeviceError device_err = VL53L0X_DEVICEERROR_NONE;
	VL53L0X_Error rv = VL53L0X_GetDeviceErrorStatus(&device_config,
			&device_err);

	LOG_ON_ERR("Failed to get device error status", rv);
	bool success = (rv == VL53L0X_ERROR_NONE)
			&& (device_err == VL53L0X_DEVICEERROR_NONE);

	if (device_err != VL53L0X_DEVICEERROR_NONE)
	{
		char str_err[512];
		str_err[0] = '\0';
		rv = VL53L0X_GetDeviceErrorString(device_err, (char*) &str_err);
		LOG_ON_ERR("Failed to get device error string", rv);
		LOG(LL_ERROR, ("Device error (%d): %s", device_err, str_err));
	}
	else
	{
		LOG(LL_DEBUG, ("Device seems to run successfully"));
	}

	return success;
}

static bool i2c_vl53l0x_read(uint8_t reg, uint8_t * data, size_t len)
{
	struct mgos_i2c * i2c = mgos_i2c_get_global();
	assert(i2c != NULL);

	return mgos_i2c_write(i2c, I2C_VL53L0X_DEVICE_ADDR, &reg, sizeof(reg), true)
			&& mgos_i2c_read(i2c, I2C_VL53L0X_DEVICE_ADDR, data, len, true);
}

static bool i2c_vl53l0x_validate_test_register(uint8_t reg, uint16_t value,
		size_t len)
{
	assert(len <= 2);
	uint16_t readValue = 0x0000;
	bool success = i2c_vl53l0x_read(reg, (uint8_t*) &readValue, len);
	if (success)
	{
		if (len == 2)
		{
			// MSB will be sent first!
			readValue = ((readValue >> 8) & 0xff) | ((readValue & 0xff) << 8);
		}
		success = (readValue == value);
		if (!success)
		{
			LOG(LL_ERROR,
					("Register validation failed: %x != %x", readValue, value));
		}
	}
	return success;
}

static bool i2c_vl53l0x_validate_device()
{
	return i2c_vl53l0x_validate_test_register(0xC0, 0xEE, 1)
			&& i2c_vl53l0x_validate_test_register(0xC1, 0xAA, 1)
			&& i2c_vl53l0x_validate_test_register(0xC2, 0x10, 1)
			&& i2c_vl53l0x_validate_test_register(0x51, 0x0099, 2)
			&& i2c_vl53l0x_validate_test_register(0x61, 0x0000, 2);
}

bool i2c_vl53l0x_enable_device(bool enable, bool on_reset)
{
	const int gpio_xshut = mgos_sys_config_get_spark_gpio_vl53l0x_xshut();
	const bool enabled = mgos_gpio_read(gpio_xshut);
	bool success = true;

	LOG(LL_DEBUG, ("GPIO XSHUT: %s", (enabled) ? "enabled" : "disabled"));

	if (enabled != enable)
	{
		mgos_gpio_write(gpio_xshut, enable);

		if (enable)
		{
			bool device_active = false;

			for (int i = 5; (i > 0) && (!device_active); i--)
			{
				mgos_usleep(5000);
				device_active = i2c_vl53l0x_validate_device();
			}

			if (!device_active)
			{
				success = false;
				LOG(LL_ERROR, ("Device seems to be not active"));
			}

			if (success && on_reset)
			{
				LOG(LL_DEBUG, ("Data initialization"));
				const VL53L0X_Error rv = VL53L0X_DataInit(&device_config);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to init device data", rv);
			}

			if (success)
			{
				LOG(LL_DEBUG, ("Static initialization"));
				const VL53L0X_Error rv = VL53L0X_StaticInit(
						&device_config);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to static init device", rv);
			}

			if (success && device_calibration.NeedsCalibration)
			{
				LOG(LL_DEBUG, ("Performing SPAD calibration"));
				const VL53L0X_Error rv = VL53L0X_PerformRefSpadManagement(
						&device_config, &device_calibration.RefSpadCount,
						&device_calibration.IsApertureSpads);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to perform SPAD calibration", rv);
				if (success)
				{
					LOG(LL_DEBUG, ("SPAD calibration: SpadCount=%d, IsApertureSpads=%d",
							device_calibration.RefSpadCount, device_calibration.IsApertureSpads));
				}
			}

			if (success && device_calibration.NeedsCalibration)
			{
				LOG(LL_DEBUG, ("Performing refernce calibration"));
				const VL53L0X_Error rv = VL53L0X_PerformRefCalibration(
						&device_config, &device_calibration.VhvSettings,
						&device_calibration.PhaseCalibration);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to perform reference calibration", rv);
				if (success)
				{
					LOG(LL_DEBUG, ("Reference calibration: VhvSets=%d, PhaseCalibration=%d",
							device_calibration.VhvSettings, device_calibration.PhaseCalibration));
					device_calibration.Temperature = sys_get_temperature();
				}
			}

			if (success)
			{
				LOG(LL_DEBUG, ("Set power mode"));
				// set power mode
				const VL53L0X_Error rv = VL53L0X_SetPowerMode(
						&device_config,
						VL53L0X_POWERMODE_STANDBY_LEVEL1);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to set device power mode", rv);
			}

			if (success)
			{
				LOG(LL_DEBUG, ("Set device mode"));
				const VL53L0X_Error rv = VL53L0X_SetDeviceMode(
						&device_config,
						VL53L0X_DEVICEMODE_SINGLE_RANGING);
				success = (rv == VL53L0X_ERROR_NONE);
				LOG_ON_ERR("Failed to set device mode", rv);
			}

//			if (success)
//			{
//				LOG(LL_DEBUG, ("Set power mode"));
//				const VL53L0X_Error rv = VL53L0X_SetLimitCheckEnable(&device_config,
//						VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
//				success = (rv == VL53L0X_ERROR_NONE);
//				LOG_ON_ERR("Failed to enable sigma limit check", rv);
//			}
//
//			if (success)
//			{
//				LOG(LL_DEBUG, ("Enable sigma limit check value"));
//				const VL53L0X_Error rv = VL53L0X_SetLimitCheckValue(&device_config,
//						VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, float_to_fixpoint1616(2000));
//				success = (rv == VL53L0X_ERROR_NONE);
//				LOG_ON_ERR("Failed to enable sigma limit check value", rv);
//			}
//
//			if (success)
//			{
//				LOG(LL_DEBUG, ("Enable signal rate limit check"));
//				const VL53L0X_Error rv = VL53L0X_SetLimitCheckEnable(&device_config,
//						VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
//				success = (rv == VL53L0X_ERROR_NONE);
//				LOG_ON_ERR("Failed to enable signal rate limit check", rv);
//			}
//
//			if (success)
//			{
//				LOG(LL_DEBUG, ("Enable range ignore threshold limit check"));
//				const VL53L0X_Error rv = VL53L0X_SetLimitCheckEnable(&device_config,
//						VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1);
//				success = (rv == VL53L0X_ERROR_NONE);
//				LOG_ON_ERR("Failed to enable range ignore threshold limit check", rv);
//			}

			if (success)
			{
				// check current error state
				LOG(LL_DEBUG, ("Checking device state"));
				(void)i2c_vl53l0x_check_device();
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

	if (!success)
	{
		LOG(LL_ERROR, ("I2C interface not ready"));
	}

	if (success)
	{
		memset(&device_config, 0, sizeof(device_config));
		device_config.I2cDevAddr = I2C_VL53L0X_DEVICE_ADDR;
		device_config.comms_type = I2C;
		device_config.comms_speed_khz = mgos_i2c_get_freq(i2c) / 1000; // Hz -> kHz

		LOG(LL_DEBUG,
				("I2C frequency is set to %d kHz", device_config.comms_speed_khz));

		// configure XSHUT pin.
		const int gpio_xshut = mgos_sys_config_get_spark_gpio_vl53l0x_xshut();
		success = mgos_gpio_set_mode(gpio_xshut, MGOS_GPIO_MODE_OUTPUT);
		mgos_gpio_write(gpio_xshut, false);
	}

//	if (success)
//	{
//		// add interrupt handler for device signalling
//		const int gpio_int = mgos_sys_config_get_spark_gpio_vl53l0x_int();
//		success = mgos_gpio_set_mode(gpio_int, MGOS_GPIO_MODE_INPUT) &&
//				mgos_gpio_set_pull(gpio_int, MGOS_GPIO_PULL_DOWN) &&
//				mgos_gpio_set_int_handler(gpio_int, MGOS_GPIO_INT_EDGE_POS,	i2c_vl53l0x_interrupt_handler, NULL) &&
//				mgos_gpio_enable_int(gpio_int);
//
//		if (!success)
//		{
//			LOG(LL_ERROR, ("Failed to init GPIO Interrupt"));
//		}
//	}

	if (success)
	{
		success = i2c_vl53l0x_enable_device(true, true);
	}

	if (success)
	{
		// get device information
		const VL53L0X_Error rv = VL53L0X_GetDeviceInfo(&device_config,
				&device_info);
		success = (rv == VL53L0X_ERROR_NONE);
		LOG_ON_ERR("Failed to get device info", rv);
	}

	//	if (success)
//	{
//		// check current error state
//		LOG(LL_DEBUG, ("Checking device state"));
//		success = i2c_vl53l0x_check_device();
//	}

	if (success)
	{
		LOG(LL_INFO,
				("Device initialized (%s/%s/%s)", device_info.Name, device_info.Type, device_info.ProductId));
		// Set device to sleep again!
//		success = i2c_vl53l0x_enable_device(false, true);
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
//		success = i2c_vl53l0x_enable_device(true, false);
	}

	VL53L0X_RangingMeasurementData_t temp_data;
	if (success)
	{
		VL53L0X_Error rv = VL53L0X_PerformSingleRangingMeasurement(
				&device_config, &temp_data);
		success = (rv == VL53L0X_ERROR_NONE);
		LOG_ON_ERR("Failed to perform a single measurement", rv);
	}

	if (success)
	{
		LOG(LL_DEBUG, ("Single range measurement: MeasurementTimeUsec=%d, "
				"RangeDMaxMilliMeter=%d, RangeMilliMeter=%d, RangeStatus=%d, TimeStamp=%d",
				temp_data.MeasurementTimeUsec, temp_data.RangeDMaxMilliMeter,
				temp_data.RangeMilliMeter, temp_data.RangeStatus,
				temp_data.TimeStamp));

		data->MeasurementTimeUsec = temp_data.MeasurementTimeUsec;
		data->RangeDMaxMilliMeter = temp_data.RangeDMaxMilliMeter;
		data->RangeMilliMeter = temp_data.RangeMilliMeter;
		data->RangeStatus = temp_data.RangeStatus;
		data->TimeStamp = temp_data.TimeStamp;
	}

//	success = (success && i2c_vl53l0x_enable_device(false, false));

	return success;
}

void i2c_vl53l0x_final()
{
	device_config.I2cDevAddr = I2C_VL53L0X_DEVICE_ADDR_INVALID;
	device_config.comms_type = 0;
	device_config.comms_speed_khz = 0;
	memset(&device_config, 0, sizeof(device_config));
//	i2c_vl53l0x_enable_device(false);
//	// disable interrupt handler
//	const int gpio_int = mgos_sys_config_get_spark_gpio_vl53l0x_int();
//	mgos_gpio_enable_int(gpio_int);
}

