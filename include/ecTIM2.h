/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : DAEJINKIM
Created          : 05-03-2021
Modified         : 11-12-2026
Language/ver     : C/C++ in VS Code

Description      : Timer initialization, period setting, and update interrupts.
/----------------------------------------------------------------*/

#ifndef __EC_TIM2_H
#define __EC_TIM2_H

#include "stm32f411xe.h"
#include "ecPinNames.h"
#include "ecRCC2.h"
#include "ecGPIO2.h"
#include "math.h"

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */


//////////////////////////////////////////////
/* Timer Update Interrupt Configuration     */
//////////////////////////////////////////////

// Timer Update Interrupt Initialization
void TIM_UI_init(TIM_TypeDef* TIMx, uint32_t msec);     // 16-bit ARR, 10 kHz counter: 1 to 6,553 ms 
void TIM_UI_init_us(TIM_TypeDef* TIMx, uint32_t usec);  // 16-bit ARR, 1 MHz counter: 1 to 65,536 us 

// Timer Update Interrupt On/OFf
void TIM_UI_enable(TIM_TypeDef* TIMx);
void TIM_UI_disable(TIM_TypeDef* TIMx);

// Timer Update Interrupt Flag
uint32_t is_UIF(TIM_TypeDef *TIMx);
void clear_UIF(TIM_TypeDef *TIMx);


////////////////////////////////////////////////////////////////
/* Timer Counter Initialization                              */
////////////////////////////////////////////////////////////////

// Timer Counter Initialization
void TIM_init(TIM_TypeDef *TIMx, uint32_t msec); 
void TIM_init_us(TIM_TypeDef *TIMx, uint32_t usec); 

// Timer Counter Period Config: TIM_period uses ms; TIM_period_us uses us
void TIM_period(TIM_TypeDef* TIMx, uint32_t msec);		
void TIM_period_us(TIM_TypeDef* TIMx, uint32_t usec);  	

// Timer Counter Auxiliary Functions
uint32_t get_TIM_IRQn(TIM_TypeDef* TIMx);
void TIM_clock_enable(TIM_TypeDef* TIMx);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __EC_TIM2_H 
