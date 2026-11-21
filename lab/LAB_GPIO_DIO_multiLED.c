/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University

Author           : DAEJINKIM
Created          : 2026-09-21
Modified         : 2026-09-21
Language/ver     : C in VS Code

Description      : Sequential LED control using SW2 on the JKIT board.
                   Each button press moves the active LED in the
                   sequence LED0 -> LED1 -> LED2 -> LED3 -> LED0.
/----------------------------------------------------------------*/


/*----------------------------------------------------------------
 Header Files
----------------------------------------------------------------*/

// STM32F411RE register definitions
#include "stm32f411xe.h"

// RCC library for system clock initialization
#include "ecRCC2.h"

// GPIO library developed in Problem 1
#include "ecGPIO2.h"


/*----------------------------------------------------------------
 Pin Definitions
----------------------------------------------------------------*/

// SW2 on the JKIT board is connected to PA_4.
#define BUTTON_PIN      PA_4

// LED0 ~ LED3 on the JKIT board are connected to PB_12 ~ PB_15.
#define LED0_PIN        PB_12
#define LED1_PIN        PB_13
#define LED2_PIN        PB_14
#define LED3_PIN        PB_15


int main(void)
{
    /*------------------------------------------------------------
     Variable Declaration
    ------------------------------------------------------------*/

    // Store the four LED GPIO pins in an array.
    // This allows all LEDs to be controlled using one index.
    PinName_t LED_PIN[4] = {
        LED0_PIN,
        LED1_PIN,
        LED2_PIN,
        LED3_PIN
    };

    // Index of the LED that is currently ON.
    // 0 = LED0, 1 = LED1, 2 = LED2, 3 = LED3.
    uint32_t currentLED = 0;

    // Stores the current state of SW2.
    uint32_t buttonState;

    // Stores the previous state of SW2.
    // This is used to detect the moment when the button is pressed.
    uint32_t previousButtonState;


    /*------------------------------------------------------------
     System Clock Initialization
    ------------------------------------------------------------*/

    // Initialize the STM32F411RE system clock using HSI.
    RCC_HSI_init();


    /*------------------------------------------------------------
     SW2 Configuration

     PA_4
     - Digital Input
     - Pull-Up
    ------------------------------------------------------------*/

    // Configure PA_4 as a digital input.
    GPIO_init(BUTTON_PIN, INPUT);

    // Enable the internal pull-up resistor.
    // Therefore:
    // Button released = HIGH
    // Button pressed  = LOW
    GPIO_pupd(BUTTON_PIN, PU);


    /*------------------------------------------------------------
     LED0 ~ LED3 Configuration

     PB_12 ~ PB_15
     - Digital Output
     - Pull-Up
     - Open-Drain
     - Medium Speed
    ------------------------------------------------------------*/

    for (int i = 0; i < 4; i++)
    {
        // Configure each LED pin as a digital output.
        GPIO_init(LED_PIN[i], OUTPUT);

        // Configure each LED output as Open-Drain.
        GPIO_otype(LED_PIN[i], OPEN_DRAIN);

        // Enable the internal pull-up resistor.
        GPIO_pupd(LED_PIN[i], PU);

        // Set the output speed to Medium Speed
        // according to the LAB requirement.
        GPIO_ospeed(LED_PIN[i], MEDIUM_SPEED);

        // Turn all LEDs OFF initially.
        // Actual JKIT board operation:
        // LOW = LED OFF
        // HIGH = LED ON
        GPIO_write(LED_PIN[i], LOW);
    }


    /*------------------------------------------------------------
     Initial LED State
    ------------------------------------------------------------*/

    // Turn LED0 ON initially.
    // currentLED = 0, therefore LED_PIN[0] = PB_12 = LED0.
    GPIO_write(LED_PIN[currentLED], HIGH);


    // Read the initial button state.
    // Normally this should be HIGH because SW2 uses Pull-Up.
    previousButtonState = GPIO_read(BUTTON_PIN);


    /*------------------------------------------------------------
     Main Loop
    ------------------------------------------------------------*/

    while (1)
    {
        // Read the current digital state of SW2.
        buttonState = GPIO_read(BUTTON_PIN);


        /*--------------------------------------------------------
         Detect a New Button Press

         Since SW2 uses Pull-Up:

         Released : HIGH
         Pressed  : LOW

         Therefore, a new button press occurs when the signal
         changes from HIGH to LOW.
        --------------------------------------------------------*/

        if ((previousButtonState == HIGH) &&
            (buttonState == LOW))
        {
            // Turn OFF the LED that is currently ON.
            // On the actual JKIT board, LOW corresponds to LED OFF.
            GPIO_write(LED_PIN[currentLED], LOW);


            // Move the LED index to the next LED.
            currentLED++;


            // After LED3, return to LED0.
            // This creates the repeating sequence:
            // LED0 -> LED1 -> LED2 -> LED3 -> LED0
            if (currentLED >= 4)
            {
                currentLED = 0;
            }


            // Turn ON the newly selected LED.
            // On the actual JKIT board, HIGH corresponds to LED ON.
            GPIO_write(LED_PIN[currentLED], HIGH);


            // Mechanical switches can generate bouncing signals.
            // A short delay reduces unwanted multiple detections.
            delay_ms_HSI(50);
        }


        // Save the current button state.
        // It will be compared with the new state
        // during the next loop iteration.
        previousButtonState = buttonState;
    }


    // The program never reaches this point because
    // the main loop runs continuously.
    return 0;
}