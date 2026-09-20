/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University

Author           : DAEJINKIM
Created          : 2026-09-21
Modified         : 2026-09-21
Language/ver     : C in VS Code

Description      : Photodetector-controlled LED using EC_HAL GPIO library.
                   LED0 turns ON in a dark condition and turns OFF
                   in a bright condition.
/----------------------------------------------------------------*/


// STM32F411RE register definitions
#include "stm32f411xe.h"

// RCC library for system clock initialization
#include "ecRCC2.h"

// GPIO library created in Problem 1
#include "ecGPIO2.h"


/*----------------------------------------------------------------
 Pin Definitions
----------------------------------------------------------------*/

// PA_0 is connected to the photodetector digital output.
#define PHOTO_PIN       PA_0

// PB_12 is connected to LED0 on the JKIT board.
#define LED0_PIN        PB_12


int main(void)
{
    /*------------------------------------------------------------
     System Clock Initialization
    ------------------------------------------------------------*/

    // Initialize the STM32F411RE system clock using HSI.
    RCC_HSI_init();


    /*------------------------------------------------------------
     Photodetector GPIO Configuration
     
     PA_0
     - Digital Input
     - Pull-Up
    ------------------------------------------------------------*/

    // Initialize PA_0 as a digital input pin.
    GPIO_init(PHOTO_PIN, INPUT);

    // Enable the internal pull-up resistor of PA_0.
    // This prevents the input from floating when the sensor
    // output does not actively drive the signal.
    GPIO_pupd(PHOTO_PIN, PU);


    /*------------------------------------------------------------
     LED0 GPIO Configuration
     
     PB_12
     - Digital Output
     - Pull-Up
     - Open-Drain
     - Medium Speed
    ------------------------------------------------------------*/

    // Initialize PB_12 as a digital output pin.
    GPIO_init(LED0_PIN, OUTPUT);

    // Configure PB_12 as an open-drain output.
    // In open-drain mode, the GPIO actively drives LOW,
    // while the HIGH state is obtained through the pull-up resistor.
    GPIO_otype(LED0_PIN, OPEN_DRAIN);

    // Enable the internal pull-up resistor for PB_12.
    GPIO_pupd(LED0_PIN, PU);

    // Configure the output switching speed as Medium Speed
    // according to the LAB specification.
    GPIO_ospeed(LED0_PIN, MEDIUM_SPEED);


    /*------------------------------------------------------------
     Initial LED State
    ------------------------------------------------------------*/

    // Set the LED output HIGH initially.
    // Because the LED is active-low in this configuration,
    // HIGH corresponds to LED OFF.
    GPIO_write(LED0_PIN, HIGH);


    /*------------------------------------------------------------
     Main Loop
    ------------------------------------------------------------*/

    while (1)
    {
        // Read the current digital state of the photodetector.
        // GPIO_read() returns either HIGH(1) or LOW(0).
        uint32_t photoState = GPIO_read(PHOTO_PIN);


        // When the photodetector output is HIGH,
        // the environment is interpreted as dark.
        if (photoState == HIGH)
        {
            // Drive PB_12 LOW.
            // Since LED0 is active-low, this turns the LED ON.
            GPIO_write(LED0_PIN, LOW);
        }

        // When the photodetector output is LOW,
        // the environment is interpreted as bright.
        else
        {
            // Release PB_12 to HIGH through the pull-up resistor.
            // This turns LED0 OFF.
            GPIO_write(LED0_PIN, HIGH);
        }
    }
}