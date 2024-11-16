//based on:
// https://github.com/dbuezas/esphome-cc1101
// and https://github.com/NorthernMan54/rtl_433_ESP/blob/main/src/rtl_433_ESP.cpp
// https://github.com/smartoctopus/RadioLib-esphome/blob/master/examples/CC1101/CC1101_Settings/CC1101_Settings.ino
// https://github.com/mag1024/esphome-rtl433/

#ifndef CC1101TRANSCEIVER_H
#define CC1101TRANSCEIVER_H

#include "esphome.h"

#define RADIOLIB_LOW_LEVEL 1

static const char *const TAG = "EH_CC1101";
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/spi/spi.h"

#include "EHRLHal.h"

#define get_cc1101(id) (*((EH_CC1101*)id))

class EH_CC1101 : public PollingComponent, public Sensor {
  float _bandwidth=58;
  int _last_rssi = 0;
  EH_RL_Hal* hal;
  InternalGPIOPin* _gd0_rx;
  InternalGPIOPin* _gd0_tx;

public:
  void setup() {
 
    // Use Radiolib CC1101 direct receive ASK-OOK

    int state = radio.begin();
    ESP_LOGD(TAG, "CC1101 setup begin state=%d", state);

    state|=radio.standby();

    state|=setFreq(_freq);
    // datarate seems to be very crtitical to be around 5.0k -- intertwined with AGC settings from smartRC below
    // see also DN022: https://www.ti.com/lit/an/swra215e/swra215e.pdf
    state|=radio.setBitRate(5);

    // set rx bw after datarate - and only specific ones work with radiolib...
    state|=setBW(_bandwidth);

    state|=radio.setOOK(true); // probably not necessary
  
    // per DN022 adjust LNA as needed
    state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_FREND1, (_bandwidth>101) ? 0xb6 : 0x56);
    // also per DN022
    state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_TEST2, (_bandwidth>325) ? 0x88 : 0x81);
    state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_TEST1, (_bandwidth>325) ? 0x31 : 0x35);
    state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_FIFOTHR, (_bandwidth>325) ? 0x07 : 0x47);

    // AGC settings from: LSatan/SmartRC-CC1101-Driver-Lib
    // They work well (possible to-do: add ability to dynamically modify in esphome)
    // AGCCTRL2[7:6] reduce maximum available DVGA gain: disable top three gain setting
    // AGCCTRL2[2:0] average amplitude target for filter: 42 dB
    state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL2, 0xc7); 
    state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL1, 0x00);
    state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL0, 0xb2);

    // start receiving onto GDO
    state|= recv();
    ESP_LOGD(TAG, "CC1101 setup end state=%d", state);
  }

  int standby() {
    // standby state: gd0 is input, radio in standby
    _gd0_rx->setup();
    return(radio.standby());
  }

  int recv() {
    // receive state: gd0 is input, radio doing receiveDirectAsync
    _gd0_rx->setup();
    return(radio.receiveDirectAsync());
  }

  int xmit() {
    // xmit state: gd0 is output
    // wip (need to test w/ sdr all is well)
    //_gd0_tx->setup();
    standby(); // wip
    return(-1);
  }

  float _freq;
  CC1101 radio=NULL;
  
  EH_CC1101(esphome::spi_device::SPIDeviceComponent* spi,
             InternalGPIOPin* gd0_rx, InternalGPIOPin* gd0_tx, 
             float freq=433.92,float bandwidth=464) : PollingComponent(100) {

    hal = new EH_RL_Hal(spi);
    radio = new Module(hal,RADIOLIB_NC, RADIOLIB_NC,RADIOLIB_NC);

    _bandwidth = bandwidth;
    _freq = freq;
    _gd0_rx=gd0_rx;
    _gd0_tx=gd0_tx;

  } 

  int setBW(float bandwidth) {
    // set to a valid value
    float possibles[16] = {58, 68, 81, 102, 116, 135, 162, 203, 232, 270, 325, 406, 464, 541, 650, 812};
    for(int i=0;i<15;i++) {
      if ((bandwidth>=possibles[i])&&(bandwidth<=possibles[i+1])) {
        _bandwidth=bandwidth-possibles[i]<possibles[i+1]-bandwidth ? possibles[i] : possibles[i+1];
        break;
      }
    }
    return radio.setRxBandwidth(_bandwidth);
  }
  int setFreq(float freq) {
    _freq=freq;
    return radio.setFrequency(_freq);
  }

  bool rssi_on = false;
  void update() override {
    // not yet implemented
    int rssi = 0;
    if (rssi_on) {
      if (rssi != _last_rssi) {
        publish_state(rssi);
        _last_rssi = rssi;
      }
    }
  }
};

#endif
