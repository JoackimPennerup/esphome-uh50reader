#pragma once

#include "esphome/components/button/button.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "obis.h"

namespace esphome {
namespace uh50_reader {

class UH50Reader : public PollingComponent, public uart::UARTDevice {
 public:
  UH50Reader(uart::UARTComponent *uart_in, uint32_t update_interval_ms);

  void set_uart_out(uart::UARTComponent *uart_out) { uart_out_ = uart_out; }
  void set_has_read_button(bool has_read_button) { has_read_button_ = has_read_button; }

  void set_cumulative_active_import_sensor(sensor::Sensor *sensor) { cumulative_active_import_sensor_ = sensor; }
  void set_cumulative_volume_sensor(sensor::Sensor *sensor) { cumulative_volume_sensor_ = sensor; }
  void set_current_power_sensor(sensor::Sensor *sensor) { current_power_sensor_ = sensor; }
  void set_flow_rate_sensor(sensor::Sensor *sensor) { flow_rate_sensor_ = sensor; }
  void set_temperature_flow_sensor(sensor::Sensor *sensor) { temperature_flow_sensor_ = sensor; }
  void set_temperature_return_sensor(sensor::Sensor *sensor) { temperature_return_sensor_ = sensor; }
  void set_temperature_diff_sensor(sensor::Sensor *sensor) { temperature_diff_sensor_ = sensor; }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void read_meter();

 protected:
  void send_data_cmd_();
  void read_telegram_();
  void publish_sensors_(OBISData *od, int count);

  uart::UARTComponent *uart_out_{nullptr};
  bool has_read_button_{false};
  char buffer_[2500]{0};

  sensor::Sensor *cumulative_active_import_sensor_{nullptr};
  sensor::Sensor *cumulative_volume_sensor_{nullptr};
  sensor::Sensor *current_power_sensor_{nullptr};
  sensor::Sensor *flow_rate_sensor_{nullptr};
  sensor::Sensor *temperature_flow_sensor_{nullptr};
  sensor::Sensor *temperature_return_sensor_{nullptr};
  sensor::Sensor *temperature_diff_sensor_{nullptr};
};

template<typename... Ts> class UH50ReadAction : public Action<Ts...>, public Parented<UH50Reader> {
 public:
  void play(Ts... x) override { this->parent_->read_meter(); }
};

class UH50ReadButton : public button::Button, public Parented<UH50Reader> {
 protected:
  void press_action() override { this->parent_->read_meter(); }
};

}  // namespace uh50_reader
}  // namespace esphome
