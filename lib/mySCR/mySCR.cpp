
#include <Arduino.h>
#include "mySCR.h"

static bool pulse = false ;
bool DEBUG_ISR = false ;

int SCR_PULSE_WIDTH = 3 ; // ms

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
{	dimOutPin = user_dimmer_pin ;
	dimZCPin = zc_dimmer_pin;
	pinMode(user_dimmer_pin, OUTPUT);
	digitalWrite(user_dimmer_pin, LOW);
}

void dimSCR::begin(DIMMER_MODE_typedef DIMMER_MODE, ON_OFF_typedef ON_OFF)
{
	dimMode = DIMMER_MODE;
	dimState = ON_OFF;
	// setup the timer used to manage SCR tops
	timer1_attachInterrupt(onTimerISR);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
	timer1_write(100*5); //100 us
	// listen for change on the pin ZERO
	pinMode(dimZCPin, INPUT_PULLUP);
	attachInterrupt(dimZCPin, onZero, RISING);
	if (DEBUG_ISR){Serial.println("[RBD] *** Init ISR Zero on pin : " + String(dimZCPin));} ;
}

void dimSCR::setPower(int power)
{	
	if (power >= 99) 
	{
		power = 99;
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

void pulseEnd(void) { // called at the end of the pulse
    if (DEBUG_ISR){Serial.println("[RBD] SCR Low on pin :" + String(dimOutPin));} ;
    digitalWrite(dimOutPin, LOW);
    pulse = false ;
}

void pulseStart(void) { // called when the delay after the zero crossing has expired
    // start the pulse and arm a timer to end it
    // generate a pulse (below 50% this is a short pulse, above the pulse has a 3ms duration)
    digitalWrite(dimOutPin, HIGH);
    if (DEBUG_ISR){Serial.println("[RBD] SCR high on pin :" + String(dimOutPin));} ;
    pulse = true ;
}

 
void IRAM_ATTR onZero()
{
    if (DEBUG_ISR){Serial.println("[RBD] DETECT Zero pulse on pin : " + String(dimZCPin));} ;
	if (dimState == ON) {
    	if(dimPower > 0) {
        	// generate a pulse after this zero
        	// power=100%: no wait, power=0%: wait 10ms
    	  	unsigned long delay = dimPower==100 ? 10 : (100-dimPower)*100;
	    	if (dimPower <= SCR_PULSE_WIDTH) {
          		delay = (100-SCR_PULSE_WIDTH-1)*100 ;
	      	}
      		timer1_write(delay * 5);
      		pulse = false ;
    	}
	}
}

void IRAM_ATTR onTimerISR()
{	
	if (DEBUG_ISR){Serial.println("[RBD] ISR TIMER occured, pulse : " + String(pulse));} ;

	if (pulse) {
        pulseEnd() ;
    } else {
        pulseStart() ;
        timer1_write(SCR_PULSE_WIDTH*100 * 5);
    }
}
