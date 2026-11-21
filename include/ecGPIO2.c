/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : DAEJINKIM
Created          : 2021-05-03
Modified         : 2026-09-21
Language/ver     : C in VS Code

Description      : GPIO Library for STM32F411RE.
                   Provides GPIO initialization, mode, speed,
                   output type, pull-up/pull-down, digital write,
                   and digital read functions.
/----------------------------------------------------------------*/

#include "ecGPIO2.h"


/*----------------------------------------------------------------
 GPIO Initialization

 This function converts PinName_t into the corresponding GPIO port
 and pin number, enables the GPIO peripheral clock, and configures
 the GPIO operating mode.
----------------------------------------------------------------*/
void GPIO_init(PinName_t pinName, uint32_t mode)
{
    // Pointer to the GPIO port corresponding to pinName.
    GPIO_TypeDef *Port;

    // Stores the GPIO pin number from 0 to 15.
    unsigned int pin;

    // Convert a symbolic pin name such as PA_0 or PB_12
    // into the actual GPIO port address and pin number.
    ecPinmap(pinName, &Port, &pin);


    // Enable GPIOA peripheral clock when the pin belongs to Port A.
    if (Port == GPIOA)
    {
        RCC_GPIOA_enable();
    }

    // Enable GPIOB peripheral clock when the pin belongs to Port B.
    else if (Port == GPIOB)
    {
        RCC_GPIOB_enable();
    }

    // Enable GPIOC peripheral clock when the pin belongs to Port C.
    else if (Port == GPIOC)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    }

    // Enable GPIOD peripheral clock when the pin belongs to Port D.
    else if (Port == GPIOD)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    }

    // Enable GPIOE peripheral clock when the pin belongs to Port E.
    else if (Port == GPIOE)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    }

    // Enable GPIOH peripheral clock when the pin belongs to Port H.
    else if (Port == GPIOH)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
    }


    // Configure the selected pin as INPUT, OUTPUT,
    // Alternate Function, or Analog mode.
    GPIO_mode(pinName, mode);
}


/*----------------------------------------------------------------
 GPIO Mode

 MODER uses two bits for each GPIO pin.

 00 : Input
 01 : Output
 10 : Alternate Function
 11 : Analog
----------------------------------------------------------------*/
void GPIO_mode(PinName_t pinName, uint32_t mode)
{
    // Pointer to the selected GPIO port.
    GPIO_TypeDef *Port;

    // GPIO pin number.
    unsigned int pin;

    // Convert PinName_t into the actual GPIO port and pin.
    ecPinmap(pinName, &Port, &pin);


    // Clear the previous two MODER bits of the selected GPIO pin.
    Port->MODER &= ~(3UL << (2 * pin));

    // Write the new mode into the corresponding two MODER bits.
    // Masking with 0x3 ensures that only two bits are used.
    Port->MODER |= ((mode & 0x3U) << (2 * pin));
}


/*----------------------------------------------------------------
 GPIO Output Speed

 OSPEEDR uses two bits for each GPIO pin.

 00 : Low Speed
 01 : Medium Speed
 10 : Fast Speed
 11 : High Speed
----------------------------------------------------------------*/
void GPIO_ospeed(PinName_t pinName, uint32_t speed)
{
    // Pointer to the selected GPIO port.
    GPIO_TypeDef *Port;

    // GPIO pin number.
    unsigned int pin;

    // Convert PinName_t into GPIO port and pin number.
    ecPinmap(pinName, &Port, &pin);


    // Clear the previous speed configuration.
    Port->OSPEEDR &= ~(3UL << (2 * pin));

    // Set the requested output speed.
    Port->OSPEEDR |= ((speed & 0x3U) << (2 * pin));
}


/*----------------------------------------------------------------
 GPIO Output Type

 OTYPER uses one bit for each GPIO pin.

 0 : Push-Pull
 1 : Open-Drain
----------------------------------------------------------------*/
void GPIO_otype(PinName_t pinName, uint32_t type)
{
    // Pointer to the selected GPIO port.
    GPIO_TypeDef *Port;

    // GPIO pin number.
    unsigned int pin;

    // Convert PinName_t into GPIO port and pin number.
    ecPinmap(pinName, &Port, &pin);


    // Clear the current output-type bit.
    Port->OTYPER &= ~(1UL << pin);

    // Set Push-Pull or Open-Drain.
    // Only the lowest bit of type is used.
    Port->OTYPER |= ((type & 0x1U) << pin);
}


