#include "esphome/core/log.h"
#include "radiolib_cc1101.h"

namespace esphome {
namespace radiolib_cc1101 {

static const char *TAG = "radiolib_cc1101.component";

void RadiolibCC1101Component::setup() {
  ESP_LOGI(TAG, "Initializing CC1101 via SPI...");

  this->spi_setup();
  hal = new EH_RL_Hal(this);

  auto *mod = new Module(hal, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC);
  radio = new CC1101(mod);

  ESP_LOGD(TAG, "Calling CC1101.begin()...");
  init_state = radio->begin();

  if (init_state == RADIOLIB_ERR_NONE) {
    ESP_LOGI(TAG, "CC1101 initialized successfully!");
  } else {
    ESP_LOGE(TAG, "CC1101 init failed, code: %d", init_state);
    state = CC1101_NOINIT;
    return;
  }

  setup_direct_mode();
  ESP_LOGI(TAG, "CC1101 setup complete.");
  state = CC1101_STANDBY;
}

void RadiolibCC1101Component::loop() {
  if (state == CC1101_RECV && _gd0_rx != nullptr) {
    bool signal = _gd0_rx->digital_read();
    if (signal) {
      float rssi = getRSSI();
      if (rssi != -1 && rssi != 0) {
        last_rx_rssi = (last_rx_rssi * 0.8f) + (rssi * 0.2f);
        ESP_LOGD(TAG, "RF Activity detected: RSSI %.1f dBm", last_rx_rssi);

        // 🔹 Disparar evento on_packet (si hay actividad de RF válida)
        if (this->on_packet_trigger_ != nullptr) {
          this->on_packet_trigger_->trigger();
        }
      }
    }
  }
}

void RadiolibCC1101Component::dump_config() {
  ESP_LOGCONFIG(TAG, "RadioLib CC1101 Component");
  ESP_LOGCONFIG(TAG, "  Frequency: %.2f MHz", _freq);
  ESP_LOGCONFIG(TAG, "  Bitrate: %d kbps", _bitrate);
  ESP_LOGCONFIG(TAG, "  Bandwidth: %.0f kHz", _bandwidth);
  ESP_LOGCONFIG(TAG, "  Modulation: %s", (_modulation == OOK_MODULATION) ? "OOK" : "FSK");
  ESP_LOGCONFIG(TAG, "  State: %d", state);
  log_status_("Init");
}


void RadiolibCC1101Component::set_registers() {
  init_state |= radio->setFrequency(_freq);
  init_state |= radio->setBitRate(_bitrate);
  adjustBW(_bandwidth);
  init_state |= radio->setRxBandwidth(_bandwidth);

  radio->SPIsetRegValue(RADIOLIB_CC1101_REG_FREND1, _REG_FREND1);
  radio->SPIsetRegValue(RADIOLIB_CC1101_REG_TEST2, _REG_TEST2);
  radio->SPIsetRegValue(RADIOLIB_CC1101_REG_TEST1, _REG_TEST1);
  radio->SPIsetRegValue(RADIOLIB_CC1101_REG_FIFOTHR, _REG_FIFOTHR);
  radio->SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL2, _REG_AGCCTRL2);
  radio->SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL1, _REG_AGCCTRL1);
  radio->SPIsetRegValue(RADIOLIB_CC1101_REG_AGCCTRL0, _REG_AGCCTRL0);

  ESP_LOGD(TAG, "Registers configured (freq=%.2fMHz, bitrate=%dkbps, bw=%.0fkHz)",
           _freq, _bitrate, _bandwidth);
}

void RadiolibCC1101Component::setup_direct_mode() {
  standby();

  _REG_FREND1 = (_bandwidth > 101) ? 0xb6 : 0x56;
  _REG_TEST2  = (_bandwidth > 325) ? 0x88 : 0x81;
  _REG_TEST1  = (_bandwidth > 325) ? 0x31 : 0x35;
  _REG_FIFOTHR= (_bandwidth > 325) ? 0x07 : 0x47;

  set_registers();

  if (_modulation == OOK_MODULATION) {
    init_state |= radio->setOOK(true);
    ESP_LOGI(TAG, "OOK modulation enabled");
  } else {
    init_state |= radio->setModulation(RADIOLIB_CC1101_MOD_2_FSK);
    ESP_LOGI(TAG, "FSK modulation enabled");
  }


  recv();
}

int RadiolibCC1101Component::standby() {
  init_state |= radio->standby();
  state = (init_state == 0) ? CC1101_STANDBY : CC1101_NOINIT;
  return init_state;
}

int RadiolibCC1101Component::recv() {
  if (state == CC1101_XMIT)
    standby();

  init_state |= radio->receiveDirectAsync();
  state = (init_state == 0) ? CC1101_RECV : CC1101_NOINIT;
  ESP_LOGI(TAG, "Radio in receive mode (direct async)");
  return init_state;
}

int RadiolibCC1101Component::xmit() {
  standby();
  init_state |= radio->transmitDirectAsync();
  state = (init_state == 0) ? CC1101_XMIT : CC1101_NOINIT;
  ESP_LOGI(TAG, "Radio in transmit mode (direct async)");
  return init_state;
}

void RadiolibCC1101Component::adjustBW(float bandwidth) {
  static const float possibles[16] = {58, 68, 81, 102, 116, 135, 162, 203, 232, 270, 325, 406, 464, 541, 650, 812};
  for (int i = 0; i < 15; i++) {
    if (bandwidth >= possibles[i] && bandwidth <= possibles[i + 1]) {
      _bandwidth = (bandwidth - possibles[i] < possibles[i + 1] - bandwidth)
                       ? possibles[i]
                       : possibles[i + 1];
      break;
    }
  }
}

float RadiolibCC1101Component::getRSSI() {
  return (state == CC1101_RECV && radio != nullptr) ? radio->getRSSI() : -1;
}

}  // namespace radiolib_cc1101
}  // namespace esphome
