#define SL 35

struct { 
    char name[SL] = "ECS" ;
  
    char ssid[SL] = "";
    char password[SL] = "";

    char mqtt_server[SL] = "";
    char topic[SL] = "regul/vload" ;
} settings;

