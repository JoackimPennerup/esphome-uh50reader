import esphome.codegen as cg
from esphome.components import uart

uh50_reader_ns = cg.esphome_ns.namespace("uh50_reader")
UH50Reader = uh50_reader_ns.class_("UH50Reader", cg.PollingComponent, uart.UARTDevice)
