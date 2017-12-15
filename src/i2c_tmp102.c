/*
 * i2c_tmp102.c
 *
 *  Created on: 24.10.2017
 *      Author: denis
 */

#include "i2c_tmp102.h"

#include <mgos.h>
#include <mgos_i2c.h>
#include <assert.h>

const uint8_t I2C_TMP102_DEVICE_ADDR = 0x48;

// Pointer register
const uint8_t I2C_TMP102_PR_TEMPERATURE = 0X00;
const uint8_t I2C_TMP102_PR_CONFIGURATION = 0X01;
const uint8_t I2C_TMP102_PR_TLOW = 0X02;
const uint8_t I2C_TMP102_PR_THIGH = 0X03;

// Configuration register
const uint8_t I2C_TMP102_CR_SHUTDOWN_DISABLE = 0X00;
const uint8_t I2C_TMP102_CR_SHUTDOWN_ENABLE = 0X01;

const uint8_t I2C_TMP102_CR_THERMOMODE_COMPARE = 0X00;
const uint8_t I2C_TMP102_CR_THERMOMODE_INTERRUPT = 0X01;

const uint8_t I2C_TMP102_CR_POLARITY_ACTIVELOW = 0X00;
const uint8_t I2C_TMP102_CR_POLARITY_ACTIVEHIGH = 0X01;

const uint8_t I2C_TMP102_CR_FAULTQUEUE_FAULTS1 = 0X00;
const uint8_t I2C_TMP102_CR_FAULTQUEUE_FAULTS2 = 0X01;
const uint8_t I2C_TMP102_CR_FAULTQUEUE_FAULTS4 = 0X02;
const uint8_t I2C_TMP102_CR_FAULTQUEUE_FAULTS6 = 0X03;

const uint8_t I2C_TMP102_CR_ONESHOT_SHOT = 0X01;
const uint8_t I2C_TMP102_CR_OS_BIT = 7;

const uint8_t I2C_TMP102_CR_EXTENDEDMODE_DISABLE = 0X00;
const uint8_t I2C_TMP102_CR_EXTENDEDMODE_ENABLE = 0X01;

const uint8_t I2C_TMP102_CR_CONVERSIONRATE_025HZ = 0X00;
const uint8_t I2C_TMP102_CR_CONVERSIONRATE_1HZ = 0X01;
const uint8_t I2C_TMP102_CR_CONVERSIONRATE_4HZ = 0X02;
const uint8_t I2C_TMP102_CR_CONVERSIONRATE_8HZ = 0X03;

#define I2C_TMP102_REG16(VAL, MASK, SHIFT) ((uint16_t)(((VAL) & (MASK)) << (SHIFT)))
#define I2C_TMP102_CR_SD(VAL)   I2C_TMP102_REG16(VAL, 0x0001, 0)  ///< SHUTDOWN
#define I2C_TMP102_CR_TM(VAL)   I2C_TMP102_REG16(VAL, 0x0001, 1)  ///< THERMO MODE
#define I2C_TMP102_CR_POL(VAL)  I2C_TMP102_REG16(VAL, 0x0001, 2)  ///< POLARITY
#define I2C_TMP102_CR_FQ(VAL)   I2C_TMP102_REG16(VAL, 0x0003, 3)  ///< FAULT QUEUE
#define I2C_TMP102_CR_OS(VAL)   I2C_TMP102_REG16(VAL, 0x0001, I2C_TMP102_CR_OS_BIT)  ///< ONESHOT
#define I2C_TMP102_CR_EM(VAL)   I2C_TMP102_REG16(VAL, 0x0001, 12) ///< EXTENDED MODE
#define I2C_TMP102_CR_AL(VAL)   I2C_TMP102_REG16(VAL, 0x0001, 13) ///< ALERT
#define I2C_TMP102_CR_CR(VAL)   I2C_TMP102_REG16(VAL, 0x0003, 14) ///< CONVERSION RATE
#define I2C_TMP102_CONFIG_REG(SD, TM, POL, FQ, EM, CR) \
		(0x0000 | (SD) | (TM) | (POL) | (FQ) | (EM) | (CR))
