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

#define LEDCOUNT 144
#define MCU_CLOCK 72000000.0f

static TIM_HandleTypeDef *htimLocal;
static uint16_t bpm = 164;
static ColourName_t primaryColour = 20; //20
static ColourName_t secondaryColour = 102; //102
static uint32_t step = 0;
//static uint8_t is_new_effect = 0;
//static uint8_t brightness=255;
//static uint8_t modifier = 0;

static uint16_t PSC; 		 //defines the tempo accuraccy (ARR is automatically computed from given PSC and multiplier)
static float multiplier = 1; //defines the speed of an animation
static uint8_t modifier = 1; //defines the variation of an animation

static uint8_t colourChanged = 0;

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
    if(step>=143)
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
		uint16_t ledNum = (colourMaxStep + i + step) % LEDCOUNT;
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

static void effect_moving_line(void) //made thx to a bug...  don't judge the code...
{
	static uint8_t colourMaxStep=LEDCOUNT/2;
    if(step>=143)
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
		uint16_t ledNum = (colourMaxStep + i + step) % LEDCOUNT;
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
            PSC = 21972;
            multiplier = 0.1;
            break;
        case 3 ... 4: //STATIC COLOUR
			current_effect_func = effect_static;
        	PSC = 21972;
            multiplier = 0.1;
            break;
        case 13 ... 14: //STATIC TWO COLOUR - (CAN USE 0 colour!)
			current_effect_func = effect_static_two_colour;
        	PSC = 21972;
            multiplier = 0.1;
            switch (effect2)
            {
                case 0 ... 24:    modifier = 2;   break;
                case 25 ... 49:   modifier = 4;   break;
                case 50 ... 74:   modifier = 6;   break;
                case 75 ... 99:   modifier = 8;   break;
                case 100 ... 124: modifier = 12;  break;
                case 125 ... 149: modifier = 16;  break;
                case 150 ... 174: modifier = 18;  break;
                case 175 ... 199: modifier = 24;  break;
                case 200 ... 224: modifier = 36;  break;
                case 225 ... 255: modifier = 72;  break;
            }
            break;
        case 15 ... 16: //STATIC TWO COLOUR w BRIGHTNESS
			//ToDo: maximal brightness should be related to brightness set by DMX channel!
			current_effect_func = effect_static_two_colour_brightness;
        	PSC = 21972;
            multiplier = 0.1;
            switch (effect2)
            {
				case 0 ... 24:    modifier = 2;   break;
				case 25 ... 49:   modifier = 4;   break;
				case 50 ... 74:   modifier = 6;   break;
				case 75 ... 99:   modifier = 8;   break;
				case 100 ... 124: modifier = 12;  break;
				case 125 ... 149: modifier = 16;  break;
				case 150 ... 174: modifier = 18;  break;
				case 175 ... 199: modifier = 24;  break;
				case 200 ... 224: modifier = 36;  break;
				case 225 ... 255: modifier = 72;  break;
            }
            break;
        case 17 ... 18: //STATIC TWO COLOUR GRADIENT
			current_effect_func = effect_static_two_colour_gradient;
        	PSC = 21972;
            multiplier = 0.1;
            switch (effect2)
            {
                case 0 ... 24:    modifier = 4;   break;
                case 25 ... 49:   modifier = 8;   break;
                case 50 ... 74:   modifier = 10;   break;
                case 75 ... 99:   modifier = 20;   break;
                case 100 ... 124: modifier = 40;  break;
                case 125 ... 149: modifier = 60;  break;
                case 150 ... 174: modifier = 80;  break;
                case 175 ... 199: modifier = 100;  break;
                case 200 ... 224: modifier = 120;  break;
                case 225 ... 255: modifier = 140;  break;
            }
            break;
         case 19 ... 20: //STATIC TWO COLOUR GRADIENT REVERSED
			current_effect_func = effect_static_two_colour_gradient_2;
         	multiplier = 1;
         	PSC = 21972;
            break;
         case 21 ... 22: //MOVING GRADIENT
			current_effect_func = effect_moving_gradient;
            colourChanged=1;
            modifier=1;
            switch (effect2)
            {
				case 0 ... 28:    multiplier = 0.25;  PSC=8789;  break;
				case 29 ... 56:   multiplier = 0.5;   PSC=4394;  break;
				case 57 ... 84:  multiplier = 1;      PSC=2197;  break;
				case 85 ... 112: multiplier = 2;      PSC=1098;  break;
				case 113 ... 140: multiplier = 4;     PSC=549;   break;
				case 141 ... 168: multiplier = 8;     PSC=274;   break;
				case 169 ... 196: multiplier = 16;    PSC=137;   break;
				case 197 ... 225: multiplier = 32;    PSC=68;    break;
				case 226 ... 255: multiplier = 64;    PSC=34;    break;
				case 226 ... 255: multiplier = 64;    PSC=34;    break;
            }
            break;
         case 23 ... 24: //MOVING GLITCHY GRADIENT
			current_effect_func = effect_glitchy_gradient;
            PSC = 85;
            colourChanged=1;
            switch (effect2)
            {
                case 0 ... 24:    multiplier = 4;   break;
                case 25 ... 49:   multiplier = 8;   break;
                case 50 ... 74:   multiplier = 10;   break;
                case 75 ... 99:   multiplier = 20;   break;
                case 100 ... 124: multiplier = 40;  break;
                case 125 ... 149: multiplier = 60;  break;
                case 150 ... 174: multiplier = 80;  break;
                case 175 ... 199: multiplier = 100;  break;
                case 200 ... 224: multiplier = 120;  break;
                case 225 ... 255: multiplier = 140;  break;
            }
            break;
         case 25 ... 26: //MOVING LINE
			current_effect_func = effect_moving_line;
            PSC = 85;
            colourChanged=1;
            switch (effect2)
            {
                case 0 ... 24:    multiplier = 4;   break;
                case 25 ... 49:   multiplier = 8;   break;
                case 50 ... 74:   multiplier = 10;   break;
                case 75 ... 99:   multiplier = 20;   break;
                case 100 ... 124: multiplier = 40;  break;
                case 125 ... 149: multiplier = 60;  break;
                case 150 ... 174: multiplier = 80;  break;
                case 175 ... 199: multiplier = 100;  break;
                case 200 ... 224: multiplier = 120;  break;
                case 225 ... 255: multiplier = 140;  break;
            }
            break;
         case 27 ... 28: //MOVING GLITCHY GRADIENT
			current_effect_func = effect_glitchy;
            PSC = 85;
            colourChanged=1;
            switch (effect2)
            {
                case 0 ... 24:    multiplier = 4;   break;
                case 25 ... 49:   multiplier = 8;   break;
                case 50 ... 74:   multiplier = 10;   break;
                case 75 ... 99:   multiplier = 20;   break;
                case 100 ... 124: multiplier = 40;  break;
                case 125 ... 149: multiplier = 60;  break;
                case 150 ... 174: multiplier = 80;  break;
                case 175 ... 199: multiplier = 100;  break;
                case 200 ... 224: multiplier = 120;  break;
                case 225 ... 255: multiplier = 140;  break;
            }
            break;
        case 5 ... 6: //STROBE
			current_effect_func = effect_strobe;
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
         case 7 ... 8: //STROBE TWO COLOURS
    		current_effect_func = effect_strobe_colours;
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
	uint16_t ARR=((60.0f/(float)bpm)/multiplier)*MCU_CLOCK/(PSC+1)-1;
    //uint16_t arr = (raw_arr > 65535.0f) ? 65535 : (uint16_t)raw_arr;
	effects_set_timer(ARR, PSC);
	current_effect_func();
}
