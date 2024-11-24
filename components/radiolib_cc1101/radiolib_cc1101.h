#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"


#define RADIOLIB_LOW_LEVEL 1
#include "EHRLHal.h"

namespace esphome {
namespace radiolib_cc1101 {

class RadiolibCC1101Component : public Component, public EH_RL_SPI {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    int standby();
    int xmit();
    int recv();
    void set_rx_pin(InternalGPIOPin *rx_pin) { _gd0_rx = rx_pin; }
    void set_tx_pin(InternalGPIOPin *tx_pin) { _gd0_tx = tx_pin; }
    void set_frequency(float freq) { _freq=freq/1e6; }
    void set_filter(float filter) { _bandwidth=filter/1e3; }
    void set_bitrate(float bitrate) { _bitrate=bitrate; }
    void set_registers();
    void setup_direct_mode(bool adjustregisters=true);

    EH_RL_Hal* hal;
    CC1101 radio=NULL;
    int state=0;
    float _freq=433.92;
    float _bandwidth=464;

    int _REG_FREND1=0xb6;
    int _REG_TEST2=0x88;
    int _REG_TEST1=0x31;
    int _REG_FIFOTHR=0x07;
    int _REG_AGCCTRL2=0xc7;
    int _REG_AGCCTRL1=0x00;
    int _REG_AGCCTRL0=0xb2;

  // datarate seems to be very crtitical to be around 5.0k sometimes, but 40k or 100k seems to make more sense
  // -- intertwined with AGC settings from smartRC below
  // see also DN022: https://www.ti.com/lit/an/swra215e/swra215e.pdf
    int _bitrate=40;

    InternalGPIOPin* _gd0_rx;
    InternalGPIOPin* _gd0_tx;

  private:
    void adjustBW(float bandwidth); // rx filter bw snapper

};


}  // namespace empty_spi_component
}  // namespace esphome
