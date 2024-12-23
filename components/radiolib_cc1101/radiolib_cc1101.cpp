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

  init_state = radio.begin();
  ESP_LOGD(TAG, "CC1101 setup begin init_state =%d", state);

  // setup direct receive mode
  setup_direct_mode();

  ESP_LOGD(TAG, "CC1101 setup end init_state =%d", state);

}

void RadiolibCC1101Component::loop() {
  // TODO: will re-implement this as a sensor that can work with the pin from the receive component
  //if ((state==CC1101_RECV)&&(_gd0_rx->digital_read())) last_rx_rssi=getRSSI();
}

void RadiolibCC1101Component::dump_config(){
    ESP_LOGCONFIG(TAG, "RadioLib-cc1101 component");
}

void RadiolibCC1101Component::set_registers() {
  init_state|=radio.setFrequency(_freq);
  init_state|=radio.setBitRate(_bitrate);
  // set rx bw after datarate - and only specific ones make sense...
  adjustBW(_bandwidth);
  init_state|=radio.setRxBandwidth(_bandwidth);

  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_FREND1,_REG_FREND1);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_TEST2,_REG_TEST2);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_TEST1,_REG_TEST1);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_FIFOTHR, _REG_FIFOTHR);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL2,_REG_AGCCTRL2);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL1,_REG_AGCCTRL1);
  init_state|= radio.SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL0,_REG_AGCCTRL0);
}

void RadiolibCC1101Component::setup_direct_mode() {
  init_state|=standby();

  // per DN022 adjust LNA as needed
  _REG_FREND1=(_bandwidth>101) ? 0xb6 : 0x56;
  // also per DN022
  _REG_TEST2= (_bandwidth>325) ? 0x88 : 0x81;
  _REG_TEST1= (_bandwidth>325) ? 0x31 : 0x35;
  _REG_FIFOTHR= (_bandwidth>325) ? 0x07 : 0x47;

  set_registers();

  init_state|=radio.setOOK(true); // probably not necessary

  // start receiving onto GDO
  init_state|= recv();

}

int RadiolibCC1101Component::standby() {
  // standby state: gd0 is input, radio in standby
  if ((_gd0_rx!=nullptr)&&(state==CC1101_XMIT)) _gd0_rx->setup();

  init_state|=radio.standby();
  state=init_state==0 ? CC1101_STANDBY : CC1101_NOINIT;
  return init_state;
}

int RadiolibCC1101Component::recv() {
  // receive state: gd0 is input, radio doing receiveDirectAsync
  if (state==CC1101_XMIT) standby();

  if (_gd0_rx!=nullptr) _gd0_rx->setup();
  init_state|=radio.receiveDirectAsync();
  state=init_state==0 ? CC1101_RECV : CC1101_NOINIT;
  return init_state;
}

int RadiolibCC1101Component::xmit() {
  // xmit state: gd0 is output
  standby(); 
  if (_gd0_tx!=nullptr) _gd0_tx->setup();

  init_state|=radio.transmitDirectAsync();
  state=init_state==0 ? CC1101_XMIT : CC1101_NOINIT;

  return init_state;
}

void RadiolibCC1101Component::adjustBW(float bandwidth) {
  // set to a valid value
  float possibles[16] = {58, 68, 81, 102, 116, 135, 162, 203, 232, 270, 325, 406, 464, 541, 650, 812};
  for(int i=0;i<15;i++) {
    if ((bandwidth>=possibles[i])&&(bandwidth<=possibles[i+1])) {
      _bandwidth=bandwidth-possibles[i]<possibles[i+1]-bandwidth ? possibles[i] : possibles[i+1];
      break;
    }
  }
}

float RadiolibCC1101Component::getRSSI() {
  return state==CC1101_RECV ? radio.getRSSI() : -1;
}

}  // namespace radiolib_cc1101
}  // namespace esphome
