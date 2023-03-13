#include "bwc.h"
#include "util.h"
#include "pitches.h"

BWC::BWC()
{
    //Initialize variables

    _dsp_brightness = 7;
    _cl_timestamp_s = DateTime.now();
    _filter_timestamp_s = DateTime.now();
    _uptime = 0;
    _pumptime = 0;
    _heatingtime = 0;
    _airtime = 0;
    _jettime = 0;
    _price = 1;
    _filter_interval = 30;
    _cl_interval = 14;
    _audio_enabled = true;
    _restore_states_on_start = false;
    _ambient_temp = 20;
    _virtual_temp_fix = -99;
    to_cio_states.power_change = 1;
};

void save_settings_cb(BWC* bwcInstance)
{
    bwcInstance->on_save_settings();
}
void scroll_text_cb(BWC* bwcInstance)
{
    bwcInstance->on_scroll_text();
}

void BWC::on_save_settings()
{
    if(++_ticker_count >= 3)
    {
        _save_settings_needed = true;
        _ticker_count = 0;
    }
}

void BWC::on_scroll_text()
{
    _scroll = true;
}

void BWC::setup(void){
    Models ciomodel;
    Models dspmodel;
    
    LittleFS.begin();
    if(!_loadHardware(ciomodel, dspmodel, pins)){
        pins[0] = D1;
        pins[1] = D2;
        pins[2] = D3;
        pins[3] = D4;
        pins[4] = D5;
        pins[5] = D6;
        pins[6] = D7;
        
    }
    // Serial.printf("Cio loaded: %d, dsp model: %d\n", ciomodel, dspmodel);
    for(int i = 0; i < 7; i++)
    {
        // Serial.printf("pin%d: %d\n", i, pins[i]);
    }
    switch(ciomodel)
    {
        case PRE2021:
            _cio = new CIO_PRE2021;
            break;
        case MIAMI2021:
            _cio = new CIO_2021;
            break;
        case MALDIVES2021:
            _cio = new CIO_2021_HJT;
            break;
        case M54149E:
            _cio = new CIO_54149E;
            break;
        case M54173:
            _cio = new CIO_54173;
            break;
        case M54154:
            _cio = new CIO_54154;
            break;
        case M54144:
            _cio = new CIO_54144;
            break;
        case M54138:
            _cio = new CIO_54138;
            break;
        case M54123:
            _cio = new CIO_54123;
            break;
        default:
            _cio = new CIO_PRE2021;
            break;
    }
    switch(dspmodel)
    {
        case PRE2021:
            _dsp = new DSP_PRE2021;
            break;
        case MIAMI2021:
            _dsp = new DSP_2021;
            break;
        case MALDIVES2021:
            _dsp = new DSP_2021_HJT;
            break;
        case M54149E:
            _dsp = new DSP_54149E;
            break;
        case M54173:
            _dsp = new DSP_54173;
            break;
        case M54154:
            _dsp = new DSP_54154;
            break;
        case M54144:
            _dsp = new DSP_54144;
            break;
        case M54138:
            _dsp = new DSP_54138;
            break;
        case M54123:
            _dsp = new DSP_54123;
            break;
        default:
            _dsp = new DSP_PRE2021;
            break;
    }
    _cio->setup(pins[0], pins[1], pins[2]);
    _dsp->setup(pins[3], pins[4], pins[5], pins[6]);
    hasjets = _cio->getHasjets();
    hasgod = _cio->getHasgod();
    begin();
}

void BWC::begin(){
    _loadSettings();
    _loadCommandQueue();
    _restoreStates();
    // _save_melody("melody.bin");
    // if(_audio_enabled) _dsp->playIntro();
    // _dsp->LEDshow();
    _save_settings_ticker.attach(3600.0f, save_settings_cb, this);
    _scroll_text_ticker.attach(0.25f, scroll_text_cb, this);

    _next_notification_time = _notification_time;
}


void BWC::loop(){
    ++loop_count;
    #ifdef ESP8266
    ESP.wdtFeed();
    #endif
    _timestamp_secs = DateTime.now();
    _updateTimes();
    if(_scroll && (to_dsp_states.text.length() > 0)) 
    {
        to_dsp_states.text.remove(0,1);
        _scroll = false;
    }
    from_cio_states = _cio->getStates();
    to_dsp_states.locked = from_cio_states.locked;
    to_dsp_states.power = from_cio_states.power;
    to_dsp_states.unit = from_cio_states.unit;
    to_dsp_states.bubbles = from_cio_states.bubbles;
    to_dsp_states.heatgrn = from_cio_states.heatgrn;
    to_dsp_states.heatred = from_cio_states.heatred;
    to_dsp_states.heat = from_cio_states.heat;
    to_dsp_states.pump = from_cio_states.pump;
    to_dsp_states.temperature = from_cio_states.temperature;
    to_dsp_states.char1 = from_cio_states.char1;
    to_dsp_states.char2 = from_cio_states.char2;
    to_dsp_states.char3 = from_cio_states.char3;
    to_dsp_states.jets = from_cio_states.jets;
    to_dsp_states.error = from_cio_states.error;
    to_dsp_states.timerled1 = from_cio_states.timerled1;
    to_dsp_states.timerled2 = from_cio_states.timerled2;
    to_dsp_states.timerbuttonled = from_cio_states.timerbuttonled;
    to_dsp_states.godmode = from_cio_states.godmode;
    
    /*Modify and use to_dsp_states here if we want to show text or something*/
    _dsp->setRawPayload(_cio->getRawPayload());
    _dsp->setStates(to_dsp_states);

    from_dsp_states = _dsp->getStates();

    /*Light up screen when pressing buttons*/
    if(from_dsp_states.pressed_button != NOBTN) _override_dsp_brt_timer = 5000;
    if(_override_dsp_brt_timer > 0)
    {
        to_dsp_states.brightness = _dsp_brightness + 1;
        if(to_dsp_states.brightness > 8) to_dsp_states.brightness = 8;
    }
    else
    {
        to_dsp_states.brightness = _dsp_brightness;
    }

    if(from_dsp_states.pressed_button == UP) 
    {
        _sweepup();
        _dsp_tgt_used = true;
    }
    if(from_dsp_states.pressed_button == DOWN) 
    {
        _sweepdown();
        _dsp_tgt_used = true;
    }
    if(from_dsp_states.pressed_button == TIMER) _beep();
    to_cio_states.locked_change = from_dsp_states.locked_change;
    to_cio_states.power_change = from_dsp_states.power_change;
    if(_dsp_tgt_used)
        to_cio_states.target = from_cio_states.target;
    if(from_dsp_states.unit_change) 
    {
        from_cio_states.unit ? to_cio_states.target = C2F(to_cio_states.target) : to_cio_states.target = F2C(to_cio_states.target); 
    }
    to_cio_states.unit_change = from_dsp_states.unit_change;
    to_cio_states.bubbles_change = from_dsp_states.bubbles_change;
    to_cio_states.heat_change = from_dsp_states.heat_change;
    to_cio_states.pump_change = from_dsp_states.pump_change;
    to_cio_states.jets_change = from_dsp_states.jets_change;
    to_cio_states.pressed_button = from_dsp_states.pressed_button;
    if(from_dsp_states.bubbles_change || from_dsp_states.heat_change || from_dsp_states.jets_change || from_dsp_states.locked_change || from_dsp_states.power_change || from_dsp_states.pump_change ||from_dsp_states.unit_change) _accord();
    
    /*following method will change target temp and set _dsp_tgt_used to false if target temp is changed*/
    _handleCommandQ();
    /*If new target was not set above, use whatever the cio says*/
    _cio->setRawPayload(_dsp->getRawPayload());
    _cio->setStates(to_cio_states);

    if(_save_settings_needed) saveSettings();
    if(_save_cmdq_needed) _saveCommandQueue();
    if(_save_states_needed) _saveStates();
    _handleNotification();
    _handleStateChanges();
    _calcVirtualTemp();
    // logstates();
}

