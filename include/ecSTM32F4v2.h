/*---------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author          : DAEJINKIM
Modified        : 30-09-2026
Language/ver    : C++ in VS Code
Description     : STM32F411 Library
/----------------------------------------------------------*/

#ifndef __EC_STM_H
#define __EC_STM_H

// STM built-In Library
#include "stm32f4xx.h"
#include "stm32f411xe.h"
#include "math.h"

// EC course Library
#include "ecPinNames.h"
#include "ecRCC2.h"
#include "ecGPIO2.h"
#include "ecEXTI2.h"
#include "ecSysTick2.h"
// #include "ecTIM2.h"
// #include "ecPWM2.h"
#include "ecStepper2.h"
// #include "ecADC2.h"
#include "ecUART2.h"

static inline void MCU_init(void){
    RCC_PLL_init(); // CLOCK PLL 84MHz
    SysTick_init(); // SysTick 1msec
    
    GPIO_init(PC_13, INPUT); // Button PC13
    GPIO_pupd(PC_13, PU); // PULL-UP for PC13
    
    GPIO_init(PA_5, OUTPUT); // LED PA5

    // 각 실습의 setup()에서 사용하는 타이머/PWM/UART만 초기화한다.

}

#endif





 