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
    void begin(int, int, int, int, int, int, int);
    void loop();
    bool qCommand(uint32_t cmd, uint32_t val, uint32_t xtime, uint32_t interval);
    bool newData();
    void saveEventlog();
    String getJSONStates();
    String getJSONTimes();
    String getJSONSettings();
    void setJSONSettings(String message);
    String getJSONCommandQueue();
    void print(String txt);
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

  private:
    CIO _cio;
    DSP _dsp;
    uint8_t _dspBrightness;
    uint32_t _commandQ[MAXCOMMANDS][4];
    int _qCommandLen = 0;  //length of commandQ
    int32_t _buttonQ[MAXBUTTONS][4];
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
    bool _audio;
    float _cost;
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
};

#endif