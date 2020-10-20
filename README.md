# WiFi-remote-for-Bestway-Lay-Z-SPA
## Hack - ESP8266 as WiFi remote control for Bestway Lay-Z spa Helsinki<br>
### Disclaimer: As mentioned, this is a hack. If anything breaks it breaks and it's your fault.<br>
### Caution - unplug mains power to the pump before trying to replicate this hack, or you can die! <br>
<br>

Code/BWC_v2.0.0 is a LIBRARY. Copy the whole folder "BWC_v2.0.0" to ...arduino/Libraries/ Then open IDE and pick FILE/EXAMPLES/BWC_v2.0.0/...ino
Follow instructions in the code. <br>

### Features:<br>
-Watch the temperature and status from your browser.<br>
-Custom text on the SPA pump display.<br>
-Custom sound instead of just beeping. Plays a melody when starting etc.<br>
-Optional: Basic logging to LittleFS (on the ESP8266).<br>
-OTA: Update firmware over the air. Super convenient when mounted inside the pump.<br>
-Simple to build. No hardware changes needed on the SPA pump. Just remove the display, disconnect the 6-pin ribbon cable and plug it into this device.<br>
-Timer for chlorine. Hit the button on the webpage when adding cl and it will count the days for you.<br>
-Electricity cost estimation and more<br>
-MQTT support! I have it installed and working with a Raspberry Pi 4 B with Docker/Mosquitto/grafana/influxdb. Thanks to faboaic and 877dev for MQTT support.
-Schedule events like heater on/off at specific dates, with repeat functionality.
-Listen to input signal on one pin and trigger a signal on another pin on desired events.

<br><br>

### BOM:<br>
-ESP8266 NodeMCU 1.0 (This version is NOT for ESP32)<br>
-8 channel bidirectional level converter<br>
-6 pin male header (0.1 in spacing) or better: JST-SM Housing Connector<br>
-6 pin female header (JST-SM Housing Connector)<br>
-Optionally 7 x 500 to 820 Ohm resistors (lower than 500 would not protect against over current which may or may not occur if both sides drives the line in opposite directions. That is not suppose to happen but just in case. So, if you only have say 430 Ohms resistors you may as well skip them. Higher than 820 Ohms can make it not work.)<br>


### If you like this project, please consider a donation: <a href="Http://PayPal.me/TLandahl">PayPal.me/TLandahl</a><br>
<br>Current web interface<br>
<img src="./pics/spacontrol.png" width="300"><br>
<img src="./pics/spaconfig.png" width="300"><br>
<br>My device<br>
<img src="./pics/device.jpg" width="300"><br>
<br>My pump<br>
<img src="./pics/pump.jpg" width="300"><br>
<br>My schematics<br>
<img src="./pics/schematic2.png"><br>
Link to Eric's PCB: https://easyeda.com/Naesstrom/lay-z-spa_remote <br>

Technical details in https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/blob/master/github-bestwayhackdocs_.xlsx

Usage: 

see .ino file

