
THIS TEXT IS OLD AND NEED TO BE UPDATED!!
  
  
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
