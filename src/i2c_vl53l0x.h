/*
 * i2c_vl53l0x.h
 *
 *  Created on: 19.09.2017
 *      Author: denis
 */

#ifndef SRC_I2C_VL53L0X_H_
#define SRC_I2C_VL53L0X_H_

#include <mgos.h>

#include "types.h"
#include "vl53l0x_types.h"

typedef struct
{
	uint32_t TimeStamp; /*!< 32-bit time stamp. */
	uint32_t MeasurementTimeUsec;
	/*!< Give the Measurement time needed by the device to do the
	 * measurement.*/

	uint16_t RangeMilliMeter; /*!< range distance in millimeter. */

	uint16_t RangeDMaxMilliMeter;
	/*!< Tells what is the maximum detection distance of the device
	 * in current setup and environment conditions (Filled when
	 *	applicable) */
	uint8_t RangeStatus;

	/*!< Range Status for the current measurement. This is device
	 *	dependent. Value = 0 means value is valid.
	 *	See \ref RangeStatusPage */
} i2c_vl53l0x_ranging_measurement_data;

typedef struct
{
	temperature_t Temperature;
	bool NeedsCalibration;
	uint32_t RefSpadCount;
	uint8_t IsApertureSpads;
	uint8_t VhvSettings;
	uint8_t PhaseCalibration;

} i2c_vl53l0x_calibration_info;

/**
 * Initializes the VL53L0X device I2C communication.
 */
bool i2c_vl53l0x_init();

/**
 * Finalizes the VL53L0X device I2C communication.
 */
void i2c_vl53l0x_final();

/**
 * Enables or disables the device to save power.
 * @param enable Indicates if the device should be enabled or disabled.
 * @param on_reset Indicates if device should be enabled after device reset.
 * @return True if the device could be enabled or disabled.
 */
bool i2c_vl53l0x_enable_device(bool enable, bool on_reset);

/**
 * Checks if the device error status has been set.
 * @return True if the device has no error state set.
 */
bool i2c_vl53l0x_check_device();

/**
 * Starts a new range measurement
 */
bool i2c_vl53l0x_get_new_range(i2c_vl53l0x_ranging_measurement_data * data);

#endif /* SRC_I2C_VL53L0X_H_ */
