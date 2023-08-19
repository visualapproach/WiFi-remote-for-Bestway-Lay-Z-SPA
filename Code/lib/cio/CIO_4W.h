#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "enums.h"
#include "CIO_BASE.h"
#include <umm_malloc/umm_heap_select.h>


class BWC;

class CIO_4W : public CIO
{
    /*These must be declared for the API to work*/
    public:
        CIO_4W(){};
        virtual ~CIO_4W(){};
        Power getPower(){return _power;}
        void setup(int cio_rx, int cio_tx, int dummy);
        void stop();
        void pause_all(bool action);
        void handleToggles();
        void updateStates();
        bool getHasgod() {return true;}
        virtual bool getHasjets() = 0;
        virtual bool getHasair() = 0;

    /*internal use*/
    protected:
        void regulateTemp();
        void antifreeze();
        void antiboil();
        void togglestates();
        virtual uint8_t getPumpBitmask() = 0;
        virtual uint8_t getBubblesBitmask() = 0;
        virtual uint8_t getJetsBitmask() = 0;
        virtual uint8_t getHeatBitmask1() = 0;
        virtual uint8_t getHeatBitmask2() = 0;
        virtual uint8_t getPowerBitmask() = 0;
        virtual uint8_t getJumptable(int row, int col) = 0;
        virtual uint8_t getAllowedstates(int row, int col) = 0;
        void generatePayload();

    private:
        Power _power = {1900, 40, 800, 2, 400};
        uint8_t _heat_bitmask = 0;
        SoftwareSerial *_cio_serial;
        uint8_t _from_CIO_buf[7] = {};
        uint8_t _to_CIO_buf[7] = {};
        uint8_t _currentStateIndex = 0;
        uint64_t _prev_ms;

        //cio
        const uint8_t TEMPINDEX = 2;
        const uint8_t ERRORINDEX = 3;
        const uint8_t CIO_CHECKSUMINDEX = 5;
        //dsp
        const uint8_t COMMANDINDEX = 2;
        const uint8_t DSP_CHECKSUMINDEX = 5;
        const uint8_t PAYLOADSIZE = 7;
        // sStates cio_states;

        const uint32_t _HEATER2_DELAY_MS = 10000;
        const uint32_t _HEATERCOOLING_DELAY_MS = 5000;
        int32_t _heater2_countdown_ms = 0;
        int32_t _cool_heater_countdown_ms = 0;
        bool _turn_off_pump_flag = false;
};

