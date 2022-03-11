
#include <Arduino.h>
#include "RBDdimmerESP8266.h"

int dim_tim[10];
int dim_max[10];
int timeoutPin = 435; // 80us
int extIntPin = 2; //z-c

int pulseWidth = 1;
volatile int current_dim = 0;
int rise_fall = true;
static int tog;
static int max_tim_for_tog = 1;
char user_zero_cross = '0';

static int toggleCounter = 0;
static int toggleReload = 25;

static dimmerLampESP8266* dimmer;
volatile int dimPower;
volatile int dimOutPin;
volatile int dimZCPin;
volatile int zeroCross;
volatile DIMMER_MODE_typedef dimMode;
volatile ON_OFF_typedef dimState;
volatile int dimCounter;
static uint16_t dimPulseBegin;
volatile bool togDir;

dimmerLampESP8266::dimmerLampESP8266(int user_dimmer_pin, int zc_dimmer_pin):
	dimmer_pin(user_dimmer_pin),
	zc_pin(zc_dimmer_pin)
{
	
}

void dimmerLampESP8266::timer_init(void)
{
	timer1_attachInterrupt(onTimerISR);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
	timer1_write(timeoutPin); //100 us
}

void dimmerLampESP8266::ext_int_init(void) 
{
	int inPin = dimZCPin;
	pinMode(inPin, INPUT_PULLUP);
	attachInterrupt(inPin, onZero, RISING);
}


void dimmerLampESP8266::begin(DIMMER_MODE_typedef DIMMER_MODE, ON_OFF_typedef ON_OFF)
{
	dimMode = DIMMER_MODE;
	dimState = ON_OFF;
	timer_init();
	ext_int_init();	
}

void dimmerLampESP8266::setPower(int power)
{	
	if (power >= 99) 
	{
		power = 99;
	}
	dimPower = power;
	dimPulseBegin = 1;
	delay(1);
}

int dimmerLampESP8266::getPower(void)
{
	if (dimState == ON)
		return dimPower ;
	else return 0;
}

void dimmerLampESP8266::setState(ON_OFF_typedef ON_OFF)
{
	dimState = ON_OFF;
}

bool dimmerLampESP8266::getState(void)
{
	bool ret;
	if (dimState == ON) ret = true;
	else ret = false;
	return ret;
}

void dimmerLampESP8266::changeState(void)
{
	if (dimState == ON) dimState = OFF;
	else 
		dimState = ON;
}

DIMMER_MODE_typedef dimmerLampESP8266::getMode(void)
{
	return dimMode;
}

void dimmerLampESP8266::setMode(DIMMER_MODE_typedef DIMMER_MODE)
{
	dimMode = DIMMER_MODE;
}

 
void IRAM_ATTR onZero()
{
	for (int i = 0; i < current_dim; i++ ) 
		if (dimState == ON) 
		{
			zeroCross = 1;
		}
}


static int k;
void IRAM_ATTR onTimerISR()
{	
	timer1_write(timeoutPin);	
}
