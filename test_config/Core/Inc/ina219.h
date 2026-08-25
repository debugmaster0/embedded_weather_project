#ifndef INA219_H
#define INA219_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define INA219_ADDRESS (0x40 << 1)

/* Registers */
#define INA219_REG_CONFIG        0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01
#define INA219_REG_BUS_VOLTAGE   0x02
#define INA219_REG_POWER         0x03
#define INA219_REG_CURRENT       0x04
#define INA219_REG_CALIBRATION   0x05

HAL_StatusTypeDef INA219_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef INA219_ReadBusVoltage(float *voltage);

#endif