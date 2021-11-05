# **this project is on going...**

# Wifi_Mqtt_Power_Variator
 * Adjust 'Power Consumption' of a resistive load
 * Power command received trought MQTT topic
 * Publish power values on MQTT 'vload topic' and on 'domoticz/in' topic (for Domoticz)
 * Display Power/Percent/Max on mini screen    

 * Wifi Access Point WebServer and set custom parameters
 * WebOTA : On The Air firmware update (url http://<vload_ip>:8080/webota - 60s after boot)



## Hardware requirements:   ~10 €
 * IGBT Power Variator  (homebrew, is coming...)
    - no human frequency sound
    - reasonable frequency spurious (vs TRIAC)
    - very low resistive on transcient region and can handle more current
    - no zero crossing managment
    **→** others like Mosfet, Triac ... can be used but the code need to be implemented anymore

 * ESP Board : Wemos d1 mini (CH341 uart), esp8266
   - SoftSerial Method used on D5 D6 
   - When choosing GPIO pins to use, it's best to avoid GPIO 0, 2 and 15 (D3, D4, D8)

* Oled Shield 64x48 
   - I2C wired on D1-SCL D2-SDA

* AC-DC 5V 700mA-Small