/*
 * effects.c
 *
 *  Created on: Oct 14, 2025
 *      Author: Vilem Broucek
 *
 *
 *      !IMPORTANT!
 *      The time between ARGB_Show shouldn't go under 6ms (for 144LED) otherwise the effect won't show correctly
 *
 */
#include "effects.h"
#include "math.h"
#include <stdlib.h> // Nutné pro funkci rand()

#define LEDCOUNT 144
#define TUBENUMBER 1
#define MCU_CLOCK 72000000.0f

static TIM_HandleTypeDef *htimLocal;
static uint16_t bpm = 164;
static ColourName_t primaryColour = 233; //20
static ColourName_t secondaryColour = 102; //102
static uint32_t step = 0;
//static uint8_t is_new_effect = 0;
//static uint8_t brightness=255;
//static uint8_t modifier = 0;

static uint16_t PSC; 		 //defines the tempo accuraccy (ARR is automatically computed from given PSC and multiplier)
static float multiplier = 1; //defines the speed of an animation
static uint16_t modifier = 1; //defines the variation of an animation

static uint8_t colourChanged = 0; //for effects that has to recalculate the influence of colour change (gradients, etc.)
static uint8_t ownTempo=0;  	  //for effects that use fixed individual refresh rate
static uint8_t modifier2=0;

void effects_init(TIM_HandleTypeDef *htim)
{
	htimLocal = htim;
	//is_new_effect = 1;
	ARGB_Init();
	ARGB_Clear();
	ARGB_Show();
	ARGB_SetBrightness(255);
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

static void effect_static_two_colour(void)
{   //modifier must be selected so LEDCOUNT is divided to an integer
	uint8_t blocks = LEDCOUNT/modifier;
	for(uint8_t a=0; a<blocks; a++) //BLOCK
	{
		if(a%2==0)
		{
			for(uint8_t b=0; b<modifier; b++)
			{
				ARGB_SetRGB(a*modifier+b, colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
				ARGB_SetWhite(a*modifier+b, colourTable[secondaryColour].w);
			}
		}
		else
		{
			for(uint8_t b=0; b<modifier; b++)
			{
				ARGB_SetRGB(a*modifier+b, colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
				ARGB_SetWhite(a*modifier+b, colourTable[secondaryColour].w);
			}
		}
	}
	ARGB_Show();
}

static void effect_static_two_colour_brightness(void)
{   //modifier must be selected so LEDCOUNT is divided to an integer
	uint8_t blocks = LEDCOUNT/modifier;
	uint8_t brightnessStep = 256/modifier;
	for(uint8_t a=0; a<blocks; a++) //BLOCK
	{
		//ARGB_SetBrightness(256-(256/blocks)*a);
		if(a%2==0)
		{
			for(uint8_t b=0; b<modifier; b++)
			{
				ARGB_SetBrightness(brightnessStep*b);
				ARGB_SetRGB(a*modifier+b, colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
				ARGB_SetWhite(a*modifier+b, colourTable[secondaryColour].w);
			}
		}
		else
		{
			for(uint8_t b=0; b<modifier; b++)
			{
				ARGB_SetBrightness(brightnessStep*(modifier-1-b));
				ARGB_SetRGB(a*modifier+b, colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
				ARGB_SetWhite(a*modifier+b, colourTable[secondaryColour].w);
			}
		}
	}
	ARGB_Show();
}

static void effect_static_two_colour_gradient(void) //ToDo: Colour change needs to reset effect
{
	float colourChangeVector[4];
	uint8_t colourMaxStep=LEDCOUNT-modifier;
	//if(colourChanged==1)
	//{
	colourChangeVector[0]=((int16_t)colourTable[secondaryColour].r-(int16_t)colourTable[primaryColour].r)/(float)colourMaxStep; //144-1 = max step count step count
	colourChangeVector[1]=((int16_t)colourTable[secondaryColour].g-(int16_t)colourTable[primaryColour].g)/(float)colourMaxStep;
	colourChangeVector[2]=((int16_t)colourTable[secondaryColour].b-(int16_t)colourTable[primaryColour].b)/(float)colourMaxStep;
	colourChangeVector[3]=((int16_t)colourTable[secondaryColour].w-(int16_t)colourTable[primaryColour].w)/(float)colourMaxStep;

	for(uint16_t i=0; i<modifier/2; i++)
	{
		ARGB_SetRGB(i, colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_SetWhite(i, colourTable[primaryColour].w);
	}
	for(uint16_t i=LEDCOUNT-(modifier/2); i<LEDCOUNT; i++)
	{
		ARGB_SetRGB(i, colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
		ARGB_SetWhite(i, colourTable[secondaryColour].w);
	}
	for(uint16_t i=0; i<=LEDCOUNT-multiplier; i++)
	{
		uint8_t ledNum=i+(modifier/2-1);
		ARGB_SetRGB(ledNum,
					round(colourTable[primaryColour].r+colourChangeVector[0]*(float)i),
					round(colourTable[primaryColour].g+colourChangeVector[1]*(float)i),
					round(colourTable[primaryColour].b+colourChangeVector[2]*(float)i));
		ARGB_SetWhite(ledNum,
					round(colourTable[primaryColour].w+colourChangeVector[3]*(float)i));
	}
	//}
	ARGB_Show();

}

static void effect_static_two_colour_gradient_2(void)
{
	float colourChangeVector[4];
	uint8_t colourMaxStep=LEDCOUNT/2;
	colourChangeVector[0]=((int16_t)colourTable[secondaryColour].r-(int16_t)colourTable[primaryColour].r)/(float)colourMaxStep; //144-1 = max step count step count
	colourChangeVector[1]=((int16_t)colourTable[secondaryColour].g-(int16_t)colourTable[primaryColour].g)/(float)colourMaxStep;
	colourChangeVector[2]=((int16_t)colourTable[secondaryColour].b-(int16_t)colourTable[primaryColour].b)/(float)colourMaxStep;
	colourChangeVector[3]=((int16_t)colourTable[secondaryColour].w-(int16_t)colourTable[primaryColour].w)/(float)colourMaxStep;

	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		ARGB_SetRGB(i,
						round(colourTable[primaryColour].r+colourChangeVector[0]*(float)i),
						round(colourTable[primaryColour].g+colourChangeVector[1]*(float)i),
						round(colourTable[primaryColour].b+colourChangeVector[2]*(float)i));
		ARGB_SetWhite(i,
						round(colourTable[primaryColour].w+colourChangeVector[3]*(float)i));
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		ARGB_SetRGB(colourMaxStep+i,
						round(colourTable[secondaryColour].r-colourChangeVector[0]*(float)i),
						round(colourTable[secondaryColour].g-colourChangeVector[1]*(float)i),
						round(colourTable[secondaryColour].b-colourChangeVector[2]*(float)i));
		ARGB_SetWhite(colourMaxStep+i,
						round(colourTable[secondaryColour].w-colourChangeVector[3]*(float)i));
	}
    ARGB_Show();
}

static void effect_moving_gradient(void) //ToDo: add safetyLines, add if step>143 step=0
{
	//rather than saving data for each LED into an array and the shifting it, program calculates it each time the timer runs
	//uint16_t localStep = step%144;
	static float colourChangeVector[4];
	static uint8_t colourMaxStep=LEDCOUNT/2;
	if(step>=LEDCOUNT)
    {
    	step=0;
    }
	if(colourChanged==1) //prevents dividing with each cycle -> less HW intensive; re-calculates colour vector only when DMX colour changes
	{
		colourChangeVector[0]=((int16_t)colourTable[secondaryColour].r-(int16_t)colourTable[primaryColour].r)/(float)colourMaxStep; //144-1 = max step count step count
		colourChangeVector[1]=((int16_t)colourTable[secondaryColour].g-(int16_t)colourTable[primaryColour].g)/(float)colourMaxStep;
		colourChangeVector[2]=((int16_t)colourTable[secondaryColour].b-(int16_t)colourTable[primaryColour].b)/(float)colourMaxStep;
		colourChangeVector[3]=((int16_t)colourTable[secondaryColour].w-(int16_t)colourTable[primaryColour].w)/(float)colourMaxStep;
		colourChanged=0;
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		//uint16_t ledNum = (i + localStep) % LEDCOUNT;
		uint16_t ledNum = i + step;
		if (ledNum >= LEDCOUNT) //instead of modulo
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum, //firstStep=0 - sets half of the strip to it's gradient colour; then moves effect one LED up
						round(colourTable[primaryColour].r+colourChangeVector[0]*(float)i),
						round(colourTable[primaryColour].g+colourChangeVector[1]*(float)i),
						round(colourTable[primaryColour].b+colourChangeVector[2]*(float)i));
	    ARGB_SetWhite(ledNum,
	    				round(colourTable[primaryColour].w+colourChangeVector[3]*(float)i));
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = colourMaxStep + i + step;
		if (ledNum >= LEDCOUNT) //instead of modulo
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum,
						round(colourTable[secondaryColour].r-colourChangeVector[0]*(float)i),
						round(colourTable[secondaryColour].g-colourChangeVector[1]*(float)i),
						round(colourTable[secondaryColour].b-colourChangeVector[2]*(float)i));
		ARGB_SetWhite(ledNum,
						round(colourTable[secondaryColour].w-colourChangeVector[3]*(float)i));
	}
    ARGB_Show();
}

static void effect_moving_gradient_reverse(void) //ToDo: MERGE with effect_moving_gradient
{
	//rather than saving data for each LED into an array and the shifting it, program calculates it each time the timer runs
	//uint16_t localStep = step%144;
	static float colourChangeVector[4];
	static uint8_t colourMaxStep=LEDCOUNT/2;
    if(step>=LEDCOUNT)
    {
    	step=0;
    }
	if(colourChanged==1) //prevents dividing with each cycle -> less HW intensive; re-calculates colour vector only when DMX colour changes
	{
		colourChangeVector[0]=((int16_t)colourTable[secondaryColour].r-(int16_t)colourTable[primaryColour].r)/(float)colourMaxStep; //144-1 = max step count step count
		colourChangeVector[1]=((int16_t)colourTable[secondaryColour].g-(int16_t)colourTable[primaryColour].g)/(float)colourMaxStep;
		colourChangeVector[2]=((int16_t)colourTable[secondaryColour].b-(int16_t)colourTable[primaryColour].b)/(float)colourMaxStep;
		colourChangeVector[3]=((int16_t)colourTable[secondaryColour].w-(int16_t)colourTable[primaryColour].w)/(float)colourMaxStep;
		colourChanged=0;
	}
	uint16_t reverseStep = LEDCOUNT - step;
	if (reverseStep >= LEDCOUNT)
	{
		reverseStep = 0;
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		//uint16_t ledNum = (i + localStep) % LEDCOUNT;
		uint16_t ledNum = i + reverseStep;
		if (ledNum >= LEDCOUNT) //instead of modulo
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum, //firstStep=0 - sets half of the strip to it's gradient colour; then moves effect one LED up
						round(colourTable[primaryColour].r+colourChangeVector[0]*(float)i),
						round(colourTable[primaryColour].g+colourChangeVector[1]*(float)i),
						round(colourTable[primaryColour].b+colourChangeVector[2]*(float)i));
	    ARGB_SetWhite(ledNum,
	    				round(colourTable[primaryColour].w+colourChangeVector[3]*(float)i));
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = colourMaxStep + i + reverseStep;
		if (ledNum >= LEDCOUNT) //instead of modulo
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum,
						round(colourTable[secondaryColour].r-colourChangeVector[0]*(float)i),
						round(colourTable[secondaryColour].g-colourChangeVector[1]*(float)i),
						round(colourTable[secondaryColour].b-colourChangeVector[2]*(float)i));
		ARGB_SetWhite(ledNum,
						round(colourTable[secondaryColour].w-colourChangeVector[3]*(float)i));
	}
    ARGB_Show();
}

static void effect_moving_gradient_reverse_faster(void) //ToDo: MERGE with effect_moving_gradient_faster
{
	//rather than saving data for each LED into an array and the shifting it, program calculates it each time the timer runs
	static float colourChangeVector[4];
	static uint8_t colourMaxStep=LEDCOUNT/2;
	uint16_t localStep = (step * modifier)%LEDCOUNT; //modifies step -> making animation faster (must be modulo so it doesn't step out of strip - just get onto the other end)
	localStep = LEDCOUNT - localStep; //reverse
	if(colourChanged==1) //prevents dividing with each cycle -> less HW intensive; re-calculates colour vector only when DMX colour changes
	{
		colourChangeVector[0]=((int16_t)colourTable[secondaryColour].r-(int16_t)colourTable[primaryColour].r)/(float)colourMaxStep; //144-1 = max step count step count
		colourChangeVector[1]=((int16_t)colourTable[secondaryColour].g-(int16_t)colourTable[primaryColour].g)/(float)colourMaxStep;
		colourChangeVector[2]=((int16_t)colourTable[secondaryColour].b-(int16_t)colourTable[primaryColour].b)/(float)colourMaxStep;
		colourChangeVector[3]=((int16_t)colourTable[secondaryColour].w-(int16_t)colourTable[primaryColour].w)/(float)colourMaxStep;
		colourChanged=0;
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = i + localStep;
		if (ledNum >= LEDCOUNT) //instead of modulo, that prevents animation from going out of bounds during !each cycle! -> still needed
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum, //firstStep=0 - sets half of the strip to it's gradient colour; then moves effect one LED up
						round(colourTable[primaryColour].r+colourChangeVector[0]*(float)i),
						round(colourTable[primaryColour].g+colourChangeVector[1]*(float)i),
						round(colourTable[primaryColour].b+colourChangeVector[2]*(float)i));
	    ARGB_SetWhite(ledNum,
	    				round(colourTable[primaryColour].w+colourChangeVector[3]*(float)i));
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = colourMaxStep + i + localStep;
		if (ledNum >= LEDCOUNT) //instead of modulo
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum,
						round(colourTable[secondaryColour].r-colourChangeVector[0]*(float)i),
						round(colourTable[secondaryColour].g-colourChangeVector[1]*(float)i),
						round(colourTable[secondaryColour].b-colourChangeVector[2]*(float)i));
		ARGB_SetWhite(ledNum,
						round(colourTable[secondaryColour].w-colourChangeVector[3]*(float)i));
	}
    ARGB_Show();
}


