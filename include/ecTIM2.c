/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : DAEJINKIM
Created          : 05-03-2021
Modified         : 11-12-2026
Language/ver     : C in VS Code

Description      : Timer initialization, period setting, and update interrupts.
/----------------------------------------------------------------*/

#include "ecTIM2.h"

////////////////////////////////////////////////////////////////
/*						EXERCISE                             */
////////////////////////////////////////////////////////////////
/*
// Implemented timer initialization and period configuration functions.
void TIM_init(TIM_TypeDef* TIMx, uint32_t msec); 
void TIM_init_us(TIM_TypeDef* TIMx, uint32_t usec); 
void TIM_period_us(TIM_TypeDef* TIMx, uint32_t usec);
*/




//////////////////////////////////////////////
/* Timer Update Interrupt Configuration     */
//////////////////////////////////////////////


void TIM_UI_init(TIM_TypeDef* TIMx, uint32_t msec){
// 1. Initialize Timer	
	TIM_init(TIMx,msec);

// 2. Enable Update Interrupt
	TIM_UI_enable(TIMx);	

// 3. NVIC Setting
	uint32_t IRQn_reg = get_TIM_IRQn(TIMx);
	NVIC_EnableIRQ(IRQn_reg);				
	NVIC_SetPriority(IRQn_reg,2);
}

void TIM_UI_init_us(TIM_TypeDef* TIMx, uint32_t usec){
// 1. Initialize Timer	
	TIM_init_us(TIMx,usec);
	
// 2. Enable Update Interrupt
	TIM_UI_enable(TIMx);
	
// 3. NVIC Setting
	uint32_t IRQn_reg = get_TIM_IRQn(TIMx);
	NVIC_EnableIRQ(IRQn_reg);				
	NVIC_SetPriority(IRQn_reg,2);
}


// Enable Timer Update Interrupt		
void TIM_UI_enable(TIM_TypeDef* TIMx){
	TIMx->SR = ~TIM_SR_UIF;
	TIMx->DIER |= TIM_DIER_UIE;				
}

// Disable Timer Update Interrupt
void TIM_UI_disable(TIM_TypeDef* TIMx){
	TIMx->DIER &= ~(TIM_DIER_UIE);					
}

uint32_t is_UIF(TIM_TypeDef *TIMx){
	return TIMx->SR & TIM_SR_UIF;
}
void clear_UIF(TIM_TypeDef *TIMx){
	TIMx->SR &= ~TIM_SR_UIF;
}




////////////////////////////////////////////////////////////////
/* Timer Counter Initialization                              */
////////////////////////////////////////////////////////////////


// Timer Counter Initialization
// Default : f_cnt =  10kHz, msec= 1 to 6,553 msec 
void TIM_init(TIM_TypeDef* TIMx, uint32_t msec){
// 1. Enable Timer CLOCK and stop the counter during configuration
	TIM_clock_enable(TIMx);
	TIMx->CR1 &= ~TIM_CR1_CEN;

// 2. Set CNT period in milliseconds
	TIM_period(TIMx, msec);

// 3. Select edge-aligned up-counting
	TIMx->CR1 &= ~(TIM_CR1_DIR | TIM_CR1_CMS);

// 4. Apply the prescaler and initialize the counter
	TIMx->CNT = 0;
	TIMx->EGR = TIM_EGR_UG;
	TIMx->SR = ~TIM_SR_UIF;

// 5. Enable Timer Counter
	TIMx->CR1 |= TIM_CR1_CEN;
}


// Timer Counter Initialization
// Default : f_cnt =  1 MHz, usec= 1 to 65,536 usec
void TIM_init_us(TIM_TypeDef* TIMx, uint32_t usec){
// 1. Enable Timer CLOCK and stop the counter during configuration
	TIM_clock_enable(TIMx);
	TIMx->CR1 &= ~TIM_CR1_CEN;

// 2. Set CNT period in microseconds
	TIM_period_us(TIMx, usec);

// 3. Select edge-aligned up-counting
	TIMx->CR1 &= ~(TIM_CR1_DIR | TIM_CR1_CMS);

// 4. Apply the prescaler and initialize the counter
	TIMx->CNT = 0;
	TIMx->EGR = TIM_EGR_UG;
	TIMx->SR = ~TIM_SR_UIF;

// 5. Enable Timer Counter
	TIMx->CR1 |= TIM_CR1_CEN;
}


// Limits    : 16-bit ARR -> f_cnt =  10kHz, msec= 1 to 6,553 msec (ARR < 0xFFFF)
void TIM_period(TIM_TypeDef* TIMx, uint32_t msec){

	uint32_t Sys_CLK = EC_SYSCLK;					// 84MHz (PLL) or 16MHz (HSI)

	// Counter Frequency (f_cnt) 
	uint32_t f_cnt   =  10000UL;					// 16-bit CNT: f_cnt=10kHz (default)
	
	// Prescaler Value
	uint32_t PSCval = Sys_CLK / f_cnt;				// 84MHz -> 10 kHz (division=8400, PSC=8399)
	TIMx->PSC = PSCval - 1;

	// Reload Value (ARR) for msec unit
	uint32_t ARRval = (f_cnt / 1000UL) * msec;		// Reload values needed for msec 	
	TIMx->ARR = ARRval - 1;
}


// Limits    : 16-bit ARR -> f_cnt =  1 MHz, usec= 1 to 65,536 usec (ARR <= 0xFFFF)
void TIM_period_us(TIM_TypeDef* TIMx, uint32_t usec){

	uint32_t Sys_CLK = EC_SYSCLK;					// 84MHz (PLL) or 16MHz (HSI)
	uint32_t ARRval=0;

	// Counter Frequency (f_cnt) 
	uint32_t f_cnt   =  1000000UL;					// 16-bit CNT: f_cnt=1MHz (default)
	
	// Prescaler Value
	uint32_t PSCval = Sys_CLK / f_cnt;				// 84MHz -> 1 MHz (division=84, PSC=83)
	TIMx->PSC = PSCval - 1;

	// Reload Value (ARR) for microsecond units
	ARRval = (f_cnt / 1000000UL) * usec;
	TIMx->ARR = ARRval - 1;
}




void TIM_clock_enable(TIM_TypeDef* TIMx){ 
	if(TIMx ==TIM1) RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	else if(TIMx ==TIM2) RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	else if(TIMx ==TIM3) RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	else if(TIMx ==TIM4) RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
	else if(TIMx ==TIM5) RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
	else if(TIMx ==TIM9) RCC->APB2ENR |= RCC_APB2ENR_TIM9EN;
	else if(TIMx ==TIM10) RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;
	else if(TIMx ==TIM11) RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;	
}



uint32_t get_TIM_IRQn(TIM_TypeDef* TIMx){
	uint32_t IRQn_reg = 0x00000000;
	if(TIMx == TIM1)       IRQn_reg = TIM1_UP_TIM10_IRQn;
	else if(TIMx == TIM2)  IRQn_reg = TIM2_IRQn;
	else if(TIMx == TIM3)  IRQn_reg = TIM3_IRQn;
	else if(TIMx == TIM4)  IRQn_reg = TIM4_IRQn;
	else if(TIMx == TIM5)  IRQn_reg = TIM5_IRQn;
	else if(TIMx == TIM9)  IRQn_reg = TIM1_BRK_TIM9_IRQn;
	else if(TIMx == TIM10) IRQn_reg = TIM1_UP_TIM10_IRQn;
	else if(TIMx == TIM11) IRQn_reg = TIM1_TRG_COM_TIM11_IRQn;
	return IRQn_reg;	
}