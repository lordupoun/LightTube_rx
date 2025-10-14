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
    BASTARD_AMBER = 0,
    MEDIUM_AMBER,
    PALE_AMBER_GOLD,
    GALLO_GOLD,
    GOLDEN_AMBER,
    PRIMARY_RED,
    MEDIUM_RED,
    MEDIUM_PINK,
    BROADWAY_PINK,
    FOLLIES_PINK,
    LIGHT_LAVENDER,
    SPECIAL_LAVENDER,
    LAVENDER,
    INDIGO,
    HEMSLEY_BLUE,
    TIPTON_BLUE,
    LIGHT_STEEL_BLUE,
    LIGHT_SKY_BLUE,
    SKY_BLUE,
    BRILLIANT_BLUE,
    LIGHT_GREEN_BLUE,
    BRIGHT_BLUE,
    PRIMARY_BLUE,
    CONGO_BLUE,
    PALE_YELLOW_GREEN,
    MOSS_GREEN,
    PRIMARY_GREEN,
    DOUBLE_CTB,
    FULL_CTB,
    HALF_CTB,
    DARK_BLUE,
    WHITE,
    COLOUR_COUNT
};

typedef struct {
    uint8_t r, g, b, w;
} ColourRGB_t;

extern const ColourRGB_t colourTable[COLOUR_COUNT];

#endif /* INC_COLOURS_H_ */
