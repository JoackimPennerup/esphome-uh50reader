import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import button, sensor, uart
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
)

from . import UH50Reader, uh50_reader_ns

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["button"]

CONF_UART_IN_ID = "uart_in_id"
CONF_UART_OUT_ID = "uart_out_id"
CONF_CUMULATIVE_ACTIVE_IMPORT = "cumulative_active_import"
CONF_CUMULATIVE_VOLUME = "cumulative_volume"
CONF_CURRENT_POWER = "current_power"
CONF_FLOW_RATE = "flow_rate"
CONF_TEMPERATURE_FLOW = "temperature_flow"
CONF_TEMPERATURE_RETURN = "temperature_return"
CONF_TEMPERATURE_DIFF = "temperature_diff"
CONF_READ_BUTTON = "read_button"
CONF_STARTUP_READ_DELAY = "startup_read_delay"

UH50ReadAction = uh50_reader_ns.class_("UH50ReadAction", automation.Action)
UH50ReadButton = uh50_reader_ns.class_("UH50ReadButton", button.Button)

ENERGY_SENSOR_SCHEMA = sensor.sensor_schema(
    accuracy_decimals=3,
    state_class=STATE_CLASS_TOTAL_INCREASING,
    device_class=DEVICE_CLASS_ENERGY,
)

POWER_SENSOR_SCHEMA = sensor.sensor_schema(
    accuracy_decimals=2,
    state_class=STATE_CLASS_MEASUREMENT,
    device_class=DEVICE_CLASS_ENERGY,
)

FLOW_SENSOR_SCHEMA = sensor.sensor_schema(
    accuracy_decimals=3,
    state_class=STATE_CLASS_MEASUREMENT,
)

TEMP_SENSOR_SCHEMA = sensor.sensor_schema(
    accuracy_decimals=2,
    state_class=STATE_CLASS_MEASUREMENT,
    device_class=DEVICE_CLASS_TEMPERATURE,
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UH50Reader),
        cv.Required(CONF_UART_IN_ID): cv.use_id(uart.UARTComponent),
        cv.Required(CONF_UART_OUT_ID): cv.use_id(uart.UARTComponent),
        cv.Optional(CONF_READ_BUTTON): button.button_schema(UH50ReadButton, icon="mdi:gauge"),
        cv.Optional(CONF_STARTUP_READ_DELAY, default="60s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_CUMULATIVE_ACTIVE_IMPORT): ENERGY_SENSOR_SCHEMA,
        cv.Optional(CONF_CUMULATIVE_VOLUME): sensor.sensor_schema(
            accuracy_decimals=3,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_CURRENT_POWER): POWER_SENSOR_SCHEMA,
        cv.Optional(CONF_FLOW_RATE): FLOW_SENSOR_SCHEMA,
        cv.Optional(CONF_TEMPERATURE_FLOW): TEMP_SENSOR_SCHEMA,
        cv.Optional(CONF_TEMPERATURE_RETURN): TEMP_SENSOR_SCHEMA,
        cv.Optional(CONF_TEMPERATURE_DIFF): TEMP_SENSOR_SCHEMA,
    }
).extend(cv.polling_component_schema("1min"))

UH50_READ_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.use_id(UH50Reader),
    }
)


@automation.register_action(
    "uh50_reader.read",
    UH50ReadAction,
    UH50_READ_ACTION_SCHEMA,
    synchronous=True,
)
async def uh50_read_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


async def to_code(config):
    uart_in = await cg.get_variable(config[CONF_UART_IN_ID])
    uart_out = await cg.get_variable(config[CONF_UART_OUT_ID])

    var = cg.new_Pvariable(config[CONF_ID], uart_in, config[CONF_UPDATE_INTERVAL])
    cg.add(var.set_uart_out(uart_out))
    cg.add(var.set_startup_read_delay_ms(config[CONF_STARTUP_READ_DELAY]))

    await cg.register_component(var, config)

    if read_button_config := config.get(CONF_READ_BUTTON):
        btn = await button.new_button(read_button_config)
        await cg.register_parented(btn, config[CONF_ID])
        cg.add(var.set_has_read_button(True))

    if CONF_CUMULATIVE_ACTIVE_IMPORT in config:
        sens = await sensor.new_sensor(config[CONF_CUMULATIVE_ACTIVE_IMPORT])
        cg.add(var.set_cumulative_active_import_sensor(sens))
    if CONF_CUMULATIVE_VOLUME in config:
        sens = await sensor.new_sensor(config[CONF_CUMULATIVE_VOLUME])
        cg.add(var.set_cumulative_volume_sensor(sens))
    if CONF_CURRENT_POWER in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT_POWER])
        cg.add(var.set_current_power_sensor(sens))
    if CONF_FLOW_RATE in config:
        sens = await sensor.new_sensor(config[CONF_FLOW_RATE])
        cg.add(var.set_flow_rate_sensor(sens))
    if CONF_TEMPERATURE_FLOW in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_FLOW])
        cg.add(var.set_temperature_flow_sensor(sens))
    if CONF_TEMPERATURE_RETURN in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_RETURN])
        cg.add(var.set_temperature_return_sensor(sens))
    if CONF_TEMPERATURE_DIFF in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_DIFF])
        cg.add(var.set_temperature_diff_sensor(sens))
