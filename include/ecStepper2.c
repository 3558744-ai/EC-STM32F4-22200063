/*----------------------------------------------------------------\
@ Embedded Controller by Young-Keun Kim - Handong Global University
Author           : Daejin Kim
Created          : 05-03-2021
Modified         : 10-12-2026
Language/ver     : C++ in VS Code

Description      : Full/half-step GPIO FSM with RPM timing and stop control.
/----------------------------------------------------------------*/

#include "stm32f4xx.h"
#include "ecStepper2.h"

// Stepper Motor variable
static PinName_t stepperPins[4]; // A, B, AN, BN
static volatile uint32_t _steps; // number of steps to run
static volatile uint32_t _step_delay = 100;
static const StateStepper_t *fsm;				// points at fsm_full or fsm_half (chosen in stepper_init)
static uint32_t steps_per_rev = STEP_PER_REV;
static volatile uint32_t state = 0;				// current FSM state

// Output: A B A' B'
static const StateStepper_t fsm_Full[4] = {	// 4 states for full stepping
 {{S1,S3},{HIGH,HIGH,LOW,LOW}},	
 {{S2,S0},{LOW,HIGH,HIGH,LOW}},
 {{S3,S1},{LOW,LOW,HIGH,HIGH}},
 {{S0,S2},{HIGH,LOW,LOW,HIGH}}
};

static const StateStepper_t fsm_Half[8] = {  // 8 states for half stepping
	{{S1,S7},{1,0,0,0}},
	{{S2,S0},{1,1,0,0}},
	{{S3,S1},{0,1,0,0}},
	{{S4,S2},{0,1,1,0}},
	{{S5,S3},{0,0,1,0}},
	{{S6,S4},{0,0,1,1}},
	{{S7,S5},{0,0,0,1}},
	{{S0,S6},{1,0,0,1}}
};

// Initialization of Stepper Motor
void stepper_init(uint32_t mode, PinName_t *pinStepper){
	state = 0;
    steps_per_rev = (mode == HALF) ? STEP_PER_REV * 2U : STEP_PER_REV;
	if (mode == HALF)  // 어떤 모드로 돌릴지 선택
		fsm = fsm_Half;
	else
		fsm = fsm_Full;

	//  GPIO Digital Out Initiation
	// For A, B, AN, BN

	for (int i=0; i<4; i++){
		stepperPins[i] = pinStepper[i]; //핀번호를 입력받음
		GPIO_init(stepperPins[i], EC_DOUT); //핀번호를 출력으로 설정
	}
}

// Converts Motor [rpm] to step delay in [msec]
void stepper_speed (uint32_t speedRPM){      // rpm 완료
		if (speedRPM == 0U) { stepper_stop(); _step_delay = 0; return; }
        _step_delay = (uint32_t)(60000ULL / ((uint64_t)steps_per_rev * speedRPM));
        if (_step_delay == 0U) _step_delay = 1U; 		//in [msec]	
		// [usec] can be used for a higher accuracy
		// But needs delay_us() 
}

// Run Stepper Motor for given steps and direction
void stepper_step(uint32_t steps, uint32_t direction){
	// Exit if stepper_init() not called 
	if (fsm == 0 || _step_delay == 0U) return; // fsm이 초기화되지 않았으면 종료
	_steps = steps;

	// run for step size: 
	for (uint32_t i = 0; i < _steps; i++){
		// Update Present State
		if (direction == DIR_CW){ // 방향이 시계방향이면 next[0]으로 이동, 반시계방향이면 next[1]로 이동
			state = fsm[state].next[0];
		}
		else{
			state = fsm[state].next[1];
		}
		
		// Output of Present State
		stepper_out(state); // 현재 상태에 맞는 출력값을 stepper_out 함수로 전달하여 GPIO 핀에 출력

		// Delay for steppermotor speed
		delay_ms(_step_delay);
	}
}

// Stepper Motor Output for given state
void stepper_out(uint32_t state){
	
	// stepperPins[0]=A, stepperPins[1]=B, ...
	// out[0]=A, out[1]=B, ...
	for (int i = 0; i < 4; i++){
		GPIO_write(stepperPins[i], fsm[state].out[i]); // 현재 상태에 맞는 출력값을 GPIO 핀에 출력
	}
}

// Stop Stepper Motor
void stepper_stop(void) { // 완료
    _steps = 0;    
    
	// All pins DigitalOut '0'
	for (int i = 0; i < 4; i++){
		GPIO_write(stepperPins[i], 0); // 모든 핀을 LOW로 설정하여 스테퍼 모터를 정지
	}
}



