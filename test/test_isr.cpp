#include <Arduino.h>

/* This code is for executing the interrupt in ESP8266.

    The main purpose is to solve the ISR not in RAM isssue.
    ISR Function : The interrupt pin [GPIO5 ] once changes state from HIGH to LOW
    ISR reads the value on GPIO4 and changes the state of the BUILTIN led based on the value read
    */

const byte pin5 = D5;
const byte pin4 = D4;

void IRAM_ATTR ISRoutine ();

void setup () {
Serial.begin(115200);
pinMode(pin5,INPUT_PULLUP);
pinMode(LED_BUILTIN,OUTPUT);
attachInterrupt(digitalPinToInterrupt(pin5),ISRoutine,FALLING);
pinMode(pin4,INPUT);
}

void loop () {
    unsigned long startTime = millis();
    delay(5000) ;
    Serial.print("loop time (ms) : ") ;
    Serial.println((millis()-startTime)); // print spare time in loop 

}

void ISRoutine () {
int value;
Serial.println("I am in ISR");
value = digitalRead(pin4);
Serial.print("Value read = ");
Serial.println(value);
digitalWrite(LED_BUILTIN,!value);
}