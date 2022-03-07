#include <Arduino.h>

// SCR identifier, there might be more than one power regulation device on the network, select an id here
#define SCR_ID 0

// these pins are used to connect to the SCR
#define PIN_ZERO D6
#define PIN_SCR D5

// current power (as a percentage of time) : power off at startup.

extern float power ;

// this function pointer is used to store the next timer action (see call_later and onTimerISR below)
// void (*timer_callback)(void);


extern void call_later(unsigned long duration_us, void(*callback)(void)) ;

// timer interrupt routine : call the function which gas been registered earlier (see call_later)
extern void IRAM_ATTR onTimerISR();

// called at the end of the pulse
void onPulseEnd() ;

// called when the delay after the zero crossing has expired
void onDelayExpired() ;

// pin ZERO interrupt routine
void onZero();

void setupISR();
