/*
 * Copyright (C) 2021-2022 Coturex - F5RQG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Wemos pin use : it's best to avoid GPIO 0, 2 and 15 (D3, D4, D8)
// D5 : unused 
// D6 : ZeroCrossing
// D7 : Triac Dimmer - PWM IGBT Gate   (1023 Hz)
// D1 : I2C clock - OLED
// D2 : I2C data  - OLED

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>       // Mqtt lib
#include <SoftwareSerial.h>     // ESP8266/wemos requirement
#include <WiFiManager.h>        // Manage Wifi Access Point if wifi connect failure (todo : and mqtt failure)

// I2C OLED screen stuff
#ifdef USE_OLED
#include "Adafruit_SSD1306.h"   // OLED requirement
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET); // Wemos I2C : D1-SCL D2-SDA
#endif

#ifdef USE_OTA
#include "WebOTA.h"
#endif

#ifndef USE_IGBT
#include "RBDdimmerESP8266.h"
dimmerLampESP8266 dimmer(D7, D6); //initialase port for dimmer(outPin, ZeroCrossing)
#else
#include "igbt_pwm_dimmer.h"
dimmerLampESP8266 dimmer(D7); //initialase port for dimmer(outPin)
#endif

//#include "myConfig_sample.h"  // Personnal settings - 'gited file'
//#include "myConfig.h"           // Personnal settings - Not 'gited file'
#include <EEPROM.h>             // EEPROM access...

#define FW_VERSION "1.0"
#define DOMO_TOPIC "domoticz/in"

// WiFi + MQTT stuff.
WiFiClient espClient ;
PubSubClient mqtt_client(espClient);  
String cmdTopic;
String outTopic;
#define MQTT_RETRY 5     // How many retries before starting AccessPoint

//  TEST & DEBUG OPTION FLAGS
bool DEBUG = true ;
bool TEST_CP         = false; // AP : always start the ConfigPortal, even if ap found
int  TESP_CP_TIMEOUT = 30;    // AP : AccessPoint timeout and leave AP
bool ALLOWONDEMAND   = true;  // AP : enable on demand - e.g Trigged on Gpio Input, On Mqtt Msg etc...

// current power (as a percentage of time) : power off at startup.
float power = 0;

#define PWM_PIN D7

#define MAX_STRING_LENGTH 35
struct { 
    char name[MAX_STRING_LENGTH] = "";
    char mqtt_server[MAX_STRING_LENGTH] = "";
    char mqtt_port[MAX_STRING_LENGTH] = "";
    char vload_topic[MAX_STRING_LENGTH] = "";
    char vload_id[MAX_STRING_LENGTH] = "";
    char idx_power[MAX_STRING_LENGTH] = "";
    char idx_percent[MAX_STRING_LENGTH] = "";
    char power_max[MAX_STRING_LENGTH] = "";
    int AP = 0 ;
  } settings;

WiFiManager wm;
WiFiManagerParameter custom_name("name", "Oled Title", "", 15);
WiFiManagerParameter custom_mqtt_server("mqtt_server", "mqtt IP server", "", 15);
WiFiManagerParameter custom_mqtt_port("mqtt_port", "mqtt Port", "", 4);
WiFiManagerParameter custom_vload_topic("vload_topic", "vload Topic", "", 35);
WiFiManagerParameter custom_vload_id("vload_id", "vload ID", "", 15);
WiFiManagerParameter custom_idx_power("idx_power", "Domoticz idx power", "", 4); 
WiFiManagerParameter custom_idx_percent("idx_volt", "Domoticz idx voltage", "", 4);
WiFiManagerParameter custom_power_max("power_max", "Maximum load power", "", 4);

#ifdef USE_OLED
void oled_cls(int size) {
    // OLED : set cursor on top left corner and clear
    display.clearDisplay();
    display.setTextSize(size);
    display.setTextColor(WHITE); // seems only WHITE exist on this oled model :(
    display.setCursor(0,0);
}
#endif

void read_Settings () { // From EEPROM
    unsigned int addr=0 ;  
    //Serial.println("[READ EEPROM] read_Settings");  
    EEPROM.get(addr, settings); //read data from array in ram and cast it to settings
    Serial.println("[READ EEPROM] Oled name : " + String(settings.name) ) ;
    Serial.println("[READ EEPROM] mqtt_server : " + String(settings.mqtt_server) ) ;
    Serial.println("[READ EEPROM] mqtt_port : " + String(settings.mqtt_port) ) ;
    Serial.println("[READ EEPROM] vload_topic : " + String(settings.vload_topic) ) ;
    Serial.println("[READ EEPROM] vload_id : " + String(settings.vload_id) ) ;
    Serial.println("[READ EEPROM] idx_power : " + String(settings.idx_power) ) ;
    Serial.println("[READ EEPROM] idx_percent : " + String(settings.idx_percent) ) ;
    Serial.println("[READ EEPROM] power_max : " + String(settings.power_max) ) ;
    Serial.println("[READ EEPROM] power_max : " + String(settings.AP) ) ;
}

void saveWifiCallback() { // Save settings to EEPROM
    unsigned int addr=0 ;
    read_Settings() ;
    Serial.println("[CALLBACK] saveParamCallback fired"); 
    if (custom_name.getValue()[0] != '.') { 
        strncpy(settings.name, custom_name.getValue(), MAX_STRING_LENGTH);  
    }
    if (custom_mqtt_server.getValue()[0] != '.') { 
        strncpy(settings.mqtt_server, custom_mqtt_server.getValue(), MAX_STRING_LENGTH);  
    }
    if (custom_mqtt_port.getValue()[0] != '.') { 
        strncpy(settings.mqtt_port, custom_mqtt_port.getValue(), MAX_STRING_LENGTH);  
    }
    if (custom_vload_topic.getValue()[0] != '.') { 
        strncpy(settings.vload_topic, custom_vload_topic.getValue(), MAX_STRING_LENGTH);  
    }
    if (custom_vload_id.getValue()[0] != '.') { 
        strncpy(settings.vload_id, custom_vload_id.getValue(), MAX_STRING_LENGTH);  
    }
    if (custom_idx_power.getValue()[0] != '.') { 
        strncpy(settings.idx_power, custom_idx_power.getValue(), MAX_STRING_LENGTH);  
    }
    if (custom_idx_percent.getValue()[0] != '.') { 
        strncpy(settings.idx_percent, custom_idx_percent.getValue(), MAX_STRING_LENGTH);  
    }
    if (custom_power_max.getValue()[0] != '.') { 
        strncpy(settings.power_max, custom_power_max.getValue(), MAX_STRING_LENGTH);  
    }
    settings.AP = 0 ;
    EEPROM.put(addr, settings); //write data to array in ram 
    EEPROM.commit();  //write data from ram to flash memory. Do nothing if there are no changes to EEPROM data in ram
}

void wifi_connect () {
    // Wait for connection (even it's already done)
    while (WiFi.status() != WL_CONNECTED) {
        #ifdef USE_OLED
        oled_cls(1);
        display.println("Connecting");
        display.println("wifi");
        display.display();
        #endif
        delay(250);
        Serial.print(".");
        delay(250);
    }

    Serial.println("");
    Serial.print("Wifi Connected");
    Serial.println("");
    Serial.print("Connected to Network : ");
    Serial.println(WiFi.localIP());  //IP address assigned to ESP
    #ifdef USE_OLED
    oled_cls(1);
    display.println("Wifi on");
    display.println(WiFi.localIP());
    display.display();
    #endif
}

void setup_wifi () {
    delay(10);
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP   
    // WiFi.setSleepMode(WIFI_NONE_SLEEP); // disable sleep, can improve ap stability

    // wm.debugPlatformInfo();
    //reset settings - for testing
    // wm.resetSettings();
    // wm.erase();  

    // setup some parameters
    WiFiManagerParameter custom_html("<p>EEPROM Custom Parameters</p>"); // only custom html
    wm.addParameter(&custom_html);
    wm.addParameter(&custom_name);   
    wm.addParameter(&custom_mqtt_server);
    wm.addParameter(&custom_mqtt_port);
    wm.addParameter(&custom_vload_topic); 
    wm.addParameter(&custom_vload_id);
    wm.addParameter(&custom_idx_power);
    wm.addParameter(&custom_idx_percent);
    wm.addParameter(&custom_power_max);               
    // callbacks
    //wm.setAPCallback(configModeCallback);
    wm.setSaveConfigCallback(saveWifiCallback);
    wm.setBreakAfterConfig(true); // needed to use saveWifiCallback
    
    // set values later if I want
    // custom_html.setValue("test",4);
    // custom_token.setValue("test",4);

    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep in seconds
    wm.setConfigPortalTimeout(120);
    WiFi.printDiag(Serial);
    if(!wm.autoConnect("vload_AP","admin")) {
        Serial.println("failed to connect and hit timeout");
    } 
    else if(TEST_CP or settings.AP) {
        // start configportal always
        delay(1000);
        wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
        switch (settings.AP) {
            case 1: 
                wm.startConfigPortal("request_vload_AP");
                Serial.println("AP Config Portal : requested on topic/cmd");
                break ;
            case 2:    
                wm.startConfigPortal("mqtt_vload_AP");
                Serial.println("AP Config Portal : mqtt connection failure");
                break ;
        } 
    }
    else {
        //Here connected to the WiFi
        Serial.println("connected...yeaaa :)");
    }
    WiFi.printDiag(Serial);
}

bool mqtt_connect(int retry) {
    bool ret = false ;
    while (!mqtt_client.connected() && WiFi.status() == WL_CONNECTED && retry) {
        String clientId = "vload-" + String(settings.vload_id);
        Serial.print("[mqtt_connect] (re)connecting (" + String(retry) + ") ") ;
        retry--;
        Serial.println("[mqtt_connect]"+String(settings.mqtt_server)+":"+String(settings.mqtt_port)) ; 
        #ifdef USE_OLED
        oled_cls(1);
        display.println("Connecting");
        display.println("mqtt - (" + String(retry)+")");  
        display.println("idx_v :" + String (settings.idx_power) );
        display.println("idx_p :" + String (settings.idx_percent) );
        display.display();
        #endif
        if (!mqtt_client.connect(clientId.c_str())) {
            ret = false ;
            delay(5000);
        } else {
            ret = true ;
            Serial.println("[mqtt_connect] Subscribing : "+ cmdTopic) ; 
            delay(2000);
            mqtt_client.subscribe(cmdTopic.c_str());
        }
    }
    return ret ;
}

void bootPub() {
        String  msg = "{\"type\": \"vload\"";	
                msg += ", \"id\": ";
                msg += "\"" + String(settings.vload_id) + "\"" ;
                msg += ", \"fw_version\": ";
                msg += "\"" + String(FW_VERSION) + "\"" ;
                msg += ", \"vload_version\": ";
                msg += "\"v3.0\"" ;
                msg += ", \"vload_idx1\": ";
                msg += "\"" + String(settings.idx_power) + "\"" ;
                msg += ", \"vload_idx2\": ";
                msg += "\"" + String(settings.idx_percent) + "\"" ;
                msg += ", \"ip\": ";  
                msg += WiFi.localIP().toString().c_str() ;
                msg += "}" ;
        if (DEBUG) { Serial.println("Sending Bootstrap on topic : " + String(settings.vload_topic));}
        mqtt_client.publish(String(settings.vload_topic).c_str(), msg.c_str()); 
}

void domoPub(String idx, float value) {
    if (idx.toInt() > 0) {
      String msg = "{\"idx\": ";	 // {"idx": 209, "nvalue": 0, "svalue": "2052"}
      msg += idx;
      msg += ", \"nvalue\": 0, \"svalue\": \"";
      msg += value ;
      msg += "\"}";

      String domTopic = DOMO_TOPIC;             // domoticz topic
      if (DEBUG) {
        Serial.println("domoPub on topic : " + domTopic);
        Serial.println("domoPub : " + msg);
      }  
      mqtt_client.publish(domTopic.c_str(), msg.c_str()); 
    }
}

void statusPub() {
    String msg = "{";
    msg += "\"percent\": ";
    msg += String(dimmer.getPower()) ;
    msg += ", \"power\": ";
    msg += String("");
    msg += "}";
    String topic = String(settings.vload_topic)+"/"+String(settings.vload_id) ;
    if (DEBUG) {
        Serial.println("statusPub on topic : " + topic);
        Serial.println("statusPub : " + msg);
      }
    mqtt_client.publish(String(topic).c_str(), msg.c_str()); 
} 

void rebootOnAP(int ap){
        Serial.println("Force Rebooting on Acess Point");
        settings.AP = ap ;
        unsigned int addr=0 ;
        EEPROM.put(addr, settings); //write data to array in ram 
        EEPROM.commit();  // write data from ram to flash memory. Do nothing if there are no changes to EEPROM data in ram
        ESP.restart();    // call AP directly doesn't works cleanly, a reboot is needed
}

void on_message(char* topic, byte* payload, unsigned int length) {
    if (DEBUG) { Serial.print("[on_message] receiving msg on "); Serial.println(String(topic));}; 
    char buffer[length+1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    
    if (String(buffer) == "bs") { // Bootstrap is requested
            if (DEBUG) { Serial.println("     Bootstrap resquested") ; } ;
            bootPub();
    } else if (String(buffer) == "ap") { // AccessPoint is requested
            if (DEBUG) { Serial.println("     AccessPoint resquested") ; } ;
            rebootOnAP(1);
    } else if (String(buffer) == "reboot") { // Reboot is requested
            if (DEBUG) { Serial.println("     Reboot resquested") ; } ;
            ESP.restart(); 
    } else {
        float p = String(buffer).toFloat();
        if (DEBUG) { Serial.println("     Set power to (%) : " + String(p)) ; } ;
        if(p >= 0 && p<=100) {
            dimmer.setPower(p) ;

        }
    }
}

void setup() {  
    randomSeed(micros());  // initializes the pseudo-random number generator
    Serial.begin(115200);
    dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
   
    #ifdef USE_OLED
    // OLED Shield 
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
    display.display();
    #endif

    //load eeprom data (sizeof(settings) bytes) from flash memory into ram
    EEPROM.begin(sizeof(settings));
    Serial.println("EEPROM size: " + String(sizeof(settings)) + " bytes");
    read_Settings(); // read EEPROM

    setup_wifi() ;
    delay(5000) ;
    uint16_t port ;
    char *ptr ;
    port = strtoul(settings.mqtt_port,&ptr,10) ;
    cmdTopic = String(settings.vload_topic) + "/" + String(settings.vload_id) +"/cmd";  //e.g topic :regul/vload/id-00/cmd
    outTopic = String(settings.vload_topic) + "/" + String(settings.vload_id) ;         //e.g topic :regul/vload/id-00/

    mqtt_client.setServer(settings.mqtt_server, port); // data will be published
    mqtt_client.setCallback(on_message); // subscribing/listening mqtt cmdTopic
    // OTA 
    #ifdef USE_OTA
    webota.init(8080,"/update"); // Init WebOTA server 
    #endif
    statusPub() ;
}

void loop() {
    unsigned long startTime = millis();
    if (WiFi.status() != WL_CONNECTED) {
        wifi_connect();
    }
    if (!mqtt_client.connected() && WiFi.status() == WL_CONNECTED ) {
        if (mqtt_connect(MQTT_RETRY)) { 
            bootPub() ;
        } else {
            rebootOnAP(2);
        }
    }
    mqtt_client.loop(); // seems it blocks for 100ms
    
    #ifdef USE_OLED
    oled_cls(1);
    display.println(String(settings.name));
    display.println("");
    display.print("% : ");
    display.println("---");
    display.print("W : ");
    display.println("---");
    display.println("");
    display.print("max : ");
    display.print(String(settings.power_max));
    display.print(" W");
    display.display();
    #endif
    #ifdef USE_OTA
    webota.handle();
    webota.delay(100);
    #else
    delay(100) ;
    #endif

    if (DEBUG) {
        //Serial.print("loop time (ms) : ") ;
        //Serial.println((millis()-startTime)); // print spare time in loop 
        }   
}

