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
    dsp_toggles.locked_change = 0;
    dsp_toggles.power_change = 0;
    dsp_toggles.pump_change = 0;
    dsp_toggles.unit_change = 0;
    dsp_toggles.target = dsp_states.target;
    
    if(btn != _prev_btn)
    {
        if(dsp_states.power && !dsp_states.locked)
        {
            /*Pump is ON and UNLOCKED*/
            switch(btn)
            {
                case LOCK:
                /*Need a way to determine long press*/
                    dsp_toggles.locked_change = 1;
                break;
                case TIMER:
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
                break;
                case UP:
                break;
                case POWER:
                    dsp_toggles.power_change = 1;
                break;
                case HYDROJETS:
                    dsp_toggles.jets_change = 1;
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
                    dsp_toggles.power_change = 1;
                    break;
                case LOCK:
                    if(dsp_states.power) dsp_toggles.locked_change = 1;
                    break;
                case NOBTN:
                default:
                    break;
            }
        }
    }
    _prev_btn = btn;
    return;
}

