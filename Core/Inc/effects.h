/*
 * effects.h
 *
 *  Created on: Oct 14, 2025
 *      Author: Vilem Broucek
 */
//For proper function, effect.c needs an assigned timer with NVIC enabled. It also expects an interrupt callback
//in main, with basic iteration (steps), that are handed back via a function.

#ifndef INC_EFFECTS_H_
#define INC_EFFECTS_H_

//#include "stm32f1xx_hal.h"
//#include "stm32f1xx_hal_tim.h"
#include "ARGB.h"
#include "colours.h"

//extern TIM_HandleTypeDef htim2;

void effects_init(TIM_HandleTypeDef *htim);
void effects_set_bpm(uint16_t new_bpm);
void effects_set_eff(uint16_t effect, ColourName_t currentColour,uint16_t bpm);
void effects_step_iterate();

#endif /* INC_EFFECTS_H_ */
