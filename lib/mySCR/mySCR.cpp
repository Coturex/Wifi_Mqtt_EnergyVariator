
#include <Arduino.h>
#include "mySCR.h"

static bool pulse = false ;
bool DEBUG_ISR = false ;

#define TICKS 5 // 5 ticks/us
int SCR_PULSE_WIDTH = 1 ; // 1 ms

#ifdef USE_MYSCR_GREEN_PCB
bool reverse_pulse = true ;
#else
bool reverse_pulse = false ;
#endif

volatile int current_dim = 0;
char user_zero_cross = '0';

volatile int dimPower;
volatile int dimZCPin;
volatile int dimOutPin;

volatile DIMMER_MODE_typedef dimMode;
volatile ON_OFF_typedef dimState;

dimSCR::dimSCR(int user_dimmer_pin, int zc_dimmer_pin):
	dimmer_pin(user_dimmer_pin),
	zc_pin(zc_dimmer_pin)
{	
	dimOutPin = user_dimmer_pin ;
	dimZCPin = zc_dimmer_pin;
	pinMode(user_dimmer_pin, OUTPUT);
	if (reverse_pulse) {
		digitalWrite(dimOutPin, HIGH);
	} else {
		digitalWrite(dimOutPin, LOW);
	}
}

void dimSCR::begin(DIMMER_MODE_typedef DIMMER_MODE, ON_OFF_typedef ON_OFF)
{
	dimMode = DIMMER_MODE;
	dimState = ON_OFF;

	pinMode(dimZCPin, INPUT);

	// setup the timer used to manage SCR tops
	timer1_isr_init();
	timer1_attachInterrupt(onTimerISR);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
	//timer1_write(100*TICKS); //100 us

	// listen for change on the pin ZERO
	attachInterrupt(dimZCPin, onZero, RISING);
	if (DEBUG_ISR){Serial.println("[RBD] *** Init ISR Zero on pin : " + String(dimZCPin));} ;
}

// this function pointer is used to store the next timer action (see call_later and onTimerISR below)
void (*timer_callback)(void);

void call_later(unsigned long duration_us, void(*callback)(void)) {
    timer_callback = callback;
    // 5 ticks/us
    timer1_write(duration_us * TICKS);
}

void pulseEnd(void) { // called at the end of the pulse
    if (DEBUG_ISR){Serial.println("[RBD] SCR Low on pin :" + String(dimOutPin));} ;
    if (!reverse_pulse) {
		digitalWrite(dimOutPin, LOW);
	} else {
		digitalWrite(dimOutPin, HIGH);
	}
}

void pulseStart(void) { // called when the delay after the zero crossing has expired
    // start the pulse and arm a timer to end it
    // generate a pulse (below 50% this is a short pulse, above the pulse has a 3ms duration)
	if (!reverse_pulse) {
		digitalWrite(dimOutPin, HIGH);
	} else {
		digitalWrite(dimOutPin, LOW);
	}
    if (DEBUG_ISR){Serial.println("[RBD] SCR high on pin :" + String(dimOutPin));} ;
	#ifdef USE_MYSCR_GREEN_PCB
	call_later(10, pulseEnd);
	#else
	call_later(dimPower < 50 ? 5 : 3000, pulseEnd);
	#endif
}

void IRAM_ATTR onZero()
{
    if (DEBUG_ISR){Serial.println("[RBD] DETECT Zero pulse on pin : " + String(dimZCPin));} ;
	if (dimState == ON) {
    	if(dimPower > 0) {
			 // generate a pulse after this zero
        	// power=100%: no wait, power=0%: wait 10ms
        	//unsigned long delay = dimPower==100 ? 30 : (100-dimPower)*100;
        	unsigned long delay = dimPower==100 ? 30 : (100-dimPower)*100;
        	call_later(delay, pulseStart);
    	}
	}
}

void IRAM_ATTR onTimerISR()
{	
	if (DEBUG_ISR){Serial.println("[RBD] ISR TIMER occured, pulse : " + String(pulse));} ;
    void (*f)(void) = timer_callback;
    timer_callback = NULL;
    if(f) {
        f();
    }
}

void dimSCR::setPower(int power) {	
	if (power > 100) {
		power = 100;
	}
	dimPower = power;
}

int dimSCR::getPower(void)
{
	if (dimState == ON)
		return dimPower ;
	else return 0;
}

void dimSCR::setState(ON_OFF_typedef ON_OFF)
{
	dimState = ON_OFF;
	dimPower = 0 ;
}

bool dimSCR::getState(void)
{
	bool ret;
	if (dimState == ON) ret = true;
	else ret = false;
	return ret;
}

void dimSCR::changeState(void)
{
	if (dimState == ON) dimState = OFF;
	else 
		dimState = ON;
}

DIMMER_MODE_typedef dimSCR::getMode(void)
{
	return dimMode;
}

void dimSCR::setMode(DIMMER_MODE_typedef DIMMER_MODE)
{
	dimMode = DIMMER_MODE;
}
