#ifndef BWC_common_H
#define BWC_common_H

#ifndef ESP8266
#error "This library supports 8266 only"
#endif

#include "Arduino.h"
//long long needed in arduino core v3+
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include "ESPDateTime.h"
#include <LittleFS.h>
#include <Ticker.h>
#include "pitches.h"
#include "model.h"
#include "BWC_const.h"

#if defined(MODEL54149E)
#include "BWC_6w_type2.h"
#else
#include "BWC_6w_type1.h"
#endif

class BWC {

  public:
    BWC();
    void begin(void);
    void begin2();
    void loop();
    bool qCommand(int64_t cmd, int64_t val, int64_t xtime, int64_t interval);
    bool newData();
    void saveEventlog();
    String getJSONStates();
    String getJSONTimes();
    String getJSONSettings();
    void setJSONSettings(const String& message);
    String getJSONCommandQueue();
    void print(const String& txt);
    uint8_t getState(int state);
    Ticker saveSettingsTimer;
    void saveSettingsFlag();
    void saveSettings();
    bool maxeffort = false;
    String getPressedButton();
    void reloadCommandQueue();
    void reloadSettings();
    String getButtonName();
    void saveDebugInfo(String s);
    void stop(void);
    void saveRebootInfo();
    bool getBtnSeqMatch();
    void setAmbientTemperature(int64_t amb, bool unit);
    void unlock(void);
    bool notify;
    int notification_time, next_notification_time;
    String reboottime;

  private:
    CIO _cio;
    DSP _dsp;
    uint8_t _dspBrightness;
    int16_t override_dsp_brt_timer;
    int64_t _commandQ[MAXCOMMANDS][4]; //64 bits to house both timestamp > 15 years in future, and negative values
    int _qCommandLen = 0;  //length of commandQ
    int32_t _buttonQ[MAXBUTTONS][4];
    int _buttonQLen = 0;  //length of buttonQ
    uint32_t _timestamp; // seconds
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
    bool _audio;
    float _energyTotal;
    double _energyDaily; //Wattseconds internally
    int _energyPower;
    bool _restoreStatesOnStart = false;
    bool _saveSettingsNeeded = false;
    bool _saveEventlogNeeded = false;
    bool _saveCmdqNeeded = false;
    bool _saveStatesNeeded = false;
    int _sliderTarget;
    int _tickerCount;
    bool _sliderPrio = true;
    uint32_t _tttt_time0;  //time at previous temperature change
    uint32_t _tttt_time1;  //time at last temperature change
    int _tttt_temp0;  //temp after previous change
    int _tttt_temp1;  //temp after last change
    int _tttt;  //time to target temperature after subtracting running time since last calculation
    int _tttt_calculated;  //constant between calculations
    int _btnSequence[4] = {NOBTN,NOBTN,NOBTN,NOBTN}; //keep track of the four latest button presses
    void _qButton(uint32_t btn, uint32_t state, uint32_t value, int32_t maxduration);
    void _handleCommandQ(void);
    void _handleButtonQ(void);
    void _startNTP();
    void _loadSettings();
    void _loadCommandQueue();
    void _saveCommandQueue();
    void _updateTimes();
    void _restoreStates();
    void _saveStates();
    int _CodeToButton(uint16_t val);
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
    float _C2F(float c);
    float _F2C(float f);
    bool _newDataToSend = false;
    void _handleNotification();
};

#endif