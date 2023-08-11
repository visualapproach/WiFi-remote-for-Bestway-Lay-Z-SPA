#include "DSP_6W.h"

    /*getbutton and make states from that*/
void DSP_6W::updateToggles()
{
    Buttons btn = getPressedButton();
    dsp_toggles.pressed_button = btn;
    /*Reset all fields*/
    dsp_toggles.bubbles_change = 0;
    dsp_toggles.heat_change = 0;
    dsp_toggles.jets_change = 0;
    dsp_toggles.locked_pressed = 0;
    dsp_toggles.power_change = 0;
    dsp_toggles.pump_change = 0;
    dsp_toggles.unit_change = 0;
    dsp_toggles.timer_pressed = 0;
    dsp_toggles.up_pressed = 0;
    dsp_toggles.down_pressed = 0;
    dsp_toggles.target = dsp_states.target;
    
    if(btn != _prev_btn)
    {
        if(dsp_states.power && !dsp_states.locked)
        {
            /*Pump is ON and UNLOCKED*/
            switch(btn)
            {
                case LOCK:
                    dsp_toggles.locked_pressed = 1;
                break;
                case TIMER:
                    dsp_toggles.timer_pressed = 1;
                break;
                case BUBBLES:
                    dsp_toggles.bubbles_change = 1;
                break;
                case UNIT:
                    dsp_toggles.unit_change = 1;
                break;
                case HEAT:
                    dsp_toggles.heat_change = 1;
                break;
                case PUMP:
                    dsp_toggles.pump_change = 1;
                break;
                case DOWN:
                    dsp_toggles.down_pressed = 1;
                break;
                case UP:
                    dsp_toggles.up_pressed = 1;
                break;
                case POWER:
                    dsp_toggles.power_change = 1;
                break;
                case HYDROJETS:
                    dsp_toggles.jets_change = 1;
                break;
                case NOBTN:
                break;
                default:
                break;
            }
        }
        else
        {
            /*Pump is LOCKED or OFF*/
            switch(btn)
            {
                case POWER:
                    dsp_toggles.power_change = 1;
                    break;
                case LOCK:
                    if(dsp_states.power) dsp_toggles.locked_pressed = 1;
                    break;
                case NOBTN:
                default:
                    break;
            }
        }
    }
    else
    /*no change in button pressed*/
    {
        switch(btn)
        {
            case LOCK:
                dsp_toggles.locked_pressed = 1;
                break;
            case TIMER:
                dsp_toggles.timer_pressed = 1;
                break;
            case UP:
                dsp_toggles.up_pressed = 1;
                break;
            case DOWN:
                dsp_toggles.down_pressed = 1;
                break;
            default:
                break;
        }
    }
    _prev_btn = btn;

    /* Filter enabled buttons */
    dsp_toggles.locked_pressed &= EnabledButtons[LOCK];
    dsp_toggles.timer_pressed &= EnabledButtons[TIMER];
    dsp_toggles.bubbles_change &= EnabledButtons[BUBBLES];
    dsp_toggles.unit_change &= EnabledButtons[UNIT];
    dsp_toggles.heat_change &= EnabledButtons[HEAT];
    dsp_toggles.pump_change &= EnabledButtons[PUMP];
    dsp_toggles.down_pressed &= EnabledButtons[DOWN];
    dsp_toggles.up_pressed &= EnabledButtons[UP];
    dsp_toggles.power_change &= EnabledButtons[POWER];
    dsp_toggles.jets_change &= EnabledButtons[HYDROJETS];
    
    return;
}

