/*
 * system.h
 *
 *  Created on: 23.11.2017
 *      Author: denis
 */

#ifndef SRC_SYSTEM_H_
#define SRC_SYSTEM_H_


#include "types.h"

/**
 * Returns the system temperature.
 */
temperature_t sys_get_temperature();

/**
 * Sets a new system temperature;
 */
void sys_set_temperature(temperature_t temp);

#endif /* SRC_SYSTEM_H_ */
