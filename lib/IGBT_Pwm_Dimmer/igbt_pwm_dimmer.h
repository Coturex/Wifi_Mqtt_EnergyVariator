typedef enum
{
    NORMAL_MODE = 0,
    TOGGLE_MODE = 1
} DIMMER_MODE_typedef;

typedef enum
{
    OFF = false,
    ON = true
} ON_OFF_typedef;

class dimmerLampESP8266 
{         
    private:
        int current_num;
		int timer_num;
        bool toggle_state;
        int tog_current;
		
    public:   
        int dimmer_pin ;
        dimmerLampESP8266(int pwn_dimmer_pin);
        void begin(DIMMER_MODE_typedef DIMMER_MODE, ON_OFF_typedef ON_OFF);
        void setPower(int percent);
        int getPower(void);
};
