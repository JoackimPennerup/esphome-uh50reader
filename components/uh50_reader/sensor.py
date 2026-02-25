import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor

CONF_CUMULATIVE_ACTIVE_IMPORT = "cumulative_active_import"
CONF_CUMULATIVE_VOLUME = "cumulative_volume"
CONF_CURRENT_POWER = "current_power"
CONF_FLOW_RATE = "flow_rate"
CONF_TEMPERATURE_FLOW = "temperature_flow"
CONF_TEMPERATURE_RETURN = "temperature_return"
CONF_TEMPERATURE_DIFF = "temperature_diff"

SENSORS_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_CUMULATIVE_ACTIVE_IMPORT): sensor.sensor_schema(),
        cv.Required(CONF_CUMULATIVE_VOLUME): sensor.sensor_schema(),
        cv.Required(CONF_CURRENT_POWER): sensor.sensor_schema(),
        cv.Required(CONF_FLOW_RATE): sensor.sensor_schema(),
        cv.Required(CONF_TEMPERATURE_FLOW): sensor.sensor_schema(),
        cv.Required(CONF_TEMPERATURE_RETURN): sensor.sensor_schema(),
        cv.Required(CONF_TEMPERATURE_DIFF): sensor.sensor_schema(),
    }
)


async def setup_sensors(var, config):
    cumulative_active_import = await sensor.new_sensor(config[CONF_CUMULATIVE_ACTIVE_IMPORT])
    cg.add(var.set_cumulative_active_import_sensor(cumulative_active_import))

    cumulative_volume = await sensor.new_sensor(config[CONF_CUMULATIVE_VOLUME])
    cg.add(var.set_cumulative_volume_sensor(cumulative_volume))

    current_power = await sensor.new_sensor(config[CONF_CURRENT_POWER])
    cg.add(var.set_current_power_sensor(current_power))

    flow_rate = await sensor.new_sensor(config[CONF_FLOW_RATE])
    cg.add(var.set_flow_rate_sensor(flow_rate))

    temperature_flow = await sensor.new_sensor(config[CONF_TEMPERATURE_FLOW])
    cg.add(var.set_temperature_flow_sensor(temperature_flow))

    temperature_return = await sensor.new_sensor(config[CONF_TEMPERATURE_RETURN])
    cg.add(var.set_temperature_return_sensor(temperature_return))

    temperature_diff = await sensor.new_sensor(config[CONF_TEMPERATURE_DIFF])
    cg.add(var.set_temperature_diff_sensor(temperature_diff))