// String BWC::get_fromcio()
// {
//     DynamicJsonDocument doc(512);

    // Set the values in the document
    // doc["CONTENT"] = "from_cio";
    // doc["TIME"] = _timestamp_secs;
    // doc["LCK"] = from_cio_states.locked;
    // doc["PWR"] = from_cio_states.power;
    // doc["UNT"] = from_cio_states.unit;
    // doc["AIR"] = from_cio_states.bubbles;
    // doc["CH1"] = from_cio_states.char1;
    // doc["CH2"] = from_cio_states.char2;
    // doc["CH3"] = from_cio_states.char3;
    // doc["BRT"] = from_cio_states.brightness;
    // doc["TGT"] = from_cio_states.target;
    // doc["TMP"] = from_cio_states.temperature;
    // doc["cod"] = _cio->_button_code;
    // doc["bql"] = _cio->_button_que_len;
    // doc["str"] = _cio->debugmsg;
    // Serialize JSON to string
//     String jsonmsg;
//     if (serializeJson(doc, jsonmsg) == 0) {
//         jsonmsg = "{\"error\": \"Failed to serialize message\"}";
//     }
//     _cio->debugmsg = "";
//     return jsonmsg;
// }

// String BWC::get_todsp()
// {
    // DynamicJsonDocument doc(1536);

    // // Set the values in the document
    // doc["CONTENT"] = "to_dsp";
    // doc["TIME"] = _timestamp_secs;
    // doc["LCK"] = to_dsp_states.locked;
    // doc["PWR"] = to_dsp_states.power;
    // doc["UNT"] = to_dsp_states.unit;
    // doc["AIR"] = to_dsp_states.bubbles;
    // doc["CH1"] = to_dsp_states.char1;
    // doc["CH2"] = to_dsp_states.char2;
    // doc["CH3"] = to_dsp_states.char3;
    // doc["BRT"] = to_dsp_states.brightness;
    // doc["TGT"] = to_dsp_states.target;
    // doc["TMP"] = to_dsp_states.temperature;
    // doc["TXT"] = to_dsp_states.text;
    // doc["LOOP"] = loop_count;
    // loop_count = 0;
    // // Serialize JSON to string
    // String jsonmsg;
    // if (serializeJson(doc, jsonmsg) == 0) {
    //     jsonmsg = "{\"error\": \"Failed to serialize message\"}";
    // }
    // return jsonmsg;
// }

// String BWC::get_fromdsp()
// {
    // DynamicJsonDocument doc(1536);

    // // Set the values in the document
    // doc["CONTENT"] = "from_dsp";
    // doc["TIME"] = _timestamp_secs;
    // doc["LCK"] = from_dsp_states.locked;
    // doc["PWR"] = from_dsp_states.power;
    // doc["UNT"] = from_dsp_states.unit;
    // doc["AIR"] = from_dsp_states.bubbles;
    // doc["CH1"] = from_dsp_states.char1;
    // doc["CH2"] = from_dsp_states.char2;
    // doc["CH3"] = from_dsp_states.char3;
    // doc["BRT"] = from_dsp_states.brightness;
    // doc["TGT"] = from_dsp_states.target;
    // doc["TMP"] = from_dsp_states.temperature;
    // // Serialize JSON to string
    // String jsonmsg;
    // if (serializeJson(doc, jsonmsg) == 0) {
    //     jsonmsg = "{\"error\": \"Failed to serialize message\"}";
    // }
    // return jsonmsg;
// }

// String BWC::get_tocio()
// {
//     DynamicJsonDocument doc(1536);

//     // Set the values in the document
//     doc["CONTENT"] = "to_cio";
//     doc["TIME"] = _timestamp_secs;
//     doc["LCK"] = to_cio_states.locked;
//     doc["PWR"] = to_cio_states.power;
//     doc["UNT"] = to_cio_states.unit;
//     doc["AIR"] = to_cio_states.bubbles;
//     doc["CH1"] = to_cio_states.char1;
//     doc["CH2"] = to_cio_states.char2;
//     doc["CH3"] = to_cio_states.char3;
//     doc["BRT"] = to_cio_states.brightness;
//     doc["TGT"] = to_cio_states.target;
//     doc["TMP"] = to_cio_states.temperature;
//     // Serialize JSON to string
//     String jsonmsg;
//     if (serializeJson(doc, jsonmsg) == 0) {
//         jsonmsg = "{\"error\": \"Failed to serialize message\"}";
//     }
//     return jsonmsg;
// }

void BWC::stop(){
    _save_settings_ticker.detach();
    _scroll_text_ticker.detach();
    _cio->stop();
    delete _cio;
    _dsp->stop();
    delete _dsp;
}

