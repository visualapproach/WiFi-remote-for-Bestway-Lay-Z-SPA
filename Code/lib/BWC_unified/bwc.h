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
#include "ESPDateTime.h"
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

constexpr int MAXCOMMANDS = 20;

struct command_que_item
{
    uint8_t cmd;
    int64_t val;
    uint64_t xtime;
    uint32_t interval;
    String text = "";
};

class BWC {

    public:
        BWC();
        void setup(void);
        void begin();
        void on_save_settings();
        void on_scroll_text();
        void loop();
        // String get_fromcio();
        // String get_todsp();
        // String get_fromdsp();
        // String get_tocio();
        void stop(void);
        bool add_command(command_que_item command_item);
        // bool qCommand(int64_t cmd, int64_t val, int64_t xtime, int64_t interval);
        bool newData();
        String getJSONStates();
        String getJSONTimes();
        String getJSONSettings();
        void setJSONSettings(const String& message);
        String getJSONCommandQueue();
        uint8_t getState(int state);
        // void saveSettingsFlag();
        void saveSettings();
        void reloadCommandQueue();
        void reloadSettings();
        String getButtonName();
        Buttons getButton();
        void saveDebugInfo(const String& s);
        void saveRebootInfo();
        bool getBtnSeqMatch();
        void setAmbientTemperature(int64_t amb, bool unit);
        String getModel();
        void print(const String& txt);

    public:
        String reboot_time;
        sStates from_cio_states, to_dsp_states;
        sToggles to_cio_states, from_dsp_states;
        int pins[7];
        unsigned int loop_count = 0;
        bool hasjets, hasgod;

    private:
        void _loadHardware(Models& cioNo, Models& dspNo, int pins[]);
        bool _handlecommand(int64_t cmd, int64_t val, String txt);
        void _handleCommandQ();
        void _startNTP();
        void _loadSettings();
        void _loadCommandQueue();
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
        void _add_melody(const String& filename);
        void _save_melody(const String& filename);
        void _sweepdown();
        void _sweepup();
        void _beep();
        void _accord();

    private:
        CIO* _cio;
        DSP* _dsp;
        bool _notify;
        int _notification_time, _next_notification_time;
        Ticker _save_settings_ticker;
        Ticker _scroll_text_ticker;
        bool _scroll = false;
        uint64_t _timestamp_secs; // seconds
        uint8_t _dsp_brightness;
        int16_t _override_dsp_brt_timer;
        std::vector<command_que_item> _command_que;
        std::vector<sNote> _notes;
        int _note_duration;
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
        float _price;
        uint32_t _filter_interval;
        uint32_t _cl_interval;
        bool _audio_enabled;
        float _energy_total_kWh;
        double _energy_daily_Ws; //Wattseconds internally
        int _energy_power_W;
        bool _restore_states_on_start = false;
        bool _save_settings_needed = false;
        bool _save_cmdq_needed = false;
        bool _save_states_needed = false;
        int _ticker_count;
        int _btn_sequence[4] = {NOBTN,NOBTN,NOBTN,NOBTN}; //keep track of the four latest button presses
        float _R_COOLING = 40;
        int _ambient_temp; //always in C internally
        float _heating_degperhour = 1.5; //always in C internally
        float _virtual_temp; //=virtualtempfix+calculated diff, always in C internally
        float _virtual_temp_fix; //last fixed data point to add or subtract temp from, always in C internally
        uint32_t _virtual_temp_fix_age;
        bool _new_data_available = false;
        bool _dsp_tgt_used = true;
        sStates _prev_cio_states, _prev_dsp_states;
        unsigned long _temp_change_timestamp_ms, _heatred_change_timestamp_ms;
        unsigned long _pump_change_timestamp_ms, _bubbles_change_timestamp_ms;
        int _deltatemp;
        bool _vt_calibrated = false;
};

void save_settings_cb(BWC *bwcInstance);

void scroll_text_cb(BWC *bwcInstance);
