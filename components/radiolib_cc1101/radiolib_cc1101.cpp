#include "esphome/core/log.h"
#include "radiolib_cc1101.h"

//based on:
// https://github.com/dbuezas/esphome-cc1101
// and https://github.com/NorthernMan54/rtl_433_ESP/blob/main/src/rtl_433_ESP.cpp
// https://github.com/jgromes/RadioLib/blob/master/examples/CC1101/CC1101_Settings/CC1101_Settings.ino
// https://github.com/mag1024/esphome-rtl433/

namespace esphome {
namespace radiolib_cc1101 {

static const char *TAG = "radiolib_cc1101.component";

void RadiolibCC1101Component::setup() {
  ESP_LOGI(TAG, "SPI Setup");
  this->spi_setup();

  // Use Radiolib CC1101 direct receive ASK-OOK
  hal = new EH_RL_Hal(this);
  radio = new Module(hal,RADIOLIB_NC, RADIOLIB_NC,RADIOLIB_NC);

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

void RadiolibCC1101Component::loop() {

}

void RadiolibCC1101Component::dump_config(){
    ESP_LOGCONFIG(TAG, "RadioLib-cc1101 component");
}

int RadiolibCC1101Component::standby() {
  // standby state: gd0 is input, radio in standby
  _gd0_rx->setup();
  return(radio.standby());
}

int RadiolibCC1101Component::recv() {
  // receive state: gd0 is input, radio doing receiveDirectAsync
  _gd0_rx->setup();
  return(radio.receiveDirectAsync());
}

int RadiolibCC1101Component::xmit() {
  // xmit state: gd0 is output
  // wip (need to test w/ sdr all is well)
  //_gd0_tx->setup();
  standby(); // wip
  return(-1);
}

int RadiolibCC1101Component::setBW(float bandwidth) {
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

int RadiolibCC1101Component::setFreq(float freq) {
  _freq=freq;
  return radio.setFrequency(_freq);
}

}  // namespace radiolib_cc1101
}  // namespace esphome
