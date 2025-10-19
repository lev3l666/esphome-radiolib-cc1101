#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/core/automation.h"  // 🔹 para on_packet automations


#define RADIOLIB_LOW_LEVEL 1
#include "EHRLHal.h"

namespace esphome {
namespace radiolib_cc1101 {

enum CC1101_state {CC1101_NOINIT,CC1101_STANDBY,CC1101_RECV,CC1101_XMIT};
enum CC1101Modulation {OOK_MODULATION=0, FSK_MODULATION};

class RadiolibCC1101Component : public Component, public spi::SPIDevice, public EH_RL_SPI {
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
    void set_modulation(CC1101Modulation modulation) { _modulation=modulation; }
    void set_filter(float filter) { _bandwidth=filter/1e3; }
    void set_bitrate(float bitrate) { _bitrate=bitrate; }
    void set_reg_agcctrl0(uint8_t reg_agcctrl0) { _REG_AGCCTRL0 = reg_agcctrl0; }
    void set_reg_agcctrl1(uint8_t reg_agcctrl1) { _REG_AGCCTRL1 = reg_agcctrl1; }
    void set_reg_agcctrl2(uint8_t reg_agcctrl2) { _REG_AGCCTRL2 = reg_agcctrl2; }
    void set_registers();
    void setup_direct_mode();
    float getRSSI();

    EH_RL_Hal* hal;
    CC1101* radio;
    int init_state=0;
    CC1101_state state=CC1101_NOINIT;
    float _freq=433.92;
    CC1101Modulation _modulation=OOK_MODULATION;
    float _bandwidth=464;
    
    // these are bandwidth specific
    uint8_t _REG_FREND1=0xb6;
    uint8_t _REG_TEST2=0x88;
    uint8_t _REG_TEST1=0x31;
    uint8_t _REG_FIFOTHR=0x07;

    // AGC settings from: LSatan/SmartRC-CC1101-Driver-Lib
    uint8_t _REG_AGCCTRL2=0xc7;
    uint8_t _REG_AGCCTRL1=0x00;
    uint8_t _REG_AGCCTRL0=0xb2;

    int _bitrate=5;

    InternalGPIOPin* _gd0_rx=nullptr;
    InternalGPIOPin* _gd0_tx=nullptr;

    float last_rx_rssi=0;

    void log_status_(const char *msg) {
      ESP_LOGD("cc1101", "[%s] Freq: %.2f MHz, Bitrate: %d kbps, RSSI: %.1f dBm", 
               msg, _freq, _bitrate, last_rx_rssi);
    }

// 🔹 Nuevo: soporte de callback tipo automation
void set_on_packet_callback(Trigger<> *trigger) { this->on_packet_trigger_ = trigger; }
Trigger<> *get_on_packet_trigger() { return this->on_packet_trigger_; }  // ✅ añadido

protected:
  Trigger<> *on_packet_trigger_{nullptr};

  private:
    void adjustBW(float bandwidth); // rx filter bw snapper
};

}  // namespace radiolib_cc1101
}  // namespace esphome
