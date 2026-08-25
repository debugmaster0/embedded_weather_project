#ifndef BME280_H
#define BME280_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define BME280_ADDRESS (0x77 << 1)

#define BME280_REG_ID        0xD0
#define BME280_REG_RESET     0xE0
#define BME280_REG_CTRL_HUM  0xF2
#define BME280_REG_STATUS    0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG    0xF5
#define BME280_REG_PRESS_MSB 0xF7

typedef struct
{
    float temperature_c;
    float humidity;
    float pressure_hpa;
} BME280_Data;

HAL_StatusTypeDef BME280_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef BME280_Read(BME280_Data *data);

#endif