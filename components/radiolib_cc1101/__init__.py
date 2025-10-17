import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome import pins, automation
from esphome.const import *
from esphome.cpp_helpers import gpio_pin_expression
from esphome.core import CORE
import os

DEPENDENCIES = ["spi"]

MULTI_CONF = True

radiolib_cc1101_ns = cg.esphome_ns.namespace("radiolib_cc1101")
RadiolibCC1101Component = radiolib_cc1101_ns.class_(
    "RadiolibCC1101Component", cg.Component, spi.SPIDevice
)

# --- Add a trigger for "on_packet" ---
RadiolibCC1101PacketTrigger = radiolib_cc1101_ns.class_(
    "RadiolibCC1101PacketTrigger", automation.Trigger.template()
)

CONF_MODULATION = "modulation"
CC1101Modulation = radiolib_cc1101_ns.enum("CC1101Modulation")
CC1101_MODULATIONS = {
    "OOK": CC1101Modulation.OOK_MODULATION,
    "FSK": CC1101Modulation.FSK_MODULATION,
}

CONF_ON_PACKET = "on_packet"

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(RadiolibCC1101Component),
        cv.Optional(CONF_RX_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_FREQUENCY, default="433.92MHz"): cv.frequency,
        cv.Optional(CONF_MODULATION, default="ook"): cv.enum(CC1101_MODULATIONS, upper=True, space="_"),
        cv.Optional(CONF_FILTER, default="464kHz"): cv.frequency,
        cv.Optional('bitrate',default=5): cv.float_range(0.025,600),
        cv.Optional('reg_agcctrl0',default=0xb2): cv.hex_uint8_t,
        cv.Optional('reg_agcctrl1',default=0x00): cv.hex_uint8_t,
        cv.Optional('reg_agcctrl2',default=0xc7): cv.hex_uint8_t,
        cv.Optional(CONF_ON_PACKET): automation.validate_automation(single=True),
        })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)

async def to_code(config):

    if CORE.using_arduino:
        prescript=os.path.join(os.path.dirname(__file__), "hack_radiolib_buildopt.py")
        cg.add_platformio_option("extra_scripts", [f"pre:{prescript}"])

    cg.add_library("RadioLib", None)

    var = cg.new_Pvariable(config[CONF_ID])

    if CONF_RX_PIN in config:
        pin = await gpio_pin_expression(config[CONF_RX_PIN])
        cg.add(var.set_rx_pin(pin))

    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_modulation(config[CONF_MODULATION]))
    cg.add(var.set_filter(config[CONF_FILTER]))
    cg.add(var.set_bitrate(config['bitrate']))
    cg.add(var.set_reg_agcctrl0(config['reg_agcctrl0']))
    cg.add(var.set_reg_agcctrl1(config['reg_agcctrl1']))
    cg.add(var.set_reg_agcctrl2(config['reg_agcctrl2']))

    # Register the "on_packet" automation
    if CONF_ON_PACKET in config:
        trigger = cg.new_Pvariable(config[CONF_ON_PACKET][CONF_TRIGGER_ID])
        await cg.register_component(trigger, config)
        await automation.build_automation(trigger, [], config[CONF_ON_PACKET])

        cg.add(var.set_on_packet_trigger(trigger))

    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
