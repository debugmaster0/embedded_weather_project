#include "ina219.h"

static I2C_HandleTypeDef *ina219_i2c;

HAL_StatusTypeDef INA219_Init(I2C_HandleTypeDef *hi2c)
{
    ina219_i2c = hi2c;

    return HAL_I2C_IsDeviceReady(
        ina219_i2c,
        INA219_ADDRESS,
        3,
        100
    );
}