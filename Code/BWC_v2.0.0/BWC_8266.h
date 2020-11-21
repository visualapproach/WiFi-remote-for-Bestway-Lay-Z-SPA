#ifndef BWC_8266_H
#define BWC_8266_H

#ifndef ESP8266
#error "This library supports 8266 only"
#endif

#include "Arduino.h"

#ifndef BWC_8266_globals_h
#include "BWC_8266_globals.h"
#endif

#include <ArduinoJson.h>
#include "ESPDateTime.h"
#include <LittleFS.h>
#include <Ticker.h>
#include "pitches.h"

class CIO {

  public:
    void begin(void);
	void loop(void);
    void eopHandler(void);
    void packetHandler(void);
    void clkHandler(void);

    volatile bool newData = false;
	bool dataAvailable = false;
    volatile uint16_t button = 0x1B1B; //no button
    uint8_t payload[11];
	uint8_t states[13];
	uint8_t brightness;
	bool targetIsDisplayed = false;

  private:
    //add anti glitch method
    volatile int _byteCount = 0;
	volatile int _bitCount = 0;
    volatile bool _dataIsOutput = false;
    volatile byte _receivedByte;
    volatile int _CIO_cmd_matches = 0;
    volatile bool _packet = false;
    volatile int _sendBit = 8;
    volatile uint8_t _brightness;
    volatile uint8_t _payload[11];
    const int _CS_PIN = D1;
    const int _CLK_PIN = D2;
    const int _DATA_PIN = D7;
	uint8_t _prevPayload[11];
	
	char _getChar(uint8_t value);
};

class DSP {

  public:
    uint8_t payload[11] = {0xC0, 0xFB, 0xFF, 0xDB, 0xFF, 0xCD, 0xFF, B10001001, 0xFF, 0x01, 0xFF};

    void begin(void);
	uint16_t getButton(void);
    void updateDSP(uint8_t brightness);
    void textOut(String txt);
	void playIntro();
	void beep();

  private:
    void _sendBitsToDSP(uint32_t outBits, int bitsToSend);
    uint16_t _receiveBitsFromDSP();
    char _getCode(char value);
	
	unsigned long _dspLastRefreshTime = 0;
	unsigned long _dspLastGetButton = 0;
	uint16_t _oldButton = ButtonCodes[NOBTN];
	//Pins
	const int _CS_PIN = D3;
	const int _CLK_PIN = D4;
	const int _DATA_PIN = D5;
	const int _AUDIO_PIN = D6;
};

class BWC {

  public:
    void begin(void);
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

  private:
    CIO _cio;
    DSP _dsp;
	uint32_t _commandQ[MAXCOMMANDS][4];
	int _qCommandLen = 0;		//length of commandQ
	uint32_t _buttonQ[MAXBUTTONS][4];
	int _qButtonLen = 0;	//length of buttonQ
	uint32_t _timestamp;
	bool _newData = false;
	uint32_t _cltime;
	uint32_t _uptime;
	uint32_t _pumptime;
	uint32_t _heatingtime;
	uint32_t _airtime;
	uint32_t _uptime_ms;
	uint32_t _pumptime_ms;
	uint32_t _heatingtime_ms;
	uint32_t _airtime_ms;
	int32_t _timezone;
	float _price;
	uint32_t _audio;
	float _cost;
	bool _saveSettingsNeeded = false;
	bool _saveEventlogNeeded = false;
	bool _saveCmdqNeeded = false;
	int _latestTarget;
	int _tickerCount;

	void _qButton(uint32_t btn, uint32_t state, uint32_t value, uint32_t maxduration);
	void _handleCommandQ(void);
	void _handleButtonQ(void);
	void _startNTP();
	void _loadSettings();
	void _loadCommandQueue();
	void _saveCommandQueue();
	void _saveRebootInfo();
	void _updateTimes();
};

#endif
