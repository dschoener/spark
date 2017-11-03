/*
 * i2c_vl53l0x.h
 *
 *  Created on: 19.09.2017
 *      Author: denis
 */

#ifndef SRC_I2C_VL53L0X_H_
#define SRC_I2C_VL53L0X_H_

#include <mgos.h>

typedef struct {
	uint32_t TimeStamp;		/*!< 32-bit time stamp. */
	uint32_t MeasurementTimeUsec;
		/*!< Give the Measurement time needed by the device to do the
		 * measurement.*/


	uint16_t RangeMilliMeter;	/*!< range distance in millimeter. */

	uint16_t RangeDMaxMilliMeter;
		/*!< Tells what is the maximum detection distance of the device
		 * in current setup and environment conditions (Filled when
		 *	applicable) */

	uint8_t RangeStatus;
		/*!< Range Status for the current measurement. This is device
		 *	dependent. Value = 0 means value is valid.
		 *	See \ref RangeStatusPage */
} i2c_vl53l0x_ranging_measurement_data;

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
 * @return True if the device could be enabled or disabled.
 */
bool i2c_vl53l0x_enable_device(bool enable);

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
