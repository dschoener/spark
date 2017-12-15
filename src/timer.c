/*
 * timer.c
 *
 *  Created on: 15.12.2017
 *      Author: denis
 */

#include "timer.h"

#include "communication.h"

typedef enum
{
	tt_range_measurement = 0,
	tt_alive,

	tt_count
} timer_type_t;

typedef struct
{
	mgos_timer_id id;
	int timeout_ms;
} timer_info_t;

static timer_info_t spark_timer_info[tt_count] =
{
		{MGOS_INVALID_TIMER_ID, 0},
		{MGOS_INVALID_TIMER_ID, 0}
};

static void timer_callback_range_measurement(void *param)
{
	(void)param;
	LOG(LL_DEBUG, ("timeout elapsed for range measurement"));

	com_range_measurement();
}

static void timer_callback_alive(void *param)
{
	(void)param;
	LOG(LL_DEBUG, ("timeout elapsed for alive signal"));

	com_alive();
}

bool timer_init()
{
	spark_timer_info[tt_range_measurement].timeout_ms = mgos_sys_config_get_spark_timeout_measurement();
	spark_timer_info[tt_range_measurement].id = mgos_set_timer(
			spark_timer_info[tt_range_measurement].timeout_ms,
			MGOS_TIMER_REPEAT, timer_callback_range_measurement, NULL);
	LOG(LL_DEBUG, ("Timer [%d] for range measurement set (%d ms)",
			spark_timer_info[tt_range_measurement].timeout_ms,
			spark_timer_info[tt_range_measurement].id));

	spark_timer_info[tt_alive].timeout_ms = mgos_sys_config_get_spark_timeout_max_alive();
	spark_timer_info[tt_alive].id = mgos_set_timer(
			spark_timer_info[tt_alive].timeout_ms,
			MGOS_TIMER_REPEAT, timer_callback_alive, NULL);
	LOG(LL_DEBUG, ("Timer [%d] for alive signal set (%d ms)",
			spark_timer_info[tt_alive].timeout_ms,
			spark_timer_info[tt_alive].id));

	return true;
}

