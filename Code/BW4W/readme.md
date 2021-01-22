READ EVERYTHING BEFORE BUILDING!
ONLY FOR #54138 (maybe #54123 with modification to "model.h" but that is not tested and it has different pinout.)

Hardware: NodeMCU ESP12E and a level converter. Prototype board/PCB and cables. <br>
Pinout: <br>
  Black(+5 V)<br>
  Red (GND)<br>
  Yellow (DSP TX / CIO RX)<br>
  Green (DSP RX / CIO TX)<br>
You MUST double check your hardware before connecting anything!<br>
<br>
From CIO:<br>
5 V to ESP Vin, Level converter HV, and DSP 5V pin.<br>
GND to ESP GND, Level converter GND, and DSP GND.<br>
CIO RX (yellow) to D3<br>
CIO TX (green) to D2<br>
<br>
From DSP:<br>
5V see above<br>
GND see above<br>
DSP RX (green) to D7<br>
DSP TX (yellow) to D6<br>
<br>
Other:<br>
ESP 3V3 to LLC LV<br>
<br>
This SPA model is having a more "intelligent" display that sends commands to the pump unit. Therefore we cannot simulate button presses. Instead we take over control of the pump which
can potentially be risky. It appears the pump has some safety features, but I cannot guarantee that the program covers all such features that the display might have. I have tried to incorporate 
safety, but I am not responsible for what happens if you use this.
If the CIO send error messages the ESP stops acting like a controller and passes control to the display. No guarantee though since it can hang or break otherwise.
Don't leave the device unsupervised until you are confident that everything works. And even then, DO NOT BLAME ME!

Arduino users: Sorry, but I wrote this in platformIO on Visual Studio Code. If you desperately needs an arduino IDE version it should be relatively easy to convert.
MQTT is commented out in the code. To enable you need to uncomment and set your credentials in 4_wire.h
