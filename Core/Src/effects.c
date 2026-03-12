/*
 * effects.c
 *
 *  Created on: Oct 14, 2025
 *      Author: Vilem Broucek
 */
#include "effects.h"
#include "math.h"

#define LEDCOUNT 144
#define MCU_CLOCK 72000000.0f

static TIM_HandleTypeDef *htimLocal;
static uint16_t bpm = 164;
static ColourName_t primaryColour = PRIMARY_RED;
static ColourName_t secondaryColour = PRIMARY_GREEN;
static uint32_t step = 0;
//static uint8_t is_new_effect = 0;
//static uint8_t brightness=255;

static uint16_t PSC;
static uint8_t multiplier;

void effects_init(TIM_HandleTypeDef *htim)
{
	htimLocal = htim;
	is_new_effect = 1;
	ARGB_Init();
	ARGB_Clear();
	ARGB_Show();
	ARGB_SetBrightness(10);
}

//Pointer to selected function
static void (*current_effect_func)(void);

// --- EFFECT DEFINITIONS ---
static void effect_lights_down(void)
{
	ARGB_Clear();
	ARGB_Show();
}

static void effect_static(void)
{
	ARGB_FillRGB(
		colourTable[primaryColour].r,
		colourTable[primaryColour].g,
		colourTable[primaryColour].b);
	ARGB_FillWhite(colourTable[primaryColour].w);
	ARGB_Show();
}

static void effect_strobe(void)
{
    if (step==1)
    {
        ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
        ARGB_FillWhite(colourTable[primaryColour].w);
        ARGB_Show();
    }
    else
    {
        ARGB_Clear();
        ARGB_Show();
        step = 0;
    }
}

static void effect_strobe_colours(void)
{
    switch(step)
    {
		case 1:
			ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_FillWhite(colourTable[primaryColour].w);
			break;
		case 2:
			ARGB_Clear();
			break;
		case 3:
			ARGB_FillRGB(colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_FillWhite(colourTable[secondaryColour].w);
			break;
		default:
			ARGB_Clear();
			step = 0;
			break;
    }
	ARGB_Show();
}

static void effect_switch_colours(void)
{
    if (step==1)
    {
		ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_FillWhite(colourTable[primaryColour].w);
        ARGB_Show();
    }
    else
    {
		ARGB_FillRGB(colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
		ARGB_FillWhite(colourTable[secondaryColour].w);
		ARGB_Show();
        step = 0;
    }
}

// --- SETTERS ---
void effects_set_timer(uint16_t ARR, uint16_t PSC)
{
	__HAL_TIM_SET_AUTORELOAD(htimLocal, ARR);
	__HAL_TIM_SET_PRESCALER(htimLocal, PSC);
	__HAL_TIM_SET_COUNTER(htimLocal, 0);
}

void effects_set_brightness(uint8_t new_brightness)
{
    //brightness=new_brightness;
    ARGB_SetBrightness(brightness);
}

void effects_set_tempo(uint8_t new_bpm)
{
    bpm = new_bpm;
}

void effects_set_primaryColour(ColourName_t new_colour)
{
	primaryColour = new_colour;
}

void effects_set_secondaryColour(ColourName_t new_colour)
{
	secondaryColour = new_colour;
}


// --- TIMER ---
void effects_next_step(void)
{
    step++;
    current_effect_func(); //calls
    //if (active_effect_func != 0)
    //{


    //}
}

// --- DMX DECODE ---
void effects_set_effect(uint8_t effect1, uint8_t effect2)
{
    switch (effect1)
    {
        case 0 ... 2: //LIGHTS DOWN
            current_effect_func = effect_lights_down;
            PSC = 10000;
            multiplier = 1;
            break;
        case 3 ... 4: //STATIC COLOUR
			current_effect_func = effect_static;
            PSC = 30000;
            multiplier = 1;
            break;
        case 5 ... 6: //STROBE
			current_effect_func = effect_strobe;
            switch (effect2)
            {
                case 0 ... 36:    multiplier = 1;  PSC=2200;  break; // x1
                case 37 ... 72:   multiplier = 2;  PSC=1100;  break; // x2
                case 73 ... 108:  multiplier = 4;  PSC=550;  break; // x4
                case 109 ... 144: multiplier = 8;  PSC=275;  break; // x4
                case 145 ... 180: multiplier = 16; PSC=137;  break; // x16
                case 181 ... 216: multiplier = 32; PSC=69;  break; // x32
                case 217 ... 255: multiplier = 64; PSC=34;  break; // x64
            }
            break;
         case 7 ... 8: //STROBE TWO COLOURS
    		current_effect_func = effect_strobe_colours;
             switch (effect2)
             {
                case 0 ... 36:    multiplier = 1;  PSC=2200;  break; // x1
                case 37 ... 72:   multiplier = 2;  PSC=1100;  break; // x2
                case 73 ... 108:  multiplier = 4;  PSC=550;  break; // x4
                case 109 ... 144: multiplier = 8;  PSC=275;  break; // x4
                case 145 ... 180: multiplier = 16; PSC=137;  break; // x16
                case 181 ... 216: multiplier = 32; PSC=69;  break; // x32
                case 217 ... 255: multiplier = 64; PSC=34;  break; // x64
             }
             break;
          case 9 ... 10: //SWITCH TWO COLOURS
     		current_effect_func = effect_switch_colours;
              switch (effect2)
              {
                 case 0 ... 36:    multiplier = 1;  PSC=2200;  break; // x1
                 case 37 ... 72:   multiplier = 2;  PSC=1100;  break; // x2
                 case 73 ... 108:  multiplier = 4;  PSC=550;  break; // x4
                 case 109 ... 144: multiplier = 8;  PSC=275;  break; // x4
                 case 145 ... 180: multiplier = 16; PSC=137;  break; // x16
                 case 181 ... 216: multiplier = 32; PSC=69;  break; // x32
                 case 217 ... 255: multiplier = 64; PSC=34;  break; // x64
              }
              break;
          //SLIDE TWO COLOURS
          //SLID IN STATIC - with different speeds
          //SWITCH TWO COLOURS CONTINOUSLY - with different speeds
          //PULSE IN BRIGHTNESS - with different speeds

    }
    //is_new_effect = 0;
    step = 0;
	uint16_t ARR=((60.0f/(float)bpm)/(float)multiplier)*MCU_CLOCK/(PSC+1)-1;
    //uint16_t arr = (raw_arr > 65535.0f) ? 65535 : (uint16_t)raw_arr;
	effects_set_timer(ARR, PSC);
	current_effect_func();
}
