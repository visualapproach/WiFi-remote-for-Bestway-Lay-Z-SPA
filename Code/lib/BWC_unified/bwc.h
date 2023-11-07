#pragma once

#if defined (ESP8266)
#elif defined (ESP32)
#else
#error "This library supports 8266/32 only"
#endif

#include "Arduino.h"
//long long needed in arduino core v3+
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
// #include "ESPDateTime.h"
#include <LittleFS.h>
#include <Ticker.h>
#include <vector>
#include "enums.h"
#include "CIO_PRE2021.h"
#include "CIO_2021.h"
#include "CIO_2021HJT.h"
#include "CIO_54149E.h"
#include "CIO_54173.h"
#include "CIO_54154.h"
#include "CIO_54144.h"
#include "CIO_54138.h"
#include "CIO_54123.h"

#include "DSP_PRE2021.h"
#include "DSP_2021.h"
#include "DSP_2021HJT.h"
#include "DSP_54149E.h"
#include "DSP_54173.h"
#include "DSP_54154.h"
#include "DSP_54144.h"
#include "DSP_54138.h"
#include "DSP_54123.h"

#include "FW_VERSION.h"

constexpr int MAXCOMMANDS = 20;

struct command_que_item
{
    int64_t val;
    uint64_t xtime;
    Commands cmd;
    uint32_t interval;
    String text = "";
};

class BWC {

    public:
        BWC();
        ~BWC();
        void setup(void);
        void begin();
        void on_save_settings();
        void on_scroll_text();
        void loop();
        void adjust_brightness();
        void play_sound();
        // String get_fromcio();
        // String get_todsp();
        // String get_fromdsp();
        // String get_tocio();
        void stop(void);
        void pause_all(bool action);
        bool add_command(command_que_item command_item);
        bool edit_command(uint8_t index, command_que_item command_item);
        bool del_command(uint8_t index);
        // bool qCommand(int64_t cmd, int64_t val, int64_t xtime, int64_t interval);
        bool newData();
        void getJSONStates(String &rtn);
        void getJSONTimes(String &rtn);
        void getJSONSettings(String &rtn);
        void setJSONSettings(const String& message);
        String getJSONCommandQueue();
        uint8_t getState(int state);
        // void saveSettingsFlag();
        void saveSettings();
        void reloadCommandQueue();
        void reloadSettings();
        void getButtonName(String &rtn);
        Buttons getButton();
        void saveDebugInfo(const String& s);
        void saveRebootInfo();
        bool getBtnSeqMatch();
        void setAmbientTemperature(int64_t amb, bool unit);
        String getModel();
        void print(const String& txt);
        void loadCommandQueue();

        // String getDebugData();

    public:
        time_t reboot_time_t;
        String reboot_time_str;
        int pins[8];
        int tempSensorPin;
        unsigned int loop_count = 0;
        CIO* cio = nullptr;
        DSP* dsp = nullptr;
        bool hasjets, hasgod;
        bool BWC_DEBUG = false;
        bool hasTempSensor = false;

    private:
        bool _loadHardware(Models& cioNo, Models& dspNo, int pins[]);
        bool _handlecommand(Commands cmd, int64_t val, const String& txt);
        void _handleCommandQ();
        void _loadSettings();
        void _saveCommandQueue();
        void _updateTimes();
        void _restoreStates();
        void _saveStates();
        float _estHeatingTime();
        void _calcVirtualTemp();
        void _updateVirtualTempFix_ontempchange();
        void _updateVirtualTempFix_onheaterchange();
        void _handleStateChanges();
        void _handleNotification();
        static bool _compare_command(const command_que_item& i1, const command_que_item& i2);
        bool _load_melody_json(const String &filename);
        void _add_melody(const String &filename);
        void _save_melody(const String& filename);
        void _sweepdown();
        void _sweepup();
        void _beep();
        void _accord();
        void _log();

    private:
        uint64_t _timestamp_secs; // seconds
        double _energy_daily_Ws; //Wattseconds internally
        unsigned long _temp_change_timestamp_ms, _heatred_change_timestamp_ms;
        unsigned long _pump_change_timestamp_ms, _bubbles_change_timestamp_ms;
        Ticker _save_settings_ticker;
        Ticker _scroll_text_ticker;
        std::vector<command_que_item> _command_que;
        std::vector<sNote> _notes;
        sStates _prev_cio_states, _prev_dsp_states;
        uint32_t _cl_timestamp_s;
        uint32_t _filter_timestamp_s;
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
        uint32_t _filter_interval;
        uint32_t _cl_interval;
        uint32_t _virtual_temp_fix_age;
        int _note_duration;
        int _notification_time, _next_notification_time;
        int _energy_power_W;
        int _ticker_count;
        int _btn_sequence[4] = {NOBTN,NOBTN,NOBTN,NOBTN}; //keep track of the four latest button presses
        int _ambient_temp; //always in C internally
        int _deltatemp;
        float _price;
        float _energy_total_kWh;
        float _R_COOLING = 40;
        float _heating_degperhour = 1.5; //always in C internally
        float _virtual_temp; //=virtualtempfix+calculated diff, always in C internally
        float _virtual_temp_fix; //last fixed data point to add or subtract temp from, always in C internally
        Buttons _prevbutton = NOBTN;
        int16_t _override_dsp_brt_timer;
        uint8_t _dsp_brightness;
        uint8_t _web_target = 20; 
        bool _scroll = false;
        bool _audio_enabled;
        bool _restore_states_on_start = false;
        bool _save_settings_needed = false;
        bool _save_cmdq_needed = false;
        bool _save_states_needed = false;
        bool _new_data_available = false;
        bool _dsp_tgt_used = true;
        bool _notify;
        bool _vt_calibrated = false;
};

void save_settings_cb(BWC *bwcInstance);

void scroll_text_cb(BWC *bwcInstance);
