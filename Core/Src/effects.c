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

static uint8_t colourChanged = 0; //for effects that has to recalculate the influence of colour change (gradients, etc.)
static uint8_t ownTempo=0;  	  //for effects that use fixed individual refresh rate

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

static void effect_strobe_fading(void) //PREDELAT TAK ABY VYPADAL GRAFICKY HEZKY! - nejde o to s jakym krokem jdou po sobe, ale kolik casu ktery jas setrva?!
{									   //ToDo: VZTAHNOUT JAS K MAX JASU
	static uint8_t brightness=0;
	if(step>31)
	{
		step=0;
	}
    if(step>0&&step<15)
    {

    	brightness=brightness+17;
        ARGB_FillRGB((colourTable[primaryColour].r*brightness)/255, (colourTable[primaryColour].g*brightness)/255, (colourTable[primaryColour].b*brightness)/255);
        ARGB_FillWhite(colourTable[primaryColour].w);
        ARGB_Show();
    }
    else if(step==15)
    {
    	brightness=0;
    	ARGB_SetBrightness(255);
        ARGB_FillRGB(colourTable[primaryColour].r, colourTable[primaryColour].g, colourTable[primaryColour].b);
        ARGB_FillWhite(colourTable[primaryColour].w);
        ARGB_Show();

    }
    else if(step==31)
    {
        ARGB_Clear();
        ARGB_Show();
    }

}

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
	uint16_t ARR;
    switch (effect1)
    {
        case 0 ... 2: //LIGHTS DOWN
            current_effect_func = effect_lights_down;
            PSC = 21972;
            multiplier = 0.1;
            ownTempo=0;
            break;
        case 3 ... 4: //STATIC COLOUR
			current_effect_func = effect_static;
        	PSC = 21972;
            multiplier = 0.1;
            ownTempo=0;
            break;
        case 13 ... 14: //STATIC TWO COLOUR - (CAN USE 0 colour!)
			current_effect_func = effect_static_two_colour;
        	PSC = 21972;
            multiplier = 0.1;
            ownTempo=0;
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
            ownTempo=0;
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
            ownTempo=0;
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
         	ownTempo=0;
            break;
         case 21 ... 22: //MOVING GRADIENT; multiplier increases speed; modifier increases speed while decreasing smoothness and HW requirements
			current_effect_func = effect_moving_gradient;
            colourChanged=1;
            modifier=1;
            ownTempo=0;
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
         case 23 ... 24: //MOVING GLITCHY GRADIENT
			current_effect_func = effect_glitchy_gradient;
			colourChanged=1;
			modifier=1;
			ownTempo=0;
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
            ownTempo=0;
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
         case 27 ... 28: //MOVING GLITCHY GRADIENT
			current_effect_func = effect_glitchy;
            colourChanged=1;
            modifier=1;
            ownTempo=0;
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
         	ownTempo=0;
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
         case 35 ... 36: //JUMPING OWN
     		current_effect_func = effect_falling_drop;
         	modifier=1;
         	ownTempo=0;
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
        	ownTempo=0;
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
       		current_effect_func = effect_switch_colours;
         	modifier=1;
         	ownTempo=0;
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
         case 37 ... 38: //STROBE w FADING
       		current_effect_func = effect_strobe_fading;
         	modifier=1;
         	ownTempo=0;
            switch (effect2)
            {
               case 0 ... 36:    multiplier = 16;  PSC=137;  break; // x1
               case 37 ... 72:   multiplier = 2;  PSC=1100;  break; // x2
               case 73 ... 108:  multiplier = 4;  PSC=550;  break; // x4
               case 109 ... 144: multiplier = 8;  PSC=275;  break; // x4
               case 145 ... 180: multiplier = 16; PSC=137;  break; // x16
               case 181 ... 216: multiplier = 32; PSC=69;  break; // x32
               case 217 ... 255: multiplier = 64; PSC=34;  break; // x64
            }
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