static void effect_moving_gradient_faster(void) //ToDo: Maybe merge into effect_moving_gradient, if there's not enough space in flash
{
	//rather than saving data for each LED into an array and the shifting it, program calculates it each time the timer runs
	static float colourChangeVector[4];
	static uint8_t colourMaxStep=LEDCOUNT/2;
	uint16_t localStep = (step * modifier)%LEDCOUNT; //modifies step -> making animation faster (must be modulo so it doesn't step out of strip - just get onto the other end)
	if(colourChanged==1) //prevents dividing with each cycle -> less HW intensive; re-calculates colour vector only when DMX colour changes
	{
		colourChangeVector[0]=((int16_t)colourTable[secondaryColour].r-(int16_t)colourTable[primaryColour].r)/(float)colourMaxStep; //144-1 = max step count step count
		colourChangeVector[1]=((int16_t)colourTable[secondaryColour].g-(int16_t)colourTable[primaryColour].g)/(float)colourMaxStep;
		colourChangeVector[2]=((int16_t)colourTable[secondaryColour].b-(int16_t)colourTable[primaryColour].b)/(float)colourMaxStep;
		colourChangeVector[3]=((int16_t)colourTable[secondaryColour].w-(int16_t)colourTable[primaryColour].w)/(float)colourMaxStep;
		colourChanged=0;
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = i + localStep;
		if (ledNum >= LEDCOUNT) //instead of modulo, that prevents animation from going out of bounds during !each cycle! -> still needed
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum, //firstStep=0 - sets half of the strip to it's gradient colour; then moves effect one LED up
						round(colourTable[primaryColour].r+colourChangeVector[0]*(float)i),
						round(colourTable[primaryColour].g+colourChangeVector[1]*(float)i),
						round(colourTable[primaryColour].b+colourChangeVector[2]*(float)i));
	    ARGB_SetWhite(ledNum,
	    				round(colourTable[primaryColour].w+colourChangeVector[3]*(float)i));
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = colourMaxStep + i + localStep;
		if (ledNum >= LEDCOUNT) //instead of modulo
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum,
						round(colourTable[secondaryColour].r-colourChangeVector[0]*(float)i),
						round(colourTable[secondaryColour].g-colourChangeVector[1]*(float)i),
						round(colourTable[secondaryColour].b-colourChangeVector[2]*(float)i));
		ARGB_SetWhite(ledNum,
						round(colourTable[secondaryColour].w-colourChangeVector[3]*(float)i));
	}
    ARGB_Show();
}

static void effect_glitchy_gradient(void) //safe thanks to ARGB library
{
	static float colourChangeVector[4];
	static uint8_t colourMaxStep=LEDCOUNT/2;
	uint16_t localStep = step%144;
	if(colourChanged==1)
	{
		colourChangeVector[0]=((int16_t)colourTable[secondaryColour].r-(int16_t)colourTable[primaryColour].r)/(float)colourMaxStep; //144-1 = max step count step count
		colourChangeVector[1]=((int16_t)colourTable[secondaryColour].g-(int16_t)colourTable[primaryColour].g)/(float)colourMaxStep;
		colourChangeVector[2]=((int16_t)colourTable[secondaryColour].b-(int16_t)colourTable[primaryColour].b)/(float)colourMaxStep;
		colourChangeVector[3]=((int16_t)colourTable[secondaryColour].w-(int16_t)colourTable[primaryColour].w)/(float)colourMaxStep;
		colourChanged=0;
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = (i+localStep) % LEDCOUNT;
		ARGB_SetRGB(ledNum,
						round(colourTable[primaryColour].r+colourChangeVector[0]*(float)i),
						round(colourTable[primaryColour].g+colourChangeVector[1]*(float)i),
						round(colourTable[primaryColour].b+colourChangeVector[2]*(float)i));
	    ARGB_SetWhite(ledNum,
	    				round(colourTable[primaryColour].w+colourChangeVector[3]*(float)i));
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = (colourMaxStep+i-step) % LEDCOUNT;
		ARGB_SetRGB(ledNum,
						round(colourTable[secondaryColour].r-colourChangeVector[0]*(float)i),
						round(colourTable[secondaryColour].g-colourChangeVector[1]*(float)i),
						round(colourTable[secondaryColour].b-colourChangeVector[2]*(float)i));
		ARGB_SetWhite(ledNum,
						round(colourTable[secondaryColour].w-colourChangeVector[3]*(float)i));
	}
    ARGB_Show();
}

//rozdeli obrazovku na dve casti - dva for cykly - v kazdem pripravi rozsviceni prislusne casti - stavajici polovinu posune o pixel nahoru. Pokud uz presahuje nad LEDCOUNT - udela modulo -> respektive odecte LEDCOUNT cimz presahujici diody "zalomi" zpet na zacatek
static void effect_moving_line(void) //made thx to a bug...  don't judge the code...
{
	static uint8_t colourMaxStep=LEDCOUNT/2;
	if(step>=LEDCOUNT)
    {
    	step=0;
    }
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = i + step;
		if (ledNum >= LEDCOUNT)
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum,
						colourTable[primaryColour].r,
						colourTable[primaryColour].g,
						colourTable[primaryColour].b);
	    ARGB_SetWhite(ledNum,
	    				colourTable[primaryColour].w);
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = colourMaxStep + i + step;
		if (ledNum >= LEDCOUNT) //instead of modulo
		{
			ledNum = ledNum - LEDCOUNT;
		}
		ARGB_SetRGB(ledNum,
						colourTable[secondaryColour].r,
						colourTable[secondaryColour].g,
						colourTable[secondaryColour].b);
		ARGB_SetWhite(ledNum,
						colourTable[secondaryColour].w);
	}
    ARGB_Show();
}

static void effect_glitchy(void) //made thx to a bug... don't judge the code...
{
	static uint8_t colourMaxStep=LEDCOUNT/2;
	//float colourChangeVector[4];
	uint16_t localStep = step%144;
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = (i+localStep) % LEDCOUNT;
		ARGB_SetRGB(ledNum,
						colourTable[primaryColour].r,
						colourTable[primaryColour].g,
						colourTable[primaryColour].b);
	    ARGB_SetWhite(ledNum,
	    				colourTable[primaryColour].w);
	}
	for(uint16_t i=0; i<colourMaxStep; i++)
	{
		uint16_t ledNum = (colourMaxStep+i-step) % LEDCOUNT;
		ARGB_SetRGB(ledNum,
						colourTable[secondaryColour].r,
						colourTable[secondaryColour].g,
						colourTable[secondaryColour].b);
		ARGB_SetWhite(ledNum,
						colourTable[secondaryColour].w);
	}
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

static void effect_strobe_fading(void) //ToDo: VZTAHNOUT JAS K MAX JASU; POKLES JASU BY MEL RESPEKTOVAT LOGARITMICKOU KRIVKU VNIMANI
{
	static float brightness=0;
	float brightnessStep=255.0f/(modifier/2.0f-1.0f);
	if(step>modifier-1)
	{
        step=0;
	}
    if(step<modifier/2-1)
    {
        brightness+=brightnessStep;
    }
    else if(step>modifier/2-1&&step<=modifier-2)
    {
        brightness-=brightnessStep;
    }
    ARGB_FillRGB((colourTable[primaryColour].r*(uint8_t)brightness)/255, (colourTable[primaryColour].g*(uint8_t)brightness)/255, (colourTable[primaryColour].b*(uint8_t)brightness)/255);
    ARGB_FillWhite((colourTable[primaryColour].w*(uint8_t)brightness)/255);
    ARGB_Show();

}

/*static void effect_strobe_fading_64(void)
{									   //ToDo: VZTAHNOUT JAS K MAX JASU
	static float brightness=0;
	if(step>63)
	{
        step=0;
	}
    if(step<31)
    {
        brightness=brightness+8.22f;
    }
    else if(step>31&&step<=62)
    {
        brightness=brightness-8.22f;
    }
    ARGB_FillRGB((colourTable[primaryColour].r*(uint8_t)brightness)/255, (colourTable[primaryColour].g*(uint8_t)brightness)/255, (colourTable[primaryColour].b*(uint8_t)brightness)/255);
    ARGB_FillWhite((colourTable[primaryColour].w*(uint8_t)brightness)/255);
    ARGB_Show();

}*/

