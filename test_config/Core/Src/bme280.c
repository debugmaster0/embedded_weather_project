#include "bme280.h"

static I2C_HandleTypeDef *bme280_i2c;

/* Factory calibration values */
static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;

static uint16_t dig_P1;
static int16_t  dig_P2;
static int16_t  dig_P3;
static int16_t  dig_P4;
static int16_t  dig_P5;
static int16_t  dig_P6;
static int16_t  dig_P7;
static int16_t  dig_P8;
static int16_t  dig_P9;

static uint8_t dig_H1;
static int16_t dig_H2;
static uint8_t dig_H3;
static int16_t dig_H4;
static int16_t dig_H5;
static int8_t  dig_H6;

static int32_t t_fine;


/* ---------------------------------------------------------
 * Internal I2C helpers
 * --------------------------------------------------------- */

static HAL_StatusTypeDef BME280_ReadRegs(
    uint8_t reg,
    uint8_t *data,
    uint16_t len)
{
    return HAL_I2C_Mem_Read(
        bme280_i2c,
        BME280_ADDRESS,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        data,
        len,
        100
    );
}


static HAL_StatusTypeDef BME280_WriteReg(
    uint8_t reg,
    uint8_t value)
{
    return HAL_I2C_Mem_Write(
        bme280_i2c,
        BME280_ADDRESS,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        &value,
        1,
        100
    );
}


/* ---------------------------------------------------------
 * Initialization
 * --------------------------------------------------------- */

