/*
 * communication.h
 *
 *  Created on: 03.12.2017
 *      Author: denis
 */

#ifndef SRC_COMMUNICATION_H_
#define SRC_COMMUNICATION_H_

#include "i2c_vl53l0x.h"

/**
 * Initializes this module
 */
bool com_init();

/**
 * Triggers a new range measurement and result mesage.
 */
void com_range_measurement();

/**
 * Triggers an alive message if none has been sent since last timeout.
 */
void com_alive();

#endif /* SRC_COMMUNICATION_H_ */
