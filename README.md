DEV VERSION MOMENTARILY NOT STABLE

# WiFi-remote-for-Bestway-Lay-Z-SPA (including 2021 year model)
## Hack - ESP8266 as WiFi remote control for Bestway Lay-Z spa Helsinki<br>
### Disclaimer: As mentioned, this is a hack. If anything breaks it breaks and it's your fault.<br>
### Caution - Unplug mains power to the pump before trying to replicate this hack, or you can die! <br><br>

### Features:<br>
-Watch the temperature and status from your browser.<br>
-Custom text on the SPA pump display.<br>
-Custom sound instead of just beeping is possible.<br>
-OTA: Update firmware over the air. Super convenient when mounted inside the pump.<br>
-Simple to build. No hardware changes needed on the SPA pump. Just remove the display, disconnect the 6- or 4-pin ribbon cable and plug it into this device.<br>
-Timer for chlorine. Hit the button on the webpage when adding cl and it will count the days for you.<br>
-Timer for filter change. Pulled from @Bankaifan.<br>
-Electricity cost estimation and more<br>
-MQTT support! I have it installed and working with a Raspberry Pi 4 B with Home Assistant. Thanks to faboaic and 877dev for MQTT support! Now you can control the SPA from Home assistant etc.<br>
-Schedule events like heater on/off at specific dates, with repeat functionality.<br>
-Listen to input signal on one pin and trigger a signal on another pin on desired events. For instance let solar panels turn on/off heater.<br>
<br>
### BOM:<br>
-ESP8266 NodeMCU 1.0 (This version is NOT for ESP32)<br>
-8 channel bidirectional level converter<br>
-6 or 4 pin male header (0.1 in spacing) or better: JST-SM Housing Connector<br>
-6 or 4 pin female header (JST-SM Housing Connector)<br>
see build instructions for more info.

### If you like this project, please consider a donation: <a href="Http://PayPal.me/TLandahl">Buy me a coffee</a><br>
<br>Current web interface<br>
<img src="./pics/spacontrol.png" width="300"> 
<img src="./pics/spaconfig.png" width="300">
<img src="./pics/mqtt.png" width="300"><br>
<br>My device and pump<br>
<img src="./pics/pcb.jpg" width="300">
<img src="./pics/pump.jpg" width="300"><br>
<br>My schematics<br>
<img src="./pics/schematic.png"><br>


### Installation:<br>

Link to my version of the PCB (use with LLC below): <br>
https://oshwlab.com/Visualapproach/bestway-wifi-controller <br>
https://easyeda.com/Visualapproach/bestway-wifi-controller <br>
<img src="./pics/txs0108e.jpg" width="300"><br>
Open the project in editor and download gerber files. Upload them to a PCB factory like JLCPCB.com. <br>
Technical details in <a href="./bwc_docs.xlsx">Documentation</a> <br>
Build instructions and more: <a href="./Build-instructions-Bestway-WiFi-remote.pdf">Instructions</a> <br><br>

Alternative PCB: Eric's PCB (use with LLC below, choose 1x8 ch or 2x4 ch according to the PCB): https://easyeda.com/Naesstrom/lay-z-spa_remote <br>
Replace the resistors with wires. They cause more problems than they solve.<br>
<img src="./pics/LLC_eric.jpg" width="300"><br>


Problems? Read <a href="https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/discussions/46">FAQ</a> and other discussions and current issues
