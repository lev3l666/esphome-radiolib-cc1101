# wip
Uses [Radiolib](https://github.com/jgromes/RadioLib) to enable CC1101 module as Esphome RF remote receiver/transmitter.
# current example
- Use rtl_433 to decode pulses streamed to mqtt:
```stdbuf -o0 mosquitto_sub  -h mqtthostname -I rx -t 'esphome/rawrf/#' | rtl_433 -r ook:-```
# todo
- Make it a proper [external compoenent](https://esphome.io/components/external_components ).
- get transmit working
- Create radiolib hal to use ESPHome SPI/hal.
- verify esp8266 works
- add sensors and controls
# based on:
- https://github.com/dbuezas/esphome-cc1101
- and https://github.com/NorthernMan54/rtl_433_ESP/blob/main/src/rtl_433_ESP.cpp
- and https://github.com/smartoctopus/RadioLib-esphome/blob/master/examples/CC1101/CC1101_Settings/CC1101_Settings.ino
- and https://github.com/LSatan/SmartRC-CC1101-Driver-Lib