void BWC::pause_resume(bool action)
{
    if(action)
    {
        _save_settings_ticker.detach();
        _scroll_text_ticker.detach();
    } else
    {
        _save_settings_ticker.attach(3600.0f, save_settings_cb, this);
        _scroll_text_ticker.attach(0.25f, scroll_text_cb, this);
    }
    _cio->pause_resume(action);
    _dsp->pause_resume(action);
}

/*Sort by xtime, ascending*/
bool BWC::_compare_command(const command_que_item& i1, const command_que_item& i2)
{
    return i1.xtime < i2.xtime;
}

void BWC::_handleNotification()
{
    /* user don't want a notification*/
    if(!_notify) return;
    /* there is no upcoming command*/
    if(_command_que.size() == 0)
    {
        _next_notification_time = _notification_time;
        return;
    }
    /* not the time yet*/
    if((int64_t)_command_que[0].xtime - (int64_t)_timestamp_secs > (int64_t)_next_notification_time) return;
    /* only _notify for these commands*/
    if(!(_command_que[0].cmd == SETBUBBLES || _command_que[0].cmd == SETHEATER || _command_que[0].cmd == SETJETS || _command_que[0].cmd == SETPUMP)) return;

    if(_audio_enabled) _sweepup();
    to_dsp_states.text += "  --" + String(_next_notification_time) + "--";
    // to_dsp_states.text = "i-i-";
    if(_next_notification_time <= 2)
        _next_notification_time = -10; //postpone "alarm" until after the command xtime (will be reset on command execution)
    else
        _next_notification_time /= 2;
}

void BWC::_handleCommandQ() {
    bool restartESP = false;
    if(_command_que.size() < 1) return;
    /* time for next command? */
    if (_timestamp_secs < _command_que[0].xtime) return;
    //If interval > 0 then append to commandQ with updated xtime.
    if(_command_que[0].interval > 0)
    {
       _command_que[0].xtime += _command_que[0].interval;
       _command_que.push_back(_command_que[0]);
    } 
    _handlecommand(_command_que[0].cmd, _command_que[0].val, _command_que[0].text);
    restartESP = _command_que[0].cmd == REBOOTESP;
    //remove from commandQ
    _command_que.erase(_command_que.begin());
    _next_notification_time = _notification_time; //reset alarm time
    _save_cmdq_needed = true;
    if(restartESP) {
        saveSettings();
        _saveCommandQueue();
        stop();
        delay(3000);
        ESP.restart();
    }
    /*If we pushed back an item, we need to re-sort the que*/
    std::sort(_command_que.begin(), _command_que.end(), _compare_command);
}

