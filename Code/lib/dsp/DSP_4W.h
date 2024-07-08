#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "enums.h"
#include "DSP_BASE.h"
#include <umm_malloc/umm_heap_select.h>


class DSP_4W : public DSP
{
    public:
        DSP_4W(){};
        virtual ~DSP_4W(){};
        void setup(int dsp_tx, int dsp_rx, int dummy, int dummy2);
        void stop();
        void pause_all(bool action) override;
        void updateToggles();
        void handleStates();
        EspSoftwareSerial::UART *_dsp_serial;
        bool getSerialReceived() override;
        void setSerialReceived(bool txok) override;
  
    protected:
        virtual uint8_t getPumpBitmask() = 0;
        virtual uint8_t getBubblesBitmask() = 0;
        virtual uint8_t getJetsBitmask() = 0;
        virtual uint8_t getHeatBitmask1() = 0;
        virtual uint8_t getHeatBitmask2() = 0;
        virtual uint8_t getPowerBitmask() = 0;
        virtual uint8_t getJumptable(int row, int col) = 0;
        virtual uint8_t getAllowedstates(int row, int col) = 0;
        virtual bool getHasjets() = 0;
        virtual bool getHasair() = 0;
        void generatePayload();

    private:
        int _time_since_last_transmission_ms = 0;
        const int _max_time_between_transmissions_ms = 2000;
        /*ESP to DSP*/
        uint8_t _to_DSP_buf[7] = {};
        /*DSP to ESP. We can ignore this message and send our own when ESP is in charge.*/
        uint8_t _from_DSP_buf[7] = {};
        const uint8_t COMMANDINDEX = 2;
        const uint8_t DSP_CHECKSUMINDEX = 5;
        const uint8_t PAYLOADSIZE = 7;
        uint8_t _bubbles, _pump, _jets;
        bool _serialreceived = false;
        bool _readyToTransmit = false;

};
