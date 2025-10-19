import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome import pins
from esphome.const import *
from esphome.components import event
from esphome.cpp_helpers import gpio_pin_expression
from esphome.core import CORE
from esphome import automation
import os

DEPENDENCIES = ["spi"]
MULTI_CONF = True

radiolib_cc1101_ns = cg.esphome_ns.namespace("radiolib_cc1101")
RadiolibCC1101Component = radiolib_cc1101_ns.class_(
    "RadiolibCC1101Component", cg.Component, spi.SPIDevice
)

# Configuration keys
CONF_MODULATION = "modulation"
CONF_FILTER = "filter"
CONF_ON_PACKET = "on_packet"

# Modulation enum mapping (will be converted to the C++ enum values)
CC1101Modulation = radiolib_cc1101_ns.enum("CC1101Modulation")
CC1101_MODULATIONS = {
    "OOK": CC1101Modulation.OOK_MODULATION,
    "FSK": CC1101Modulation.FSK_MODULATION,
}

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(RadiolibCC1101Component),
        cv.Optional(CONF_RX_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_FREQUENCY, default="433.92MHz"): cv.frequency,
        # allow strings like "ook" or "FSK" (cv.enum will normalize when upper=True)
        cv.Optional(CONF_MODULATION, default="ook"): cv.enum(CC1101_MODULATIONS, upper=True, space="_"),
        cv.Optional(CONF_FILTER, default="464kHz"): cv.frequency,
        cv.Optional('bitrate', default=5): cv.float_range(0.025, 600),  # units: k? user-defined, kept as before
        cv.Optional('reg_agcctrl0', default=0xb2): cv.hex_uint8_t,
        cv.Optional('reg_agcctrl1', default=0x00): cv.hex_uint8_t,
        cv.Optional('reg_agcctrl2', default=0xc7): cv.hex_uint8_t,
        cv.Optional(CONF_ON_PACKET): automation.validate_automation(single=True),
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)


async def to_code(config):

    # When using Arduino, RadioLib expects SPI.h to be available.
    # We add RadioLib and a small pre-build script helper when building with Arduino.
    if CORE.using_arduino:
        prescript = os.path.join(os.path.dirname(__file__), "hack_radiolib_buildopt.py")
        cg.add_platformio_option("extra_scripts", [f"pre:{prescript}"])

    # Ensure PlatformIO will pull RadioLib
    cg.add_library("RadioLib", None)

    # Create the C++ component instance
    var = cg.new_Pvariable(config[CONF_ID])

    # Optionally configure RX pin (GDO0)
    if CONF_RX_PIN in config:
        pin = await gpio_pin_expression(config[CONF_RX_PIN])
        cg.add(var.set_rx_pin(pin))

    # Register scalar configuration parameters with the C++ object
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_modulation(config[CONF_MODULATION]))
    cg.add(var.set_filter(config[CONF_FILTER]))
    cg.add(var.set_bitrate(config['bitrate']))
    cg.add(var.set_reg_agcctrl0(config['reg_agcctrl0']))
    cg.add(var.set_reg_agcctrl1(config['reg_agcctrl1']))
    cg.add(var.set_reg_agcctrl2(config['reg_agcctrl2']))

    # Register as a normal ESPHome component and as an SPI device (cs_pin comes from spi_device_schema)
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    # Optional on_packet automation (YAML trigger)
    if CONF_ON_PACKET in config:
        trigger = await automation.build_automation(
            var,
            [(cg.std_string, "data")],
            config[CONF_ON_PACKET],
        )
        cg.add(var.set_on_packet_callback(trigger))