bool BWC::_handlecommand(int64_t cmd, int64_t val, String txt="")
{
    
    to_dsp_states.text += String(" ") + txt;
    switch (cmd)
    {
    case SETTARGET:
    {
        if(! ((val > 0 && val < 41) || (val > 50 && val < 105)) ) break;
        bool implied_unit_is_celsius = (val < 41);
        bool required_unit = from_cio_states.unit;
        if(implied_unit_is_celsius && !required_unit)
            to_cio_states.target = round(C2F(val));
        else if(!implied_unit_is_celsius && required_unit)
            to_cio_states.target = round(F2C(val));
        else
            to_cio_states.target = val;
        /*Send this value to cio instead of results from button presses on the display*/
        _dsp_tgt_used = false;
        break;
    }
    case SETUNIT:
        if(hasgod && !to_cio_states.godmode) break;
        if(val == 1 && from_cio_states.unit == 0) to_cio_states.target = round(F2C(to_cio_states.target)); 
        if(val == 0 && from_cio_states.unit == 1) to_cio_states.target = round(C2F(to_cio_states.target)); 
        if((uint8_t)val != from_cio_states.unit) to_cio_states.unit_change = 1;
        break;
    case SETBUBBLES:
        if(val != from_cio_states.bubbles) to_cio_states.bubbles_change = 1;
        break;
    case SETHEATER:
        if(val != from_cio_states.heat) to_cio_states.heat_change = 1;
        break;
    case SETPUMP:
        if(val != from_cio_states.pump) to_cio_states.pump_change = 1;
        break;
    case GETTARGET:
        /*Not used atm*/
        break;
    case RESETTIMES:
        _uptime = 0;
        _pumptime = 0;
        _jettime = 0;
        _heatingtime = 0;
        _airtime = 0;
        _uptime_ms = 0;
        _pumptime_ms = 0;
        _jettime_ms = 0;
        _heatingtime_ms = 0;
        _airtime_ms = 0;
        _energy_total_kWh = 0;
        _save_settings_needed = true;
        break;
    case RESETCLTIMER:
        _cl_timestamp_s = _timestamp_secs;
        _save_settings_needed = true;
        break;
    case RESETFTIMER:
        _filter_timestamp_s = _timestamp_secs;
        _save_settings_needed = true;
        break;
    case SETJETS:
        if(val != from_cio_states.jets) to_cio_states.jets_change = 1;
        break;
    case SETBRIGHTNESS:
        _dsp_brightness = val;
        break;
    case SETBEEP:
        if(val == 0) _beep();
        if(val == 1) _accord();
        break;
    case SETAMBIENTF:
        setAmbientTemperature(val, false);
        break;
    case SETAMBIENTC:
        setAmbientTemperature(val, true);
        break;
    case RESETDAILY:
        _energy_daily_Ws = 0;
        break;
    case SETGODMODE:
        to_cio_states.godmode = val > 0;
        break;
    case SETREADY:
        {
            Serial.print(_timestamp_secs);
            Serial.print("  ");
            Serial.print((val - _estHeatingTime() * 3600.0f - 7200));
            Serial.println((int64_t)_timestamp_secs > (int64_t)(val - _estHeatingTime() * 3600.0f - 7200));
            command_que_item item;
            if((int64_t)_timestamp_secs > (int64_t)(val - _estHeatingTime() * 3600.0f - 7200)) //2 hours extra margin
            {
                /*time to start heating*/
                item.cmd = SETHEATER;
                item.interval = 0;
                item.text = "";
                item.val = 1;
                item.xtime = _timestamp_secs + 1;
                add_command(item);
            }
            else
            {
                /*Not time yet, so add check in one minute*/
                item.cmd = cmd;
                item.interval = 0;
                item.text = "";
                item.val = val;
                item.xtime = _timestamp_secs + 60;
                /*We can't use addcommand() because it will copy xtime to val again*/
                _command_que.push_back(item);
                std::sort(_command_que.begin(), _command_que.end(), _compare_command);
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void BWC::_handleStateChanges()
{
    if(_prev_cio_states != from_cio_states || _prev_dsp_states.brightness != to_dsp_states.brightness) _new_data_available = true;
    if(from_cio_states.temperature != _prev_cio_states.temperature)
    {
        _deltatemp = from_cio_states.temperature - _prev_cio_states.temperature;
        _updateVirtualTempFix_ontempchange();
        _temp_change_timestamp_ms = millis();
    }

    // Store virtual temp data point
    if(from_cio_states.heatred != _prev_cio_states.heatred)
    {
        _heatred_change_timestamp_ms = millis();
        _updateVirtualTempFix_onheaterchange();
    }

    if(from_cio_states.pump != _prev_cio_states.pump)
    {
        _pump_change_timestamp_ms = millis();
    }

    if(from_cio_states.bubbles != _prev_cio_states.bubbles)
    {
        _bubbles_change_timestamp_ms = millis();
    }

    if(from_cio_states.unit != _prev_cio_states.unit || from_cio_states.pump != _prev_cio_states.pump || from_cio_states.heat != _prev_cio_states.heat)
        _save_states_needed = true;

    _prev_cio_states = from_cio_states;
    _prev_dsp_states = to_dsp_states;
    /* check changes from DSP 4W - go to antigodmode if someone presses a button*/
}

// return how many hours until pool is ready. (provided the heater is on)
float BWC::_estHeatingTime()
{
    int targetInC = from_cio_states.target;
    if(!from_cio_states.unit) targetInC = F2C(targetInC);
    if(_virtual_temp > targetInC) return -2;  //Already

    // float degAboveAmbient = _virtual_temp - (float)_ambient_temp;
    // float fraction = 1.0f - (degAboveAmbient - floor(degAboveAmbient));
    // int deltaTemp = targetInC - _virtual_temp;

    // //integrate the time needed to reach target
    // //how long to next integer temp
    // double coolingPerHour = degAboveAmbient / _R_COOLING;
    // double netRisePerHour;
    // netRisePerHour = _heating_degperhour - coolingPerHour;

    // double hoursRemaining = fraction / netRisePerHour;

    double degAboveAmbient;
    double deltaTemp = targetInC - _virtual_temp;
    double coolingPerHour;
    double netRisePerHour;
    double hoursRemaining = 0;
    //iterate up to target
    for(float i = 0; i <= deltaTemp; i += 0.01)
    {
        degAboveAmbient = _virtual_temp + i - _ambient_temp;
        coolingPerHour = degAboveAmbient / _R_COOLING;
        netRisePerHour = _heating_degperhour - coolingPerHour;
        if(netRisePerHour <= 0) return -1; //Never
        hoursRemaining += 0.01 / netRisePerHour;
    }

    if(hoursRemaining >= 0)
        return hoursRemaining;
    else 
        return -1; //Never
}

//virtual temp is always C in this code and will be converted when sending externally
void BWC::_calcVirtualTemp()
{
    //startup init
    if(millis() < 30000)
    {
        int tempInC = from_cio_states.temperature;
        if(!from_cio_states.unit) {
            tempInC = F2C(tempInC);
        }
        _virtual_temp_fix = tempInC;
        _virtual_temp = _virtual_temp_fix;
        _virtual_temp_fix_age = 0;
        return;
    }

    // calculate from last updated VTFix.
    double netRisePerHour;
    float degAboveAmbient = _virtual_temp - _ambient_temp;
    double coolingPerHour = degAboveAmbient / _R_COOLING;

    if(from_cio_states.heatred)
    {
        netRisePerHour = _heating_degperhour - coolingPerHour;
    }
    else
    {
        netRisePerHour = - coolingPerHour;
    }
    double elapsed_hours = _virtual_temp_fix_age / 3600.0 / 1000.0;
    float newvt = _virtual_temp_fix + netRisePerHour * elapsed_hours;

    // clamp VT to +/- 1 from measured temperature if pump is running
    if(from_cio_states.pump && ((millis()-_pump_change_timestamp_ms) > 5*60000))
    {
        float tempInC = from_cio_states.temperature;
        float limit = 0.99;
        if(!from_cio_states.unit)
        {
            tempInC = F2C(tempInC);
            limit = 1/1.8;
        }
        float dev = newvt-tempInC;
        if(dev > limit) dev = limit;
        if(dev < -limit) dev = -limit;
        newvt = tempInC + dev;
    }

    // Rebase start of calculation from new integer temperature
    if(int(_virtual_temp) != int(newvt))
    {
        _virtual_temp_fix = newvt;
        _virtual_temp_fix_age = 0;
    }
    _virtual_temp = newvt;

    /* Using Newtons law of cooling
        T(t) = Tenv + (T(0) - Tenv)*e^(-t/r)
        r = -t / ln( (T(t)-Tenv) / (T(0)-Tenv) )
        dT/dt = (T(t) - Tenv) / r
        ----------------------------------------
        T(t) : Temperature at time t
        Tenv : _ambient_temp (considered constant)
        T(0) : Temperature at time 0 (_virtual_temp_fix)
        e    : natural number 2,71828182845904
        r    : a constant we need to find out by measurements
    */

}

//Called on temp change
void BWC::_updateVirtualTempFix_ontempchange()
{
    int tempInC = from_cio_states.temperature;
    float conversion = 1;
    if(!from_cio_states.unit) {
        tempInC = F2C(tempInC);
        conversion = 1/1.8;
    }
    //Do not process if temperature changed > 1 degree (reading spikes)
    if(abs(_deltatemp) != 1) return;

    //readings are only valid if pump is running and has been running for 5 min.
    if(!from_cio_states.pump || ((millis()-_pump_change_timestamp_ms) < 5*60000)) return;

    _virtual_temp = tempInC;
    _virtual_temp_fix = tempInC;
    _virtual_temp_fix_age = 0;
    /*
    update_coolingDegPerHourArray
    Measured temp has changed by 1 degree over a certain time
    1 degree/(temperature age in ms / 3600 / 1000)hours = 3 600 000 / temperature age in ms
    */

    // We can only know something about rate of change if we had continous cooling since last update
    // (Nobody messed with the heater during the 1 degree change)
    if(_heatred_change_timestamp_ms < _temp_change_timestamp_ms) return;
    // rate of heating is not subject to change (fixed wattage and pool size) so do this only if cooling
    // and do not calibrate if bubbles has been on
    if(_vt_calibrated) return;
    if(from_cio_states.heatred || from_cio_states.bubbles || (_bubbles_change_timestamp_ms < _temp_change_timestamp_ms)) return;
    if(_deltatemp > 0 && _virtual_temp > _ambient_temp) return; //temp is rising when it should be falling. Bail out
    if(_deltatemp < 0 && _virtual_temp < _ambient_temp) return; //temp is falling when it should be rising. Bail out
    float degAboveAmbient = _virtual_temp - _ambient_temp;
    // can't calibrate if ambient ~ virtualtemp
    if(abs(degAboveAmbient) <= 1) return;
    _R_COOLING = ((millis()-_temp_change_timestamp_ms)/3600000.0) / log((conversion*degAboveAmbient) / (conversion*(degAboveAmbient + _deltatemp)));
    _vt_calibrated = true;
}

//Called on heater state change
void BWC::_updateVirtualTempFix_onheaterchange()
{
    _virtual_temp_fix = _virtual_temp;
    _virtual_temp_fix_age = 0;
}

void BWC::print(const String &txt)
{
    to_dsp_states.text += txt;
}

void BWC::setAmbientTemperature(int64_t amb, bool unit)
{
    _ambient_temp = (int)amb;
    if(!unit) _ambient_temp = F2C(_ambient_temp);

    _virtual_temp_fix = _virtual_temp;
    _virtual_temp_fix_age = 0;
}

String BWC::getModel()
{
    return _cio->getModel();
}

bool BWC::add_command(command_que_item command_item)
{
    _save_cmdq_needed = true;
    if(command_item.cmd == RESETQ)
    {
        _command_que.clear();
        return true;
    }
    if(command_item.cmd == SETREADY)
    {
        command_item.val = (int64_t)command_item.xtime; //Use val field to store the time to be ready
        command_item.xtime = 0; //And start checking now
        command_item.interval = 0;
    }
    //add parameters to _command_que[rows][parameter columns] and sort the array on xtime.
    _command_que.push_back(command_item);
    std::sort(_command_que.begin(), _command_que.end(), _compare_command);
    return true;
}

//check for special button sequence
bool BWC::getBtnSeqMatch()
{
    if( _btn_sequence[0] == POWER &&
        _btn_sequence[1] == LOCK &&
        _btn_sequence[2] == TIMER &&
        _btn_sequence[3] == POWER)
    {
        return true;
    }
    return false;
}

String BWC::getJSONStates() {
        // Allocate a temporary JsonDocument
        // Don't forget to change the capacity to match your requirements.
        // Use arduinojson.org/assistant to compute the capacity.
    //feed the dog
    #ifdef ESP8266
    ESP.wdtFeed();
    #endif
    DynamicJsonDocument doc(1536);

    // Set the values in the document
    doc["CONTENT"] = "STATES";
    doc["TIME"] = _timestamp_secs;
    doc["LCK"] = from_cio_states.locked;
    doc["PWR"] = from_cio_states.power;
    doc["UNT"] = from_cio_states.unit;
    doc["AIR"] = from_cio_states.bubbles;
    doc["GRN"] = from_cio_states.heatgrn;
    doc["RED"] = from_cio_states.heatred;
    doc["FLT"] = from_cio_states.pump;
    doc["CH1"] = from_cio_states.char1;
    doc["CH2"] = from_cio_states.char2;
    doc["CH3"] = from_cio_states.char3;
    doc["HJT"] = from_cio_states.jets;
    doc["BRT"] = to_dsp_states.brightness;
    doc["ERR"] = from_cio_states.error;
    doc["GOD"] = from_cio_states.godmode;
    doc["TGT"] = from_cio_states.target;
    doc["TMP"] = from_cio_states.temperature;
    doc["VTMC"] = _virtual_temp;
    doc["VTMF"] = C2F(_virtual_temp);
    doc["AMBC"] = _ambient_temp;
    doc["AMBF"] = round(C2F(_ambient_temp));
    if(from_cio_states.unit)
    {
        //celsius
        doc["AMB"] = _ambient_temp;
        doc["VTM"] = _virtual_temp;
        doc["TGTC"] = from_cio_states.target;
        doc["TMPC"] = from_cio_states.temperature;
        doc["TGTF"] = round(C2F((float)from_cio_states.target));
        doc["TMPF"] = round(C2F((float)from_cio_states.temperature));
        doc["VTMF"] = C2F(_virtual_temp);
    }
    else
    {
        //farenheit
        doc["AMB"] = round(C2F(_ambient_temp));
        doc["VTM"] = C2F(_virtual_temp);
        doc["TGTF"] = from_cio_states.target;
        doc["TMPF"] = from_cio_states.temperature;
        doc["TGTC"] = round(F2C((float)from_cio_states.target));
        doc["TMPC"] = round(F2C((float)from_cio_states.temperature));
        doc["VTMC"] = _virtual_temp;
    }

    // Serialize JSON to string
    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
        jsonmsg = "{\"error\": \"Failed to serialize message\"}";
    }
    return jsonmsg;
}

String BWC::getJSONTimes() {
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    //feed the dog
    #ifdef ESP8266
    ESP.wdtFeed();
    #endif
    DynamicJsonDocument doc(1024);

    // Set the values in the document
    doc["CONTENT"] = "TIMES";
    doc["TIME"] = _timestamp_secs;
    doc["CLTIME"] = _cl_timestamp_s;
    doc["FTIME"] = _filter_timestamp_s;
    doc["UPTIME"] = _uptime + _uptime_ms/1000;
    doc["PUMPTIME"] = _pumptime + _pumptime_ms/1000;
    doc["HEATINGTIME"] = _heatingtime + _heatingtime_ms/1000;
    doc["AIRTIME"] = _airtime + _airtime_ms/1000;
    doc["JETTIME"] = _jettime + _jettime_ms/1000;
    doc["COST"] = _energy_total_kWh * _price;
    doc["FINT"] = _filter_interval;
    doc["CLINT"] = _cl_interval;
    doc["KWH"] = _energy_total_kWh;
    doc["KWHD"] = _energy_daily_Ws / 3600000.0; //Ws -> kWh
    doc["WATT"] = _energy_power_W;
    float t2r = _estHeatingTime();
    String t2r_string = String(t2r);
    if(t2r == -2) t2r_string = F("Already");
    if(t2r == -1) t2r_string = F("Never");
    doc["T2R"] = t2r_string;
    String s = _cio->debug();
    doc["DBG"] = s;
    //_cio->clk_per = 1000;  //reset minimum clock period

    // Serialize JSON to string
    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
        jsonmsg = "{\"error\": \"Failed to serialize message\"}";
    }
    return jsonmsg;
}

String BWC::getJSONSettings(){
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/assistant to compute the capacity.
    //feed the dog
    #ifdef ESP8266
    ESP.wdtFeed();
    #endif
    DynamicJsonDocument doc(1024);

    // Set the values in the document
    doc["CONTENT"] = "SETTINGS";
    doc["PRICE"] = _price;
    doc["FINT"] = _filter_interval;
    doc["CLINT"] = _cl_interval;
    doc["AUDIO"] = _audio_enabled;
    #ifdef ESP8266
    doc["REBOOTINFO"] = ESP.getResetReason();
    #endif
    doc["REBOOTTIME"] = DateTime.getBootTime();
    doc["RESTORE"] = _restore_states_on_start;
    doc["MODEL"] = _cio->getModel();
    doc["NOTIFY"] = _notify;
    doc["NOTIFTIME"] = _notification_time;

    // Serialize JSON to string
    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
        jsonmsg = "{\"error\": \"Failed to serialize message\"}";
    }
    return jsonmsg;
}

String BWC::getJSONCommandQueue(){
    //feed the dog
    #ifdef ESP8266
    ESP.wdtFeed();
    #endif
    DynamicJsonDocument doc(1024);
    // Set the values in the document
    doc["LEN"] = _command_que.size();
    for(unsigned int i = 0; i < _command_que.size(); i++){
        doc["CMD"][i] = _command_que[i].cmd;
        doc["VALUE"][i] = _command_que[i].val;
        doc["XTIME"][i] = _command_que[i].xtime;
        doc["INTERVAL"][i] = _command_que[i].interval;
        doc["TXT"][i] = _command_que[i].text;
    }

    // Serialize JSON to file
    String jsonmsg;
    if (serializeJson(doc, jsonmsg) == 0) {
        jsonmsg = "{\"error\": \"Failed to serialize message\"}";
    }
    return jsonmsg;
}

/*TODO:*/
uint8_t BWC::getState(int state){
    // return _cio->getState(state);
    return 0;
}

String BWC::getButtonName() {
    return ButtonNames[from_dsp_states.pressed_button];
}

Buttons BWC::getButton()
{
    return from_dsp_states.pressed_button;
}

void BWC::setJSONSettings(const String& message){
    //feed the dog
    // ESP.wdtFeed();
    DynamicJsonDocument doc(1024);

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        // Serial.println(F("Failed to read config file"));
        return;
    }

    // Copy values from the JsonDocument to the variables
    _price = doc["PRICE"];
    _filter_interval = doc["FINT"];
    _cl_interval = doc["CLINT"];
    _audio_enabled = doc["AUDIO"];
    _restore_states_on_start = doc["RESTORE"];
    _notify = doc["NOTIFY"];
    _notification_time = doc["NOTIFTIME"];
    _vt_calibrated = doc["VTCAL"];
    saveSettings();
}

