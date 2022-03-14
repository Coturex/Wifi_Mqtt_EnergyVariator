#include <Arduino.h>



// SCR identifier, there might be more than one power regulation device on the network, select an id here
#define SCR_ID 0

// these pins are used to connect to the SCR
//#define PIN_ZERO D6
//#define PIN_SCR D5
extern int PIN_ZERO ;
extern int PIN_SCR ;

// current power (as a percentage of time) : power off at startup.
extern float percent_power ;

// pin ZERO interrupt routine
void onZero(void);

// timer interrupt routine : call the function which gas been registered earlier (see call_later)
void IRAM_ATTR onTimerISR();

// called at the end of the pulse
void pulseEnd() ;

// called when the delay after the zero crossing has expired
void pulseStart() ;



void setupISR();
