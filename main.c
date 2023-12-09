/*
* GccApplication1.c
*
* Created: 25-08-2018 21:47:57
* Author : Rajendra Prasad Reddy H V
*/

#include <avr/io.h>
#include <avr/iotn24.h>
#include <avr/interrupt.h>
#include <stdbool.h>
//#include <avr/sleep.h>
//#include <avr/builtins.h>

//#define OUTPUT					1
//#define INPUT					0
//
//#define HIGH					1
//#define LOW						0

#define SET(BIT)				(1 << BIT)
#define CLEAR(BIT)				(0 << BIT)

/// constants related to operation
#define OPERATION_DURATION		1			// in minutes

/// pin assignments
//#define MOTOR_ENABLE_PIN		PA3
//#define LID_SENSE_PIN			PA1
//#define RUN_SWITCH_PIN			PA2
//#define RED_LED_PIN				PB2
//#define GREEN_LED_PIN			PB1
//#define BLUE_LED_PIN			PB0
//#define PORT_A_PIN_DIR_REG		DDRA
//#define PORT_A_PIN_OUT_VAL_REG	PORTA
//#define PORT_A_PIN_IN_VAL_REG	PINA
//#define PORT_B_PIN_DIR_REG		DDRB
//#define PORT_B_PIN_OUT_VAL_REG	PORTB
//#define PORT_B_PIN_IN_VAL_REG	PINB

// states involved in operation
enum _states {
	IDLE_STATE = 0,
	READY_TO_USE_STATE,
	RUNNING_STATE,
	COMPLETED_STATE
};

// status
int StartButtonPrevState = 0x04;
volatile int StartButtonState = 0x04;
volatile int LidSenseState = 0x02;

// operation related variables
volatile bool IsStartButtonPressed = false;
volatile bool IsLidOpen = true;
volatile bool IsRunning = false;

// timer variables
volatile uint32_t _250_millis_count = 1;

// counter variables
uint32_t Count = 0;
uint32_t Seconds = 0;
uint32_t Minutes = 0;

void init_gpio() {
	/// set direction of port pins
	DDRA = 0xF9;
	DDRB = 0xFF;

	/// set initial values to gpio
	//PORTA = 0x06;
	PORTB = 0x07;
}

void init_timer() {
	TIFR0 = 0x01;
	TIMSK0 = 0x01;
	TCCR0B = 0x05;
}

void enable_interrupts() {
	GIMSK = 0x30;
	PCMSK0 = 0x06;
	PCMSK1 = 0x00;
	//GIFR = 0x00;
	sei();
}

void init_vars() {
	LidSenseState = (PINA & 0x02);
	if (0x00 == LidSenseState) {
		IsLidOpen = false;
	}
	else if (0x02 == LidSenseState) {
		IsLidOpen = true;
	}
	StartButtonState = (PINA & 0x04);
	if (StartButtonPrevState != StartButtonState) {
		StartButtonPrevState = StartButtonState;
		if ((0x04 == StartButtonState) && (false == IsLidOpen) && (false == IsStartButtonPressed)) {
			IsStartButtonPressed = true;
		}
		else {
			IsStartButtonPressed = false;
		}
	}
}

void IdleState() {
	PORTB = 0x01;
	PORTA |= (1 << 3);
}

void ReadyToUseState() {
	PORTB = 0x02;
	PORTA |= (1 << 3);
}

void RunningState() {
	PORTB = 0x04;
	PORTA &= (0 << 3);
}

void CompleteState() {
	PORTB = 0x07;
	PORTA |= (1 << 3);
}

uint32_t millis() {
	return _250_millis_count;
}

ISR(PCINT0_vect) {
	LidSenseState = (PINA & 0x02);
	if (0x00 == LidSenseState) {
		IsLidOpen = false;
	}
	else if (0x02 == LidSenseState) {
		IsLidOpen = true;
	}
	StartButtonState = (PINA & 0x04);
	if (StartButtonPrevState != StartButtonState) {
		StartButtonPrevState = StartButtonState;
		if ((0x04 == StartButtonState) && (false == IsLidOpen) && (false == IsStartButtonPressed)) {
			IsStartButtonPressed = true;
		}
		else {
			IsStartButtonPressed = false;
		}
	}
}

ISR(TIM0_OVF_vect) {
	_250_millis_count++;
}

int apps_init() {
	init_gpio();
	init_timer();
	enable_interrupts();
	init_vars();
	return 0;
}

int apps_run()
{
	// state machine variables
	static enum _states State = IDLE_STATE;

	// counter for operation
	if ((millis() - Count) > 3) {
		Count = millis();
		Seconds++;
		if (Seconds > 55) {
			Minutes++;
		}
	}

	// reset to idle state if lid open
	if (true == IsLidOpen) {
		State = IDLE_STATE;
	}

	// state machine
	switch (State) {
		
		// idle state - lid open
		case IDLE_STATE: {
			IdleState();
			if (false == IsLidOpen) {
				State = READY_TO_USE_STATE;
				IsStartButtonPressed = false;
			}
			break;
		}

		// ready to use state - lid close, waiting for start
		case READY_TO_USE_STATE: {
			ReadyToUseState();
			if (true == IsStartButtonPressed) {
				IsStartButtonPressed = false;
				State = RUNNING_STATE;
				IsRunning = true;
				Count = millis();
				Seconds = 0;
				Minutes = 0;
			}
			break;
		}

		// running state - operation running, waiting for complete
		case RUNNING_STATE: {
			RunningState();
			// counter, count only if running.
			if ((true == IsRunning) && (Minutes > OPERATION_DURATION)) {
				IsRunning = false;
				Count = 0;
			}
			else if (false == IsRunning) {
				State = COMPLETED_STATE;
			}
			break;
		}

		// complete state - operation completed
		case COMPLETED_STATE: {
			CompleteState();
			break;
		}
		
		default: {
			State = IDLE_STATE;
		}
		break;
	}
	return 0;
}

int main(void)
{
	apps_init();
	while (1)
	{
		apps_run();
	}
}