bool BWC::newData(){
    bool result = _new_data_available;
    _new_data_available = false;
    return result;
}

void BWC::_startNTP() {
    // setup this after wifi connected
    DateTime.setServer("pool.ntp.org");
    DateTime.begin();
    DateTime.begin();
    int c = 0;
    while (!DateTime.isTimeValid()) {
        // Serial.println(F("Failed to get time from server. Trying again."));
        delay(1000);
        //DateTime.setServer("time.cloudflare.com");
        DateTime.begin();
        if (c++ > 5) break;
    }
    // Serial.println(DateTime.format(DateFormatter::SIMPLE));
}

void BWC::_updateTimes(){
    uint32_t now = millis();
    static uint32_t prevtime = now;
    int elapsedtime_ms = now-prevtime;
    prevtime = now;
    // //(some of) these age-counters resets when the state changes
    // for(unsigned int i = 0; i < _cio->getSizeofStates(); i++)
    // {
    //     _cio->setStateAge(i, _cio->getStateAge(i) + elapsedtime_ms);
    // }
    _virtual_temp_fix_age += elapsedtime_ms;

    if (elapsedtime_ms < 0) return; //millis() rollover every 24,8 days
    if(from_cio_states.heatred){
        _heatingtime_ms += elapsedtime_ms;
    }
    if(from_cio_states.pump){
        _pumptime_ms += elapsedtime_ms;
    }
    if(from_cio_states.bubbles){
        _airtime_ms += elapsedtime_ms;
    }
    if(from_cio_states.jets){
        _jettime_ms += elapsedtime_ms;
    }
    _uptime_ms += elapsedtime_ms;


    if(_uptime_ms > 1000000000){
        _heatingtime += _heatingtime_ms/1000;
        _pumptime += _pumptime_ms/1000;
        _airtime += _airtime_ms/1000;
        _jettime += _jettime_ms/1000;
        _uptime += _uptime_ms/1000;
        _heatingtime_ms = 0;
        _pumptime_ms = 0;
        _airtime_ms = 0;
        _jettime_ms = 0;
        _uptime_ms = 0;
    }

    if(_override_dsp_brt_timer > 0) _override_dsp_brt_timer -= elapsedtime_ms; //counts down to or below zero

    // watts, kWh today, total kWh
    float heatingEnergy = (_heatingtime+_heatingtime_ms/1000)/3600.0 * _cio->getPower().HEATERPOWER;
    float pumpEnergy = (_pumptime+_pumptime_ms/1000)/3600.0 * _cio->getPower().PUMPPOWER;
    float airEnergy = (_airtime+_airtime_ms/1000)/3600.0 * _cio->getPower().AIRPOWER;
    float idleEnergy = (_uptime+_uptime_ms/1000)/3600.0 * _cio->getPower().IDLEPOWER;
    float jetEnergy = (_jettime+_jettime_ms/1000)/3600.0 * _cio->getPower().JETPOWER;
    _energy_total_kWh = (heatingEnergy + pumpEnergy + airEnergy + idleEnergy + jetEnergy)/1000; //Wh -> kWh
    _energy_power_W = from_cio_states.heatred * _cio->getPower().HEATERPOWER;
    _energy_power_W += from_cio_states.pump * _cio->getPower().PUMPPOWER;
    _energy_power_W += from_cio_states.bubbles * _cio->getPower().AIRPOWER;
    _energy_power_W += _cio->getPower().IDLEPOWER;
    _energy_power_W += from_cio_states.jets * _cio->getPower().JETPOWER;

    _energy_daily_Ws += elapsedtime_ms * _energy_power_W / 1000.0;

    if(_notes.size())
    {
        to_dsp_states.audiofrequency = _notes.back().frequency_hz;
        _note_duration += elapsedtime_ms;
        if(_note_duration >= _notes.back().duration_ms)
        {
            _note_duration -= _notes.back().duration_ms;
            _notes.pop_back();
            _note_duration = 0;
        }
    }
    else
    {
        to_dsp_states.audiofrequency = 0;
    }
}

