# WiFi-remote-for-Bestway-Lay-Z-SPA
## Hack - ESP8266 as WiFi remote control for Bestway Lay-Z spa Helsinki<br>
### Disclaimer: As mentioned, this is a hack. If anything breaks it breaks and it's your fault.<br>
### Caution - unplug mains power to the pump before trying to replicate this hack, or you can die! <br>
<br>

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
-Electricity cost estimation and more<br>
-MQTT support! I have it installed and working with a Raspberry Pi 4 B with Docker/Mosquitto/grafana/influxdb. Thanks to faboaic and 877dev for MQTT support.<br>
-Schedule events like heater on/off at specific dates, with repeat functionality.<br>
-Listen to input signal on one pin and trigger a signal on another pin on desired events.<br>

<br><br>


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
Link to Eric's PCB: https://easyeda.com/Naesstrom/lay-z-spa_remote <br>

Technical details in https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/blob/master/github-bestwayhackdocs_.xlsx

Usage: 

/*

  Bestway Spa Controller Library by Thomas Landahl (Visualapproach)

  https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA
   
  Installation and first run:
  
  Copy files to documents/Arduino/Libraries/
  Start Arduino IDE and open File/examples/BWC_v2.0.0/[choose one]
  Select the right board, and set "FS 2MB/OTA 1MB", speed 80 MHz.

  Compile and upload sketch. 
  **Upload LittleFS files.** 
    
  An Access Point is created called "Auto portal". Log in and enter wifi credentials.

  Visit IP/ and click "Go to config page"
  Enter your settings, click SAVE.
   
  This version is using a command queue with repeatable commands. 
  11 commands can be queued, of which 10 should be used by the user. 
  Please leave 1 for internal use.
  The library will unlock the device automatically if necessary.
  Just send what you want to happen when, and the library will take care of it.
  You can even directly set desired target temperature!!
  
 LittleFS files saved from this program:
 
 settings.txt   - stores JSON formatted running times and configurations. Updates every 2 hrs.
 cmdq.txt       - stores JSON formatted command queue.
 eventlog.txt   - stores JSON formatted states and a timestamp (Mostly for debugging. Not human friendly as is)
 bootlog.txt    - stores JSON formatted boot times and reasons.

 These files can be read by visiting the address "IP/filename.txt"
 You can if you want edit the file and upload it by visiting "IP/upload.html"
 
 To remove the files you need to re-upload LittleFS from Arduino IDE. Or write a function to do so :-)

  
-------------------Info to coders below--------------------  

  Public functions in BWC class:

  void begin(void);       //Must be run once in setup.
  void loop(void);        //As always, don't use blocking code. This needs to be run fairly frequently.
  void print(String txt); //Send text to display. Not all characters can be shown on a 7 segment display and will be replaced by spaces.
  bool qCommand(uint32_t cmd, uint32_t val, uint32_t xtime, uint32_t interval);  


  bool newData();                         //true when a state has changed
  void saveEventlog();                    //save states to LittleFS (flash) (I don't recommend using this unless you are really a log freak)
  String getJSONStates();                 //get a JSON formatted string to send to clients
  String getJSONTimes();                  //get a JSON formattes string to send to clients
  String getJSONSettings();               //get a JSON formattes string to send to clients (internal use)
  void setJSONSettings(String message);   //save settings to flash. Must be formatted correctly. (internal use)
  String getJSONCommandQueue();           //get a JSON formatted string to send to clients

States index constants, for use in bwc.getState(int state):
  0  LOCKEDSTATE
  1  POWERSTATE
  2  UNITSTATE
  3  BUBBLESSTATE
  4  HEATGRNSTATE
  5  HEATREDSTATE
  6  HEATSTATE
  7  PUMPSTATE
  8  TARGET
  9  TEMPERATURE
  10 CHAR1
  11 CHAR2
  12 CHAR3

  These are defined in the library so you can call bwc.getState(TEMPERATURE) for instance.
 
  Commands index constants, for use in bwc.qCommand(uint32_t cmd, uint32_t val, uint32_t xtime, uint32_t interval)
  (Sending commands from web pages or MQTT)
  cmd:
  0 SETTARGET
  1 SETUNIT
  2 SETBUBBLES
  3 SETHEATER
  4 SETPUMP
  5 RESETQ
  6 REBOOTESP
  7 GETTARGET
  8 RESETTIMES
  9 RESETCLTIMER
    ...may be extended
	
  val:
    1 = ON/Fahrenheit, 0 = OFF/Celsius, or any value for SETTARGET temperature
  xtime: eXecute at timestamp (unix timestamp in seconds)
  interval: repeat every INTERVAL seconds. 0 = no repeat.

 These are defined in the library so you can call bwc.qCommand(SETPUMP, 1, 1603321200, 3600) to
 turn on filter pump at 10/21/2020 @ 11:00pm, repeating every hour (3600s)
    
 The library is running an NTP and LittleFS so no need to include those here

*/

