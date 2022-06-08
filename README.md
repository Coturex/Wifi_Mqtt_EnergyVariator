# Wifi_Mqtt_EnergyVariator
 * Adjust 'Power Consumption' of a resistive load (up to 3kW)
 * Data sent to your Domotic Box, raspberry, PC... using an MQTT Broker
 * Listen  Power command received on MQTT topic
 * Publish Power values on MQTT 'vload topic' 
 * WebOTA : On The Air firmware update

 URL : http://<pzem_ip>:8080/update
    

## Hardware requirements:   ~20 €
 * Power variator
 
 
  ![aliExpressScr](https://github.com/Coturex/Wifi_Mqtt_EnergyVariator/blob/main/doc/scr_aliExpress.jpeg)
  
 * ESP Board : Wemos d1 mini (CH341 uart), esp8266
   - When choosing GPIO pins to use, it's best to avoid GPIO 0, 2 and 15 (D3, D4, D8)
   - SCR : D5 on Zero crossing, D6 on SCR signal
   (- IGBT PWM on D7, TRIAC Dimmer D7-D6)

* Oled Shield 64x48 option _(let use 'define' or 'undef' directive in src/main.cpp code)_
   - I2C wired on D1-SCL D2-SDA

* AC-DC 5V 700mA-Small

## FYI : 
some Linux distrib (Ubuntu 20.x) failed on connect Uart CH340/1 while flashing ESP8266

     →  "Timed out waiting for packet header"
fixed in kernels 5.13.14 and maybe upper 
(https://cdn.kernel.org/pub/linux/kernel/v5.x/ChangeLog-5.13.14)

## Todo :
 IGBT :
    - no human frequency sound
    - reasonable frequency spurious (vs TRIAC)
    - very low resistive on transcient region (_dv/dt_) and can handle more current
      (less overheating )
    - no zero crossing managment
 
 Display Power/Percent/Max on mini screen