/*          */
/* LOADERS  */
/*          */

bool BWC::_loadHardware(Models& cioNo, Models& dspNo, int pins[])
{
    File file = LittleFS.open("/hwcfg.json", "r");
    if (!file)
    {
        // Serial.println(F("Failed to open hwcfg.json"));
        return false;
    }
    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        // Serial.println(F("Failed to read settings.txt"));
        file.close();
        return false;
    }
    file.close();
    cioNo = doc["cio"];
    dspNo = doc["dsp"];
    String pcbname = doc["pcb"].as<String>();
    // int pins[7];
    #ifdef ESP8266
    int DtoGPIO[] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};
    #endif
    for(int i = 0; i < 7; i++)
    {
        pins[i] = doc["pins"][i];
    #ifdef ESP8266
        pins[i] = DtoGPIO[pins[i]];
    #endif
    }
    return true;
}

void BWC::reloadSettings(){
    _loadSettings();
    return;
}

void BWC::_loadSettings(){
    File file = LittleFS.open("/settings.json", "r");
    if (!file) {
        // Serial.println(F("Failed to load settings.json"));
        return;
    }
    DynamicJsonDocument doc(1024);

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        // Serial.println(F("Failed to deser. settings.json"));
        file.close();
        return;
    }

    // Copy values from the JsonDocument to the variables
    _cl_timestamp_s = doc["CLTIME"];
    _filter_timestamp_s = doc["FTIME"];
    _uptime = doc["UPTIME"];
    _pumptime = doc["PUMPTIME"];
    _heatingtime = doc["HEATINGTIME"];
    _airtime = doc["AIRTIME"];
    _jettime = doc["JETTIME"];
    _price = doc["PRICE"];
    _filter_interval = doc["FINT"];
    _cl_interval = doc["CLINT"];
    _audio_enabled = doc["AUDIO"];
    _notify = doc["NOTIFY"];
    _notification_time = doc["NOTIFTIME"];
    _energy_total_kWh = doc["KWH"];
    _energy_daily_Ws = doc["KWHD"];
    _restore_states_on_start = doc["RESTORE"];
    _R_COOLING = doc["R"] | 20; //else use default
    _ambient_temp = doc["AMB"] | 20;
    _dsp_brightness = doc["BRT"] | 7;
    _vt_calibrated = doc["VTCAL"] | false;

    file.close();
}

