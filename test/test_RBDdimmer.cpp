#include <Arduino.h>

/* This code is for executing the interrupt in ESP8266.

    The main purpose is to solve the ISR not in RAM isssue.
    ISR Function : The interrupt pin [GPIO5 ] once changes state from HIGH to LOW
    ISR reads the value on GPIO4 and changes the state of the BUILTIN led based on the value read
    */

// these pins are used to connect to the SCR
int PIN_ZERO=D6 ;
int PIN_SCR=D5 ;
#include "RBDdimmerESP8266.h"

//initialase port for dimmer(outPin, ZeroCrossing)
dimmerLampESP8266 dimmer(PIN_SCR, PIN_ZERO) ; 

void setup () {
Serial.begin(115200);
dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
}


void loop () {
    unsigned long startTime = millis();
    //delay(5000) ;
    Serial.println("Power % [0-100] ? : ");
    while (Serial.available() == 0) {
    }
    int inValue = Serial.parseInt();
    if(inValue >= 0 && inValue<=100) {
        dimmer.setPower(inValue) ;
    }
    //Serial.print("loop time (ms) : ") ;
    //Serial.println((millis()-startTime)); // print spare time in loop 
    Serial.print("percent_power : ") ;
    Serial.println(String(inValue));
}