#define I2C_UINT16_LOW(VAL) ((uint8_t)(VAL))
#define I2C_UINT16_HIGH(VAL) ((uint8_t)((VAL) >> 8))
#define I2C_IS_SET(VAL, BIT) (((VAL) & (0x01 << BIT)) > 0)
#define I2C_TMP102_DEF_CONFIG_REG I2C_TMP102_CONFIG_REG( \
		I2C_TMP102_CR_SD(I2C_TMP102_CR_SHUTDOWN_ENABLE), \
		I2C_TMP102_CR_TM(I2C_TMP102_CR_THERMOMODE_COMPARE), \
		I2C_TMP102_CR_POL(I2C_TMP102_CR_POLARITY_ACTIVEHIGH), \
		I2C_TMP102_CR_FQ(I2C_TMP102_CR_FAULTQUEUE_FAULTS1), \
		I2C_TMP102_CR_EM(I2C_TMP102_CR_EXTENDEDMODE_DISABLE), \
		I2C_TMP102_CR_CR(I2C_TMP102_CR_CONVERSIONRATE_025HZ))

const float TMP102_CELCIUS_DEGREE_PER_BIT = 0.0625f;

bool i2c_tmp102_write(uint8_t reg, uint16_t val)
{
	struct mgos_i2c * i2c = mgos_i2c_get_global();
	assert(i2c != NULL);

	const uint8_t write_buf[3] = { reg, I2C_UINT16_LOW(val), I2C_UINT16_HIGH(val)};
	return mgos_i2c_write(i2c, I2C_TMP102_DEVICE_ADDR,
			write_buf, sizeof(write_buf), true);
}

bool i2c_tmp102_read(uint8_t reg, uint16_t * val)
{
	struct mgos_i2c * i2c = mgos_i2c_get_global();
	assert(i2c != NULL);

	return mgos_i2c_write(i2c, I2C_TMP102_DEVICE_ADDR, &reg, sizeof(reg), true) &&
		mgos_i2c_read(i2c, I2C_TMP102_DEVICE_ADDR, val, sizeof(uint16_t), true);
}

bool i2c_tmp102_init()
{
	struct mgos_i2c * i2c = mgos_i2c_get_global();
	bool success = (i2c != NULL);

	// Configure register
	if (success)
	{
		const uint16_t default_config_reg = I2C_TMP102_DEF_CONFIG_REG;
		success = i2c_tmp102_write(I2C_TMP102_PR_CONFIGURATION,
				default_config_reg);
	}

	return success;
}

void i2c_tmp102_final()
{

}

bool i2c_tmp102_get_temperature(sys_temperature * temp)
{
	const uint16_t default_config_reg = I2C_TMP102_DEF_CONFIG_REG;
	// make one-shot
	bool success = i2c_tmp102_write(I2C_TMP102_PR_CONFIGURATION,
			(default_config_reg | I2C_TMP102_CR_OS(I2C_TMP102_CR_ONESHOT_SHOT)));;

	if (success)
	{
		success = false;
		// wait until conversion has finished
		for (uint16_t i = 5; (i > 0) && !success; i--)
		{
			mgos_usleep(20000); // sleep 20ms
			uint16_t reg = 0;
			success = i2c_tmp102_read(I2C_TMP102_PR_CONFIGURATION, &reg);
			if (success)
			{
				success = I2C_IS_SET(reg, I2C_TMP102_CR_OS_BIT);
			}
		}
	}

	if (success)
	{
		uint16_t _temp = 0;
		success = i2c_tmp102_read(I2C_TMP102_PR_TEMPERATURE, &_temp);
		if (success)
		{
			_temp = (((_temp) << 4) & 0x0ff0) | (((_temp) >> 12) & 0x000f);
			*temp = TMP102_CELCIUS_DEGREE_PER_BIT * _temp;
		}
	}

	return success;
}
