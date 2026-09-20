/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University

Author           : DAEJINKIM
Created          : 2021-05-03
Modified         : 2026-09-21
Language/ver     : C in VS Code

Description      : GPIO Library for STM32F411RE.
                   Provides GPIO mode, output speed, output type,
                   pull-up/pull-down, digital input, and
                   digital output functions.
/----------------------------------------------------------------*/

#ifndef __ECGPIO2_H
#define __ECGPIO2_H


/*----------------------------------------------------------------
 Header Files
----------------------------------------------------------------*/

// STM32F411RE register definitions
#include "stm32f411xe.h"

// RCC functions used to enable GPIO peripheral clocks
#include "ecRCC2.h"

// PinName_t and ecPinmap() definitions
#include "ecPinNames.h"


/*----------------------------------------------------------------
 GPIO Mode

 MODER register:
 00 : Input
 01 : Output
 10 : Alternate Function
 11 : Analog
----------------------------------------------------------------*/

#define INPUT       0x00
#define OUTPUT      0x01
#define AF          0x02
#define ANALOG      0x03


/*----------------------------------------------------------------
 Digital Logic Level

 HIGH : Logic 1
 LOW  : Logic 0
----------------------------------------------------------------*/

#define HIGH        1
#define LOW         0


/*----------------------------------------------------------------
 GPIO Output Speed

 OSPEEDR register:
 00 : Low Speed
 01 : Medium Speed
 10 : Fast Speed
 11 : High Speed
----------------------------------------------------------------*/

#define LOW_SPEED       0x00
#define MEDIUM_SPEED    0x01
#define FAST_SPEED      0x02
#define HIGH_SPEED      0x03


/*----------------------------------------------------------------
 GPIO Output Type

 OTYPER register:
 0 : Push-Pull
 1 : Open-Drain
----------------------------------------------------------------*/

#define PUSH_PULL       0x00
#define OPEN_DRAIN      0x01


/*----------------------------------------------------------------
 GPIO Pull-Up / Pull-Down

 PUPDR register:
 00 : No Pull-Up / Pull-Down
 01 : Pull-Up
 10 : Pull-Down
 11 : Reserved
----------------------------------------------------------------*/

#define NO_PUPD         0x00
#define PU              0x01
#define PD              0x02


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*----------------------------------------------------------------
 GPIO Functions
----------------------------------------------------------------*/

// Initialize the selected GPIO pin and configure its mode.
void GPIO_init(PinName_t pinName, uint32_t mode);


// Configure the GPIO pin as Input, Output,
// Alternate Function, or Analog mode.
void GPIO_mode(PinName_t pinName, uint32_t mode);


// Configure the output speed of the selected GPIO pin.
void GPIO_ospeed(PinName_t pinName, uint32_t speed);


// Configure the selected GPIO pin as
// Push-Pull or Open-Drain output.
void GPIO_otype(PinName_t pinName, uint32_t type);


// Configure the internal pull-up or pull-down resistor
// of the selected GPIO pin.
void GPIO_pupd(PinName_t pinName, uint32_t pupd);


// Write HIGH or LOW to the selected GPIO output pin.
void GPIO_write(PinName_t pinName, uint32_t Output);


// Read the digital input state of the selected GPIO pin.
// The return value is LOW(0) or HIGH(1).
uint32_t GPIO_read(PinName_t pinName);


/*----------------------------------------------------------------
 Arduino-like GPIO Functions
 Optional functions
----------------------------------------------------------------*/

// Configure the mode of a GPIO pin using Arduino-style syntax.
void pinMode(PinName_t pin, uint32_t mode);


// Write a digital value using Arduino-style syntax.
void digitalWrite(PinName_t pin, uint32_t value);


// Read a digital value using Arduino-style syntax.
uint32_t digitalRead(PinName_t pin);


/*----------------------------------------------------------------
 7-Segment Display Functions
----------------------------------------------------------------*/

// Lookup table containing segment patterns for the FND.
extern int numberFND[12][8];


// Initialize eight GPIO pins connected to
// segments a, b, c, d, e, f, g, and dp.
void FND_init(PinName_t *pinFND);


// Initialize four GPIO pins used to select FND0 to FND3.
void FND_select_init(PinName_t *selectFND);


// Display a number or predefined FND pattern.
void FND_display(uint8_t num);


// Select one of the four 7-segment displays.
void FND_select(uint8_t digit);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // __ECGPIO2_H