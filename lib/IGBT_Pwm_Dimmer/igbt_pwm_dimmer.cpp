#include "igbt_pwm_dimmer.h"
#include <Arduino.h>

volatile int dimPower ;
volatile int maxPower ;
volatile int dimState ;


dimmerLampESP8266::dimmerLampESP8266(int pwm_dimmer_pin):
	dimmer_pin(pwm_dimmer_pin) {
    pinMode(pwm_dimmer_pin, OUTPUT);
}

void dimmerLampESP8266::begin(DIMMER_MODE_typedef DIMMER_MODE, ON_OFF_typedef ON_OFF) {
	dimState = ON_OFF;
}


void dimmerLampESP8266::setPower(int percent) {	
	if (percent >= 99) 
	{
		percent = 99;
	}
	dimPower = percent;
	analogWrite(this->dimmer_pin, percent) ;

	
	delay(1);
}

int dimmerLampESP8266::getPower(void) {
	if (dimState == ON)
		return dimPower;
	else return 0;
}
