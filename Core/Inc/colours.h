/*
 * colours.h
 *
 *  Created on: Oct 11, 2025
 *      Author: Vilem Broucek
 */

#ifndef INC_COLOURS_H
#define INC_COLOURS_H

#include <stdint.h>

typedef uint8_t ColourName_t;

enum {
    BLACK = 0,
    WHITE,
    RED,
    GREEN,
    BLUE,
    COLOUR_COUNT
};

typedef struct {
    uint8_t r, g, b;
} ColourRGB_t;

extern const ColourRGB_t colourTable[COLOUR_COUNT];

#endif /* INC_COLOURS_H_ */
