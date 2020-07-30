From Arduino IDE, upload the sketch via serial and then SPIFFS data. 
How this is done is beyond the scope of this readme.
Settings:
Board: nodemcu esp12e v1.0
80 MHz
Sketch+FS+OTA (I use 2MB fs)

Remove usb serial.
Connect everything according to "pinouts" tab. To reach the 6 pin cable just unscrew the 6 screws around the display.
From now on you can update over the air. There is plenty of 
Instructions online if you don't know how to do it.

At first start up the device will start an access point.
Login to that and type in your credentials (ssid and password)
You then switch to your normal wifi and browse to the ip shown on the pump display.
