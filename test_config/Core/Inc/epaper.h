#ifndef EPAPER_H
#define EPAPER_H

#include "stm32f4xx_hal.h"
#include "font.h"

#define EPD_WIDTH        122
#define EPD_HEIGHT       250
#define EPD_BUFFER_SIZE  4000

typedef enum
{
    EPD_WHITE = 0,
    EPD_BLACK = 1,
    EPD_RED   = 2
} EPD_Color;

uint8_t EPD_Init(void);
uint8_t EPD_Clear(void);

void EPD_ClearBuffer(void);
void EPD_DrawPixel(uint16_t x, uint16_t y, EPD_Color color);
uint8_t EPD_Display(void);


void EPD_DrawChar(uint16_t x, uint16_t y,
                  char c,
                  const FontDef *font,
                  EPD_Color color);

void EPD_DrawString(uint16_t x, uint16_t y,
                    const char *str,
                    const FontDef *font,
                    EPD_Color color);

#endif