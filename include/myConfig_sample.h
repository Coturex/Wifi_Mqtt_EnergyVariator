#define SL 35

struct { 
    char name[SL] = "ECS" ;
    char ssid[SL] = "XXX";
    char password[SL] = "12345678";

    char mqtt_server[SL] = "192.168.43.118";
    int  mqtt_port  = 1883 ;
    char topic[SL] = "regul/vload" ;
} settings;