static void effect_moving_dots(void) //AI GENEROVÁNO!
{
    // Konfigurace vlastností teček
    // speed: Kladná = vpřed, záporná = vzad. Hodnota 10 znamená posun o 1 LED za krok.
    //        (5 znamená půl LED za krok atd. - umožňuje to plynulejší různé rychlosti)
    // tail_length: Určuje vizuální velikost/intenzitu tečky.
    typedef struct {
        int8_t speed;
        uint8_t tail_length;
    } DotDef_t;

    // Fixní definice pro 10 nezávislých teček (můžeš si s parametry pohrát)
    static const DotDef_t dots[10] = {
        { 10, 15},  // Normální rychlost vpřed, střední intenzita
        {-15, 20},  // Rychlejší vzad, vysoká intenzita (dlouhý ocas)
        {  5,  8},  // Pomalá vpřed, malá intenzita
        {-10, 12},  // Normální vzad
        { 20, 18},  // Velmi rychlá vpřed, silná
        { -5, 10},  // Pomalá vzad
        { 12, 14},  // Středně rychlá vpřed
        {-18, 16},  // Rychlá vzad
        {  8,  9},  // ...
        {-12, 11}
    };

    // Statické pole pro uchování reálných pozic (násobeno 10 pro desetinnou přesnost).
    // Začínají rozmístěné pseudo-náhodně po pásku (0, 45.0, 89.0 atd.).
    // Upozornění: Při resetu efektu se nevrátí na začátek, pokračují tam, kde skončily.
    static int32_t dot_positions[10] = {0, 450, 890, 230, 1010, 640, 130, 1200, 750, 330};

    // Z modifieru určíme počet aktivních teček (např. 1 až 10)
    uint8_t num_dots = modifier > 0 ? modifier : 1;
    if (num_dots > 10) num_dots = 10;

    // 1. Vyplnění podkladovou barvou
    ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    ARGB_FillWhite(colourTable[primaryColour].w);

    // Výpočet celkového rozdílu barev pro lineární interpolaci gradientu (ocasů)
    int16_t r_diff = (int16_t)colourTable[primaryColour].r - (int16_t)colourTable[secondaryColour].r;
    int16_t g_diff = (int16_t)colourTable[primaryColour].g - (int16_t)colourTable[secondaryColour].g;
    int16_t b_diff = (int16_t)colourTable[primaryColour].b - (int16_t)colourTable[secondaryColour].b;
    int16_t w_diff = (int16_t)colourTable[primaryColour].w - (int16_t)colourTable[secondaryColour].w;

    // 2. Vykreslení teček
    for (uint8_t i = 0; i < num_dots; i++)
    {
        // Posun tečky
        dot_positions[i] += dots[i].speed;

        // Ošetření přetečení pásku (udržení v mezích 0 až LEDCOUNT * 10)
        if (dot_positions[i] >= LEDCOUNT * 10) {
            dot_positions[i] -= LEDCOUNT * 10;
        } else if (dot_positions[i] < 0) {
            dot_positions[i] += LEDCOUNT * 10;
        }

        // Získání reálné hlavy tečky
        int16_t head_pos = dot_positions[i] / 10;

        // Směr ocasu (ocas kreslíme do protisměru)
        int8_t tail_dir = (dots[i].speed > 0) ? -1 : 1;
        uint8_t current_tail = dots[i].tail_length;

        // Vykreslení samotného ocasu a hlavy
        for (uint8_t j = 0; j <= current_tail; j++)
        {
            // Pozice pixelu ocasu na pásku
            int16_t draw_pos = head_pos + (j * tail_dir);

            // Zalamování ocasu přes konce pásku
            if (draw_pos >= LEDCOUNT) draw_pos -= LEDCOUNT;
            else if (draw_pos < 0) draw_pos += LEDCOUNT;

            // Výpočet barvy rychlou celočíselnou interpolací (0 = plná hlava, current_tail = plný podklad)
            uint8_t r = colourTable[secondaryColour].r + (r_diff * j) / current_tail;
            uint8_t g = colourTable[secondaryColour].g + (g_diff * j) / current_tail;
            uint8_t b = colourTable[secondaryColour].b + (b_diff * j) / current_tail;
            uint8_t w = colourTable[secondaryColour].w + (w_diff * j) / current_tail;

            ARGB_SetRGB((uint16_t)draw_pos, r, g, b);
            ARGB_SetWhite((uint16_t)draw_pos, w);
        }
    }
    ARGB_Show();
}

static void effect_jumping(void) //AI GENEROVÁNO!
{
	static const float H = 143.0f;
	static const float beatsPerJump = 2.0f;
	static float phase = 0.0f; //immediate phase of sinus curve
	static float f;
	static float phaseInc;
	static float y;
	f = bpm / 60.0f / beatsPerJump;  //BPM to jump frequency in Hz //ToDo: don't have to repeat each cycle! REMOVE
    phaseInc = 2.0f * M_PI * f / 120.0f; //increment

	phase += phaseInc;
	if (phase > 2.0f * M_PI)
		phase -= 2.0f * M_PI;
	y = 0.5f * H * (1.0f - cosf(phase));

	ARGB_Clear();
	ARGB_SetRGB((uint8_t) (y + 0.5f), colourTable[primaryColour].r,
			colourTable[primaryColour].g, colourTable[primaryColour].b);
	ARGB_SetWhite((uint8_t) (y + 0.5f), colourTable[primaryColour].w);
	ARGB_Show();
}

static void effect_jumping_own(void)
{
	static float y = 100.0f; //initial pos; max jump height
	static float vy = 0.0f;  //initial speed
	static const float g = -0.01f; //gravity, +- changes orientation //-0.03f

	vy += g;
	y += vy;

	if (y > 143.0f) //if top reached
			{
		y = 143.0f - (y - 143.0f); //fixes attenuation
		vy *= -1.0f; //changes direction of speed
	}
	if (y < 0.0f) //if bottom reached
			{
		y = -y;
		vy *= -1.0f;
	}
	ARGB_Clear();
	ARGB_SetRGB((uint8_t) (y + 0.5f), colourTable[primaryColour].r,
			colourTable[primaryColour].g, colourTable[primaryColour].b); //rounds and casts to int
	ARGB_SetWhite((uint8_t) (y + 0.5f), colourTable[primaryColour].w);
	ARGB_Show();
}

static void effect_falling_drop(void)
{
	static uint8_t colourMaxStep=132; //defines a step, where colour stops changing; maximum is maxAnimationSteps-1
	static float colourChangeVector[4];
	static float currentColourChanged[4];
	if(step>=LEDCOUNT)
	{
		step=0;
		currentColourChanged[0]=colourTable[primaryColour].r;
		currentColourChanged[1]=colourTable[primaryColour].g;
		currentColourChanged[2]=colourTable[primaryColour].b;
		currentColourChanged[3]=colourTable[primaryColour].w;
	}
	if(colourChanged==1)
	{
		 colourChangeVector[0]=(colourTable[secondaryColour].r-colourTable[primaryColour].r)/(float)colourMaxStep; //144-1 = max step count step count
		 colourChangeVector[1]=(colourTable[secondaryColour].g-colourTable[primaryColour].g)/(float)colourMaxStep;
		 colourChangeVector[2]=(colourTable[secondaryColour].b-colourTable[primaryColour].b)/(float)colourMaxStep;
		 colourChangeVector[3]=(colourTable[secondaryColour].w-colourTable[primaryColour].w)/(float)colourMaxStep;

		 currentColourChanged[0]=colourTable[primaryColour].r; //A: faster to compute
		 currentColourChanged[1]=colourTable[primaryColour].g;
		 currentColourChanged[2]=colourTable[primaryColour].b;
		 currentColourChanged[3]=colourTable[primaryColour].w;

		 colourChanged=0;
	}
	if(step!=0)
	{
		 currentColourChanged[0]+=colourChangeVector[0];
		 currentColourChanged[1]+=colourChangeVector[1];
		 currentColourChanged[2]+=colourChangeVector[2];
		 currentColourChanged[3]+=colourChangeVector[3];
	}
	ARGB_Clear();
	ARGB_SetRGB(LEDCOUNT-step-1,
	(uint8_t)fminf(fmaxf((currentColourChanged[0]+0.5f), 0.0f), 255.0f), //ToDo: Add clamps - there can be blinking due to float inconsitency, or colourMaxStep-1
	(uint8_t)fminf(fmaxf((currentColourChanged[1]+0.5f), 0.0f), 255.0f), //0.5f smooth round
	(uint8_t)fminf(fmaxf((currentColourChanged[2]+0.5f), 0.0f), 255.0f));//(uint8_t)fminf(fmaxf((currentColourChanged[2]+0.5f), 0.0f), 255.0f));
	ARGB_SetWhite(LEDCOUNT-step-1, (uint8_t)fminf(fmaxf((currentColourChanged[3]+0.5f), 0.0f), 255.0f));
	ARGB_Show();
}

static void effect_odd_even(void)
{
	ARGB_Clear();
	if(step>1)
	{
        step=0;
	}
	if(step==0)
	{
        for(uint16_t i=0; i<LEDCOUNT; i+=2)
        {
        	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
        	ARGB_SetWhite(i,colourTable[primaryColour].w);
        }
	}
	else
	{
        for(uint16_t i=1; i<LEDCOUNT; i+=2)
        {
        	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
        	ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }
	}
    ARGB_Show();
}

static void effect_sectors_fadein(void)//ToDo: optimize!
{
	if(step<8)
	{
		switch(step)
		{
		case 0:
			for(uint16_t i=0; i<18; i++)
			{
				ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
				ARGB_SetWhite(i,colourTable[primaryColour].w);
				ARGB_Show();
			}
			break;
		case 1:
			for(uint16_t i=126; i<144; i++)
			{
				ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
				ARGB_SetWhite(i,colourTable[primaryColour].w);
				ARGB_Show();
			}
		break;
		case 2:
			for(uint16_t i=18; i<36; i++)
			{
	    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
				ARGB_Show();
			}
		break;
		case 3:
			for(uint16_t i=108; i<126; i++)
			{
	    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
				ARGB_Show();
			}
		break;
		case 4:
			for(uint16_t i=36; i<54; i++)
			{
				ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
				ARGB_SetWhite(i,colourTable[primaryColour].w);
				ARGB_Show();
			}
		break;
		case 5:
			for(uint16_t i=90; i<108; i++)
			{
				ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
				ARGB_SetWhite(i,colourTable[primaryColour].w);
				ARGB_Show();
			}
		break;
		case 6:
			for(uint16_t i=54; i<72; i++)
			{
	    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
				ARGB_Show();
			}
		break;
		case 7:
			for(uint16_t i=72; i<90; i++)
			{
	    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
				ARGB_Show();
				//ARGB_Clear();
			}
		break;
		}
	}

}

