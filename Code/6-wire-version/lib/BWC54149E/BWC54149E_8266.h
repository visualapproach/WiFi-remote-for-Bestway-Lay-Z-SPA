#ifndef BWC54149E_8266_H
#define BWC54149E_8266_H

#ifndef ESP8266
#error "This library supports 8266 only"
#endif

#include "Arduino.h"

#ifndef BWC54149E_8266_globals_h
#include "BWC54149E_8266_globals.h"
#endif

//long long needed in arduino core v3+
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include "ESPDateTime.h"
#include <LittleFS.h>
#include <Ticker.h>
#include "pitches.h"



class CIO {

  public:
    void begin(int cio_cs_pin, int cio_data_pin, int cio_clk_pin);
	void loop(void);
    void eopHandler(void);
    void LEDdataHandler(void);
    void clkHandler(void);
	void stop(void);

    volatile bool newData = false;
	bool dataAvailable = false;
	bool stateChanged = false; //save states when true
    volatile uint16_t button = NOBTN;
    uint8_t payload[5];
	uint8_t states[14];
	uint8_t brightness;
	bool targetIsDisplayed = false;
	//uint16_t lastPressedButton;


  private:
    volatile int _byteCount = 0;
	volatile int _bitCount = 0;
    volatile byte _receivedByte;
    volatile bool _packet = false;
    volatile int _sendBit = 8;
    volatile uint8_t _brightness;
    volatile uint8_t _payload[5];
    int _CIO_TD_PIN;
    int _CIO_CLK_PIN;
    int _CIO_LD_PIN;
	uint8_t _prevPayload[5];
	bool _prevUNT;
	bool _prevHTR;
	bool _prevFLT;
	uint8_t _received_cmd = 0;	//temporary storage of command message

	char _getChar(uint8_t value);
};

class DSP {

  public:
    uint8_t payload[5];

    void begin(int dsp_cs_pin, int dsp_data_pin, int dsp_clk_pin, int dsp_audio_pin);
	uint16_t getButton(void);
    void updateDSP(uint8_t brightness);
    void textOut(String txt);
	void LEDshow();
	void playIntro();
	void beep();
	void beep2();

  private:
    void _sendBitsToDSP(uint32_t outBits, int bitsToSend);
    uint16_t _receiveBitsFromDSP();
    char _getCode(char value);
	
	unsigned long _dspLastRefreshTime = 0;
	unsigned long _dspLastGetButton = 0;
	uint16_t _oldButton = ButtonCodes[NOBTN];
	uint16_t _prevButton = ButtonCodes[NOBTN];
	//Pins
	int _DSP_LD_PIN;
	int _DSP_TD_PIN;
	int _DSP_CLK_PIN;
	int _DSP_AUDIO_PIN;
};

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

  private:
    CIO _cio;
    DSP _dsp;
	uint8_t _dspBrightness;
	uint32_t _commandQ[MAXCOMMANDS][4];
	int _qCommandLen = 0;		//length of commandQ
	int32_t _buttonQ[MAXBUTTONS][4];
	int _qButtonLen = 0;	//length of buttonQ
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
	int _latestTarget;
	int _tickerCount;
	bool _sliderPrio = true;
	uint32_t _tttt_time0;	//time at previous temperature change
	uint32_t _tttt_time1;	//time at last temperature change
	int _tttt_temp0;		//temp after previous change
	int _tttt_temp1;		//temp after last change
	int _tttt;				//time to target temperature after subtracting running time since last calculation
	int _tttt_calculated;	//constant between calculations

	void _qButton(uint32_t btn, uint32_t state, uint32_t value, uint32_t maxduration);
	void _handleCommandQ(void);
	void _handleButtonQ(void);
	void _startNTP();
	void _loadSettings();
	void _loadCommandQueue();
	void _saveCommandQueue();
	void _saveRebootInfo();
	void _updateTimes();
	void _restoreStates();
	void _saveStates();
	int _CodeToButton(uint16_t val);
};

#endif
