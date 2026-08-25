/*
Test script for
Adafruit 2.13 250×122 tri-color ePaper with SRAM, SSD1680Z, product 4947. 
*/
#include "epaper.h"
#include "main.h"
#include <string.h>

extern SPI_HandleTypeDef hspi2;

static uint8_t black_buffer[EPD_BUFFER_SIZE];
static uint8_t red_buffer[EPD_BUFFER_SIZE];
static void EPD_Reset(void);
static void EPD_SendCommand(uint8_t cmd);
static void EPD_SendData(uint8_t data);
static uint8_t EPD_WaitBusy(void);

static void EPD_SendCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(EP_DC_GPIO_Port, EP_DC_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EP_CS_GPIO_Port, EP_CS_Pin, GPIO_PIN_RESET);

    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(EP_CS_GPIO_Port, EP_CS_Pin, GPIO_PIN_SET);
}

static void EPD_SendData(uint8_t data)
{
    HAL_GPIO_WritePin(EP_DC_GPIO_Port, EP_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(EP_CS_GPIO_Port, EP_CS_Pin, GPIO_PIN_RESET);

    HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(EP_CS_GPIO_Port, EP_CS_Pin, GPIO_PIN_SET);
}

static void EPD_Reset(void)
{
    HAL_GPIO_WritePin(EP_RST_GPIO_Port, EP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    HAL_GPIO_WritePin(EP_RST_GPIO_Port, EP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
}

static uint8_t EPD_WaitBusy(void)
{
    uint32_t start = HAL_GetTick();

    // Give SSD1680 time to assert BUSY
    HAL_Delay(10);

    while (HAL_GPIO_ReadPin(EP_BUSY_GPIO_Port, EP_BUSY_Pin) == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - start) > 22000)
        {
            return 0;
        }

        HAL_Delay(1);
    }

    return 1;
}

uint8_t EPD_Init(void)
{
    EPD_Reset();
    
    if (!EPD_WaitBusy())
        return 0;

    // Software reset
    EPD_SendCommand(0x12);

    if (!EPD_WaitBusy())
        return 0;

    // Driver Output Control: 250 gates
    EPD_SendCommand(0x01);
    EPD_SendData(0xF9);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    // Data Entry Mode
    EPD_SendCommand(0x11);
    EPD_SendData(0x03);

    // RAM X range: 16 bytes = 128 controller pixels
    EPD_SendCommand(0x44);
    EPD_SendData(0x00);
    EPD_SendData(0x0F);

    // RAM Y range: 0–249
    EPD_SendCommand(0x45);
    EPD_SendData(0x00);
    EPD_SendData(0x00);
    EPD_SendData(0xF9);
    EPD_SendData(0x00);

    // Border waveform
    EPD_SendCommand(0x3C);
    EPD_SendData(0x05);

    // Start RAM counters at 0,0
    EPD_SendCommand(0x4E);
    EPD_SendData(0x00);

    EPD_SendCommand(0x4F);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    return 1;
}

uint8_t EPD_Clear(void)
{
    uint16_t i;

    /*
     * 16 bytes per row × 250 rows = 4000 bytes
     * per color plane.
     */

    // Set RAM address counters back to 0,0
    EPD_SendCommand(0x4E);
    EPD_SendData(0x00);

    EPD_SendCommand(0x4F);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    // Write first image plane
    EPD_SendCommand(0x24);

    for (i = 0; i < 4000; i++)
    {
        EPD_SendData(0xFF);
    }

    // Reset RAM address counters again
    EPD_SendCommand(0x4E);
    EPD_SendData(0x00);

    EPD_SendCommand(0x4F);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    // Write second image plane
    EPD_SendCommand(0x26);

    for (i = 0; i < 4000; i++)
    {
        EPD_SendData(0x00);
    }

    // Tell SSD1680 to perform the physical display update
    EPD_SendCommand(0x22);
    EPD_SendData(0xF7);

    EPD_SendCommand(0x20);

    if (!EPD_WaitBusy())
    {
        return 0;
    }

    return 1;
}

void EPD_ClearBuffer(void)
{
    memset(black_buffer, 0xFF, EPD_BUFFER_SIZE);
    memset(red_buffer, 0x00, EPD_BUFFER_SIZE);
}

void EPD_DrawPixel(uint16_t x, uint16_t y, EPD_Color color)
{
    if (x >= EPD_WIDTH || y >= EPD_HEIGHT)
        return;

    uint16_t byte_index = (x / 8) + (y * 16);
    uint8_t bit_mask = 0x80 >> (x % 8);

    switch (color)
    {
        case EPD_BLACK:
            black_buffer[byte_index] &= ~bit_mask;
            red_buffer[byte_index]   &= ~bit_mask;
            break;

        case EPD_RED:
            black_buffer[byte_index] |= bit_mask;
            red_buffer[byte_index]   |= bit_mask;
            break;

        case EPD_WHITE:
        default:
            black_buffer[byte_index] |= bit_mask;
            red_buffer[byte_index]   &= ~bit_mask;
            break;
    }
}

void EPD_DrawChar(uint16_t x, uint16_t y,
                  char c,
                  const FontDef *font,
                  EPD_Color color)
{
    if (c < 32 || c > 126)
        c = '?';

    uint16_t offset = (uint16_t)(c - 32) * font->width;

    for (uint8_t col = 0; col < font->width; col++)
    {
        uint8_t column = font->data[offset + col];

        for (uint8_t row = 0; row < font->height; row++)
        {
            if (column & (1U << row))
            {
                EPD_DrawPixel(x + col, y + row, color);
            }
        }
    }
}

void EPD_DrawString(uint16_t x, uint16_t y,
                    const char *str,
                    const FontDef *font,
                    EPD_Color color)
{
    while (*str)
    {
        EPD_DrawChar(x, y, *str, font, color);

        x += font->width + 1;
        str++;
    }
}

uint8_t EPD_Display(void)
{
    uint16_t i;

    EPD_SendCommand(0x4E);
    EPD_SendData(0x00);

    EPD_SendCommand(0x4F);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    EPD_SendCommand(0x24);
    for (i = 0; i < EPD_BUFFER_SIZE; i++)
    {
        EPD_SendData(black_buffer[i]);
    }

    EPD_SendCommand(0x4E);
    EPD_SendData(0x00);

    EPD_SendCommand(0x4F);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    EPD_SendCommand(0x26);
    for (i = 0; i < EPD_BUFFER_SIZE; i++)
    {
        EPD_SendData(red_buffer[i]);
    }

    EPD_SendCommand(0x22);
    EPD_SendData(0xF7);

    EPD_SendCommand(0x20);

    return EPD_WaitBusy();
}