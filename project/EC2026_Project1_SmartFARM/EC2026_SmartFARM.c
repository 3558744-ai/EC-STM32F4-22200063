/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Project          : Smart-Farm Management System
Created          : 11-15-2026
Language/ver     : C in VS Code, PlatformIO (framework = cmsis)

Interrupts
  TIM1   500 ms : control loop  (alarm, ventilation, lighting, outputs)
  ADC    125 ms : TIM5 TRGO -> JDR1 light, JDR2 moisture
  USART6 / USART2 RX : command byte  (A = AUTO, M = MANUAL ...)
Main loop : command, DHT11 read (blocking ~25 ms), messages, LCD
/----------------------------------------------------------------*/

#include "ecSTM32F4v2.h"

// ---------------- Pins (fixed) ----------------
#define FAN_PIN     PA_15       // TIM2 CH1, 1 kHz
#define SERVO_PIN   PC_7        // TIM3 CH2, 50 Hz
#define LED_PANEL   PB_7        // TIM4 CH2, 1 kHz
#define LIGHT_PIN   PA_0        // ADC1 JDR1
#define MOIST_PIN   PA_1        // ADC1 JDR2
#define DHT_PIN     PA_6

// ---------------- Thresholds (light % : the higher, the darker) ----------------
#define DARK_ON     40                  // light > 40 -> DARK   (LED 100 %)
#define DARK_OFF    (DARK_ON - 5)       // light < 35 -> leave DARK
#define BRIGHT_OFF  20                  // light > 20 -> leave BRIGHT
#define BRIGHT_ON   (BRIGHT_OFF-5)      // light < 15 -> BRIGHT (LED 0 %)

#define DRY_TH      30          // moisture % < 30 for 5 s -> DRY
#define WET_TH      40          // moisture % > 40 for 5 s -> clear
#define T_HIGH      27          // vent ON  : T > 27 OR  H > 53
#define T_LOW       25          // vent OFF : T < 25 AND H < 50
#define H_HIGH      53          // humidity % upper threshold
#define H_LOW       50          // humidity % lower threshold
#define MOIST_INVERT 1          // 1 : ADC rises as the soil dries

// ---------------- Timing (500 ms ticks) ----------------
#define TICK_5S     10          // AUTO light detection, DRY / clear hold
#define TICK_MIN_ON 20          // LED panel minimum on-time (10 s)
#define DHT_FAULT   3           // 3 failed reads in a row = fault

// ---------------- Actuators ----------------
#define FAN_DUTY    0.8f
#define SERVO_CLOSE 500         // usec, 0 deg
#define SERVO_OPEN  1500        // usec, 90 deg

enum { MANUAL, AUTO };
enum { DARK, MID, BRIGHT };

// ---------------- State ----------------
volatile int      mode = MANUAL;                    // default MANUAL
volatile int      alarmDry = 0, alarmAck = 0;
volatile int      autoWait = 0;                     // ticks left of the 5 s detection
volatile uint32_t lightRaw = 0, moistRaw = 0;       // averaged ADC 0~4095
volatile int      light = 0, moist = 0;             // %
volatile int      temp = 0, hum = 0;                // last valid DHT11
volatile int      dhtFail = 0;                      // failures in a row
int               dhtFailTotal = 0;
volatile int      zone = MID;
volatile float    autoDuty = 0, manDuty = 0, ledDuty = 0;
volatile int      onTicks = 0;
volatile int      vent = 0, manFan = 0, manWin = 0, fanOn = 0, winOpen = 0;
volatile int      dryCnt = 0, wetCnt = 0, blink = 0;
volatile uint32_t tick = 0;
volatile int      dhtFlag = 0, uiFlag = 0, cmdFlag = 0;
volatile char     cmd = 0;

void setup(void);
void command(char c);
void read_dht(void);
void report(void);
void lcd_update(void);
void send(const char *s);


/*================ Main ================*/

int main(void){
	setup();
	send("SMART FARM READY (MANUAL)\r\n");

	while(1){
		if(cmdFlag){ cmdFlag = 0; command(cmd); }
		if(dhtFlag){ dhtFlag = 0; read_dht();   }   // never inside an ISR
		if(uiFlag) { uiFlag  = 0; report(); lcd_update(); }
	}
}


