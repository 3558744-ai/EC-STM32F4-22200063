/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : [ YOUR NAME GOES HERE !!!!!]
Created          : 10-23-2026
Modified         : 11-12-2026
Language/ver     : C in VS Code

Description      : Sensor check - Light / Moisture (JADC, TIM5 125 ms IRQ)
                                + DHT11 (PA_6, every 3 s in main)
/----------------------------------------------------------------*/
#include "ecSTM32F4v2.h"
#include "ecADC2.h"
#include "ecDHT11.h"

#define CDS_PIN         PA_0
#define MOISTURE_PIN    PA_1
#define DHT11_PIN       PA_6
#define DHT_PERIOD_MS   3000            // >= DHT11_MIN_PERIOD_MS (2 s)

volatile uint32_t light, moist;
PinName_t seqCHn[2] = {CDS_PIN, MOISTURE_PIN};  // JDR1 = light, JDR2 = moisture

void setup(void);

int main(void){
	uint32_t tDHT = 0, tPrint = 0;
	int dhtErr = DHT11_OK;

	setup();

	while(1){
		// DHT11 : every 3 s. Blocks ~25 ms -> main only, never in an ISR.
		// On error the library keeps the last valid temperature / humidity.
		if(msTicks - tDHT >= DHT_PERIOD_MS){
			tDHT   = msTicks;
			dhtErr = DHT11_read();
			if(dhtErr != DHT11_OK)	printf("[DHT11] error %d\r\n", dhtErr);
		}

		// Print : every 1 s (no delay_ms)
		if(msTicks - tPrint >= 1000){
			tPrint = msTicks;
			printf("light=%4lu moist=%4lu | T=%dC H=%d%%\r\n",
			       light, moist, DHT11_temperature(), DHT11_humidity());
		}
	}
}

void setup(void){
	RCC_PLL_init();                     // 84 MHz
	USART2_init();                      // debug printf
	SysTick_init();                     // msTicks + DHT11 us timing

	DHT11_init(DHT11_PIN);

	JADC_init(CDS_PIN);
	JADC_init(MOISTURE_PIN);
	JADC_sequence(seqCHn, 2);
	JADC_trigger_init(TIM5, 125, ADC_RISE);     // TIM5 TRGO 125 ms + JEOC IRQ
}

void ADC_IRQHandler(void){
	if(is_ADC_JEOC()){                  // once per whole sequence
		light = JADC_read(1);
		moist = JADC_read(2);
		clear_ADC_JEOC();
	}
}