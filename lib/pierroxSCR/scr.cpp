#include <Arduino.h>
#include "scr.h"

// SCR identifier, there might be more than one power regulation device on the network, select an id here
#define SCR_ID 0

// these pins are used to connect to the SCR
#define PIN_ZERO D6
#define PIN_SCR D5

// this function pointer is used to store the next timer action (see call_later and onTimerISR below)
void (*timer_callback)(void);

void scrDimmer::call_later(unsigned long duration_us, void(*callback)(void)) {
    timer_callback = callback;
    // 5 ticks/us
    timer1_write(duration_us * 5);
}

// timer interrupt routine : call the function which gas been registered earlier (see call_later)
void ICACHE_RAM_ATTR onTimerISR(){
    void (*f)(void) = timer_callback;
    timer_callback = NULL;
    if(f) {
        f();
    }
}

// called at the end of the pulse
void scrDimmer::onPulseEnd(void) {
    digitalWrite(PIN_SCR, LOW);
}

// called when the delay after the zero crossing has expired
void scrDimmer::onDelayExpired(void) {
    // start the pulse and arm a timer to end it
    // generate a pulse (below 50% this is a short pulse, above the pulse has a 3ms duration)
    digitalWrite(PIN_SCR, HIGH);

    call_later(power < 50 ? 5 : 3000, onPulseEnd);
}

// pin ZERO interrupt routine
void scrDimmer::onZero(void) {
    if(power > 0) {
        // generate a pulse after this zero
        // power=100%: no wait, power=0%: wait 10ms
        unsigned long delay = power==100 ? 30 : (100-power)*100;
        call_later(delay, onDelayExpired);
    }
}
scrDimmer::scrDimmer(void) {
    // current power (as a percentage of time) : power off at startup.
    float power = 0;
    // usual pin configuration
    pinMode(PIN_SCR, OUTPUT);
    digitalWrite(PIN_SCR, LOW);
    pinMode(PIN_ZERO, INPUT);
}

void scrDimmer::begin(void) {
    // setup the timer used to manage SCR tops
    timer1_isr_init();
    timer1_attachInterrupt(onTimerISR);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE); // 5 ticks/us
    
    // listen for change on the pin ZERO
    attachInterrupt(digitalPinToInterrupt(PIN_ZERO), this->onZero(), CHANGE);
}
