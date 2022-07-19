// Uncomment your model and comment out the rest

//#define MODEL54149E   //Paris airjet 54149E
#define PRE2021  //the older one, no hydrojets
//#define MIAMI2021  //no hydrojets
//#define MALDIVES2021  //hydrojets

//If using/testing the new PCB choose PCB_V2
//#define PCB_V1
#define PCB_V2  //The PCB with rounded corners
/*
There is no PCB_V2B yet. Use PCB_V2 (easiest) and connect wires like this:
CIO_DATA (wire #3) : D1 (port H1A)
CIO_CLK (wire #4)  : D2 (port H1A)
CIO_CS (wire #5)   : D5 (port H2A)

DSP_DATA (wire #3) : D6 (port H2A)
DSP_CLK (wire #4)  : D4 (port H2A)
DSP_CS (wire #5)   : D3 (port H1A)

author: @SigmaPic
*/
//#define PCB_V2B  
#warning "USING PINOUT FOR PCB V2 (PCB WITH ROUND CORNERS). EDIT lib/BWC/model.h IF USING OTHER PCB!"