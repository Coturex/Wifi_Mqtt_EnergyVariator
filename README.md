# **this project is on going...**

# Wifi_Mqtt_EnergyVariator
 * Adjust 'Power Consumption' of a resistive load (up to 3kW)
 * Data sent to your Domotic Box, raspberry, PC... using an MQTT Broker
 * Listening  Power command received on MQTT topic
 * Publish Power values on MQTT 'vload topic' and on 'domoticz/in' topic (for Domoticz)
 * Display Power/Percent/Max on mini screen    

 * Wifi Access Point WebServer and set custom parameters
 * WebOTA : On The Air firmware update (url http://<vload_ip>:8080/update - 60s after boot)

![vload_AP](https://user-images.githubusercontent.com/53934994/140624657-ae7fd28b-fc34-4558-865f-514c03860ca4.png)

## Hardware requirements:   ~25-28 €
 * IGBT Power Variator  (homebrew...)
    - no human frequency sound
    - reasonable frequency spurious (vs TRIAC)
    - very low resistive on transcient region (_dv/dt_) and can handle more current
      (less overheating )
    - no zero crossing managment

![PWM-Control-in-AC11](https://user-images.githubusercontent.com/53934994/140613898-13044e00-b3ac-4ed6-af85-960940436992.jpg)

PCB/Schematic of the Electronic card ... coming soon...
![pcb](https://user-images.githubusercontent.com/53934994/146585658-820fab47-7e99-483e-8af0-4b72cec1ef72.png)

 * ESP Board : Wemos d1 mini (CH341 uart), esp8266
   - When choosing GPIO pins to use, it's best to avoid GPIO 0, 2 and 15 (D3, D4, D8)

* Oled Shield 64x48 option _(let use 'define' or 'undef' directive in src/main.cpp code)_
   - I2C wired on D1-SCL D2-SDA

* AC-DC 5V 700mA-Small

## FYI : 
some Linux distrib (Ubuntu 20.x) failed on connect Uart CH340/1 while flashing ESP8266

     →  "Timed out waiting for packet header"
fixed in kernels 5.13.14 and maybe upper 
(https://cdn.kernel.org/pub/linux/kernel/v5.x/ChangeLog-5.13.14)

## Todo :