void setup(void){
	RCC_PLL_init();                         // 84 MHz
	SysTick_init();                         // 1 ms

	// Outputs first : LED OFF, fan OFF, window CLOSED (PWM_init gives 50 %)
	PWM_init_us(LED_PANEL, 1000);	PWM_duty(LED_PANEL, 0.0f);
	PWM_init_us(FAN_PIN, 1000);		PWM_duty(FAN_PIN, 0.0f);
	PWM_init(SERVO_PIN, 20);		PWM_pulsewidth_us(SERVO_PIN, SERVO_CLOSE);

	// LCD
	I2C1_init();                            // LCD_init() does not start I2C1
	LCD_init();

	// Serial : USART2 = PC, USART6 = Bluetooth (9600, RX interrupt)
	USART2_init();
	USART6_init();

	DHT11_init(DHT_PIN);

	// ADC injected : light JDR1, moisture JDR2, TIM5 TRGO 125 ms
	JADC_init(LIGHT_PIN);
	JADC_init(MOIST_PIN);
	PinName_t seq[2] = {LIGHT_PIN, MOIST_PIN};
	JADC_sequence(seq, 2);
	JADC_trigger_init(TIM5, 125, ADC_RISE);

	TIM_UI_init(TIM1, 500);                 // control loop, last
}


/*================ Interrupts ================*/

// ---- 500 ms control loop ----
void TIM1_UP_TIM10_IRQHandler(void){
	if(!is_UIF(TIM1)) return;
	tick++;
	blink ^= 1;

	light = lightRaw * 100 / 4095;
	moist = moistRaw * 100 / 4095;
	if(MOIST_INVERT) moist = 100 - moist;

	// 1. Moisture alarm : 5 s below DRY_TH -> DRY, 5 s above WET_TH -> clear
	if(!alarmDry){
		dryCnt = (moist < DRY_TH) ? dryCnt + 1 : 0;
		if(dryCnt >= TICK_5S){ alarmDry = 1; alarmAck = 0; dryCnt = 0; }
	}
	else{
		wetCnt = (moist > WET_TH) ? wetCnt + 1 : 0;
		if(wetCnt >= TICK_5S){ alarmDry = 0; wetCnt = 0; }
	}

	// 2. Ventilation (the alarm is not used here)
	if(mode == AUTO){
		if(dhtFail >= DHT_FAULT)               vent = 1;   // invalid DHT11 : fail-safe
		else if(temp > T_HIGH || hum > H_HIGH) vent = 1;   // OR  : start
		else if(temp < T_LOW  && hum < H_LOW)  vent = 0;   // AND : stop
		fanOn = winOpen = vent;
	}
	else{
		fanOn   = manFan;
		winOpen = manWin;
	}

	// 3. Lighting (AUTO)
	if(mode == AUTO){
		if(autoWait > 0){                                   // 5 s detection, panel OFF
			autoWait--;
			autoDuty = 0;  onTicks = 0;
			if(autoWait == 0) zone = (light < DARK_ON) ? DARK : (light > BRIGHT_ON) ? BRIGHT : MID;
		}
		else{
			// zone with hysteresis  (the higher light %, the darker)
			if(zone == DARK   && light < DARK_OFF)   zone = MID;
			if(zone == BRIGHT && light > BRIGHT_OFF) zone = MID;
			if(zone == MID){
				if(light > DARK_ON)        zone = DARK;
				else if(light < BRIGHT_ON) zone = BRIGHT;
			}

			float target;
			if(zone == DARK)        target = 1.0f;
			else if(zone == BRIGHT) target = 0.0f;
			else{
				target = (float)(light - BRIGHT_ON) / (DARK_ON - BRIGHT_ON);
				if(target > 1.0f) target = 1.0f;
				if(target < 0.0f) target = 0.0f;
			}

			// minimum on-time
			if(autoDuty > 0) onTicks++;
			if(target == 0.0f && autoDuty > 0 && onTicks < TICK_MIN_ON) target = autoDuty;
			if(target == 0.0f) onTicks = 0;
			autoDuty = target;
		}
	}

	// 4. Outputs : LED priority ALARM blink > AUTO > MANUAL
	if(alarmDry)          ledDuty = blink ? 1.0f : 0.0f;   // 1 s blink period
	else if(mode == AUTO) ledDuty = autoDuty;
	else                  ledDuty = manDuty;

	PWM_duty(LED_PANEL, ledDuty);
	PWM_duty(FAN_PIN, fanOn ? FAN_DUTY : 0.0f);
	PWM_pulsewidth_us(SERVO_PIN, winOpen ? SERVO_OPEN : SERVO_CLOSE);

	if(tick % 6 == 0) dhtFlag = 1;                          // DHT11 every 3 s (6 ticks)
	uiFlag = 1;
	clear_UIF(TIM1);
}

// ---- ADC : injected sequence done (125 ms) ----
void ADC_IRQHandler(void){
	if(is_ADC_JEOC()){
		lightRaw = (lightRaw * 7 + JADC_read(1)) / 8;       // running average ~1 s
		moistRaw = (moistRaw * 7 + JADC_read(2)) / 8;
		clear_ADC_JEOC();
	}
}

// ---- Bluetooth / PC : one command byte (CR, LF ignored) ----
void USART6_IRQHandler(void){
	if(is_USART_RXNE(USART6)){
		char c = USART_read(USART6);
		if(c != '\r' && c != '\n'){ cmd = c; cmdFlag = 1; }
	}
}

