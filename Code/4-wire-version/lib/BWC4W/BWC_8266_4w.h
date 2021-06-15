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
#include "SoftwareSerial.h"

class CIO {

  public:
    void begin();
	void loop(void);
	void updatePayload();
	void updateStates();

    bool newData = false;
	bool dataAvailable = false;
	bool GODMODE = false;
	uint8_t states[15];
	SoftwareSerial dsp_serial;
	SoftwareSerial cio_serial;

	uint8_t from_CIO_buf[7];  //CIO to ESP. We will copy it straight to display, and getting the temperature
	uint8_t to_DSP_buf[7];    //ESP to DSP
	uint8_t from_DSP_buf[7];  //DSP to ESP. We can ignore this message and send our own when ESP is in charge.
	uint8_t to_CIO_buf[7];    //Otherwise copy here. Buffer to send from ESP to CIO
	bool cio_tx;			  //set to true when data received. Send to webinterface+serial for debugging
	bool dsp_tx;			  //set to true when data received. Send to webinterface+serial for debugging

  private:

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
	void saveSettingsFlag();
	void saveSettings();
	Ticker saveSettingsTimer;
	bool cio_tx;
	bool dsp_tx;

  private:
    CIO _cio;
	uint32_t _commandQ[MAXCOMMANDS][4];
	int _qCommandLen = 0;		//length of commandQ
	uint32_t _buttonQ[MAXBUTTONS][4];
	int _qButtonLen = 0;	//length of buttonQ
	uint32_t _timestamp;
	bool _newData = false;
	uint32_t _cltime;
	uint32_t _ftime;
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
	uint32_t _finterval;
	uint32_t _clinterval;
	uint32_t _audio;
	float _cost;
	bool _saveSettingsNeeded = false;
	bool _saveEventlogNeeded = false;
	bool _saveCmdqNeeded = false;
	int _latestTarget;
	int _tickerCount;
	bool _sliderPrio = true;

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
