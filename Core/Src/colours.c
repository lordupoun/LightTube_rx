/*
 * colours.c
 *
 *  Created on: Oct 11, 2025
 *      Author: Vilem Broucek
 */
#include "colours.h"

const ColourRGB_t colourTable[COLOUR_COUNT] = {
    [BLACK] = {0, 0, 0},
    [WHITE] = {255, 255, 255},
    [RED]   = {255, 0, 0},
    [GREEN] = {0, 255, 0},
    [BLUE]  = {0, 0, 255}
};


