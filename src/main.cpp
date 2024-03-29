/* Copyright (C) 2021-2022 Coturex - F5RQG
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
// D1 : I2C clock - OLED
// D2 : I2C data  - OLED
// D5 : SCR Triac Dimmer - PWM IGBT Gate   (1023 Hz) - OUTPUT
// D6 : ZeroCrossing pulse - INPUT

#define FW_VERSION "1.1b"  // will be published on mqtt
#define PCB "white" // set "white" or "green"  and uncomment // #define USE_MYSCR_GREEN_PCB
// #define USE_MYSCR_GREEN_PCB

//  TEST & DEBUG OPTION FLAGS
#ifdef NDEBUG
bool DEBUG = false ;
#else
bool DEBUG = true ;
#endif
bool DEBUG_TOPIC = true ;

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>       // Mqtt lib
#include <ArduinoJson.h>

#include "mySCR.h"
#include "myConfig.h"         // WiFi and MQTT settings in this header : NOT 'Gited'

// these pins are used to connect to the SCR
int PIN_SCR=D5 ;
int PIN_ZERO=D6 ;

#ifdef USE_OTA
#include "WebOTA.h"
#endif

// WiFi + MQTT stuff.
WiFiClient espClient ;
PubSubClient mqtt_client(espClient); 
String domTopic; 
String cmdTopic;
String outTopic;
String debugTopic;
String bootTopic;

String last_datetime = "" ; // last dateTime received from domoticz order
int last_nvalue = 999 ;   // last power value received from domoticz order

#define MQTT_RETRY 10     // How many retries before starting AccessPoint
#define WIFI_RETRY 300    // How many retries before starting AccessPoint

dimSCR dimmer(PIN_SCR, PIN_ZERO); //initialise port for dimmer(outPin, ZeroCrossing)

float previous_percent = 0 ;
bool boot_detected = false ;

void setup_wifi () {
    delay(10);
    WiFi.softAPdisconnect (true);
    Serial.println("Trying to connect wifi : " + String(settings.ssid));
    WiFi.begin(settings.ssid, settings.password);
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("");
    Serial.print("Wifi Connected");
  
    //If connection successful show IP address in serial monitor
    Serial.println("");
    Serial.print("Connected to Network/SSID : ");
    Serial.println(settings.ssid) ;
    Serial.print("IP address : ");
    Serial.println(WiFi.localIP());  //IP address assigned to your ESP
}

void wifi_connect (int retry) {
    // Wait for connection (even it's already done)
    while (WiFi.status() != WL_CONNECTED) {
        #ifdef USE_OLED
        oled_cls(1);
        display.println("Connecting");
        display.println("wifi (" + String(retry)+")");  
        display.display();
        #endif
        Serial.print(".");
        delay(1000); // 1s
        retry --;
        if (retry < 0) { // wifi timeout 
            ESP.restart() ;
        }
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

void bootPub() {
        String  msg = "{\"type\": \"vload\"";	
                msg += ", \"id\": ";
                msg += "\"" + String(settings.name) + "\"" ;
                msg += ", \"fw_version\": ";
                msg += "\"" + String(FW_VERSION) + "\"" ;
                msg += ", \"PCB\": ";
                msg += "\"" + String(PCB) + "\"" ;
                msg += ", \"chip_id\": ";
                msg += "\"" + String(ESP.getChipId()) + "\"" ;
                msg += ", \"ip\": \"";  
                msg += WiFi.localIP().toString().c_str() ;
                #ifdef USE_OTA
                msg += "\", \"OTA\": \"mqtt\""; 
                #else
                msg += ", \"OTA\": \"not implemented\""; 
                #endif
                msg += "}" ;
        if (DEBUG) { Serial.println("Sending Bootstrap on topic : " + String(bootTopic));}
        mqtt_client.publish(String(bootTopic).c_str(), msg.c_str()); 
}

void statusPub(String message) {
    String msg = "{";
    msg += "\"percent\": ";
    msg += String(dimmer.getPower()) ;
    msg += ", \"power\": 0";
    msg += ", \"mode\": ";
    msg += String(dimmer.getMode()) ;
    msg += ", \"state\": ";
    // msg += String(dimmer.getState()) ;

    if (String(dimmer.getState()) =="1") 
         msg += "\"ON\"" ;
    else msg += "\"OFF\"" ;

    msg += ", \"msg\": \"";
    msg += String(message) ;    
    msg += "\"}";

    if (DEBUG) {
        Serial.println("[statusPub] on topic : " + outTopic);
        Serial.println("[statusPub] send msg : " + msg);
      }
    mqtt_client.publish(String(outTopic).c_str(), msg.c_str()); 
} 

void debugPub(String message) {
    String msg = String(message) ;
    if (DEBUG_TOPIC) {
        mqtt_client.publish(String(debugTopic).c_str(), msg.c_str());
    }    
}

void on_message(char* topic, byte* payload, unsigned int length) {
    char buffer[length+1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    if (DEBUG) { Serial.println("\n[on_message] receiving msg on "+String(topic)+" : "+String(buffer));}; 

    // Received order FROM TOPIC DOMOTICZ/OUT/...
    if (String(topic) == String(settings.domTopic)) {
        StaticJsonDocument <1024> doc;
        deserializeJson(doc,payload);   
        int nvalue = doc["nvalue"] ;
        String lastupdate = doc["LastUpdate"] ;

        if (DEBUG) { Serial.println("[on_message] Received nvalue :" + String(nvalue)); };
        /*
        if (nvalue == last_nvalue) {
            if (DEBUG) { Serial.print("[on_message] same nvalue  last_nvalue, order ignored"); };
            debugPub (lastupdate + String(" , same last_nvalue, order ignored")) ;
            return ;
        }
        */

        if (lastupdate == last_datetime) {
            if (DEBUG) { Serial.println("[on_message] same dateTime, domoticz order ignored"); };
            debugPub (lastupdate + String(" , same datetime, domoticz order ignored")) ;
            return ;
        }

        last_datetime  = lastupdate ;
        last_nvalue = nvalue ;
        nvalue = nvalue * 100 ;
        if (DEBUG) { Serial.println("[on_message] Set power to (%) : " + String(nvalue)) ; } ;
        previous_percent = String(nvalue).toFloat() ;
        dimmer.setPower(nvalue) ;
        statusPub("setPower from Domoticz");
        debugPub(lastupdate) ;

    } else { // FROM TOPIC MQTT .../CMD
        if (String(buffer) == "bs") { // Bootstrap is requested
                if (DEBUG) { Serial.println("     Bootstrap resquested") ; } ;
                bootPub();
        } else if (String(buffer) == "ap") { // AccessPoint is requested
                if (DEBUG) { Serial.println("     AccessPoint resquested - not implemented") ; } ;
                //rebootOnAP(1);
        } else if (String(buffer) == "ota") { // OTA is requested
                #ifdef USE_OTA
                if (DEBUG) { Serial.println("     OTA requested") ; } ;
                statusPub("OTA_requested") ;
	            webota.handle(); 
                webota.delay(30000);    
                //statusPub("OTA timeout") ;
                if (DEBUG) { Serial.println("     OTA timeout") ; } ;          
                #endif
        } else if (String(buffer) == "reboot") { // Reboot is requested
                if (DEBUG) { Serial.println("     Reboot resquested") ; } ;
                statusPub("reboot_requested") ;
                delay(1000) ;
                Serial.println("Resetting due to MQTT Reboot request...");
                ESP.restart(); 
        } else if (String(buffer) == "on") {  // Power set state to ON is requested
                if (DEBUG) { Serial.println("     Power ON resquested") ; } ;
                dimmer.setState(ON) ;
                statusPub("set ON");
        } else if (String(buffer) == "off") { // Power set state to OFF is requested
                if (DEBUG) { Serial.println("     Power OFF resquested") ; } ;
                dimmer.setState(OFF) ;
                statusPub("set OFF");
        } else if (String(buffer) == "status") { // Status is requested
                if (DEBUG) { Serial.println("     Status resquested") ; } ;
                statusPub("status_requested") ;
        } else {
                float p = String(buffer).toFloat();
                if(p >= 0 && p<=100) {
                    if (DEBUG) { Serial.println("     Set power to (%) : " + String(p)) ; } ;
            /*
            if ((p - previous_percent) > 10) {
                dimmer.setMode(SMOUTH_MODE);
                // dimmer.toggleSettings(0,100);
            } else {
                dimmer.setMode(NORMAL_MODE);
            }
            */
                    previous_percent = p ;
                    dimmer.setPower(p) ;
                    statusPub("setpower");
                }
        }
    }
}

