//this version is for Bestway pump model #54138 with 4-wire connection between pump and display

#ifndef BWC_8266_4W_H
#define BWC_8266_4W_H

#ifndef ESP8266
#error "This library supports 8266 only"
#endif

#include "Arduino.h"

#ifndef BWC_8266_4W_GLOBALS_h
#include "BWC_8266_4w_globals.h"
#endif

#include <ArduinoJson.h>
#include <ESPDateTime.h>
#include <LittleFS.h>
#include <Ticker.h>
#include <SoftwareSerial.h>

#ifdef PCB_V2
const int CIO_RX = D1;
const int CIO_TX = D2;
const int DSP_TX = D4;
const int DSP_RX = D5;
#else
const int CIO_RX = D3;
const int CIO_TX = D2;
const int DSP_TX = D6;
const int DSP_RX = D7;
#endif

class CIO {

  public:
    void begin();
    void loop(void);
    void updatePayload();
    void updateStates();
    void trackStateChanges();
    bool dataAvailable = false;
    bool GODMODE = false;
    uint8_t states[15], oldStates[15];
    uint32_t state_age[15];
    bool state_changed[15];
    int deltaTemp;
    SoftwareSerial dsp_serial;
    SoftwareSerial cio_serial;

    uint8_t from_CIO_buf[7];  //CIO to ESP. We will copy it straight to display, and getting the temperature
    uint8_t to_DSP_buf[7];    //ESP to DSP
    uint8_t from_DSP_buf[7];  //DSP to ESP. We can ignore this message and send our own when ESP is in charge.
    uint8_t to_CIO_buf[7];    //Otherwise copy here. Buffer to send from ESP to CIO
    bool cio_tx_ok;              //set to true when data received. Send to webinterface+serial for debugging
    bool dsp_tx_ok;              //set to true when data received. Send to webinterface+serial for debugging

    uint8_t heatbitmask;

    /* Debug */
    uint8_t dismissed_from_CIO_buf[7];  //CIO to ESP. We will copy it straight to display, and getting the temperature
    uint8_t dismissed_from_DSP_buf[7];  //DSP to ESP. We can ignore this message and send our own when ESP is in charge.
    uint8_t dismissed_cio_len;
    uint8_t dismissed_dsp_len;
    uint32_t badCIO_checksum, badDSP_checksum;

  private:

};

class BWC {

  public:
    void begin(void);
    void loop();
    bool qCommand(int64_t cmd, int64_t val, int64_t xtime, int64_t interval);
    bool newData();
    void saveEventlog();
    String getJSONStates();
    String getJSONTimes();
    String getJSONSettings();
    void setJSONSettings(String message);
    String getJSONCommandQueue();
    void print(String txt);
    uint8_t getState(int state);
    void saveSettingsFlag();
    void saveSettings();
    Ticker saveSettingsTimer;
    bool cio_tx_ok;
    bool dsp_tx_ok;
    void reloadCommandQueue();
    String encodeBufferToString(uint8_t buf[7]);
    String getSerialBuffers();
    void setAmbientTemperature(int64_t amb, bool unit);

  private:
    CIO _cio;
    int64_t _commandQ[MAXCOMMANDS][4];
    int _qCommandLen = 0;    //length of commandQ
    uint32_t _buttonQ[MAXBUTTONS][4];
    int _qButtonLen = 0;  //length of buttonQ
    uint32_t _timestamp;
    bool _newData = false;
    uint32_t _cltime;
    uint32_t _ftime;
    uint32_t _uptime;
    uint32_t _pumptime;
    uint32_t _heatingtime;
    uint32_t _airtime;
    uint32_t _jettime;
    uint32_t _uptime_ms;
    uint32_t _pumptime_ms;
    uint32_t _heatingtime_ms;
    uint32_t _airtime_ms;
    uint32_t _jettime_ms;
    int32_t _timezone;
    float _price;
    uint32_t _finterval;
    uint32_t _clinterval;
    uint32_t _audio;
    float _energyTotal;
    float _energyDaily;
    int _energyPower;
    float _cost;
    float _kwh;
    bool _saveSettingsNeeded = false;
    bool _saveEventlogNeeded = false;
    bool _saveCmdqNeeded = false;
    int _latestTarget;
    int _tickerCount;
    bool _sliderPrio = true;
    uint8_t _currentStateIndex = 0;
    uint32_t _tttt_time0;  //time at previous temperature change
    uint32_t _tttt_time1;  //time at last temperature change
    int _tttt_temp0;    //temp after previous change
    int _tttt_temp1;    //temp after last change
    int _tttt;        //time to target temperature after subtracting running time since last calculation
    int _tttt_calculated;  //constant between calculations
    //vt stuff
    float _estHeatingTime();
    float R_COOLING = 20;
    int _ambient_temp; //always in C internally
    float _heatingDegPerHour = 1.5; //always in C internally
    float _virtualTemp; //=virtualtempfix+calculated diff, always in C internally
    float _virtualTempFix; //last fixed data point to add or subtract temp from, always in C internally
    uint32_t _virtualTempFix_age;
    void _calcVirtualTemp();
    void _updateVirtualTempFix_ontempchange();
    void _updateVirtualTempFix_onheaterchange();
    void _handleStateChanges();
    //end vt
    float _C2F(float c);
    float _F2C(float f);

    void _qButton(uint32_t btn, uint32_t state, uint32_t value, uint32_t maxduration);
    void _handleCommandQ(void);
    void _handleButtonQ(void);
    void _startNTP();
    void _loadSettings();
    void _loadCommandQueue();
    void _saveCommandQueue();
    void _saveRebootInfo();
    void _updateTimes();
    void _regulateTemp();
    void _antifreeze();
    void _antiboil();
};

#endif
