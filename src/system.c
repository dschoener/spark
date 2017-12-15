/*
 * system.c
 *
 *  Created on: 23.11.2017
 *      Author: denis
 */

#include "system.h"

#include <mgos.h>
#include <math.h>

static sys_temperature current_temperature = NAN;
static sys_temperature previous_temperature = NAN;

sys_temperature sys_get_temperature()
{
	return current_temperature;
}

/**
 * Sets a new system temperature;
 */
void sys_set_temperature(sys_temperature temp)
{
	LOG(LL_DEBUG, ("new data temperature: %2.1fC", temp));
	previous_temperature = current_temperature;
	current_temperature = temp;
}
