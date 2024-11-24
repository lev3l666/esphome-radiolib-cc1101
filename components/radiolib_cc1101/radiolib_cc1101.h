#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"


#define RADIOLIB_LOW_LEVEL 1
#include "EHRLHal.h"

namespace esphome {
namespace radiolib_cc1101 {

class RadiolibCC1101Component : public Component, 
                    public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,spi::CLOCK_POLARITY_LOW, 
                            spi::CLOCK_PHASE_LEADING,spi::DATA_RATE_2MHZ> {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    int standby();
    int xmit();
    int recv();
    int setBW(float bandwidth);
    int setFreq(float freq);
    void set_rx_pin(InternalGPIOPin *rx_pin) { _gd0_rx = rx_pin; }
    void set_tx_pin(InternalGPIOPin *tx_pin) { _gd0_rx = tx_pin; }
    void set_frequency(float freq) { _freq=freq/1e6; }
    void set_filter(float filter) { _bandwidth=filter/1e3; }

    EH_RL_Hal* hal;
    CC1101 radio=NULL;
    float _freq;
    float _bandwidth=464;

    InternalGPIOPin* _gd0_rx;
    InternalGPIOPin* _gd0_tx;
};


}  // namespace empty_spi_component
}  // namespace esphome
