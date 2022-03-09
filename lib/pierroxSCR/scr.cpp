#include <Arduino.h>
#include "scr.h"

bool DEBUG_ISR = false ;
int SCR_PULSE_WIDTH = 3 ; // ms

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
// called at the end of the pulse
void onPulseEnd() {
    if (DEBUG_ISR){Serial.println("SCR Low on pin :" + String(PIN_SCR));} ;
    digitalWrite(PIN_SCR, LOW);
}

// called when the delay after the zero crossing has expired
void onDelayExpired() {
    // start the pulse and arm a timer to end it
    // generate a pulse (below 50% this is a short pulse, above the pulse has a 3ms duration)
    digitalWrite(PIN_SCR, HIGH);
    if (DEBUG_ISR){Serial.println("SCR high on pin :" + String(PIN_SCR));} ;
    //call_later(percent_power < 50 ? 5 : 3000, onPulseEnd);
    call_later(SCR_PULSE_WIDTH*100, onPulseEnd);
}
void IRAM_ATTR onZero ();

// pin ZERO interrupt routine
void onZero() {
    if (DEBUG_ISR){Serial.println("Zero pulse detected on pin : " + String(PIN_ZERO));} ;
    // call_later(3000, onDelayExpired);
    if(percent_power > 0) {
        // generate a pulse after this zero
        // power=100%: no wait, power=0%: wait 10ms
      unsigned long delay = percent_power==100 ? 10 : (100-percent_power)*100;
      if (percent_power <= SCR_PULSE_WIDTH) {
          delay = (100-SCR_PULSE_WIDTH-1)*100 ;
      }
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
    attachInterrupt(digitalPinToInterrupt(PIN_ZERO), onZero, RISING);    // my SCR has zero detect on 1ms pulse 
    if (DEBUG_ISR){Serial.println("Setting up ISR on pin : "+ String (PIN_ZERO));} ;
}