#ifndef FONT_H
#define FONT_H

#include <stdint.h>

typedef struct
{
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
} FontDef;

extern const FontDef Font8;

#endif