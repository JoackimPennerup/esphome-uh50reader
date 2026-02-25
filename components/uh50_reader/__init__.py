import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UPDATE_INTERVAL

from . import sensor as uh50_sensor

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor"]

CONF_UART_IN_ID = "uart_in_id"
CONF_UART_OUT_ID = "uart_out_id"
CONF_SENSORS = "sensors"

uh50_reader_ns = cg.esphome_ns.namespace("uh50_reader")
UH50Reader = uh50_reader_ns.class_("UH50Reader", cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(UH50Reader),
        cv.Required(CONF_UART_IN_ID): cv.use_id(uart.UARTComponent),
        cv.Required(CONF_UART_OUT_ID): cv.use_id(uart.UARTComponent),
        cv.Required(CONF_SENSORS): uh50_sensor.SENSORS_SCHEMA,
    }
).extend(cv.polling_component_schema("30min"))


async def to_code(config):
    uart_in = await cg.get_variable(config[CONF_UART_IN_ID])
    uart_out = await cg.get_variable(config[CONF_UART_OUT_ID])

    var = cg.new_Pvariable(config[CONF_ID], uart_in, config[CONF_UPDATE_INTERVAL])
    cg.add(var.set_uart_out(uart_out))

    await cg.register_component(var, config)
    await uh50_sensor.setup_sensors(var, config[CONF_SENSORS])