HAL_StatusTypeDef BME280_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t id;
    uint8_t calib1[26];
    uint8_t calib2[7];

    bme280_i2c = hi2c;

    /* First make sure something answers at 0x77 */
    if (HAL_I2C_IsDeviceReady(
            bme280_i2c,
            BME280_ADDRESS,
            3,
            100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Verify that it is actually a BME280 */
    if (BME280_ReadRegs(BME280_REG_ID, &id, 1) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (id != 0x60)
    {
        return HAL_ERROR;
    }

    /* Soft reset */
    if (BME280_WriteReg(BME280_REG_RESET, 0xB6) != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_Delay(10);

    /*
     * Read factory calibration data.
     *
     * Temperature/pressure calibration: 0x88 through 0xA1
     * Humidity calibration:             0xE1 through 0xE7
     */
    if (BME280_ReadRegs(0x88, calib1, 26) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (BME280_ReadRegs(0xE1, calib2, 7) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* Temperature calibration */
    dig_T1 = (uint16_t)(
        ((uint16_t)calib1[1] << 8) |
        calib1[0]);

    dig_T2 = (int16_t)(
        ((uint16_t)calib1[3] << 8) |
        calib1[2]);

    dig_T3 = (int16_t)(
        ((uint16_t)calib1[5] << 8) |
        calib1[4]);

    /* Pressure calibration */
    dig_P1 = (uint16_t)(
        ((uint16_t)calib1[7] << 8) |
        calib1[6]);

    dig_P2 = (int16_t)(
        ((uint16_t)calib1[9] << 8) |
        calib1[8]);

    dig_P3 = (int16_t)(
        ((uint16_t)calib1[11] << 8) |
        calib1[10]);

    dig_P4 = (int16_t)(
        ((uint16_t)calib1[13] << 8) |
        calib1[12]);

    dig_P5 = (int16_t)(
        ((uint16_t)calib1[15] << 8) |
        calib1[14]);

    dig_P6 = (int16_t)(
        ((uint16_t)calib1[17] << 8) |
        calib1[16]);

    dig_P7 = (int16_t)(
        ((uint16_t)calib1[19] << 8) |
        calib1[18]);

    dig_P8 = (int16_t)(
        ((uint16_t)calib1[21] << 8) |
        calib1[20]);

    dig_P9 = (int16_t)(
        ((uint16_t)calib1[23] << 8) |
        calib1[22]);

    /* Humidity calibration */
    dig_H1 = calib1[25];

    dig_H2 = (int16_t)(
        ((uint16_t)calib2[1] << 8) |
        calib2[0]);

    dig_H3 = calib2[2];

    dig_H4 = (int16_t)(
        ((int16_t)((int8_t)calib2[3]) << 4) |
        (calib2[4] & 0x0F));

    dig_H5 = (int16_t)(
        ((int16_t)((int8_t)calib2[5]) << 4) |
        (calib2[4] >> 4));

    dig_H6 = (int8_t)calib2[6];

    /*
     * Humidity oversampling x1.
     * ctrl_hum must be written before ctrl_meas.
     */
    if (BME280_WriteReg(
            BME280_REG_CTRL_HUM,
            0x01) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /*
     * Temperature oversampling x1
     * Pressure oversampling x1
     * Normal mode
     */
    if (BME280_WriteReg(
            BME280_REG_CTRL_MEAS,
            0x27) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}


/* ---------------------------------------------------------
 * Read current measurement
 * --------------------------------------------------------- */

HAL_StatusTypeDef BME280_Read(BME280_Data *data)
{
    uint8_t raw[8];

    if (data == NULL)
    {
        return HAL_ERROR;
    }

    /*
     * Registers 0xF7 through 0xFE contain:
     *
     * pressure    3 bytes
     * temperature 3 bytes
     * humidity    2 bytes
     */
    if (BME280_ReadRegs(
            BME280_REG_PRESS_MSB,
            raw,
            8) != HAL_OK)
    {
        return HAL_ERROR;
    }

    int32_t adc_P =
        ((int32_t)raw[0] << 12) |
        ((int32_t)raw[1] << 4) |
        ((int32_t)raw[2] >> 4);

    int32_t adc_T =
        ((int32_t)raw[3] << 12) |
        ((int32_t)raw[4] << 4) |
        ((int32_t)raw[5] >> 4);

    int32_t adc_H =
        ((int32_t)raw[6] << 8) |
        raw[7];


    /* -----------------------------------------------------
     * Temperature compensation
     * ----------------------------------------------------- */

    int32_t var1;
    int32_t var2;

    var1 =
        ((((adc_T >> 3) -
           ((int32_t)dig_T1 << 1))) *
         ((int32_t)dig_T2)) >> 11;

    var2 =
        (((((adc_T >> 4) -
            ((int32_t)dig_T1)) *
           ((adc_T >> 4) -
            ((int32_t)dig_T1))) >> 12) *
         ((int32_t)dig_T3)) >> 14;

    t_fine = var1 + var2;

    int32_t T =
        (t_fine * 5 + 128) >> 8;

    data->temperature_c =
        (float)T / 100.0f;


    /* -----------------------------------------------------
     * Pressure compensation
     * ----------------------------------------------------- */

    int64_t pvar1;
    int64_t pvar2;
    int64_t p;

    pvar1 =
        ((int64_t)t_fine) - 128000;

    pvar2 =
        pvar1 * pvar1 *
        (int64_t)dig_P6;

    pvar2 +=
        (pvar1 *
         (int64_t)dig_P5) << 17;

    pvar2 +=
        ((int64_t)dig_P4) << 35;

    pvar1 =
        ((pvar1 * pvar1 *
          (int64_t)dig_P3) >> 8) +
        ((pvar1 *
          (int64_t)dig_P2) << 12);

    pvar1 =
        (((((int64_t)1) << 47) +
          pvar1) *
         (int64_t)dig_P1) >> 33;

    if (pvar1 == 0)
    {
        return HAL_ERROR;
    }

    p = 1048576 - adc_P;

    p =
        (((p << 31) - pvar2) *
         3125) /
        pvar1;

    pvar1 =
        ((int64_t)dig_P9 *
         (p >> 13) *
         (p >> 13)) >> 25;

    pvar2 =
        ((int64_t)dig_P8 *
         p) >> 19;

    p =
        ((p + pvar1 + pvar2) >> 8) +
        ((int64_t)dig_P7 << 4);

    /* Convert Pa to hPa */
    data->pressure_hpa =
        (float)p / 25600.0f;


    /* -----------------------------------------------------
     * Humidity compensation
     * ----------------------------------------------------- */

    int32_t h;

    h = t_fine - 76800;

    h =
        (((((adc_H << 14) -
            (((int32_t)dig_H4) << 20) -
            (((int32_t)dig_H5) * h)) +
           16384) >> 15) *
         (((((((h *
               ((int32_t)dig_H6)) >> 10) *
              (((h *
                 ((int32_t)dig_H3)) >> 11) +
               32768)) >> 10) +
            2097152) *
           ((int32_t)dig_H2) +
           8192) >> 14));

    h =
        h -
        (((((h >> 15) *
            (h >> 15)) >> 7) *
          ((int32_t)dig_H1)) >> 4);

    if (h < 0)
    {
        h = 0;
    }

    if (h > 419430400)
    {
        h = 419430400;
    }

    data->humidity =
        (float)(h >> 12) / 1024.0f;

    return HAL_OK;
}