/*
  As seen when looking into the connectors
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