void setup () {
    Serial.begin(115200);
    boot_detected = true ;

    setup_wifi() ;
    delay(5000) ;
    #ifdef USE_OTA
    webota.init(8080,"/update"); // Init WebOTA server
    #endif
    
    bootTopic = String(settings.topic) ;
    outTopic = String(settings.topic) + "/" + String(settings.name) ;         //e.g topic :regul/vload/id-00/
    debugTopic = outTopic + "/debug" ;
    cmdTopic = outTopic + "/cmd" ;
    domTopic = String(settings.domTopic) ;

    mqtt_client.setServer(settings.mqtt_server, settings.mqtt_port); // data will be published
    mqtt_client.setBufferSize(512) ;
    mqtt_client.setCallback(on_message); // subscribing/listening mqtt cmdTopic
    
    dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
    // dimmer.setState(ON) ;
    // dimmer.setPower(70) ;
}

bool mqtt_connect(int retry) {
    bool ret = false ;
    while (!mqtt_client.connected() && WiFi.status() == WL_CONNECTED && retry) {
        String clientId = "regul-"+String(settings.name);
        Serial.print("[mqtt_connect] (re)connecting (" + String(retry) + " left) ") ;
        retry--;
        Serial.println("[mqtt_connect]"+String(settings.mqtt_server)+":"+String(settings.mqtt_port)) ; 
        #ifdef USE_OLED
        oled_cls(1);
        display.println("Connecting");
        display.println("mqtt (" + String(retry)+")");  
        //display.println("idx_p :" + String (settings.idx_power) );
        //display.println("idx_v :" + String (settings.idx_voltage) );
        display.display();
        #endif
        if (!mqtt_client.connect(clientId.c_str())) {
            ret = false ;
            delay(5000);
        } else {
            ret = true ;
            Serial.println("[mqtt_connect] Subscribing : "+ cmdTopic) ; 
            mqtt_client.subscribe(cmdTopic.c_str());
            Serial.println("[mqtt_connect] Subscribing : "+ domTopic) ; 
            mqtt_client.subscribe(domTopic.c_str());
            delay(2000);
        }
    }
    return ret ;
}

void loop() {
    
    // if (DEBUG) {Serial.println("--") ;} ;
    if (WiFi.status() != WL_CONNECTED) {
        wifi_connect(WIFI_RETRY);
    }
    if (!mqtt_client.connected() && WiFi.status() == WL_CONNECTED ) {
       if (mqtt_connect(MQTT_RETRY)) { 
           bootPub();
           if (DEBUG_TOPIC) debugPub("DEBUG ACTIVATED");
        } else {
	    if (WiFi.status() == WL_CONNECTED) {
	      if (DEBUG) {Serial.println("-- mqtt retry reached, rebooting...") ;} ;
	      ESP.restart();
            }
        }
    }
    mqtt_client.loop(); // seems it blocks for 100ms
    
    if (boot_detected) { 
        statusPub("boot_detected");
        boot_detected = false ;
    }
    // DO NOT USE DELAY FUNCTION IN THIS LOOP
    // OTHERWISE IT WILL OCCURED AN ESP RESET - CONFLICT w/ ISR INTERUPT timer1_write
    // then OTA is not possible in this loop !!
}