static void effect_sectors_blinking(void)
{
	if(step>16)
	{
        step=0;
	}
	switch(step)
	{
	case 3:
    	for(uint16_t i=0; i<18; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
        break;
	case 0:
    	for(uint16_t i=126; i<144; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 1:
    	for(uint16_t i=18; i<36; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 7:
    	for(uint16_t i=108; i<126; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 5:
    	for(uint16_t i=36; i<54; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 2:
    	for(uint16_t i=90; i<108; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 6:
    	for(uint16_t i=54; i<72; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 4:
    	for(uint16_t i=72; i<90; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 8 ... 14:
    break;
	case 15:
		ARGB_Clear();
	break;
	}
    ARGB_Show();
}

static void effect_sectors(void)
{
	ARGB_Clear();
	if(step>7)
	{
        step=0;
	}
	switch(step)
	{
	case 0:
    	for(uint16_t i=0; i<18; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
        break;
	case 1:
    	for(uint16_t i=126; i<144; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 2:
    	for(uint16_t i=18; i<36; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 3:
    	for(uint16_t i=108; i<126; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 4:
    	for(uint16_t i=36; i<54; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 5:
    	for(uint16_t i=90; i<108; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 6:
    	for(uint16_t i=54; i<72; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 7:
    	for(uint16_t i=72; i<90; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	}
    ARGB_Show();
}

static void effect_sectors_together(void)
{
	ARGB_Clear();
	if(step>3)
	{
        step=0;
	}
	switch(step)
	{
	case 0:
    	for(uint16_t i=0; i<18; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    	for(uint16_t i=126; i<144; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
        break;
	case 1:
    	for(uint16_t i=18; i<36; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    	for(uint16_t i=108; i<126; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 2:
    	for(uint16_t i=36; i<54; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    	for(uint16_t i=90; i<108; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 3:
    	for(uint16_t i=54; i<72; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    	for(uint16_t i=72; i<90; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	}
    ARGB_Show();
}

static void effect_sectors_backAndForth(void)
{

	if(step>15)
	{
        step=0;
        //ARGB_Clear();
	}
	switch(step)
	{
	case 3:
    	for(uint16_t i=0; i<18; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
        break;
	case 0:
    	for(uint16_t i=126; i<144; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 1:
    	for(uint16_t i=18; i<36; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 7:
    	for(uint16_t i=108; i<126; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 5:
    	for(uint16_t i=36; i<54; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 2:
    	for(uint16_t i=90; i<108; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 6:
    	for(uint16_t i=54; i<72; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 4:
    	for(uint16_t i=72; i<90; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 9:
    	for(uint16_t i=0; i<18; i++)
    	{
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
    	}
        break;
	case 15:
    	for(uint16_t i=126; i<144; i++)
    	{
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
    	}
    break;
	case 8:
    	for(uint16_t i=18; i<36; i++)
    	{
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
    	}
    break;
	case 11:
    	for(uint16_t i=108; i<126; i++)
    	{
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
    	}
    break;
	case 13:
    	for(uint16_t i=36; i<54; i++)
    	{
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
    	}
    break;
	case 10:
    	for(uint16_t i=90; i<108; i++)
    	{
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
    	}
    break;
	case 12:
    	for(uint16_t i=54; i<72; i++)
    	{
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
    	}
    break;
	case 14:
    	for(uint16_t i=72; i<90; i++)
    	{
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
    	}
    break;
	}
    ARGB_Show();
}

static void effect_sectors_random(void)
{
	ARGB_Clear();
	if(step>7)
	{
        step=0;
	}
	switch(step)
	{
	case 3:
    	for(uint16_t i=0; i<18; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
        break;
	case 0:
    	for(uint16_t i=126; i<144; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 1:
    	for(uint16_t i=18; i<36; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 7:
    	for(uint16_t i=108; i<126; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	case 5:
    	for(uint16_t i=36; i<54; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 2:
    	for(uint16_t i=90; i<108; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 6:
    	for(uint16_t i=54; i<72; i++)
    	{
    		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    		ARGB_SetWhite(i,colourTable[primaryColour].w);
    	}
    break;
	case 4:
    	for(uint16_t i=72; i<90; i++)
    	{
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
    	}
    break;
	}
    ARGB_Show();
}

void effect_random_static(void)
{
	static uint8_t isDone=0;
	if(isDone==0)
	{
		for (uint16_t i = 0; i < LEDCOUNT; i++)
		{
			uint8_t br = rand() % 255;
			uint8_t r = rand() % 255;
			uint8_t g = rand() % 255;
			uint8_t b = rand() % 255;
			uint8_t w = rand() % 5;
			ARGB_SetBrightness(br);
			ARGB_SetRGB(i, r, g, b);
			ARGB_SetWhite(i, w);
		}
		ARGB_Show();
		isDone=1;
	}
}
static void effect_slide_bottom_keepLowest(void)
{
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        //limit=0;
        ARGB_Clear();
	}
	limit=(uint16_t)(((float)LEDCOUNT/((float)modifier-1))*step);
	for(uint16_t i=0; i<limit; i++)
	{
		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_SetWhite(i,colourTable[primaryColour].w);
	}
	ARGB_Show();
}

static void effect_slide_bottom(void)
{
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        //limit=0;
        ARGB_Clear();
	}
	limit=(uint16_t)(((float)LEDCOUNT/((float)modifier-1))*step);
	for(uint16_t i=0; i<limit; i++)
	{
		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_SetWhite(i,colourTable[primaryColour].w);
	}
	ARGB_Show();
}
static void effect_slide_top(void)
{
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        //limit=0;
        ARGB_Clear();
	}
	limit=(uint16_t)(((float)LEDCOUNT/((float)modifier-1))*step);
	for(int16_t i=LEDCOUNT-1; i>=LEDCOUNT-limit; i--)
	{
		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_SetWhite(i,colourTable[primaryColour].w);
	}
	ARGB_Show();
}
static void effect_slide_backAndForth_special(void) //ToDo: Double colour - second colour can be zero
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction = !direction;
        ARGB_Clear();
	}
	limit=(uint16_t)(((float)LEDCOUNT/((float)modifier-1))*step);
	if(direction==1)
	{
		for(uint16_t i=0; i<limit; i++)
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
	}
	else
	{
		for(int16_t i=LEDCOUNT-1; i>=LEDCOUNT-limit; i--)
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	ARGB_Show();
}

static void effect_slide_backAndForth(void) //ToDo: Double colour - second colour can be zero
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction = !direction;
        //ARGB_Clear();
	}
	limit=(uint16_t)(((float)LEDCOUNT/((float)modifier-1))*step);
	if(direction==1)
	{
		for(uint16_t i=0; i<limit; i++)
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
	}
	else
	{
		for(int16_t i=LEDCOUNT-1; i>=LEDCOUNT-limit; i--)
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	ARGB_Show();
}

/*static void effect_slide_backAndForth(void) //ZUSTAVA SVITIT POSLEDNI DIODA!
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction = !direction;
        //ARGB_Clear();
	}
	limit=(uint16_t)((144.0f/((float)modifier-1))*step-1);
	if(direction==1)
	{
		for(uint16_t i=0; i<limit; i++)
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
	}
	else
	{
		for(uint16_t i=143; i>143-limit; i--)
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	ARGB_Show();
}*/
/*
static void effect_middle(void)
{
	modifier=64;
	if(step>modifier-1)
	{
        step=0;
        ARGB_Clear();
	}
	for(uint8_t i=60; i<84; i++) //FILL
	{
		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_SetWhite(i,colourTable[primaryColour].w);
	}
        for(uint16_t i=59; i>59-step*(64/modifier); i--) //DIRECTION
        {
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }
        for(uint16_t i=84; i<84+step*(64/modifier); i++) //DIRECTION
        {
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }

    ARGB_Show();
}*/
static void effect_fromMiddle(void)
{
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
    for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
    {
    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    	ARGB_SetWhite(i,colourTable[primaryColour].w);
    }
    for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
    {
    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
    }
	ARGB_Show();
}
static void effect_fromMiddle_trueZero(void) //DIMS COMPLETELY IN ZERO
{
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
	if(step>0) //ToDo: Can be moved into own function
	{
		for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
		for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	ARGB_Show();
}
static void effect_fromMiddle_dim(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
        //ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
	if(direction==0) //ToDo: Can be moved into own function
	{
		for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
		for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
	}
	else
	{
		for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
		for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	ARGB_Show();
}
static void effect_fromMiddle_dim_special(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
        if(modifier2==true)
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
	if(direction==0) //ToDo: Can be moved into own function
	{
		for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
		for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	else
	{
		for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
		for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
	}
	ARGB_Show();
}

static void effect_fromMiddle_dim_special2(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
	if(direction==0) //ToDo: Can be moved into own function
	{
		for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
		for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	else
	{
		for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
		for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	ARGB_Show();
}

static void effect_fromEdge(void)
{
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step)+1;
	//if(step>0) //ToDo: Can be moved into own function
	//{
    for(uint16_t i=0; i<limit; i++)
    {
    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    	ARGB_SetWhite(i,colourTable[primaryColour].w);
    }
    for(int16_t i=143; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
    {
    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
    }
	//}
	ARGB_Show();
}

static void effect_fromEdge_trueZero(void)
{
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step)+1;
	if(step>0) //ToDo: Can be moved into own function
	{
		for(uint16_t i=0; i<limit; i++)
		{
			ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			ARGB_SetWhite(i,colourTable[primaryColour].w);
		}
		for(int16_t i=143; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
		{
			ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
			ARGB_SetWhite(i,colourTable[secondaryColour].w);
		}
	}
	ARGB_Show();
}

static void effect_fromEdge_dim(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
        //ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step)+1;
	if(direction==0) //ToDo: Can be moved into own function
	{
	    for(uint16_t i=0; i<limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	    for(int16_t i=143; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	}
	else
	{
	    for(uint16_t i=0; i<limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	    for(int16_t i=143; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	}
	ARGB_Show();
}

static void effect_fromEdge_backAndForth_special(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
	if(direction==0) //ToDo: Can be moved into own function
	{
	    for(uint16_t i=0; i<limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	    for(int16_t i=LEDCOUNT-1; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	}
	else
	{
	    for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	    for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	}
	ARGB_Show();
}
static void effect_fromEdge_backAndForth(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
        if(modifier2==1)
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
	if(direction==0) //ToDo: Can be moved into own function
	{
	    for(uint16_t i=0; i<limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	    for(int16_t i=LEDCOUNT-1; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	}
	else
	{
	    for(uint16_t i=LEDCOUNT/2+1; i<LEDCOUNT/2+limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	    for(int16_t i=LEDCOUNT/2; i>=LEDCOUNT/2-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	}
	ARGB_Show();
}


static void effect_fromEdge_dim_special(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
        if(modifier2==true)
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step)+1;
	if(direction==0) //ToDo: Can be moved into own function
	{
	    for(uint16_t i=0; i<limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	    for(int16_t i=143; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	}
	else
	{
	    for(uint16_t i=0; i<limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	    for(int16_t i=143; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	}
	ARGB_Show();
}

static void effect_fromEdge_dim_special2(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
        ARGB_Clear();
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step)+1;
	if(direction==0) //ToDo: Can be moved into own function
	{
	    for(uint16_t i=0; i<limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	    for(int16_t i=143; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    	ARGB_SetWhite(i,colourTable[primaryColour].w);
	    }
	}
	else
	{
	    for(uint16_t i=0; i<limit; i++)
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	    for(int16_t i=143; i>=LEDCOUNT-limit; i--) //int - otherwise underflow
	    {
	    	ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    	ARGB_SetWhite(i,colourTable[secondaryColour].w);
	    }
	}
	ARGB_Show();
}

static void effect_twoDrops_fromEdge(void)
{
	uint16_t limit;
	ARGB_Clear();
	if(step>modifier-1)
	{
        step=0;
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
    ARGB_SetRGB(limit,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    ARGB_SetWhite(limit,colourTable[primaryColour].w);
    ARGB_SetRGB(LEDCOUNT-1-limit,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    ARGB_SetWhite(LEDCOUNT-1-limit,colourTable[secondaryColour].w);
	ARGB_Show();
}
static void effect_twoDrops_buggy(void)
{
	uint16_t limit;
	ARGB_Clear();
	if(step>modifier-1)
	{
        step=0;
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
    ARGB_SetRGB(72-limit,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    ARGB_SetWhite(72-limit,colourTable[primaryColour].w);
    ARGB_SetRGB(LEDCOUNT-1-limit,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    ARGB_SetWhite(LEDCOUNT-1-limit,colourTable[secondaryColour].w);
	ARGB_Show();
}
static void effect_twoDrops_fromMiddle(void)
{
	uint16_t limit;
	ARGB_Clear();
	if(step>modifier-1)
	{
        step=0;
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
    ARGB_SetRGB(72-limit,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
    ARGB_SetWhite(72-limit,colourTable[primaryColour].w);
    ARGB_SetRGB(72+limit-1,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    ARGB_SetWhite(72+limit-1,colourTable[secondaryColour].w);
	ARGB_Show();
}
static void effect_twoDrops_backAndForth(void)
{
	static uint8_t direction=0;
	uint16_t limit;
	ARGB_Clear();
	if(step>modifier-1)
	{
        step=0;
        direction=!direction;
	}
	limit=(uint16_t)((((float)LEDCOUNT/2)/((float)modifier-1))*step);
	if(direction==0)
	{
	    ARGB_SetRGB(72-limit,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    ARGB_SetWhite(72-limit,colourTable[primaryColour].w);
	    ARGB_SetRGB(72+limit-1,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    ARGB_SetWhite(72+limit-1,colourTable[secondaryColour].w);
	}
	else
	{
	    ARGB_SetRGB(limit,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
	    ARGB_SetWhite(limit,colourTable[primaryColour].w);
	    ARGB_SetRGB(LEDCOUNT-1-limit,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
	    ARGB_SetWhite(LEDCOUNT-1-limit,colourTable[secondaryColour].w);
	}
	ARGB_Show();
}

static void effect_drops(void)
{
	static uint16_t ticksFromStart=0;
	//POLE s pozicema
	static int16_t pole[20];
	ARGB_Clear();
	if(ticksFromStart<40&&ticksFromStart%2==0)
	{
		pole[ticksFromStart/2]=1;
	}
	for(uint8_t i=0; i<20; i++)
	{
		if(pole[i]!=0)
		{
		    ARGB_SetRGB(pole[i],colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		    ARGB_SetWhite(pole[i],colourTable[primaryColour].w);
		    pole[i]+=2;
		}
		if(pole[i]>144)
		{
			pole[i]=pole[i]%144;
		}
	}
	ARGB_Show();
	if(ticksFromStart<41) //prevents overflow
	ticksFromStart++;
}

static void effect_tubes(void) //ToDo: Rozdelit 6 trubic na cas ktery budou svitit, vymyslet vic efektu
{
	if(step>3)
	{
		step=0;
	}
	ARGB_Clear();
	switch(TUBENUMBER)
	{
	case 3 ... 4:
			if(step==0||step==1)
			{
			    ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			    ARGB_FillWhite(colourTable[primaryColour].w);
			}
		break;
	case 2:
	case 5:
			if(step==2)
			{
			    ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			    ARGB_FillWhite(colourTable[primaryColour].w);
			}
		break;
	case 1:
	case 6:
			if(step==3)
			{
			    ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
			    ARGB_FillWhite(colourTable[primaryColour].w);
			}
		break;
	}
	ARGB_Show();
}

static void effect_tubes_true_pingpong(void) //AI GENERATED - only for test
{
    if(step > 9)
    {
        step = 0;
    }

    ARGB_Clear();

    uint8_t active_tube;
    if (step <= 5)
    {
        active_tube = step + 1;
    }
    else
    {
        active_tube = 11 - step;
    }

    if (TUBENUMBER == active_tube)
    {
        ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
        ARGB_FillWhite(colourTable[primaryColour].w);
    }

    ARGB_Show();
}


static void effect_middle(void) //ToDo: Fix math - according to fromMiddle effect! use the "limit" variable!
{
	if(step>modifier-1)
	{
        step=0;
        ARGB_Clear();
	}
	for(uint8_t i=64; i<80; i++) //FILL
	{
		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_SetWhite(i,colourTable[primaryColour].w);
	}
        for(uint16_t i=63; i>63-step*(64/modifier); i--) //DIRECTION
        {
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }
        for(uint16_t i=80; i<80+step*(64/modifier); i++) //DIRECTION
        {
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }

    ARGB_Show();

}


static void effect_middle_bounce1(void) //ToDo: Fix math - according to fromMiddle effect!
{
	static uint8_t direction=0;
	if(step>modifier-1)
	{
        step=0;
        if(direction==0) //use negation instead
        {
        	direction=1;
        }
        else
        {
        	direction=0;
        }
	}
	for(uint8_t i=64; i<80; i++) //FILL
	{
		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_SetWhite(i,colourTable[primaryColour].w);
	}
	if(direction==0)
	{
        for(uint16_t i=63; i>63-step*4; i--) //DIRECTION
        {
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }
        for(uint16_t i=80; i<80+step*4; i++) //DIRECTION
        {
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }
	}
    if(direction==1)
    {
        for(uint16_t i=3; i<3+step*4; i++) //DIRECTION
        {
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
        }
        for(uint16_t i=140; i>140-step*4; i--) //DIRECTION
        {
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
        }
    }
    ARGB_Show();

}

static void effect_middle_bounce2(void) //ToDo: Fix math - according to fromMiddle effect!
{
	static uint8_t direction=0;
	if(step>63)
	{
        step=0;
        if(direction==0) //use negation instead
        {
        	direction=1;
        }
        else
        {
        	direction=0;
        }
	}
	for(uint8_t i=64; i<80; i++) //FILL
	{
		ARGB_SetRGB(i,colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
		ARGB_SetWhite(i,colourTable[primaryColour].w);
	}
	if(direction==0)
	{
        for(uint16_t i=63; i>63-step; i--) //DIRECTION
        {
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }
        for(uint16_t i=80; i<80+step; i++) //DIRECTION
        {
    		ARGB_SetRGB(i,colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
    		ARGB_SetWhite(i,colourTable[secondaryColour].w);
        }
	}
    if(direction==1)
    {
        for(uint16_t i=0; i<0+step; i++) //DIRECTION
        {
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
        }
        for(uint16_t i=143; i>143-step; i--) //DIRECTION
        {
    		ARGB_SetRGB(i,0,0,0);
    		ARGB_SetWhite(i,0);
        }
    }
    ARGB_Show();

}
/*
static void effect_middle_bounce2(void) //AI GENERATED!
{
    if (step >= modifier)
    {
        step = 0;
    }

    uint16_t half_cycle = modifier / 2;
    uint16_t current_phase;

    if (step <= half_cycle)
    {
        current_phase = step;
    } else {
        current_phase = modifier - step;
    }

    uint16_t max_expansion = 54;

    uint16_t expansion = (current_phase * max_expansion) / half_cycle;

    ARGB_Clear();

    for(uint16_t i = 64; i < 80; i++)
    {
        ARGB_SetRGB(i, colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
        ARGB_SetWhite(i, colourTable[primaryColour].w);
    }

    for(int16_t i = 63; i >= (63 - (int16_t)expansion); i--)
    {
        if (i >= 0)
        {
            ARGB_SetRGB(i, colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
            ARGB_SetWhite(i, colourTable[secondaryColour].w);
        }
    }

    for(int16_t i = 80; i <= (80 + (int16_t)expansion); i++)
    {
        if (i < LEDCOUNT)
        {
            ARGB_SetRGB(i, colourTable[secondaryColour].r, colourTable[secondaryColour].g, colourTable[secondaryColour].b);
            ARGB_SetWhite(i, colourTable[secondaryColour].w);
        }
    }

    ARGB_Show();
}*/

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
    ARGB_SetBrightness(new_brightness);
}

void effects_set_tempo(uint8_t new_bpm)
{
    bpm = new_bpm;
}

void effects_set_primaryColour(ColourName_t new_colour)
{
	primaryColour = new_colour;
	colourChanged=1;
}

void effects_set_secondaryColour(ColourName_t new_colour)
{
	secondaryColour = new_colour;
	colourChanged=1;
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
	uint16_t ARR;
	ownTempo=0;
	colourChanged=0;
    switch (effect1)
    {
        case 0 ... 1: //LIGHTS DOWN //--------------------------------------------------------------
            current_effect_func = effect_lights_down;
            PSC = 21972;
            multiplier = 0.1;

            break;
        case 2 ... 3: //STATIC COLOUR
			current_effect_func = effect_static;
        	PSC = 21972;
            multiplier = 0.1;
            break;
        case 13 ... 14: //STATIC TWO COLOUR - (CAN USE 0 colour!)
			current_effect_func = effect_static_two_colour;
        	PSC = 21972;
            multiplier = 0.1;
            break;
        case 15 ... 16: //STATIC TWO COLOUR w BRIGHTNESS
			//ToDo: maximal brightness should be related to brightness set by DMX channel!
			current_effect_func = effect_static_two_colour_brightness;
        	PSC = 21972;
            multiplier = 0.1;
            break;
        case 17 ... 18: //STATIC TWO COLOUR GRADIENT
			current_effect_func = effect_static_two_colour_gradient;
        	PSC = 21972;
            multiplier = 0.1;
            break;
         case 19 ... 20: //STATIC TWO COLOUR GRADIENT REVERSED
			current_effect_func = effect_static_two_colour_gradient_2;
         	multiplier = 1;
         	PSC = 21972;
         	ownTempo=0;
            break;
         case 21 ... 22: //MOVING GRADIENT; multiplier increases speed; modifier increases speed while decreasing smoothness and HW requirements
			current_effect_func = effect_moving_gradient;
            colourChanged=1;
            modifier=1;
            switch (effect2)
            {
				case 0 ... 18:    multiplier = 0.25;  PSC=8789;  break;
				case 19 ... 36:   multiplier = 0.5;   PSC=4394;  break;
				case 37 ... 54:   multiplier = 1;     PSC=2197;  break;
				case 55 ... 72:   multiplier = 2;     PSC=1098;  break;
				case 73 ... 90:   multiplier = 4;     PSC=549;   break;
				case 91 ... 108:  multiplier = 8;     PSC=274;   break;
				case 109 ... 126: multiplier = 16;    PSC=137;   break;
				case 127 ... 144: multiplier = 32;    PSC=68;    break;
				case 145 ... 162: multiplier = 64;    PSC=34;    break;
				case 163 ... 180: current_effect_func =effect_moving_gradient_faster; modifier=2;  multiplier = 32;    PSC=68;     break;
				case 181 ... 198: current_effect_func =effect_moving_gradient_faster; modifier=4;  multiplier = 32;    PSC=68;     break;
				case 199 ... 216: current_effect_func =effect_moving_gradient_faster; modifier=8;  multiplier = 32;    PSC=68;     break;
				case 217 ... 234: current_effect_func =effect_moving_gradient_faster; modifier=10; multiplier = 32;    PSC=68;     break;
				case 235 ... 255: current_effect_func =effect_moving_gradient_faster; modifier=16; multiplier = 32;    PSC=68;     break;
            }
            break;
         case 37 ... 38: //MOVING GRADIENT REVERSE; multiplier increases speed; modifier increases speed while decreasing smoothness and HW requirements
			current_effect_func = effect_moving_gradient_reverse;
            colourChanged=1;
            modifier=1;
            switch (effect2)
            {
				case 0 ... 18:    multiplier = 0.25;  PSC=8789;  break;
				case 19 ... 36:   multiplier = 0.5;   PSC=4394;  break;
				case 37 ... 54:   multiplier = 1;     PSC=2197;  break;
				case 55 ... 72:   multiplier = 2;     PSC=1098;  break;
				case 73 ... 90:   multiplier = 4;     PSC=549;   break;
				case 91 ... 108:  multiplier = 8;     PSC=274;   break;
				case 109 ... 126: multiplier = 16;    PSC=137;   break;
				case 127 ... 144: multiplier = 32;    PSC=68;    break;
				case 145 ... 162: multiplier = 64;    PSC=34;    break;
				case 163 ... 180: current_effect_func =effect_moving_gradient_reverse_faster; modifier=2;  multiplier = 32;    PSC=68;     break;
				case 181 ... 198: current_effect_func =effect_moving_gradient_reverse_faster; modifier=4;  multiplier = 32;    PSC=68;     break;
				case 199 ... 216: current_effect_func =effect_moving_gradient_reverse_faster; modifier=8;  multiplier = 32;    PSC=68;     break;
				case 217 ... 234: current_effect_func =effect_moving_gradient_reverse_faster; modifier=10; multiplier = 32;    PSC=68;     break;
				case 235 ... 255: current_effect_func =effect_moving_gradient_reverse_faster; modifier=16; multiplier = 32;    PSC=68;     break;
            }
            break;
         case 23 ... 24: //MOVING GLITCHY GRADIENT
			current_effect_func = effect_glitchy_gradient;
			colourChanged=1;
			modifier=1;
            switch (effect2)
            {
				case 0 ... 28:    multiplier = 0.25;  PSC=8789;  break;
				case 29 ... 56:   multiplier = 0.5;   PSC=4394;  break;
				case 57 ... 84:   multiplier = 1;     PSC=2197;  break;
				case 85 ... 112:  multiplier = 2;     PSC=1098;  break;
				case 113 ... 140: multiplier = 4;     PSC=549;   break;
				case 141 ... 168: multiplier = 8;     PSC=274;   break;
				case 169 ... 196: multiplier = 16;    PSC=137;   break;
				case 197 ... 224: multiplier = 32;    PSC=68;    break;
				case 225 ... 255: multiplier = 64;    PSC=34;    break;
            }
            break;
         case 25 ... 26: //MOVING LINE
			current_effect_func = effect_moving_line;
            colourChanged=1;
            modifier=1;
            switch (effect2)
            {
            	case 0 ... 28:    multiplier = 0.25;  PSC=8789;  break;
            	case 29 ... 56:   multiplier = 0.5;   PSC=4394;  break;
            	case 57 ... 84:   multiplier = 1;     PSC=2197;  break;
            	case 85 ... 112:  multiplier = 2;     PSC=1098;  break;
            	case 113 ... 140: multiplier = 4;     PSC=549;   break;
            	case 141 ... 168: multiplier = 8;     PSC=274;   break;
            	case 169 ... 196: multiplier = 16;    PSC=137;   break;
            	case 197 ... 224: multiplier = 32;    PSC=68;    break;
            	case 225 ... 255: multiplier = 64;    PSC=34;    break;
            }
            break;
         case 27 ... 28: //MOVING GLITCHY
			current_effect_func = effect_glitchy;
            colourChanged=1;
            modifier=1;
            switch (effect2)
            {
            	case 0 ... 28:    multiplier = 0.25;  PSC=8789;  break;
            	case 29 ... 56:   multiplier = 0.5;   PSC=4394;  break;
            	case 57 ... 84:   multiplier = 1;     PSC=2197;  break;
            	case 85 ... 112:  multiplier = 2;     PSC=1098;  break;
            	case 113 ... 140: multiplier = 4;     PSC=549;   break;
            	case 141 ... 168: multiplier = 8;     PSC=274;   break;
            	case 169 ... 196: multiplier = 16;    PSC=137;   break;
            	case 197 ... 224: multiplier = 32;    PSC=68;    break;
            	case 225 ... 255: multiplier = 64;    PSC=34;    break;
            }
            break;
         case 29 ... 30: //MOVING DOTS
     		current_effect_func = effect_moving_dots;
         	colourChanged=1;
            switch (effect2)
            {
				case 0 ... 13:    multiplier = 8;   PSC=274;   modifier=3;   break;
				case 14 ... 27:   multiplier = 16;  PSC=137;   modifier=3;   break;
				case 28 ... 41:   multiplier = 32;  PSC=68;    modifier=3;   break;
				case 42 ... 55:   multiplier = 64;  PSC=34;    modifier=3;   break;
				case 56 ... 69:   multiplier = 8;   PSC=274;   modifier=5;   break;
				case 70 ... 83:   multiplier = 16;  PSC=137;   modifier=5;   break;
				case 84 ... 97:   multiplier = 32;  PSC=68;    modifier=5;   break;
				case 98 ... 111:  multiplier = 64;  PSC=34;    modifier=5;   break;
				case 112 ... 125: multiplier = 8;   PSC=274;   modifier=8;   break;
				case 126 ... 139: multiplier = 16;  PSC=137;   modifier=8;   break;
				case 140 ... 153: multiplier = 32;  PSC=68;    modifier=8;   break;
				case 154 ... 167: multiplier = 64;  PSC=34;    modifier=8;   break;
				case 168 ... 181: multiplier = 8;   PSC=274;   modifier=10;  break;
				case 182 ... 195: multiplier = 16;  PSC=137;   modifier=10;  break;
				case 196 ... 209: multiplier = 32;  PSC=68;    modifier=10;  break;
				case 210 ... 223: multiplier = 64;  PSC=34;    modifier=10;  break;
				case 224 ... 237: multiplier = 16;  PSC=137;   modifier=16;  break;
				case 238 ... 255: multiplier = 32;  PSC=68;    modifier=16;  break;
			}
            break;
         case 31 ... 32: //JUMPING
     		current_effect_func = effect_jumping;
         	modifier=1;
         	ARR=11110;
         	PSC=47;
         	ownTempo=1;
         	break;
         case 33 ... 34: //JUMPING OWN
     		current_effect_func = effect_jumping_own;
         	modifier=1;
         	ARR=10457;
         	PSC=50;
         	ownTempo=1;
         	break;
         case 35 ... 36: //FALLING DROP
     		current_effect_func = effect_falling_drop;
         	modifier=1;
         	colourChanged=1;
         	switch (effect2)
            {
            	case 0 ... 51: multiplier = 9;        PSC=244;   break;
            	case 52 ... 102: multiplier = 18;     PSC=122;   break;
            	case 103 ... 153: multiplier = 36;    PSC=61;    break;
            	case 154 ... 204: multiplier = 72;    PSC=30;    break;
            	case 205 ... 255: multiplier = 144;   PSC=14;    break;
            }
         	break;

        case 5 ... 6: //STROBE
			current_effect_func = effect_strobe;
        	modifier=1;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;     PSC=137;   break; //16 changes per beat -> 64
			}
            break;
         case 7 ... 8: //STROBE TWO COLOURS
    		current_effect_func = effect_strobe_colours;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64
			}
            break;
         case 9 ... 10: //SWITCH TWO COLOURS
     		current_effect_func = effect_switch_colours;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64
			}
            break;
         case 11 ... 12: //SWITCH ODD/EVEN
       		current_effect_func = effect_odd_even;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64
            }
            break;
         case 39 ... 40: //STROBE w FADING
       		current_effect_func = effect_strobe_fading;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
               //case 50 ... 62:  multiplier = 8; PSC=137; current_effect_func = effect_strobe_fading_64;  break; // x32
               //case 63 ... 72:  multiplier = 16; PSC=137; current_effect_func = effect_strobe_fading_64;  break; // x32
               //case 73 ... 108:  multiplier = 32; PSC=68; current_effect_func = effect_strobe_fading_64;  break; // x32
        		case 0 ... 36:    multiplier = 32; modifier = 1024; PSC=69;  break; // x16
            	case 37 ... 72:   multiplier = 16; modifier = 256; PSC=137;  break; // x16
            	case 73 ... 108:  multiplier = 16; modifier = 128; PSC=137;  break; // x16
            	case 109 ... 144: multiplier = 16; modifier = 64; PSC=137;  break; // x16
            	case 145 ... 180: multiplier = 16; modifier = 32; PSC=137;  break; // x16
            	case 181 ... 216: multiplier = 16; modifier = 16; PSC=137;  break; // x16
            	case 217 ... 255: multiplier = 32; modifier = 16; PSC=69;  break; // x16

            }
            break;
          case 41 ... 42: //SECTORS
       		current_effect_func = effect_sectors;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64

            }
            break;
          case 43 ... 44: //SECTORS RANDOM
       		current_effect_func = effect_sectors_random;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64

            }
            break;  //------------------------------------------------------------------------------------------------
         case 57 ... 58: //SECTORS BACK AND FORTH
       		current_effect_func = effect_sectors_backAndForth;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64

            }
            break;
          case 59 ... 60: //SECTORS TOGETHER
       		current_effect_func = effect_sectors_together;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64

            }
            break;
          case 45 ... 46: //SECTORS FADEIN
         		current_effect_func = effect_sectors_fadein;
           	modifier=1;
           	ownTempo=0;
              switch (effect2)
              {
  				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
  				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
  				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
  				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
  				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
  				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
  				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64

              }
              break;
          case 47 ... 48: //SECTORS RANDOM BLINKING
       		current_effect_func = effect_sectors_blinking;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64

            }
            break;
          case 49 ... 50: //MIDDLE
       		current_effect_func = effect_middle;
         	modifier=16;
         	ownTempo=0;
            switch (effect2)
            {
            	case 0 ... 100: multiplier = 16;   modifier=64; PSC=137;   break; //whole note
				case 101 ... 200:   multiplier = 16;   modifier=32; PSC=137;  break; //half note
				case 201 ... 255:  multiplier = 16;   modifier=16; PSC=137;  break; //quarter
            }
            break;
          case 53 ... 54: //MIDDLE BACK AND FORTH
       		current_effect_func = effect_middle_bounce1;
         	modifier=16;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 36:    multiplier = 0.25;  PSC=8789;  break; //0,25 changes per beat -> whole note
				case 37 ... 72:   multiplier = 0.5;   PSC=4394;  break; //0,5 changes per beat -> half note
				case 73 ... 108:  multiplier = 1;     PSC=2197;  break; //1 changes per beat -> quarter
				case 109 ... 144: multiplier = 2;     PSC=1098;  break; //2 changes per beat -> eight
				case 145 ... 180: multiplier = 4;     PSC=549;   break; //4 changes per beat -> sixteen
				case 181 ... 216: multiplier = 8;     PSC=274;   break; //8 changes per beat -> 32
				case 217 ... 255: multiplier = 16;    PSC=137;   break; //16 changes per beat -> 64
            }
            break;
         /* case 55 ... 56:
       		current_effect_func = effect_middle_bounce2;
         	modifier=16;
         	ownTempo=0;
            switch (effect2)
            {
            	case 0 ... 50: multiplier = 32;   modifier=128; PSC=69;   break; //whole note
            	case 51 ... 100: multiplier = 16;   modifier=32; PSC=137;   break; //quarter note
				case 101 ... 200:   multiplier = 16;   modifier=32; PSC=137;  break; //half note
				case 201 ... 255:  multiplier = 16;   modifier=16; PSC=137;  break; //quarter
            }
            break;*/
         case 55 ... 56: //SLIDE FROM BOTTOM
       		current_effect_func = effect_slide_bottom_keepLowest;
         	ownTempo=0;
            switch (effect2)
            {
            	case 0 ... 28:     multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
            	case 29 ... 56:    multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
            	case 57 ... 84:    multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 85 ... 112:   multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 113 ... 140:  multiplier = 8;  modifier=16;   PSC=274;   break; //half note - mid res
				case 141 ... 168:  multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 169 ... 196:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 197 ... 224:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 225 ... 255:  multiplier = 32; modifier=16;  PSC=69;   break; //16
            }
            break;
          case 61 ... 62:
       		current_effect_func = effect_slide_bottom;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 28:     multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 29 ... 56:    multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 57 ... 84:    multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 85 ... 112:   multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 113 ... 140:  multiplier = 8;  modifier=16;   PSC=274;   break; //half note - mid res
				case 141 ... 168:  multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 169 ... 196:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 197 ... 224:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 225 ... 255:  multiplier = 32; modifier=16;  PSC=69;   break; //16
            }
            break;
          case 63 ... 64:
         		current_effect_func = effect_slide_top;
           	ownTempo=0;
              switch (effect2)
              {
  				case 0 ... 28:     multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
  				case 29 ... 56:    multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
  				case 57 ... 84:    multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
  				case 85 ... 112:   multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
  				case 113 ... 140:  multiplier = 8;  modifier=16;   PSC=274;   break; //half note - mid res
  				case 141 ... 168:  multiplier = 16; modifier=64;  PSC=137;   break; //whole note
  				case 169 ... 196:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
  				case 197 ... 224:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
  				case 225 ... 255:  multiplier = 32; modifier=16;  PSC=69;   break; //16
              }
              break;
          case 65 ... 66:
       		current_effect_func = effect_slide_backAndForth;
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 28:     multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 29 ... 56:    multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 57 ... 84:    multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 85 ... 112:   multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 113 ... 140:  multiplier = 8;  modifier=16;   PSC=274;   break; //half note - mid res
				case 141 ... 168:  multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 169 ... 196:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 197 ... 224:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 225 ... 255:  multiplier = 32; modifier=16;  PSC=69;   break; //16
            }
            break;
          case 67 ... 68:
         		current_effect_func = effect_slide_backAndForth_special;
           	ownTempo=0;
              switch (effect2)
              {
  				case 0 ... 28:     multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
  				case 29 ... 56:    multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
  				case 57 ... 84:    multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
  				case 85 ... 112:   multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
  				case 113 ... 140:  multiplier = 8;  modifier=16;   PSC=274;   break; //half note - mid res
  				case 141 ... 168:  multiplier = 16; modifier=64;  PSC=137;   break; //whole note
  				case 169 ... 196:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
  				case 197 ... 224:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
  				case 225 ... 255:  multiplier = 32; modifier=16;  PSC=69;   break; //16
              }
              break;
          case 69 ... 70:
       		current_effect_func = effect_fromMiddle;
         	ownTempo=0;
            switch (effect2)
            {
				case 0   ... 14:  multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 15  ... 29:  multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 30  ... 43:  multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 44  ... 58:  multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 59  ... 72:  multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
				case 73  ... 87:  multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 88  ... 101:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 102 ... 116:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 117 ... 130:  multiplier = 32; modifier=16;  PSC=69;   break; //16
				case 131 ... 145:  multiplier = 8;  modifier=128; current_effect_func = effect_fromMiddle_trueZero;   PSC=274;   break; //slow continous        //wider center, fully dimmed
				case 146 ... 159:  multiplier = 4;  modifier=16;  current_effect_func = effect_fromMiddle_trueZero;   PSC=549;   break; //whole - lower res              //wider center, fully dimmed
				case 160 ... 174:  multiplier = 4;  modifier=8;   current_effect_func = effect_fromMiddle_trueZero;   PSC=549;   break; //half note - lower res          //wider center, fully dimmed
				case 175 ... 188:  multiplier = 4;  modifier=4;   current_effect_func = effect_fromMiddle_trueZero;   PSC=549;   break; //quarter note - large blocks    //wider center, fully dimmed
				case 189 ... 203:  multiplier = 8;  modifier=8;   current_effect_func = effect_fromMiddle_trueZero;  PSC=274;   break; //quarter note - mid res          //wider center, fully dimmed
				case 204 ... 217:  multiplier = 16; modifier=64;  current_effect_func = effect_fromMiddle_trueZero;  PSC=137;   break; //whole note                      //wider center, fully dimmed
				case 218 ... 232:  multiplier = 16; modifier=32;  current_effect_func = effect_fromMiddle_trueZero;   PSC=137;  break; //half note                       //wider center, fully dimmed
				case 233 ... 246:  multiplier = 16; modifier=16;  current_effect_func = effect_fromMiddle_trueZero;   PSC=137;  break; //quarter note                    //wider center, fully dimmed
				case 247 ... 255:  multiplier = 32; modifier=16;  current_effect_func = effect_fromMiddle_trueZero;   PSC=69;  break; //16                               //wider center, fully dimmed
            }                                                                                                                 //wider center, fully dimmed
            break;
          case 71 ... 72: //ToDo: add trueZero? probably not...
       		current_effect_func = effect_fromMiddle_dim; //can use blank second colour!
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 28:    multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 29 ... 56:   multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 57 ... 84:   multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 85 ... 112:  multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 113 ... 140: multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
				case 141 ... 168: multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 169 ... 196:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 197 ... 224:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 225 ... 255:  multiplier = 32; modifier=16;  PSC=69;   break; //16
            }                                                                                                                 //wider center, fully dimmed
            break;
          case 73 ... 74: //ToDo: add trueZero?
       		current_effect_func = effect_fromMiddle_dim_special; //Turns off after each cycle; cant use blank second colour
         	ownTempo=0;
         	modifier=0;
            switch (effect2)
            {
				case 0   ... 9: multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 10  ... 18: multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 19  ... 28: multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 29  ... 37: multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 38  ... 47: multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
				case 48  ... 56: multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 57  ... 66:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 67  ... 75:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 76  ... 85:  multiplier = 32; modifier=16;  PSC=69;   break; //16
				case 86  ... 94: multiplier = 8;  modifier=128;  PSC=274; modifier2=1;   break; //slow continous
				case 95  ... 104: multiplier = 4;  modifier=16;   PSC=549; modifier2=1;  break; //whole - lower res
				case 105 ... 113: multiplier = 4;  modifier=8;    PSC=549; modifier2=1;  break; //half note - lower res
				case 114 ... 123: multiplier = 4;  modifier=4;    PSC=549; modifier2=1;  break; //quarter note - large blocks
				case 124 ... 132: multiplier = 8;  modifier=8;   PSC=274;  modifier2=1; break; //quarter note - mid res
				case 133 ... 142: multiplier = 16; modifier=64;  PSC=137;  modifier2=1; break; //whole note
				case 143 ... 151:  multiplier = 16; modifier=32;  PSC=137; modifier2=1;  break; //half note
				case 152 ... 161:  multiplier = 16; modifier=16;  PSC=137; modifier2=1;  break; //quarter note
				case 162 ... 170:  multiplier = 32; modifier=16;  PSC=69;  modifier2=1; break; //16
				case 171 ... 180: multiplier = 8;  modifier=128;  PSC=274; current_effect_func = effect_fromMiddle_dim_special2;   break; //slow continous
				case 181 ... 189: multiplier = 4;  modifier=16;   PSC=549; current_effect_func = effect_fromMiddle_dim_special2;  break; //whole - lower res
				case 190 ... 199: multiplier = 4;  modifier=8;    PSC=549; current_effect_func = effect_fromMiddle_dim_special2;  break; //half note - lower res
				case 200 ... 208: multiplier = 4;  modifier=4;    PSC=549; current_effect_func = effect_fromMiddle_dim_special2;  break; //quarter note - large blocks
				case 209 ... 218: multiplier = 8;  modifier=8;   PSC=274;  current_effect_func = effect_fromMiddle_dim_special2; break; //quarter note - mid res
				case 219 ... 227: multiplier = 16; modifier=64;  PSC=137;  current_effect_func = effect_fromMiddle_dim_special2; break; //whole note
				case 228 ... 237:  multiplier = 16; modifier=32;  PSC=137; current_effect_func = effect_fromMiddle_dim_special2;  break; //half note
				case 238 ... 246:  multiplier = 16; modifier=16;  PSC=137; current_effect_func = effect_fromMiddle_dim_special2;  break; //quarter note
				case 247 ... 255:  multiplier = 32; modifier=16;  PSC=69;  current_effect_func = effect_fromMiddle_dim_special2; break; //16
            }
            break;
          case 75 ... 76:
       		current_effect_func = effect_fromEdge;
         	ownTempo=0;
            switch (effect2)
            {
				case 0   ... 14:  multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 15  ... 29:  multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 30  ... 43:  multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 44  ... 58:  multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 59  ... 72:  multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
				case 73  ... 87:  multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 88  ... 101:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 102 ... 116:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 117 ... 130:  multiplier = 32; modifier=16;  PSC=69;   break; //16
				case 131 ... 145:  multiplier = 8;  modifier=128; current_effect_func = effect_fromEdge_trueZero;   PSC=274;   break; //slow continous        //wider center, fully dimmed
				case 146 ... 159:  multiplier = 4;  modifier=16;  current_effect_func = effect_fromEdge_trueZero;   PSC=549;   break; //whole - lower res              //wider center, fully dimmed
				case 160 ... 174:  multiplier = 4;  modifier=8;   current_effect_func = effect_fromEdge_trueZero;   PSC=549;   break; //half note - lower res          //wider center, fully dimmed
				case 175 ... 188:  multiplier = 4;  modifier=4;   current_effect_func = effect_fromEdge_trueZero;   PSC=549;   break; //quarter note - large blocks    //wider center, fully dimmed
				case 189 ... 203:  multiplier = 8;  modifier=8;   current_effect_func = effect_fromEdge_trueZero;  PSC=274;   break; //quarter note - mid res          //wider center, fully dimmed
				case 204 ... 217:  multiplier = 16; modifier=64;  current_effect_func = effect_fromEdge_trueZero;  PSC=137;   break; //whole note                      //wider center, fully dimmed
				case 218 ... 232:  multiplier = 16; modifier=32;  current_effect_func = effect_fromEdge_trueZero;   PSC=137;  break; //half note                       //wider center, fully dimmed
				case 233 ... 246:  multiplier = 16; modifier=16;  current_effect_func = effect_fromEdge_trueZero;   PSC=137;  break; //quarter note                    //wider center, fully dimmed
				case 247 ... 255:  multiplier = 32; modifier=16;  current_effect_func = effect_fromEdge_trueZero;   PSC=69;  break; //16                               //wider center, fully dimmed
            }                                                                                                                 //wider center, fully dimmed
            break;
          case 77 ... 78: //ToDo: add trueZero? probably not...
       		current_effect_func = effect_fromEdge_dim; //can use blank second colour!
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 28:    multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 29 ... 56:   multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 57 ... 84:   multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 85 ... 112:  multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 113 ... 140: multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
				case 141 ... 168: multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 169 ... 196:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 197 ... 224:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 225 ... 255:  multiplier = 32; modifier=16;  PSC=69;   break; //16
            }                                                                                                                 //wider center, fully dimmed
            break;
          case 79 ... 80: //ToDo: add trueZero?
       		current_effect_func = effect_fromEdge_dim_special; //Turns off after each cycle; cant use blank second colour
         	ownTempo=0;
            switch (effect2)
            {
				case 0   ... 9: multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 10  ... 18: multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 19  ... 28: multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 29  ... 37: multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 38  ... 47: multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
				case 48  ... 56: multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 57  ... 66:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 67  ... 75:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 76  ... 85:  multiplier = 32; modifier=16;  PSC=69;   break; //16
				case 86  ... 94: multiplier = 8;  modifier=128;  PSC=274; modifier2=1;   break; //slow continous
				case 95  ... 104: multiplier = 4;  modifier=16;   PSC=549; modifier2=1;  break; //whole - lower res
				case 105 ... 113: multiplier = 4;  modifier=8;    PSC=549; modifier2=1;  break; //half note - lower res
				case 114 ... 123: multiplier = 4;  modifier=4;    PSC=549; modifier2=1;  break; //quarter note - large blocks
				case 124 ... 132: multiplier = 8;  modifier=8;   PSC=274;  modifier2=1; break; //quarter note - mid res
				case 133 ... 142: multiplier = 16; modifier=64;  PSC=137;  modifier2=1; break; //whole note
				case 143 ... 151:  multiplier = 16; modifier=32;  PSC=137; modifier2=1;  break; //half note
				case 152 ... 161:  multiplier = 16; modifier=16;  PSC=137; modifier2=1;  break; //quarter note
				case 162 ... 170:  multiplier = 32; modifier=16;  PSC=69;  modifier2=1; break; //16
				case 171 ... 180: multiplier = 8;  modifier=128;  PSC=274; current_effect_func = effect_fromEdge_dim_special2;   break; //slow continous
				case 181 ... 189: multiplier = 4;  modifier=16;   PSC=549; current_effect_func = effect_fromEdge_dim_special2;  break; //whole - lower res
				case 190 ... 199: multiplier = 4;  modifier=8;    PSC=549; current_effect_func = effect_fromEdge_dim_special2;  break; //half note - lower res
				case 200 ... 208: multiplier = 4;  modifier=4;    PSC=549; current_effect_func = effect_fromEdge_dim_special2;  break; //quarter note - large blocks
				case 209 ... 218: multiplier = 8;  modifier=8;   PSC=274;  current_effect_func = effect_fromEdge_dim_special2; break; //quarter note - mid res
				case 219 ... 227: multiplier = 16; modifier=64;  PSC=137;  current_effect_func = effect_fromEdge_dim_special2; break; //whole note
				case 228 ... 237:   multiplier = 16; modifier=32;  PSC=137; current_effect_func = effect_fromEdge_dim_special2;  break; //half note
				case 238 ... 246:  multiplier = 16; modifier=16;  PSC=137; current_effect_func = effect_fromEdge_dim_special2;  break; //quarter note
				case 247 ... 255:  multiplier = 32; modifier=16;  PSC=69;  current_effect_func = effect_fromEdge_dim_special2; break; //16
            }
            break;
          case 81 ... 82: //ToDo: add trueZero?
         	  current_effect_func = effect_fromEdge_backAndForth; //Turns off after each cycle; cant use blank second colour
          	  modifier2=0;
          	  ownTempo=0;
              switch (effect2)
              {
  				case 0   ... 9: multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
  				case 10  ... 18: multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
  				case 19  ... 28: multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
  				case 29  ... 37: multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
  				case 38  ... 47: multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
  				case 48  ... 56: multiplier = 16; modifier=64;  PSC=137;   break; //whole note
  				case 57  ... 66:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
  				case 67  ... 75:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
  				case 76  ... 85:  multiplier = 32; modifier=16;  PSC=69;   break; //16
  				case 86  ... 94: multiplier = 8;  modifier=128;  PSC=274; modifier2=1;   break; //slow continous
  				case 95  ... 104: multiplier = 4;  modifier=16;   PSC=549; modifier2=1;  break; //whole - lower res
  				case 105 ... 113: multiplier = 4;  modifier=8;    PSC=549; modifier2=1;  break; //half note - lower res
  				case 114 ... 123: multiplier = 4;  modifier=4;    PSC=549; modifier2=1;  break; //quarter note - large blocks
  				case 124 ... 132: multiplier = 8;  modifier=8;   PSC=274;  modifier2=1; break; //quarter note - mid res
  				case 133 ... 142: multiplier = 16; modifier=64;  PSC=137;  modifier2=1; break; //whole note
  				case 143 ... 151:  multiplier = 16; modifier=32;  PSC=137; modifier2=1;  break; //half note
  				case 152 ... 161:  multiplier = 16; modifier=16;  PSC=137; modifier2=1;  break; //quarter note
  				case 162 ... 170:  multiplier = 32; modifier=16;  PSC=69;  modifier2=1; break; //16
  				case 171 ... 180: multiplier = 8;  modifier=128;  PSC=274; current_effect_func = effect_fromEdge_backAndForth_special;   break; //slow continous
  				case 181 ... 189: multiplier = 4;  modifier=16;   PSC=549; current_effect_func = effect_fromEdge_backAndForth_special;  break; //whole - lower res
  				case 190 ... 199: multiplier = 4;  modifier=8;    PSC=549; current_effect_func = effect_fromEdge_backAndForth_special;  break; //half note - lower res
  				case 200 ... 208: multiplier = 4;  modifier=4;    PSC=549; current_effect_func = effect_fromEdge_backAndForth_special;  break; //quarter note - large blocks
  				case 209 ... 218: multiplier = 8;  modifier=8;   PSC=274;  current_effect_func = effect_fromEdge_backAndForth_special; break; //quarter note - mid res
  				case 219 ... 227: multiplier = 16; modifier=64;  PSC=137;  current_effect_func = effect_fromEdge_backAndForth_special; break; //whole note
  				case 228 ... 237:   multiplier = 16; modifier=32;  PSC=137; current_effect_func = effect_fromEdge_backAndForth_special;  break; //half note
  				case 238 ... 246:  multiplier = 16; modifier=16;  PSC=137; current_effect_func = effect_fromEdge_backAndForth_special;  break; //quarter note
  				case 247 ... 255:  multiplier = 32; modifier=16;  PSC=69;  current_effect_func = effect_fromEdge_backAndForth_special; break; //16
              }
              break;
          case 83 ... 84: //ToDo: add trueZero?
       		current_effect_func = effect_twoDrops_fromMiddle; //Turns off after each cycle; cant use blank second colour
         	ownTempo=0;
            switch (effect2)
            {
				case 0   ... 6: multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 7   ... 13: multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 14  ... 20: multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 21  ... 27: multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 28  ... 34: multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
				case 35  ... 41: multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 42  ... 48: multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 49  ... 55: multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 56  ... 62: multiplier = 32; modifier=16;  PSC=69;   break; //16
				case 63  ... 69: multiplier = 8;  modifier=128;  PSC=274;  current_effect_func = effect_twoDrops_fromEdge;   break; //slow continous
				case 70  ... 76: multiplier = 4;  modifier=16;   PSC=549; current_effect_func = effect_twoDrops_fromEdge;  break; //whole - lower res
				case 77  ... 83: multiplier = 4;  modifier=8;    PSC=549; current_effect_func = effect_twoDrops_fromEdge;  break; //half note - lower res
				case 84  ... 90: multiplier = 4;  modifier=4;    PSC=549; current_effect_func = effect_twoDrops_fromEdge;  break; //quarter note - large blocks
				case 91  ... 97: multiplier = 8;  modifier=8;   PSC=274;  current_effect_func = effect_twoDrops_fromEdge; break; //quarter note - mid res
				case 98  ... 104: multiplier = 16; modifier=64;  PSC=137;  current_effect_func = effect_twoDrops_fromEdge; break; //whole note
				case 105 ... 111:  multiplier = 16; modifier=32;  PSC=137; current_effect_func = effect_twoDrops_fromEdge;  break; //half note
				case 112 ... 118:  multiplier = 16; modifier=16;  PSC=137; current_effect_func = effect_twoDrops_fromEdge;  break; //quarter note
				case 119 ... 125:  multiplier = 32; modifier=16;  PSC=69;  current_effect_func = effect_twoDrops_fromEdge; break; //16
				case 126 ... 132: multiplier = 8;  modifier=128;  PSC=274; current_effect_func = effect_twoDrops_backAndForth;  break; //slow continous
				case 133 ... 139: multiplier = 4;  modifier=16;   PSC=549; current_effect_func = effect_twoDrops_backAndForth; break; //whole - lower res
				case 140 ... 146: multiplier = 4;  modifier=8;    PSC=549; current_effect_func = effect_twoDrops_backAndForth; break; //half note - lower res
				case 147 ... 153: multiplier = 4;  modifier=4;    PSC=549; current_effect_func = effect_twoDrops_backAndForth; break; //quarter note - large blocks
				case 154 ... 160: multiplier = 8;  modifier=8;   PSC=274;  current_effect_func = effect_twoDrops_backAndForth; break; //quarter note - mid res
				case 161 ... 167: multiplier = 16; modifier=64;  PSC=137;  current_effect_func = effect_twoDrops_backAndForth; break; //whole note
				case 168 ... 174:   multiplier = 16; modifier=32; PSC=137; current_effect_func = effect_twoDrops_backAndForth;  break; //half note
				case 175 ... 181:  multiplier = 16; modifier=16;  PSC=137; current_effect_func = effect_twoDrops_backAndForth; break; //quarter note
				case 182 ... 188:  multiplier = 32; modifier=16;  PSC=69;  current_effect_func = effect_twoDrops_backAndForth; break; //16
				case 189 ... 195: multiplier = 8;  modifier=128;  PSC=274; current_effect_func = effect_twoDrops_buggy;   break; //slow continous
				case 196 ... 202: multiplier = 4;  modifier=16;   PSC=549; current_effect_func = effect_twoDrops_buggy;  break; //whole - lower res
				case 203 ... 209: multiplier = 4;  modifier=8;    PSC=549; current_effect_func = effect_twoDrops_buggy;  break; //half note - lower res
				case 210 ... 216: multiplier = 4;  modifier=4;    PSC=549; current_effect_func = effect_twoDrops_buggy;  break; //quarter note - large blocks
				case 217 ... 223: multiplier = 8;  modifier=8;   PSC=274;  current_effect_func = effect_twoDrops_buggy; break; //quarter note - mid res
				case 224 ... 230: multiplier = 16; modifier=64;  PSC=137;  current_effect_func = effect_twoDrops_buggy; break; //whole note
				case 231 ... 237:   multiplier = 16; modifier=32; PSC=137; current_effect_func = effect_twoDrops_buggy;  break; //half note
				case 238 ... 244:  multiplier = 16; modifier=16;  PSC=137; current_effect_func = effect_twoDrops_buggy;  break; //quarter note
				case 245 ... 255:  multiplier = 32; modifier=16;  PSC=69;  current_effect_func = effect_twoDrops_buggy; break; //16
            }
            break;
         case 85 ... 86: //ToDo: add trueZero? probably not...
       		current_effect_func = effect_drops; //can use blank second colour!
         	ownTempo=0;
            switch (effect2)
            {
				case 0 ... 28:    multiplier = 8;  modifier=128;  PSC=274;   break; //slow continous
				case 29 ... 56:   multiplier = 4;  modifier=16;   PSC=549;   break; //whole - lower res
				case 57 ... 84:   multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res
				case 85 ... 112:  multiplier = 4;  modifier=4;    PSC=549;   break; //quarter note - large blocks
				case 113 ... 140: multiplier = 8;  modifier=8;   PSC=274;   break; //quarter note - mid res
				case 141 ... 168: multiplier = 16; modifier=64;  PSC=137;   break; //whole note
				case 169 ... 196:  multiplier = 16; modifier=32;  PSC=137;   break; //half note
				case 197 ... 224:  multiplier = 16; modifier=16;  PSC=137;   break; //quarter note
				case 225 ... 255:  multiplier = 32; modifier=16;  PSC=69;   break; //16
            }
            break;
          case 87 ... 88: //ToDo: add trueZero? probably not...
       		current_effect_func = effect_tubes; //can use blank second colour!
         	ownTempo=0;
			multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res

          case 89 ... 90: //ToDo: add trueZero? probably not...
       		current_effect_func = effect_tubes_true_pingpong; //can use blank second colour!
         	ownTempo=0;
			multiplier = 4;  modifier=8;    PSC=549;   break; //half note - lower res


          case 51 ... 52: //STATIC RADOM
                current_effect_func = effect_random_static;
                PSC = 21972;
                multiplier = 0.1;
                ownTempo=0;
            break;
            //Chcci aby tukal kazdou dobu (x1), ale nastavim (x16) a v sestnacti dam podminku, ktera urci tukani te 1x

          //SLIDE TWO COLOURS
          //SLID IN STATIC - with different speeds of slide
          //SWITCH TWO COLOURS CONTINOUSLY - změna tempa vždycky se spuštěním -> na 100 kroků rychle a pak změna - tempo zvolit podle maximální obnovovačky pásku - možnost volby rychlosti změny
          //PULSE IN BRIGHTNESS - with different speeds
          //ODMRKÁVÁNÍ
          //BLIKÁNÍ NA ID
          //Efekty ze starého
          //BLIKAČKA SEKTORŮ
          //POSTUPNÝ NAJÍŽDĚNÍ -NEJDŘÍV ZAPNOUT HODNĚ RYCHLEJ ČASOVAČ - PROJET TŘEBA STO KROKŮ A V POSLEDNÍM ČASOVAČ ZPOMALIT NA STATICKOU BARVU
          //Výbuch - opačně
          //

    }
    //is_new_effect = 0;
    step = 0;
    if(ownTempo==0)
    {
		ARR=((60.0f/(float)bpm)/multiplier)*MCU_CLOCK/(PSC+1)-1;
    }
    //uint16_t arr = (raw_arr > 65535.0f) ? 65535 : (uint16_t)raw_arr;
	effects_set_timer(ARR, PSC);
	current_effect_func();
}