/*----------------------------------------------------------------
 GPIO Pull-Up / Pull-Down

 PUPDR uses two bits for each GPIO pin.

 00 : No Pull-Up / Pull-Down
 01 : Pull-Up
 10 : Pull-Down
 11 : Reserved
----------------------------------------------------------------*/
void GPIO_pupd(PinName_t pinName, uint32_t pupd)
{
    // Pointer to the selected GPIO port.
    GPIO_TypeDef *Port;

    // GPIO pin number.
    unsigned int pin;

    // Convert PinName_t into GPIO port and pin number.
    ecPinmap(pinName, &Port, &pin);


    // Clear the previous pull-up/pull-down configuration.
    Port->PUPDR &= ~(3UL << (2 * pin));

    // Set No-Pull, Pull-Up, or Pull-Down.
    Port->PUPDR |= ((pupd & 0x3U) << (2 * pin));
}


/*----------------------------------------------------------------
 GPIO Digital Write

 ODR stores the output state of each GPIO pin.

 LOW  : ODR bit = 0
 HIGH : ODR bit = 1
----------------------------------------------------------------*/
void GPIO_write(PinName_t pinName, uint32_t Output)
{
    // Pointer to the selected GPIO port.
    GPIO_TypeDef *Port;

    // GPIO pin number.
    unsigned int pin;

    // Convert PinName_t into GPIO port and pin number.
    ecPinmap(pinName, &Port, &pin);


    // Clear the output bit first.
    Port->ODR &= ~(1UL << pin);

    // Write the requested LOW or HIGH value.
    // Only bit 0 of Output is used.
    Port->ODR |= ((Output & 0x1U) << pin);
}


/*----------------------------------------------------------------
 GPIO Digital Read

 IDR contains the actual digital input state of each GPIO pin.

 The selected pin value is shifted to bit 0 so that this function
 always returns either LOW(0) or HIGH(1).
----------------------------------------------------------------*/
uint32_t GPIO_read(PinName_t pinName)
{
    // Pointer to the selected GPIO port.
    GPIO_TypeDef *Port;

    // GPIO pin number.
    unsigned int pin;

    // Convert PinName_t into GPIO port and pin number.
    ecPinmap(pinName, &Port, &pin);


    // Move the selected IDR bit to bit 0 and remove all other bits.
    return (Port->IDR >> pin) & 0x1UL;
}

////////////////////////////////////////////////////////////////////

/* Include the following in GPIO.c */

////////////////////////////////////////////////////////////////////

// FND - JKIT board

static PinName_t _fndPins[8];
static PinName_t _fndSel[4];

// Each LED that has to light up gets a 1,
// every other LED gets a 0
// order: {a, b, c, d, e, f, g, dp}

int numberFND[12][8] = {

    {1,1,1,1,1,1,0,0},      // 0
    {0,1,1,0,0,0,0,0},      // 1
    {1,1,0,1,1,0,1,0},      // 2
    {1,1,1,1,0,0,1,0},      // 3
    {0,1,1,0,0,1,1,0},      // 4
    {1,0,1,1,0,1,1,0},      // 5
    {1,0,1,1,1,1,1,0},      // 6
    {1,1,1,0,0,0,0,0},      // 7
    {1,1,1,1,1,1,1,0},      // 8
    {1,1,1,1,0,1,1,0},      // 9
    {0,0,0,0,0,0,0,1},      // dot
    {0,0,0,0,0,0,0,0}       // blank
};


// Initialize DOUT pins for 7 segment LEDs
void FND_init(PinName_t *pinFND){

    for (int i=0; i<8; i++){

        // Save corresponding GPIO pin
        _fndPins[i] = pinFND[i];

        // Configure segment pin as output
        GPIO_init(_fndPins[i], OUTPUT);

        // Initially turn off each segment
        GPIO_write(_fndPins[i], LOW);
    }
}


// Initialize select pins (FND0~FND3) as DOUT
void FND_select_init(PinName_t *selectFND){

    for (int i=0; i<4; i++){

        // Save corresponding FND selection pin
        _fndSel[i] = selectFND[i];

        // Configure selection pin as output
        GPIO_init(_fndSel[i], OUTPUT);

        // Initially turn off every FND
        GPIO_write(_fndSel[i], LOW);
    }
}


// Display a number
void FND_display(uint8_t num){

    // Prevent invalid array access
    if (num > 11)
        return;

    // Write segment pattern to a~g and dp
    for (int i=0; i<8; i++){

        GPIO_write(_fndPins[i], numberFND[num][i]);
    }
}


// Select display: FND0 to FND3
void FND_select(uint8_t digit){

    // FND0~FND3
    for (int i=0; i<4; i++){

        if (i == digit){

            // Turn ON selected FND
            GPIO_write(_fndSel[i], HIGH);
        }
        else {

            // Turn OFF other FNDs
            GPIO_write(_fndSel[i], LOW);
        }
    }
}

void led_toggle(PinName_t pinName) {
    GPIO_TypeDef *Port;
    unsigned int pin;
    ecPinmap(pinName, &Port, &pin);

    Port->ODR ^= (1UL << pin);
}