#include <Arduino.h>

typedef enum
{
    NORMAL_MODE = 0,
    SMOUTH_MODE = 1
} DIMMER_MODE_typedef;

typedef enum
{
    OFF = false,
    ON = true
} ON_OFF_typedef;


void onZero(void);
void IRAM_ATTR onTimerISR();

class dimmerLampESP8266 
{         
    private:
        int current_num;
		int timer_num;	
		void port_init(void);
		void timer_init(void);
		void ext_int_init(void);
		
    public:   
        uint16_t pulse_begin;
        int dimmer_pin;
        int zc_pin;

        dimmerLampESP8266(int user_dimmer_pin, int zc_dimmer_pin);
        void begin(DIMMER_MODE_typedef DIMMER_MODE, ON_OFF_typedef ON_OFF);
        void setPower(int power);
		int  getPower(void);
		void setState(ON_OFF_typedef ON_OFF);
        bool getState(void);
		void changeState(void);
        void setMode(DIMMER_MODE_typedef DIMMER_MODE);
        DIMMER_MODE_typedef getMode(void); 
};
