/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : DAEJINKIM
Created          : 05-03-2021
Modified         : 10-01-2026
Language/ver     : C++ in VS Code

Description      : External Interrupt (EXTI) control library
/----------------------------------------------------------------*/

#include "ecGPIO2.h"
#include "ecSysTick2.h"
#include "ecEXTI2.h"


void EXTI_init(PinName_t pinName, uint32_t trig_type, uint32_t priority){

    GPIO_TypeDef *Port;
    unsigned int pin;
    ecPinmap(pinName, &Port, &pin);

    // Enable EXTI Clock with SYSCFG controller clock
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;


    // Connect EXTI to the GPIO Port_y Pin_x
    // Pin 0~3   : EXTICR1 // EXTICR[0]
    // Pin 4~7   : EXTICR2 // EXTICR[1]
    // Pin 8~11  : EXTICR3 // EXTICR[2]
    // Pin 12~15 : EXTICR4 // EXTICR[3]
    uint32_t EXTICR_port = 0;

    if      (Port == GPIOA) EXTICR_port = 0;
    else if (Port == GPIOB) EXTICR_port = 1;
    else if (Port == GPIOC) EXTICR_port = 2;
    else if (Port == GPIOD) EXTICR_port = 3;
    else if (Port == GPIOE) EXTICR_port = 4;

    // Clear previous 4-bit port selection
    SYSCFG->EXTICR[pin / 4] &=
        ~(0xFUL << ((pin % 4) * 4));

    // Set GPIO port for corresponding EXTI line
    SYSCFG->EXTICR[pin / 4] |=
        (EXTICR_port << ((pin % 4) * 4));


    // Clear previous trigger configuration first
    EXTI->RTSR &= ~(1UL << pin);
    EXTI->FTSR &= ~(1UL << pin);

    // Select Trigger edge (RISE, FALL, BOTH)
    if (trig_type == FALL) {
        EXTI->FTSR |= (1UL << pin);       // Falling trigger enable
    }
    else if (trig_type == RISE) {
        EXTI->RTSR |= (1UL << pin);       // Rising trigger enable
    }
    else if (trig_type == BOTH) {
        EXTI->RTSR |= (1UL << pin);       // Rising trigger enable
        EXTI->FTSR |= (1UL << pin);       // Falling trigger enable
    }


    // Enable EXTI, with Not-Masked Interrupt Request
    // Not-mask (== Enable) EXTIx
    EXTI->IMR |= (1UL << pin);


    // NVIC(IRQ) Setting
    IRQn_Type EXTI_IRQn;

    if (pin < 5)
        EXTI_IRQn = (IRQn_Type)(EXTI0_IRQn + pin);
    else if (pin < 10)
        EXTI_IRQn = EXTI9_5_IRQn;
    else
        EXTI_IRQn = EXTI15_10_IRQn;

    NVIC_SetPriority(EXTI_IRQn, priority);
    NVIC_EnableIRQ(EXTI_IRQn);
}


void EXTI_enable(PinName_t pinName) {

    GPIO_TypeDef *Port;
    unsigned int pin;
    ecPinmap(pinName, &Port, &pin);

    EXTI->IMR |= (1UL << pin);     // not masked = Interrupt enabled
}


void EXTI_disable(PinName_t pinName) {

    GPIO_TypeDef *Port;
    unsigned int pin;
    ecPinmap(pinName, &Port, &pin);

    EXTI->IMR &= ~(1UL << pin);    // masked = Interrupt disabled
}


uint32_t is_pending_EXTI(PinName_t pinName) {

    GPIO_TypeDef *Port;
    unsigned int pin;
    ecPinmap(pinName, &Port, &pin);

    // Read PRx and return only 0 or 1
    uint32_t EXTI_PRx = (EXTI->PR >> pin) & 1UL;

    return EXTI_PRx;
}


void clear_pending_EXTI(PinName_t pinName){

    GPIO_TypeDef *Port;
    unsigned int pin;
    ecPinmap(pinName, &Port, &pin);

    // EXTI_PR is Write-1-to-Clear
    EXTI->PR = (1UL << pin);
}