void BWC::_restoreStates() {
    if(!_restore_states_on_start) return;
    File file = LittleFS.open("states.txt", "r");
    if (!file) {
        // Serial.println(F("Failed to read states.txt"));
        return;
    }
    // DynamicJsonDocument doc(512);
    StaticJsonDocument<512> doc;
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        // Serial.println(F("Failed to deserialize states.txt"));
        file.close();
        return;
    }

    uint8_t unt = doc["UNT"];
    uint8_t flt = doc["FLT"];
    uint8_t htr = doc["HTR"];
    uint8_t tgt = doc["TGT"] | 20;
    command_que_item item;
    item.cmd = SETUNIT;
    item.val = unt;
    item.xtime = 0;
    item.interval = 0;
    item.text = "";
    add_command(item);
    item.cmd = SETPUMP;
    item.val = flt;
    item.xtime = 0;
    item.interval = 0;
    item.text = "";
    add_command(item);
    item.cmd = SETHEATER;
    item.val = htr;
    item.xtime = 0;
    item.interval = 0;
    item.text = "";
    add_command(item);
    item.cmd = SETTARGET;
    item.val = tgt;
    item.xtime = 0;
    item.interval = 0;
    item.text = "";
    add_command(item);
    // Serial.println(F("Restoring states"));
    file.close();
}

void BWC::reloadCommandQueue(){
    _loadCommandQueue();
    return;
}

void BWC::_loadCommandQueue(){
    File file = LittleFS.open("/cmdq.json", "r");
    if (!file) {
        // Serial.println(F("Failed to read cmdq.json"));
        return;
    }

    DynamicJsonDocument doc(1024);
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        // Serial.println(F("Failed to deserialize cmdq.json"));
        file.close();
        return;
    }

    // Set the values in the variables
    for(int i = 0; i < doc["LEN"]; i++){
        command_que_item item;
        item.cmd = doc["CMD"][i];
        item.val = doc["VALUE"][i];
        item.xtime = doc["XTIME"][i];
        item.interval = doc["INTERVAL"][i];
        String s = doc["TXT"][i] | "";
        item.text = s;
        _command_que.push_back(item);
    }

    file.close();
}

/*          */
/* SAVERS   */
/*          */

void BWC::saveRebootInfo(){
    File file = LittleFS.open("bootlog.txt", "a");
    if (!file) {
        // Serial.println(F("Failed to save bootlog.txt"));
        return;
    }

    // DynamicJsonDocument doc(1024);
    StaticJsonDocument<256> doc;

    // Set the values in the document
    time_t boot_timestamp = DateTime.getBootTime();
    tm * boot_time_tm = localtime(&boot_timestamp);
    char boot_time_str[64];
    strftime(boot_time_str, 64, DateFormatter::SIMPLE, boot_time_tm);
    #ifdef ESP8266
    doc["BOOTINFO"] = ESP.getResetReason() + " " + boot_time_str;
    #endif

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
        // Serial.println(F("Failed to write bootlog.txt"));
    }
    file.println();
    file.close();
}

