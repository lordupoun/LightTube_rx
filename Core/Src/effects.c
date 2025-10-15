/*
 * effects.c
 *
 *  Created on: Oct 14, 2025
 *      Author: Vilem Broucek
 */
#include "effects.h"
#include "math.h"

static uint16_t bpm = 82;
static uint8_t newEffect = 0;
static uint32_t ticks_per_interrupt;
static TIM_HandleTypeDef *htimLocal;
static uint32_t step;

void effects_step_iterate() {
	step++;
}

void effects_init(TIM_HandleTypeDef *htim) {
	htimLocal = htim;
	newEffect = 1;
	ARGB_Init();
	ARGB_Clear();
	ARGB_Show();
	effects_set_bpm(41);
}

void set_timer(TIM_HandleTypeDef *htim, uint16_t ARR, uint16_t PSC) {
	__HAL_TIM_SET_AUTORELOAD(htimLocal, ARR);
	__HAL_TIM_SET_PRESCALER(htimLocal, PSC);
}

void effects_set_bpm(uint16_t new_bpm) //ToDo: Rozmyslet auto reload preload -
{
	bpm = new_bpm;
	float ms = 60000.0f / bpm;  //one beat in ms; (1/(bpm/60))*1000
	ticks_per_interrupt = (uint16_t) (ms * 10.0f); // convert ms to ticks; 32-BIT TIMER -> chain two?
	__HAL_TIM_SET_AUTORELOAD(htimLocal, ticks_per_interrupt - 1);
	__HAL_TIM_SET_PRESCALER(htimLocal, 6399);
	//__HAL_TIM_SET_COUNTER(&htim2, 0);
	//__HAL_TIM_GENERATE_EVENT(&htim2, TIM_EVENTSOURCE_UPDATE);
}
//ToDo:!!remove effect, currentColour, bpm - variables that doesn't need to be passed each timer cycle (we will know if they change)
void effects_set_eff(uint16_t effect, ColourName_t currentColour, uint16_t bpm) {
	static ColourName_t secondColour = PRIMARY_RED;
	//stepLocal=step;
//SWITCH //Vypreparovat do samostatneho souboru, volat funkci animation(effect, step);
	//case STILL
	//case STROBE_1/1
	//case STROBE_1/2
	//case STROBE_1/4 pro ruzne varianty zmenit BPM, nebo predelat efekt?
	//case STROBE_1/8
	//case STROBE_1/16
	//case STROBE_1/32
	//case DROP
	switch (effect) //each effect must start with step=0;
	{
	case 0: //lights down
		if (newEffect == 1)
		{
			__HAL_TIM_SET_AUTORELOAD(htimLocal, 10000);
			__HAL_TIM_SET_PRESCALER(htimLocal, 10000);
			__HAL_TIM_SET_COUNTER(htimLocal, 0);

			ARGB_Clear();
			ARGB_Show();

			newEffect = 0;
		}
	case 1:
		if (newEffect == 1)
		{
			__HAL_TIM_SET_AUTORELOAD(htimLocal, 30000);
			__HAL_TIM_SET_PRESCALER(htimLocal, 30000);
			__HAL_TIM_SET_COUNTER(htimLocal, 0);
			newEffect = 0;
		}
		ARGB_FillRGB(
				colourTable[currentColour].r,
				colourTable[currentColour].g,
				colourTable[currentColour].b);
		ARGB_FillWhite(colourTable[currentColour].w);
		ARGB_Show();
	case 1:
		if (step == 1) {
			ARGB_FillRGB(
					colourTable[currentColour].r,
					colourTable[currentColour].g,
					colourTable[currentColour].b);
			ARGB_FillWhite(colourTable[currentColour].w);
			ARGB_Show();
			//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); //Zapne
		} else {
			ARGB_Clear();
			ARGB_Show();
			step = 0;
			//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); //vypne
		}
		break;
	case 2: //Faster; but float can overflow -> creates visual artefact; but it won't happen under approx. 100 000 steps
		static uint8_t colourMaxStep = 132; //defines a step, where colour stops changing; maximum is maxAnimationSteps-1
		static float colourChangeVector[3];
		static float currentColourChanged[3];
		if (newEffect == 1)
		{
			float ms = (60.0f / 82.0f) / 144.0f; //one beat in ms divided by LEDCOUNT; (1/(bpm/60))*1000/144
			ticks_per_interrupt = (uint16_t) ((ms * 64000000.0f) / (14 + 1)); // convert ms to ticks; 32-BIT TIMER -> chain two?
			__HAL_TIM_SET_AUTORELOAD(htimLocal, ticks_per_interrupt - 1);
			__HAL_TIM_SET_PRESCALER(htimLocal, 14);

			colourChangeVector[0] = (colourTable[secondColour].r - colourTable[currentColour].r) / (float) colourMaxStep; //144-1 = max step count step count
			colourChangeVector[1] = (colourTable[secondColour].g - colourTable[currentColour].g) / (float) colourMaxStep;
			colourChangeVector[2] = (colourTable[secondColour].b - colourTable[currentColour].b) / (float) colourMaxStep;

			currentColourChanged[0] = colourTable[currentColour].r; //A: faster to compute
			currentColourChanged[1] = colourTable[currentColour].g;
			currentColourChanged[2] = colourTable[currentColour].b;

			newEffect = 0;
		}
		if (step != 0) //displays the original primary colour at least once
		{
			if (step < colourMaxStep)
			{
				currentColourChanged[0] += colourChangeVector[0]; //A: faster to compute
				currentColourChanged[1] += colourChangeVector[1];
				currentColourChanged[2] += colourChangeVector[2];
			}
			else
			{
				currentColourChanged[0] = colourTable[secondColour].r;
				currentColourChanged[1] = colourTable[secondColour].g;
				currentColourChanged[2] = colourTable[secondColour].b;
			}
		}
		//currentColourChanged[0] = colourTable[currentColour].r + step * colourChangeVector[0]; //B: more precise
		//currentColourChanged[1] = colourTable[currentColour].g + step * colourChangeVector[1];
		//currentColourChanged[2] = colourTable[currentColour].b + step * colourChangeVector[2];
		ARGB_Clear();
		ARGB_SetRGB(
				144 - step,
				(uint8_t) currentColourChanged[0] + 0.5f, //ToDo: Add clamps - there can be blinking due to float inconsitency, or colourMaxStep-1
				(uint8_t) currentColourChanged[1] + 0.5f, //0.5f smooth round
				(uint8_t) currentColourChanged[2] + 0.5f); //(uint8_t)fminf(fmaxf((currentColourChanged[2]+0.5f), 0.0f), 255.0f));
		ARGB_SetWhite(
				144 - step,
				colourTable[currentColour].w);
		ARGB_Show();
		if (step >= 144)
		{
			step = 0;
			currentColourChanged[0] = colourTable[currentColour].r;
			currentColourChanged[1] = colourTable[currentColour].g;
			currentColourChanged[2] = colourTable[currentColour].b;
		}
		break;
		/*case 2: //Safer
		 static uint8_t colourMaxStep=132; //defines a step, where colour stops changing; maximum is maxAnimationSteps-1
		 static float colourChangeVector[3];
		 static float currentColourChanged[3];
		 if(newEffect==1)
		 {
		 float ms = (60.0f / 82.0f)/144.0f;  //one beat in ms divided by LEDCOUNT; (1/(bpm/60))*1000/144
		 ticks_per_interrupt = (uint16_t)((ms * 64000000.0f)/(14+1)); // convert ms to ticks; 32-BIT TIMER -> chain two?
		 __HAL_TIM_SET_AUTORELOAD(htimLocal, ticks_per_interrupt - 1);
		 __HAL_TIM_SET_PRESCALER(htimLocal, 14);

		 colourChangeVector[0]=(colourTable[secondColour].r-colourTable[currentColour].r)/(float)colourMaxStep; //144-1 = max step count step count
		 colourChangeVector[1]=(colourTable[secondColour].g-colourTable[currentColour].g)/(float)colourMaxStep;
		 colourChangeVector[2]=(colourTable[secondColour].b-colourTable[currentColour].b)/(float)colourMaxStep;

		 currentColourChanged[0]=colourTable[currentColour].r; //A: faster to compute
		 currentColourChanged[1]=colourTable[currentColour].g;
		 currentColourChanged[2]=colourTable[currentColour].b;

		 newEffect=0;
		 }
		 if(step!=0) //displays the original primary colour at least once
		 {
		 currentColourChanged[0]+=colourChangeVector[0]; //A: faster to compute
		 currentColourChanged[1]+=colourChangeVector[1];
		 currentColourChanged[2]+=colourChangeVector[2];
		 }
		 //currentColourChanged[0] = colourTable[currentColour].r + step * colourChangeVector[0]; //B: more precise
		 //currentColourChanged[1] = colourTable[currentColour].g + step * colourChangeVector[1];
		 //currentColourChanged[2] = colourTable[currentColour].b + step * colourChangeVector[2];

		 ARGB_Clear();
		 ARGB_SetRGB(144-step,
		 (uint8_t)fminf(fmaxf((currentColourChanged[0]+0.5f), 0.0f), 255.0f), //ToDo: Add clamps - there can be blinking due to float inconsitency, or colourMaxStep-1
		 (uint8_t)fminf(fmaxf((currentColourChanged[1]+0.5f), 0.0f), 255.0f), //0.5f smooth round
		 (uint8_t)fminf(fmaxf((currentColourChanged[2]+0.5f), 0.0f), 255.0f));;//(uint8_t)fminf(fmaxf((currentColourChanged[2]+0.5f), 0.0f), 255.0f));
		 //ARGB_SetRGB(144-step, (uint8_t)(currentColourChanged[0]), (uint8_t)(currentColourChanged[1]), (uint8_t)(currentColourChanged[2]));
		 ARGB_SetWhite(144-step, colourTable[currentColour].w);
		 ARGB_Show();
		 if(step>=144)
		 {
		 step=0;
		 currentColourChanged[0]=colourTable[currentColour].r;
		 currentColourChanged[1]=colourTable[currentColour].g;
		 currentColourChanged[2]=colourTable[currentColour].b;
		 }
		 break;*/
	case 3: //Jumpin - physics - No tempo
		if (newEffect == 1)
		{
			//ToDo: Correct refresh rate; 120Hz
			//ToDo: Get rid of float -> can work with integers
			__HAL_TIM_SET_AUTORELOAD(htimLocal, 10457);
			__HAL_TIM_SET_PRESCALER(htimLocal, 50);
			__HAL_TIM_SET_COUNTER(htimLocal, 0);
			newEffect = 0;
		}
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
		ARGB_SetRGB((uint8_t) (y + 0.5f), colourTable[currentColour].r,
				colourTable[currentColour].g, colourTable[currentColour].b); //rounds and casts to int
		ARGB_Show();
		break;
	case 4: //Jumping - goniometric
	{

		static const float H = 100.0f;
		static const float refreshRate = 120.0f;
		static const float BPM = 82.0f;
		static const float beatsPerJump = 2.0f;
		static float phase = 0.0f; //immediate phase of sinus curve
		static float f;
		static float phaseInc;
		static float y;

		if (newEffect == 1) {
			__HAL_TIM_SET_AUTORELOAD(htimLocal, 11110);
			__HAL_TIM_SET_PRESCALER(htimLocal, 47);
			__HAL_TIM_SET_COUNTER(htimLocal, 0);

			f = BPM / 60.0f / beatsPerJump;  //BPM to jump frequency in Hz
			phaseInc = 2.0f * M_PI * f / refreshRate; //increment

			newEffect = 0;
		}

		phase += phaseInc;
		if (phase > 2.0f * M_PI)
			phase -= 2.0f * M_PI;

		y = 0.5f * H * (1.0f - cosf(phase));

		ARGB_Clear();
		ARGB_SetRGB((uint8_t) (y + 0.5f), colourTable[currentColour].r,
				colourTable[currentColour].g, colourTable[currentColour].b);
		ARGB_Show();
	}
		break;
	case 5: //Jumping - complete
	{

		static const float H = 143.0f;
		static const float refreshRate = 120.0f;
		static const float BPM = 82.0f;
		static const float beatsPerJump = 2.0f;
		static float phase = 0.0f; //immediate phase of sinus curve
		static float f;
		static float phaseInc;
		static float y;

		if (newEffect == 1) {
			__HAL_TIM_SET_AUTORELOAD(htimLocal, 11110);
			__HAL_TIM_SET_PRESCALER(htimLocal, 47);
			__HAL_TIM_SET_COUNTER(htimLocal, 0);

			f = BPM / 60.0f / beatsPerJump;  //BPM to jump frequency in Hz
			phaseInc = 2.0f * M_PI * f / refreshRate; //increment

			newEffect = 0;
		}

		phase += phaseInc;
		if (phase > 2.0f * M_PI)
			phase -= 2.0f * M_PI;

		y = 0.5f * H * (1.0f - cosf(phase));

		ARGB_Clear();
		ARGB_SetRGB((uint8_t) (y + 0.5f), colourTable[currentColour].r,
				colourTable[currentColour].g, colourTable[currentColour].b);
		ARGB_Show();
	}
		break;
	}
}

