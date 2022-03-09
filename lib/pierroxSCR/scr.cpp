#include <Arduino.h>
#include "scr.h"


// called at the end of the pulse
void onPulseEnd() {
    digitalWrite(PIN_SCR, LOW);
}

// called when the delay after the zero crossing has expired
void onDelayExpired() {
    // start the pulse and arm a timer to end it
    // generate a pulse (below 50% this is a short pulse, above the pulse has a 3ms duration)
    digitalWrite(PIN_SCR, HIGH);

    call_later(power < 50 ? 5 : 3000, onPulseEnd);
}

// pin ZERO interrupt routine
void onZero() {
    if(power > 0) {
        // generate a pulse after this zero
        // power=100%: no wait, power=0%: wait 10ms
        unsigned long delay = power==100 ? 30 : (100-power)*100;
        call_later(delay, onDelayExpired);
    }
}

void setupISR() {
    pinMode(PIN_SCR, OUTPUT);
    digitalWrite(PIN_SCR, LOW);
    pinMode(PIN_ZERO, INPUT);
    
    // setup the timer used to manage SCR tops
    timer1_isr_init();
    timer1_attachInterrupt(onTimerISR);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE); // 5 ticks/us
    
    // listen for change on the pin ZERO
    //attachInterrupt(digitalPinToInterrupt(PIN_ZERO), onZero, CHANGE);  // Pierrox's SCR has  zero detect trigger on CHANGE state
    attachInterrupt(digitalPinToInterrupt(PIN_ZERO), onZero, RISING);    // my SCR has zero detect on 1ms pulse -> on RISING state

}