void BWC::_saveStates() {
    // //kill the dog
    // // ESP.wdtDisable();
    #ifdef ESP8266
    ESP.wdtFeed();
    #endif
    _save_states_needed = false;
    File file = LittleFS.open("states.txt", "w");
    if (!file) {
        // Serial.println(F("Failed to save states.txt"));
        return;
    }

    // DynamicJsonDocument doc(1024);
    StaticJsonDocument<256> doc;

    // Set the values in the document
    doc["UNT"] = from_cio_states.unit;
    doc["HTR"] = from_cio_states.heat;
    doc["FLT"] = from_cio_states.pump;
    doc["TGT"] = from_cio_states.target;

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
        // Serial.println(F("Failed to write states.txt"));
    }
    file.close();
    // //revive the dog
    // // ESP.wdtEnable(0);
}

void BWC::_saveCommandQueue(){
    _save_cmdq_needed = false;
    //kill the dog
    // ESP.wdtDisable();
    #ifdef ESP8266
    ESP.wdtFeed();
    #endif
    File file = LittleFS.open("cmdq.json", "w");
    if (!file) {
        // Serial.println(F("Failed to save cmdq.json"));
        return;
    } else {
        // Serial.println(F("Wrote cmdq.json"));
    }
    /*Do not save instant reboot command. Don't ask me how I know.*/
    if(_command_que.size())
        if(_command_que[0].cmd == REBOOTESP && _command_que[0].interval == 0) return;
    DynamicJsonDocument doc(1024);

    // Set the values in the document
    doc["LEN"] = _command_que.size();
    for(unsigned int i = 0; i < _command_que.size(); i++){
        doc["CMD"][i] = _command_que[i].cmd;
        doc["VALUE"][i] = _command_que[i].val;
        doc["XTIME"][i] = _command_que[i].xtime;
        doc["INTERVAL"][i] = _command_que[i].interval;
        doc["TXT"][i] = _command_que[i].text;
    }

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
        // Serial.println(F("Failed to write cmdq.json"));
    } else {
        String s;
        serializeJson(doc, s);
        // Serial.println(s);
    }
    file.close();
    //revive the dog
    // ESP.wdtEnable(0);
}

void BWC::saveSettings(){
    //kill the dog
    // ESP.wdtDisable();
    #ifdef ESP8266
    ESP.wdtFeed();
    #endif
    _save_settings_needed = false;
    File file = LittleFS.open("settings.json", "w");
    if (!file) {
        // Serial.println(F("Failed to save settings.json"));
        return;
    }

    DynamicJsonDocument doc(1024);
    _heatingtime += _heatingtime_ms/1000;
    _pumptime += _pumptime_ms/1000;
    _airtime += _airtime_ms/1000;
    _jettime += _jettime_ms/1000;
    _uptime += _uptime_ms/1000;
    _heatingtime_ms = 0;
    _pumptime_ms = 0;
    _airtime_ms = 0;
    _uptime_ms = 0;
    // Set the values in the document
    doc["CLTIME"] = _cl_timestamp_s;
    doc["FTIME"] = _filter_timestamp_s;
    doc["UPTIME"] = _uptime;
    doc["PUMPTIME"] = _pumptime;
    doc["HEATINGTIME"] = _heatingtime;
    doc["AIRTIME"] = _airtime;
    doc["JETTIME"] = _jettime;
    doc["PRICE"] = _price;
    doc["FINT"] = _filter_interval;
    doc["CLINT"] = _cl_interval;
    doc["AUDIO"] = _audio_enabled;
    doc["KWH"] = _energy_total_kWh;
    doc["KWHD"] = _energy_daily_Ws;
    doc["SAVETIME"] = DateTime.format(DateFormatter::SIMPLE);
    doc["RESTORE"] = _restore_states_on_start;
    doc["R"] = _R_COOLING;
    doc["AMB"] = _ambient_temp;
    doc["BRT"] = _dsp_brightness;
    doc["NOTIFY"] = _notify;
    doc["NOTIFTIME"] = _notification_time;
    doc["VTCAL"] = _vt_calibrated;

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
        // Serial.println(F("Failed to write json to settings.json"));
    }
    file.close();
    //revive the dog
    // ESP.wdtEnable(0);
}

//save out debug text to file "debug.txt" on littleFS
void BWC::saveDebugInfo(const String& s){
    File file = LittleFS.open("debug.txt", "a");
    if (!file) {
        // Serial.println(F("Failed to save debug.txt"));
        return;
    }

    DynamicJsonDocument doc(1024);

    // Set the values in the document
    doc["timestamp"] = DateTime.format(DateFormatter::SIMPLE);
    doc["message"] = s;
    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
        // Serial.println(F("Failed to write debug.txt"));
    }
    file.close();
}

/* SOUND */

void BWC::_save_melody(const String& filename)
{
    File file = LittleFS.open(filename, "w");
    if (!file) return;
    sNote n = {1000, 500};
    file.write((byte*)&n, sizeof(n));
    file.close();
}

void BWC::_add_melody(const String &filename)
{
    if(_notes.size() || !_audio_enabled) return;
    File file = LittleFS.open(filename, "r");
    if (!file) return;
    while(file.available())
    {
        sNote n;
        file.readBytes((char*)&n, sizeof(n));
        _notes.push_back(n);
    }
    file.close();
    /* We read and erase from the back of the vector (faster) so if notes are stored in the natural order we need to reverse*/
    std::reverse(_notes.begin(), _notes.end());
}

void BWC::_sweepdown()
{
    if(_notes.size() || !_audio_enabled) return;
    for(int i = 0; i < 128; i++)
    {
        sNote n;
        n.duration_ms = 2;
        n.frequency_hz = 1000 + 8*i;
        _notes.push_back(n);
    }
}

void BWC::_sweepup()
{
    if(_notes.size() || !_audio_enabled) return;
    for(int i = 0; i < 128; i++)
    {
        sNote n;
        n.duration_ms = 2;
        n.frequency_hz = 2000 - 8*i;
        _notes.push_back(n);
    }
}

void BWC::_beep()
{
    if(_notes.size() || !_audio_enabled) return;
    sNote n;
    n.duration_ms = 50;
    n.frequency_hz = 2400;
    _notes.push_back(n);
    n.duration_ms = 50;
    n.frequency_hz = 800;
    _notes.push_back(n);
}

void BWC::_accord()
{
    if(_notes.size() || !_audio_enabled) return;
    sNote n;
    for(int i = 0; i < 5; i++)
    {
        n.duration_ms = 10;
        n.frequency_hz = NOTE_C6;
        _notes.push_back(n);
        n.duration_ms = 10;
        n.frequency_hz = NOTE_E6;
        _notes.push_back(n);
    }
}
