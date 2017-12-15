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
sys_temperature sys_get_temperature();

/**
 * Sets a new system temperature;
 */
void sys_set_temperature(sys_temperature temp);

/**
 * Returns the product id
 */
const char* sys_get_product_id();

#endif /* SRC_SYSTEM_H_ */
