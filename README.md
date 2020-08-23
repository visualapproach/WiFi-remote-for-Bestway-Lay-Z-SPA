# WiFi-remote-for-Bestway-Lay-Z-SPA
## Hack - ESP8266 as WiFi remote control for Bestway Lay-Z spa Helsinki<br>
### Disclaimer: As mentioned, this is a hack. If anything breaks it breaks and it's your fault.<br>
### Caution - unplug mains power to the pump before trying to replicate this hack, or you can die! <br>
<br>
Features:<br>
-Watch the temperature and status from your browser.<br>
-Custom text on the SPA pump display.<br>
-Custom sound instead of just beeping. Plays a melody when starting etc.<br>
-Basic logging to SPIFFS (on the ESP8266). See accumulated heating time etc.<br>
-Update firmware over the air<br>
-Simple to build. No hardware changes needed on the SPA pump. Just remove the display, disconnect the 6-pin ribbon cable and plug it into this device.<br>
-Timer for clorine<br>
-Heater on % of time<br>
<br>
BOM:<br>
-ESP8266 NodeMCU 1.0 (This version is NOT for ESP32)<br>
-8 channel bidirectional level converter<br>
-6 pin male header (0.1 in spacing)<br>
-6 pin female header<br>
-Optionally 7 x 1KOhm resistors<br><br>

If you like this project, please consider a donation: <a href="Http://PayPal.me/TLandahl">PayPal.me/TLandahl</a><br>
<br>Current web interface<br>
<img src="./spacontrol2.jpg"><br>
<br>My device<br>
<img src="./device.jpg"><br>
<br>My pump<br>
<img src="./pump.jpg"><br>
<br>My schematics<br>
<img src="./schematic2.png"><br>

Technical details in https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/blob/master/github-bestwayhackdocs_.xlsx

Usage:
 * To remove the log file, go to webadress IP/remove.html
 * To look at the log file, go to webadress IP/eventlog.csv
 * To upload a file to the filesystem, go to webadress IP/upload.html
 * "IP" is the local IP of the ESP.
 * 
 * The main web page "SPA control" should be easy to understand since it mimics the pump display panel. Sort of.
 * The slider to set target temperature is not implemented yet, so it's basically just a monitor.
 * Use the UP/DOWN buttons to change the temp.
 * 
 * The "reset timer" is meant to be pressed when new chlorine is added. That way you know when it's time to do it again.
 * 
 * To force AP-mode, press "C/F" and then POWER on the pump. I thought this key combination would be unusual enough to
 * not trigger AP mode accidentally. Not that it hurts anything, but it might be annoying, and leaves an open wifi for a short time.
 <br>
 You may want to edit some values in "a_globals.h":<br>
 //********USER PARAMETERS*************<br>
 const char *OTAName = "BW-1.26";           // A name and a password for the OTA service<br>
 const char *OTAPassword = "esp8266";       // Not used!<br>
 const char* mdnsName = "spa";              // Not used! Domain name for the mDNS responder<br>
 int filterOffHour = 3;                     //hardcoded for the moment. Stops the pump at 03:00 (Set to 24 or higher to disable auto off)<br>
 int filterOnHour = 9;                       //set to 24 or higher to disable auto on<br>
 int TZ = 2;                                //timezone (UTC + TZ = your local time) DST seems to not be implemented in the ESPDateTime.h library<br>
 bool forbiddenHeaterHours[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //Not fully used. Not tested. <br>

