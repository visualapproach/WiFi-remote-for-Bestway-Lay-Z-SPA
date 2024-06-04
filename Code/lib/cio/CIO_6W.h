#pragma once
#include <Arduino.h>
#include "enums.h"
#include "CIO_BASE.h"

struct sButton_queue_item
{
    uint16_t btncode;
    uint8_t sStates::*p_state;
    int value;
    int duration_ms;
};

class CIO_6W : public CIO
{
    /*These must be declared for the API to work*/
    public:
        CIO_6W();
        virtual ~CIO_6W(){};
        void handleToggles();
        bool getHasgod() {return false;}
        virtual bool getHasjets() = 0;
        virtual bool getHasair() = 0;

        /*internal use*/
    protected:
        virtual uint16_t getButtonCode(Buttons buttonIndex) = 0;
        void _qButton(sButton_queue_item item);
        void _handleButtonQ(void);
        void unlock();

    public:
        uint8_t brightness;

    protected:
        sButton_queue_item _button_que[MAXBUTTONS];
        /*Hex code, like 0x1B1B*/
        volatile uint16_t _button_code;
        const uint8_t DSP_DIM_BASE = 0x80;
        const uint8_t DSP_DIM_ON = 0x8;
        uint8_t _pressed_button = NOBTN;
        volatile bool _new_packet_available;
        bool _packet_transm_active;
};

