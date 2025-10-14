/*
 * effects.c
 *
 *  Created on: Oct 14, 2025
 *      Author: Vilem Broucek
 */
#include "effects.h"

static uint16_t bpm = 82;
static uint8_t newEffect = 0;
static uint32_t ticks_per_interrupt;
static TIM_HandleTypeDef *htimLocal;
static uint32_t step;

void effects_step_iterate()
{
	step++;
}

void effects_init(TIM_HandleTypeDef *htim)
{
	htimLocal=htim;
	newEffect=1;
	ARGB_Init();
	ARGB_Clear();
	ARGB_Show();
	effects_set_bpm(41);
}

void set_timer(TIM_HandleTypeDef *htim, uint16_t ARR, uint16_t PSC)
{
    __HAL_TIM_SET_AUTORELOAD(htimLocal, ARR);
    __HAL_TIM_SET_PRESCALER(htimLocal, PSC);
}

void effects_set_bpm(uint16_t new_bpm) //ToDo: Rozmyslet auto reload preload -
{
    bpm = new_bpm;
    float ms = 60000.0f / bpm;  //one beat in ms; (1/(bpm/60))*1000
    ticks_per_interrupt = (uint16_t)(ms * 10.0f); // convert ms to ticks; 32-BIT TIMER -> chain two?
    __HAL_TIM_SET_AUTORELOAD(htimLocal, ticks_per_interrupt - 1);
    __HAL_TIM_SET_PRESCALER(htimLocal, 6399);
    //__HAL_TIM_SET_COUNTER(&htim2, 0);
    //__HAL_TIM_GENERATE_EVENT(&htim2, TIM_EVENTSOURCE_UPDATE);
}
//ToDo:!!remove effect, currentColour, bpm - variables that doesn't need to be passed each timer cycle (we will know if they change)
void effects_set_eff(uint16_t effect, ColourName_t currentColour, uint16_t bpm)
{
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
		 switch(effect) //each effect must start with step=0;
		 {
		 case 1:
			  if(step==1)
			  {
				  ARGB_FillRGB(colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
				  ARGB_FillWhite(colourTable[currentColour].w);
				  ARGB_Show();
				  //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); //Zapne
			  }
			  else
			  {
				  ARGB_Clear();
				  ARGB_Show();
				  step=0;
				  //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); //vypne
			  }
			 break;
		 case 2: //ToDo: overflow can happen due to float low resolution?
			 static float colourChangeVector[3];
			 static float currentColourChanged[3];
			 if(newEffect==1)
			 {
				    float ms = (60.0f / 41.0f)/144.0f;  //one beat in ms divided by LEDCOUNT; (1/(bpm/60))*1000/144
				    ticks_per_interrupt = (uint16_t)((ms * 64000000.0f)/(14+1)); // convert ms to ticks; 32-BIT TIMER -> chain two?
				    __HAL_TIM_SET_AUTORELOAD(htimLocal, ticks_per_interrupt - 1);
				    __HAL_TIM_SET_PRESCALER(htimLocal, 14);

					colourChangeVector[0]=(colourTable[secondColour].r-colourTable[currentColour].r)/(144); //144 = step count
					colourChangeVector[1]=(colourTable[secondColour].g-colourTable[currentColour].g)/(144);
					colourChangeVector[2]=(colourTable[secondColour].b-colourTable[currentColour].b)/(144);

				 	currentColourChanged[0]=colourTable[currentColour].r;
				 	currentColourChanged[1]=colourTable[currentColour].g;
				 	currentColourChanged[2]=colourTable[currentColour].b;

				    newEffect=0;
			 }

			 currentColourChanged[0]+=colourChangeVector[0];
			 currentColourChanged[1]+=colourChangeVector[1];
			 currentColourChanged[2]+=colourChangeVector[2];

			 ARGB_Clear();
			 ARGB_SetRGB(144-step, (uint8_t)(currentColourChanged[0]+0.5f), (uint8_t)(currentColourChanged[1]+0.5f), (uint8_t)(currentColourChanged[2]+0.5f));
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
			 break;
		  case 3:

				 if(newEffect==1)
				 {
					    //ToDo: Correct refresh rate; 120Hz
					 	//ToDo: Get rid of float -> can work with integers
					 	__HAL_TIM_SET_AUTORELOAD(htimLocal, 10457);
					    __HAL_TIM_SET_PRESCALER(htimLocal, 50);



					    newEffect=0;
				 }
				 static float y = 100.0f; //initial pos; max jump height
				 static float vy = 0.0f;  //initial speed
				 static const float g = -0.03f; //gravity, +- changes orientation



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
				 ARGB_SetRGB((uint32_t)(y + 0.5f), 255, 255, 255); //rounds and casts to int
				 ARGB_Show();
		 	 break;
		  case 4:
			 switch(step)
			 {
			 case 0:
			         ARGB_Clear();
			         ARGB_SetRGB(5, colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
			         ARGB_Show();
			         break;

			     case 1:
			         ARGB_Clear();
			         ARGB_SetRGB(25, colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
			         ARGB_Show();
			         break;

			     case 2:
			         ARGB_Clear();
			         ARGB_SetRGB(50, colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
			         ARGB_Show();
			         break;

			     case 3:
			         ARGB_Clear();
			         ARGB_SetRGB(8, colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
			         ARGB_Show();
			         break;

			     case 4:
			         ARGB_Clear();
			         ARGB_SetRGB(80, colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
			         ARGB_Show();
			         break;

			     case 5:
			         ARGB_Clear();
			         ARGB_SetRGB(55, colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
			         ARGB_Show();
			         break;

			     case 6:
			         ARGB_Clear();
			         ARGB_SetRGB(100, colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
			         ARGB_Show();
			         break;

			     case 7:
			         ARGB_Clear();
			         ARGB_SetRGB(28, colourTable[currentColour].r, colourTable[currentColour].g, colourTable[currentColour].b);
			         ARGB_Show();
			         step=0;
			         break;
			     default:
			         step=0;
			         break;

			 }
			 break;
		 }
}

