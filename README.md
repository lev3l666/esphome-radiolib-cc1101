# esphome-radiolib-cc1101
Uses [Radiolib](https://github.com/jgromes/RadioLib) to enable a TI-CC1101 module as an ESPHome RF direct OOK remote receiver/transmitter.
# Basics
The included yaml files include configuration examples for ESP8266 and ESP32 micro-controllers.  You should start by enabling dump_raw and making sure noise isn't being received when no transmissions are occurring.  Adjusting bandwidth, data-rate, and AGC parameters can help with creating an optimal receive setup.  The example includes the ability to expose these as controls in ESPHome or HomeAssistant.  
- More bandwidth will likely enable receiving more varying devices on the same frequency, but will increase noise and may decrease range.

## debugging signals
The ESP32 example includes an example UDP dumper that can help with analyzing pulse data. 
- Use rtl_433 on a host with the socat tool to decode pulses streamed to udp broadcast:
```socat -u UDP4-RECV:5009 STDOUT | rtl_433 -r ook:-```

## rtl_433 decoding with ESPHome directly
See [esphome-rtl_433-decoder](https://github.com/juanboro/esphome-rtl_433-decoder)

## notes
- Transmit/Receive was verified to work with esp8266 and ESP-32 board.
    - I cannot currently get transmit to reliably work with the Arduiono platform on esp32.  Some have gotten this to work by setting remote_transmit/receive setup ordering and several other workarounds and these at one point worked for me - but with the latest ESPhome releases I don't see them working (the older remote transmit module appears to me to always drive zero and not an open-drain output)  Current suggestions: If you need to have both transmit and receive on ESP-32 platforms, use the esp-idf platform, if you need only receive, then you can use either platform.  If you must have transmit and the arduiono platform - more work will likely be required (see the Radiolib code - you can put the RX data on GDO2 and keep the RX and TX pins seperate)

# see also:
 - Direct cc1101 support in ESPHOME component (no radiolib dependency): https://github.com/esphome/esphome/pull/6300
# based on:
- https://github.com/dbuezas/esphome-cc1101
- and https://github.com/NorthernMan54/rtl_433_ESP/blob/main/src/rtl_433_ESP.cpp
- and https://github.com/smartoctopus/RadioLib-esphome/blob/master/examples/CC1101/CC1101_Settings/CC1101_Settings.ino
- and https://github.com/LSatan/SmartRC-CC1101-Driver-Lib
