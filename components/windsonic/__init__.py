import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor, text_sensor, binary_sensor
from esphome.const import (
    CONF_ADDRESS,
    CONF_ID,
    CONF_TIMEOUT,
    ENTITY_CATEGORY_DIAGNOSTIC,
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_WIND_DIRECTION,
    DEVICE_CLASS_WIND_SPEED,
    STATE_CLASS_MEASUREMENT,
    UNIT_DEGREES,
    UNIT_METER_PER_SECOND,
)

MULTI_CONF = False
AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor"]

CONF_DATA_PIN = "data_pin"
CONF_POWER_PIN = "power_pin"
CONF_RAW_RESPONSE = "raw_response"
CONF_STATUS = "status"
CONF_SPEED = "speed"
CONF_DIRECTION = "direction"

windsonic_ns = cg.esphome_ns.namespace("windsonic")
WindSonicComponent = windsonic_ns.class_(
    "WindSonicComponent",
    cg.PollingComponent,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WindSonicComponent),
            cv.Optional(CONF_ADDRESS, default="0"): cv.string,
            cv.Required(CONF_DATA_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_POWER_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TIMEOUT, default="500ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_RAW_RESPONSE): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_STATUS): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_DIRECTION): sensor.sensor_schema(
                unit_of_measurement=UNIT_DEGREES,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_WIND_DIRECTION,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SPEED): sensor.sensor_schema(
                unit_of_measurement=UNIT_METER_PER_SECOND,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_WIND_SPEED,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    ).extend(cv.polling_component_schema("60s")),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    data_pin = await cg.gpio_pin_expression(config[CONF_DATA_PIN])
    cg.add(var.set_data_pin(data_pin))

    if CONF_POWER_PIN in config:
        power_pin = await cg.gpio_pin_expression(config[CONF_POWER_PIN])
        cg.add(var.set_power_pin(power_pin))

    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_timeout(config[CONF_TIMEOUT].total_milliseconds))

    if CONF_RAW_RESPONSE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_RAW_RESPONSE])
        cg.add(var.set_raw_response_sensor(sens))

    if CONF_STATUS in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_STATUS])
        cg.add(var.set_status_sensor(sens))

    if CONF_DIRECTION in config:
        sens = await sensor.new_sensor(config[CONF_DIRECTION])
        cg.add(var.set_direction_sensor(sens))

    if CONF_SPEED in config:
        sens = await sensor.new_sensor(config[CONF_SPEED])
        cg.add(var.set_speed_sensor(sens))

    cg.add_library("SDI-12", "2.3.2")
