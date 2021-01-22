# WiFi-remote-for-Bestway-Lay-Z-SPA
## Hack - ESP8266 as WiFi remote control for Bestway Lay-Z spa Helsinki<br>
### Disclaimer: As mentioned, this is a hack. If anything breaks it breaks and it's your fault.<br>
### Caution - unplug mains power to the pump before trying to replicate this hack, or you can die! <br>
<br>
### News: A four wire version uploaded! Only tested with the pump inside the house. No pool or water connected yet. More info in the CODE/BW4W folder.
<br><br>
Code/BWC_v2.0.0 is a *LIBRARY*. Copy the whole folder "BWC_v2.0.0" to ...arduino/Libraries/ Then open IDE and pick FILE/EXAMPLES/BWC_v2.0.0/...ino
Follow instructions in the code. <br>

### Features:<br>
-Watch the temperature and status from your browser.<br>
-Custom text on the SPA pump display.<br>
-Custom sound instead of just beeping. Plays a melody when starting etc.<br>
-Optional: Basic logging to LittleFS (on the ESP8266).<br>
-OTA: Update firmware over the air. Super convenient when mounted inside the pump.<br>
-Simple to build. No hardware changes needed on the SPA pump. Just remove the display, disconnect the 6-pin ribbon cable and plug it into this device.<br>
-Timer for chlorine. Hit the button on the webpage when adding cl and it will count the days for you.<br>
-Timer for filter change. Pulled from @Bankaifan.<br>
-Electricity cost estimation and more<br>
-MQTT support! I have it installed and working with a Raspberry Pi 4 B with Docker/Mosquitto/grafana/influxdb. Thanks to faboaic and 877dev for MQTT support.<br>
-Schedule events like heater on/off at specific dates, with repeat functionality.<br>
-Listen to input signal on one pin and trigger a signal on another pin on desired events. For instance let solar panels turn on/off heater.<br>
<br>
### BOM:<br>
-ESP8266 NodeMCU 1.0 (This version is NOT for ESP32)<br>
-8 channel bidirectional level converter<br>
-6 pin male header (0.1 in spacing) or better: JST-SM Housing Connector<br>
-6 pin female header (JST-SM Housing Connector)<br>
-Optionally 7 x 500 to 820 Ohm resistors (lower than 500 would not protect against over current which may or may not occur if both sides drives the line in opposite directions. That is not suppose to happen but just in case. So, if you only have say 430 Ohms resistors you may as well skip them. Higher than 820 Ohms can make it not work.)<br>

### If you like this project, please consider a donation: <a href="Http://PayPal.me/TLandahl">PayPal.me/TLandahl</a><br>
<br>Current web interface<br>
<img src="./pics/spacontrol.png" width="300"> 
<img src="./pics/spaconfig.png" width="300"><br>
<br>My device and pump<br>
<img src="./pics/device.jpg" width="300">
<img src="./pics/pump.jpg" width="300"><br>
<br>My schematics<br>
<img src="./pics/schematic2.png"><br>

Link to Eric's PCB: https://easyeda.com/Naesstrom/lay-z-spa_remote 
Technical details in https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/blob/master/bwc_docs.xlsx

### Installation:<br>
Using Arduino IDE<br>
Copy BWC_2.0.0 to arduino libraries/ folder<br>

Download and install these Libraries<br>
Basic sketch:<br>
ArduinoJSON (Benoit Blanchon)<br>
ESPDateTime https://github.com/mcxiaoke/ESPDateTime<br>
WebSockets https://github.com/Links2004/arduinoWebSockets<br>
WiFiManager https://github.com/tzapu/WiFiManager<br>
<br>
MQTT sketch (in addition to basic):<br>
PubSubClient https://github.com/knolleary/pubsubclient
<br>
LittleFS install:<br>
https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system<br>
Link to LittleFS upload tool: https://github.com/esp8266/arduino-esp8266fs-plugin/releases/<br>

Open File/examples/BWC2.0.0 from the arduino IDE. 

Select the right board, and set "FS 2MB/OTA 1MB", speed 80 MHz.
Upload LittleFS files.
Upload sketch.
An Access Point is created called "Auto portal". Log in and enter wifi credentials.
Visit IP/ and click "Go to config page"
Enter your settings, click SAVE.

Problems? Read FAQ in discussions and current issues
