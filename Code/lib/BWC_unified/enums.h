#pragma once
#include <Arduino.h>

/*Indicies to access binary codes or pretty button names.*/
enum Buttons: byte
{
    NOBTN,
    LOCK,
    TIMER,
    BUBBLES,
    UNIT,
    HEAT,
    PUMP,
    DOWN,
    UP,
    POWER,
    HYDROJETS
};

/*Pretty names for buttons. Must be in same order as "enum Buttons" in file "enums.h"*/
const String ButtonNames[] = {
    "NOBTN",
    "LOCK",
    "TIMER",
    "BUBBLES",
    "UNIT",
    "HEAT",
    "PUMP",
    "DOWN",
    "UP",
    "POWER",
    "HYDROJETS"
};

enum States: byte
{
    LOCKEDSTATE,
    POWERSTATE,
    UNITSTATE,
    BUBBLESSTATE,
    HEATGRNSTATE,
    HEATREDSTATE,
    HEATSTATE,
    PUMPSTATE,
    TEMPERATURE,
    TARGET,
    CHAR1,
    CHAR2,
    CHAR3,
    JETSSTATE,
    ERROR
};

const uint8_t CHARS[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '-', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 
    'h', 'H', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'x', 'y', 'z'
};

/*TODO: report that this is breaking changes. GODMODE and FULLPOWER have new values.*/
enum Commands: byte
{
    SETTARGET,
    SETUNIT,
    SETBUBBLES,
    SETHEATER,
    SETPUMP,
    RESETQ,
    REBOOTESP,
    GETTARGET,
    RESETTIMES,
    RESETCLTIMER,
    RESETFTIMER,
    SETJETS,
    SETBRIGHTNESS,
    SETBEEP,
    SETAMBIENTF,
    SETAMBIENTC,
    RESETDAILY,
    SETGODMODE,
    SETFULLPOWER,
    PRINTTEXT,
    SETREADY
};

enum ToggleButtons: byte
{
    BUBBLETOGGLE,
    JETSTOGGLE,
    PUMPTOGGLE,
    HEATTOGGLE
};

enum Models: uint8_t
{
    PRE2021,
    MIAMI2021,
    MALDIVES2021,
    M54149E,
    M54173,
    M54154,
    M54144,
    M54138,
    M54123
};

struct Power
{
    int HEATERPOWER;
    int PUMPPOWER;
    int AIRPOWER;
    int IDLEPOWER;
    int JETPOWER;
};

struct sStates
{
    uint8_t locked = 0;
    uint8_t power = 0;
    uint8_t unit = 0;
    uint8_t bubbles = 0;
    uint8_t heatgrn = 0;
    uint8_t heatred = 0;
    uint8_t heat = 0;
    uint8_t pump = 0;
    uint8_t temperature = 25;
    uint8_t target = 20;
    uint8_t char1 = 'a';
    uint8_t char2 = 'b';
    uint8_t char3 = 'c';
    uint8_t jets = 0;
    uint8_t error = 0;
    // uint8_t timerbutton = 0;
    // uint8_t upbutton = 0;
    // uint8_t downbutton = 0;
    uint8_t timerled1 = 0;
    uint8_t timerled2 = 0;
    uint8_t timerbuttonled = 0;
    uint8_t brightness = 8;
    bool gettarget = false;
    // bool fullpower = false;
    uint8_t no_of_heater_elements_on = 0;
    bool godmode = false;
    String text ="";
    int audiofrequency = 0;

    inline bool operator==(const sStates& rhs)
    {
        bool result = false;
        result = locked == rhs.locked;
        result &= power == rhs.power;
        result &= unit == rhs.unit;
        result &= bubbles == rhs.bubbles;
        result &= heatgrn == rhs.heatgrn;
        result &= heatred == rhs.heatred;
        result &= heat == rhs.heat;
        result &= pump == rhs.pump;
        result &= temperature == rhs.temperature;
        result &= target == rhs.target;
        result &= char1 == rhs.char1;
        result &= char2 == rhs.char2;
        result &= char3 == rhs.char3;
        result &= jets == rhs.jets;
        // result &= timerbutton == rhs.timerbutton;
        // result &= upbutton == rhs.upbutton;
        // result &= downbutton == rhs.downbutton;
        result &= timerled1 == rhs.timerled1;
        result &= timerled2 == rhs.timerled2;
        result &= brightness == rhs.brightness;
        result &= gettarget == rhs.gettarget;
        // result &= fullpower == rhs.fullpower;
        result &= no_of_heater_elements_on == rhs.no_of_heater_elements_on;
        result &= godmode == rhs.godmode;
        result &= text == rhs.text;
        result &= audiofrequency == rhs.audiofrequency;
        
        return result;
    }  
    inline bool operator!=(const sStates& rhs){return ! (*this == rhs);}
};

struct sToggles
{
    bool locked_change = 0;
    bool power_change = 0;
    bool unit_change = 0;
    bool bubbles_change = 0;
    bool heat_change = 0;
    bool pump_change = 0;
    bool jets_change = 0;
    Buttons pressed_button = NOBTN;
    /*Requested state, not toggled*/
    bool godmode = false;
    /*Requested state, not toggled*/
    uint8_t target = 20;
    /*Requested state, not toggled*/
    uint8_t no_of_heater_elements_on = 0;

    inline bool operator==(const sToggles& rhs)
    {
        bool result = false;
        result = locked_change == rhs.locked_change;
        result &= power_change == rhs.power_change;
        result &= unit_change == rhs.unit_change;
        result &= bubbles_change == rhs.bubbles_change;
        result &= heat_change == rhs.heat_change;
        result &= pump_change == rhs.pump_change;
        result &= jets_change == rhs.jets_change;
        result &= no_of_heater_elements_on == rhs.no_of_heater_elements_on;
        result &= pressed_button == rhs.pressed_button;
        result &= godmode == rhs.godmode;
        result &= target == rhs.target;
        return result;
    }
    inline bool operator!=(const sToggles& rhs){return !(*this == rhs);}
};

struct sNote
{
    int frequency_hz;
    int duration_ms;
};