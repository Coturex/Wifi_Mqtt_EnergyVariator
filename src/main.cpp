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

#define FW_VERSION "1.0"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>       // Mqtt lib

#include "mySCR.h"
#include "myConfig.h"         // WiFi and MQTT settings in this header : NOT 'Gited'

// these pins are used to connect to the SCR
int PIN_SCR=D5 ;
int PIN_ZERO=D6 ;

#ifdef USE_OTA
#include <ArduinoOTA.h>
#endif

//  TEST & DEBUG OPTION FLAGS
#ifdef NDEBUG
bool DEBUG = false ;
#else
bool DEBUG = true ;
#endif

// WiFi + MQTT stuff.
WiFiClient espClient ;
PubSubClient mqtt_client(espClient); 
String cmdTopic;
String outTopic;
String bootTopic;
#define MQTT_RETRY 5     // How many retries before starting AccessPoint

dimSCR dimmer(PIN_SCR, PIN_ZERO); //initialise port for dimmer(outPin, ZeroCrossing)

float previous_percent = 0 ;
bool boot_detected = false ;

void setup_wifi () {
    delay(10);
    WiFi.softAPdisconnect (true);
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

void wifi_connect () {
    // Wait for connection (even it's already done)
    int wifi_retry = 300 ; // retries during 5 min 

    while (WiFi.status() != WL_CONNECTED) {
        #ifdef USE_OLED
        oled_cls(1);
        display.println("Connecting");
        display.println("wifi (" + String(wifi_retry)+")");  
        display.display();
        #endif
        Serial.print(".");
        delay(1000); // 1s
        wifi_retry --;
        if (wifi_retry < 0) { // wifi timeout 
            Serial.println("Resetting due to Wifi timeout...");
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
                msg += ", \"chip_id\": ";
                msg += "\"" + String(ESP.getChipId()) + "\"" ;
                msg += ", \"vload_version\": ";
                msg += "\"" + String(FW_VERSION) + "\"" ;
                msg += ", \"ip\": ";  
                msg += WiFi.localIP().toString().c_str() ;
                msg += "}" ;
        if (DEBUG) { Serial.println("Sending Bootstrap on topic : " + String(bootTopic));}
        mqtt_client.publish(String(bootTopic).c_str(), msg.c_str()); 
}

void statusPub() {
    String msg = "{";
    msg += "\"percent\": ";
    msg += String(dimmer.getPower()) ;
    msg += ", \"power\": ";
    msg += ", \"mode\": ";
    msg += String(dimmer.getMode()) ;
    msg += ", \"state\": ";
    msg += String(dimmer.getState()) ;
    msg += "}";
    if (DEBUG) {
        Serial.println("statusPub on topic : " + outTopic);
        Serial.println("statusPub : " + msg);
      }
    mqtt_client.publish(String(outTopic).c_str(), msg.c_str()); 
} 

void on_message(char* topic, byte* payload, unsigned int length) {
    char buffer[length+1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    if (DEBUG) { Serial.println("[on_message] receiving msg on "+String(topic)+" : "+String(buffer));}; 
    
    if (String(buffer) == "bs") { // Bootstrap is requested
            if (DEBUG) { Serial.println("     Bootstrap resquested") ; } ;
            bootPub();
    } else if (String(buffer) == "ap") { // AccessPoint is requested
            if (DEBUG) { Serial.println("     AccessPoint resquested - not implemented") ; } ;
            //rebootOnAP(1);
    } else if (String(buffer) == "reboot") { // Reboot is requested
            if (DEBUG) { Serial.println("     Reboot resquested") ; } ;
            Serial.println("Resetting due to MQTT Reboot request...");
            ESP.restart(); 
    } else if (String(buffer) == "on") {  // Power set state to ON is requested
            if (DEBUG) { Serial.println("     Power ON resquested") ; } ;
            dimmer.setState(ON) ;
    } else if (String(buffer) == "off") { // Power set state to OFF is requested
            if (DEBUG) { Serial.println("     Power OFF resquested") ; } ;
            dimmer.setState(OFF) ;
    } else if (String(buffer) == "status") { // Status is requested
            if (DEBUG) { Serial.println("     Status resquested") ; } ;
            statusPub();
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
            statusPub();
        }
    }
}

void setup () {
    Serial.begin(115200);
    boot_detected = true ;

    setup_wifi() ;
    delay(5000) ;
    #ifdef USE_OTA
    // https://github.com/esp8266/Arduino/blob/master/libraries/ArduinoOTA/examples/BasicOTA/BasicOTA.ino
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
          Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
          Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();
    #endif
    
    bootTopic = String(settings.topic) ;
    outTopic = String(settings.topic) + "/" + String(settings.name) ;         //e.g topic :regul/vload/id-00/
    cmdTopic = outTopic + "/cmd" ;

    mqtt_client.setServer(settings.mqtt_server, 1883); // data will be published
    mqtt_client.setCallback(on_message); // subscribing/listening mqtt cmdTopic
    
    dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
    // dimmer.setState(ON) ;
    // dimmer.setPower(70) ;
}

void loop_manuelle () {
    //unsigned long startTime = millis();
    //delay(5000) ;
    Serial.println("Power % [0-100] ? : ");
    while (Serial.available() == 0) {
    }
    int inValue = Serial.parseInt();
    if(inValue >= 0 && inValue<=100) {
        dimmer.setPower(inValue) ;
    }
    //Serial.print("loop time (ms) : ") ;
    //Serial.println((millis()-startTime)); // print spare time in loop 
    Serial.print("percent_power : ") ;
    Serial.println(String(dimmer.getPower()));


}

void reconnect() {
    while (!mqtt_client.connected()) {
        String clientId = "scr-";
        clientId += String(settings.name);
        if (mqtt_client.connect(clientId.c_str())) {
            mqtt_client.subscribe(cmdTopic.c_str());
            if (DEBUG) { Serial.println("Subscribing topic : " + String(cmdTopic));}

        } else {
            delay(5000);
        }
    }
}

void loop() {
    if (!mqtt_client.connected()) {
        reconnect();
    }
    mqtt_client.loop(); // seems it blocks for 100ms
    
    if (boot_detected) { 
        bootPub() ;
        boot_detected = false ;
    }
    // DO NOT USE DELAY FUNCTION IN THIS LOOP
    // OTHERWI IT WILL OCCURED AN ESP RESET - CONFLICT w/ ISR INTERUPT timer1_write
    #ifdef USE_OTA
    ArduinoOTA.handle();
    #endif


}


