#include "DSP_6W.h"

    /*getbutton and make states from that*/
sToggles DSP_6W::getStates()
{
    Buttons btn = getPressedButton();
    _from_dsp_states.pressed_button = btn;
    /*Reset all fields*/
    _from_dsp_states.bubbles_change = 0;
    _from_dsp_states.heat_change = 0;
    _from_dsp_states.jets_change = 0;
    _from_dsp_states.locked_change = 0;
    _from_dsp_states.power_change = 0;
    _from_dsp_states.pump_change = 0;
    _from_dsp_states.unit_change = 0;
    
    if(btn != _prev_btn)
    {
        if(_to_dsp_states.power && !_to_dsp_states.locked)
        {
            /*Pump is ON and UNLOCKED*/
            switch(btn)
            {
                case LOCK:
                /*Need a way to determine long press*/
                    _from_dsp_states.locked_change = 1;
                break;
                case TIMER:
                break;
                case BUBBLES:
                    _from_dsp_states.bubbles_change = 1;
                break;
                case UNIT:
                    _from_dsp_states.unit_change = 1;
                break;
                case HEAT:
                    _from_dsp_states.heat_change = 1;
                break;
                case PUMP:
                    _from_dsp_states.pump_change = 1;
                break;
                case DOWN:
                break;
                case UP:
                break;
                case POWER:
                    _from_dsp_states.power_change = 1;
                break;
                case HYDROJETS:
                    _from_dsp_states.jets_change = 1;
                break;
                case NOBTN:
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
                    _from_dsp_states.power_change = 1;
                    break;
                case LOCK:
                    if(_to_dsp_states.power) _from_dsp_states.locked_change = 1;
                    break;
                case NOBTN:
                default:
                    break;
            }
        }
    }
    _prev_btn = btn;
    return _from_dsp_states;
}

