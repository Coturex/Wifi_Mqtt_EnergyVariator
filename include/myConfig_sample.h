#define SL 35

struct { 
    char name[SL] = "ECS" ;
  
    char ssid[SL] = "";
    char password[SL] = "";

    char mqtt_server[SL] = "";
    int  mqtt_port  = 1883 ;
    char topic[SL] = "regul/vload" ;
} settings;

