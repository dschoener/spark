/*
 * system.c
 *
 *  Created on: 23.11.2017
 *      Author: denis
 */

#include "system.h"

#include <mgos.h>
#include <math.h>

static temperature_t current_temperature = NAN;
static temperature_t previous_temperature = NAN;

temperature_t sys_get_temperature()
{
	return current_temperature;
}

/**
 * Sets a new system temperature;
 */
void sys_set_temperature(temperature_t temp)
{
	LOG(LL_DEBUG, ("new data temperature: %2.1fC", temp));
	previous_temperature = current_temperature;
	current_temperature = temp;
}
