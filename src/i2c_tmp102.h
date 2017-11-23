/*
 * i2c_tmp102.h
 *
 *  Created on: 24.10.2017
 *      Author: denis
 */

#ifndef SRC_I2C_TMP102_H_
#define SRC_I2C_TMP102_H_

#include "i2c_tmp102.h"
#include "types.h"

#include <mgos.h>

/**
 * Initializes the TMP102 device I2C communication.
 */
bool i2c_tmp102_init();

/**
 * Finalizes the TMP102 device I2C communication.
 */
void i2c_tmp102_final();

/**
 * Returns a new temperature value.
 */
bool i2c_tmp102_get_temperature(temperature_t * temp);

#endif /* SRC_I2C_TMP102_H_ */