void USART2_IRQHandler(void){
	if(is_USART_RXNE(USART2)){
		char c = USART_read(USART2);
		if(c != '\r' && c != '\n'){ cmd = c; cmdFlag = 1; }
	}
}


/*================ Main-loop functions ================*/

// A AUTO | M MANUAL | 0~9 LED 0~90 % | F/G fan | O/C window | K ack | S status
void command(char c){
	char buf[80];
	if(c >= 'a' && c <= 'z') c -= 32;

	if		(c == 'A'){ autoWait = TICK_5S; mode = AUTO; 	send("MODE AUTO (light detect 5 s)\r\n"); }
	else if	(c == 'M'){ mode = MANUAL; 						send("MODE MANUAL\r\n"); }
	else if	(c == 'K'){ alarmAck = 1; 						send(alarmDry ? "ACK : ADD WATER stopped\r\n" : "ACK : no alarm\r\n"); }
	else if	(c == 'S'){
		sprintf(buf, "%s%s L=%d%% M=%d%% T=%d H=%d LED=%d%% FAN=%d WIN=%d DHTerr=%d\r\n",
		        mode == AUTO ? "AUTO" : "MANUAL", alarmDry ? "+ALARM" : "",
		        light, moist, temp, hum, (int)(ledDuty * 100 + 0.5f), fanOn, winOpen, dhtFailTotal);
		send(buf);
	}
	else if(mode != MANUAL){ send("in AUTO : send 'M' first\r\n"); }
	else if(c >= '0' && c <= '9'){
		manDuty = (c - '0') / 10.0f;
		sprintf(buf, "LED %d%%\r\n", (c - '0') * 10);
		send(buf);
	}
	else if(c == 'F'){ manFan = 1; send("FAN ON\r\n");       }
	else if(c == 'G'){ manFan = 0; send("FAN OFF\r\n");      }
	else if(c == 'O'){ manWin = 1; send("WINDOW OPEN\r\n");  }
	else if(c == 'C'){ manWin = 0; send("WINDOW CLOSE\r\n"); }
	else send("unknown command\r\n");
}

// DHT11 : invalid -> keep last values, report, count
void read_dht(void){
	char buf[48];
	int r = DHT11_read();
	if(r == DHT11_OK){
		temp = DHT11_temperature();
		hum  = DHT11_humidity();
		if(dhtFail >= DHT_FAULT) send("DHT11 recovered\r\n");
		dhtFail = 0;
	}
	else if(r != DHT11_ERR_TOOSOON){
		dhtFail++;
		dhtFailTotal++;
		sprintf(buf, "DHT11 invalid (err %d) total %d\r\n", r, dhtFailTotal);
		send(buf);
		if(dhtFail == DHT_FAULT) send("DHT11 FAULT : AUTO -> fan ON, window OPEN\r\n");
	}
}

// Message whenever a condition changes
void report(void){
	static int pAlarm = 0, pVent = 0;
	static uint32_t lastMsg = 0;
	char buf[48];

	if(alarmDry != pAlarm){
		send(alarmDry ? "ALARM DRY : ADD WATER\r\n" : "ALARM CLEARED\r\n");
		pAlarm = alarmDry;
		lastMsg = tick;
	}
	else if(alarmDry && !alarmAck && tick - lastMsg >= 4){    // repeat every 2 s until ACK
		send("ADD WATER\r\n");
		lastMsg = tick;
	}

	int v = fanOn || winOpen;
	if(v != pVent){
		sprintf(buf, "VENT %s (T=%d H=%d)\r\n", v ? "ON" : "OFF", temp, hum);
		send(buf);
		pVent = v;
	}
}

// LCD : line 1 = state, line 2 = T H LED / VENT / ADD WATER
void lcd_update(void){
	char l1[17], l2[17];

	snprintf(l1, sizeof l1, "%-16s",
	         alarmDry ? "ALARM" : mode == MANUAL ? "MANUAL" : autoWait ? "AUTO (detect)" : "AUTO");

	if(alarmDry)                snprintf(l2, sizeof l2, "%-16s", blink ? "ADD WATER" : "");
	else if(fanOn || winOpen)   snprintf(l2, sizeof l2, "%2d%cC %2d%% VENT    ", temp, 0xDF, hum);
	else                        snprintf(l2, sizeof l2, "%2d%cC %2d%% LED%3d%%", temp, 0xDF, hum, (int)(ledDuty * 100 + 0.5f));

	LCD_setCursor(0, 0);	LCD_print(l1);
	LCD_setCursor(0, 1);	LCD_print(l2);
}

// Bluetooth + PC (printf -> USART2)
void send(const char *s){
	USART_write(USART6, (uint8_t *)s, strlen(s));
	printf("%s", s);
}
