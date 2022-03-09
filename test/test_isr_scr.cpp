#include <Arduino.h>

/* This code is for executing the interrupt in ESP8266.

    The main purpose is to solve the ISR not in RAM isssue.
    ISR Function : The interrupt pin [GPIO5 ] once changes state from HIGH to LOW
    ISR reads the value on GPIO4 and changes the state of the BUILTIN led based on the value read
    */

// these pins are used to connect to the SCR
int PIN_ZERO=D6 ;
int PIN_SCR=D5 ;
#include "scr.h"

float percent_power = 0 ;

// this function pointer is used to store the next timer action (see call_later and onTimerISR below)
void (*timer_callback)(void);
void call_later(unsigned long duration_us, void(*callback)(void)) {
    timer_callback = callback;
    // 5 ticks/us
    timer1_write(duration_us * 5);
}
// timer interrupt routine : call the function which gas been registered earlier (see call_later)
void IRAM_ATTR onTimerISR(){
    void (*f)(void) = timer_callback;
    timer_callback = NULL;
    if(f) {
        f();
    }
}


const byte pin5 = D5;

void setup () {
Serial.begin(115200);
setupISR();
}

void loop () {
    unsigned long startTime = millis();
    //delay(5000) ;
    Serial.println("Power % [0-100] ? : ");
    while (Serial.available() == 0) {
    }
    int inValue = Serial.parseInt();
    if(inValue >= 0 && inValue<=100) {
        percent_power = inValue ;
    }
    //Serial.print("loop time (ms) : ") ;
    //Serial.println((millis()-startTime)); // print spare time in loop 
    Serial.print("percent_power : ") ;
    Serial.println(String(percent_power));
}

