FIXED: BROKEN UPDATE FUNCTIONS
If version fields come up empty you need to update the old fashioned way through platformio. After that the UI will likely work again. Updating should also be more stable from now and not leaving zero sized files if failed.

WiFi-remote-for-Bestway-Lay-Z-SPA
=================================
ESP8266 hack to use as WiFi remote control for Bestway Lay-Z-Spa Whirlpools (including 2021 year models) <br>
Latest code found in [Development branch](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/tree/development_v4)<br>
Build instructions and more: [Read the manual](bwc-manual.pdf)

- [Features](#features)
- [BOM](#bom)
- [Web Interface](#web-interface)
- [WiFi Module / Pump](#wifi-module--pump)
- [Schematics](#schematics)
- [Installation](#installation)
- [Installation (Alternative)](#installation-alternative)
- [Problems?](#problems)

---

> #### Disclaimer
> As mentioned, this is a hack. If anything breaks it is your fault.

> #### Caution
> Pull out the mains plug before modifying hardware, or you can die!

> #### Donate
> If you like this project, please consider a donation. [Buy me a coffee](https://paypal.me/TLandahl), thanks!

#### Features
- Control buttons, watch the temperature and get current states from your browser.
- Custom text on the SPA pump display.
- Custom sound instead of just beeping is possible.
- OTA: Update firmware "over the air". Super convenient when mounted inside the pump.
- Simple to build. No hardware changes needed on the SPA pump. Just remove the display, disconnect the 6- or 4-pin ribbon cable and plug it into this device.
- Timer for chlorine addition and filter change. Hit the button on the web interface and it will count the days for you. (@Bankaifan)
- Electricity cost estimation and more..
- MQTT support! Now you can control the SPA from Home Assistant, OpenHab etc. (@faboaic, @877dev)
- Schedule events like heater on/off at specific dates, with repeat functionality.
- Listen to input signal on one pin and trigger a signal on another pin on desired events. For instance let solar panels turn on/off heater.

#### BOM
- ESP8266 NodeMCU 1.0 **(NOT for ESP32)**
- 8 channel bidirectional level converter
- 6 or 4 pin male header (0.1 in spacing) or better: JST-SM Housing Connector
- 6 or 4 pin female header (JST-SM Housing Connector)
see build instructions for more info.

#### Web Interface
<img src="./pics/web01_overview.png" width="300"><br />
<img src="./pics/web02_menu.png" width="300"><br />
<img src="./pics/web03_spa-config.png" width="300"><br />
<img src="./pics/web04_network-config.png" width="300"><br />
<img src="./pics/web05_mqtt-config.png" width="300">

#### WiFi Module / Pump
<img src="./pics/pcb.jpg" width="300"><br />
<img src="./pics/pump.jpg" width="300">

#### Schematics
It's in this project [PCB_V2B](https://oshwlab.com/visualapproach/bestway-wireless-controller-2_copy)
Open the PCB tab and go to menu Fabrication, Gerber files. Order the PCB_V2B.

#### Installation
Build instructions and more: [Instructions](bwc-manual.pdf)
Technical details in the [Documentation](bwc_docs.xlsx).

@misterpeee's wife made and shared this case for 3d printing https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/discussions/265#discussion-4062382 but it's for the PCB_V1 which is deprecated. Latest PCB is PCB_V2B.

#### Problems?
Read the [FAQ](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/discussions/46), other [discussions](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/discussions) and current [issues](https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/issues).
