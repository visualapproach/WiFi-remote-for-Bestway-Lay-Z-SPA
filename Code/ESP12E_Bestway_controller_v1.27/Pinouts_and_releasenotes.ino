/*
 * 
 * patch 2
 * 
 * smashed a bug - autofilter on didn't work in some occasions. It does now.
 * 
 * v1.27
 * Reworked code
 * Switched to LittleFS since SPIFFS is deprecated
 * SPA control page shows stats and ESP can be rebooted
 * New SPA config page
 * Selectable auto events
 * Audio on/off
 * 
 * Config: filter on/off turns the filter on and off. It remembers last state when turning on.
 * Allowed heater hours: Intended to protect main fuses to pop when you come home from work and turn on every gadget you own...
 * When auto mode is on the heater is turned off on theese hours, and you will not be able to turn it on manually.
 * 
 * In absense of mr Frankenstein, I took the artistic freedom to add my logo/avatar ;-). I bet you will replace it.
 * 
 * v1.26
 * Bug fixes:
 * Turning off heater from webpage did not work
 * Target temp did not update when using real buttons on the pump
 * 
 * v1.25
 * filter out glitches from CIO (cleaner logs)
 * 
 * v1.24
 * hardcoded filteroff/on hours (to be adjustable via wifi later on)
 * 
 * v1.23
 * Chlorine timer button added
 * Update heatingtime before logsave
 * Calculates ratio heatingtime/uptime
 * 

  As seen when looking into the connectors
  Also see pictures in /pics on github page
  Display       Pump Computer ("CIO")
  |0|     1     |.|
  |0|     2     |.|
  |0 |    3    | .|
  |0 |    4    | .|
  |0|     5     |.|
  |0|     6     |.|

   
   PINOUT DSP & CIO:
   1 - +5V
   2 - GND
   3 - DATA
   4 - CLK
   5 - SS/CS
   6 - AUDIO

   connections
   ******YOU MUST USE A LEVEL CONVERTER BETWEEN ESP AND DSP/CIO*******
   
   ESP12E PIN                       CENTRAL UNIT (CIO) (starting from 5V-pin = 1)
   VIN                              1 (+5V connected to DSP 1)
   GND                              2 (GND)
   D7(GPIO13)(via 1K Ohms)  -LLC-   3 (Data)
   D2(GPIO4) (via 1K Ohms)  -LLC-   4 (Clock)
   D1(GPIO5) (via 1K Ohms)  -LLC-   5 (Chip select)
   --        (via 1K Ohms)  -LLC-   6 (Audio)


   ESP12E PIN               DISPLAY UNIT (DSP) (starting from 5V-pin = 1)
   VIN                              1 (+5V)
   GND                              2 (GND)
   D5(GPIO14)(via 1K Ohms)  -LLC-   3 (Data)
   D4(GPIO2)(via 1K Ohms)   -LLC-   4 (Clock)
   D3(GPIO0)(via 1K Ohms)   -LLC-   5 (Chip select)
   D6(GPIO12)(via 1K Ohms)  -LLC-   6 (Audio)

*/
