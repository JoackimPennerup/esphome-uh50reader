#include "uh50_reader.h"

#include <cstdlib>
#include <cstring>

#include "esphome/core/log.h"

namespace esphome {
namespace uh50_reader {

static const char *const TAG = "uh50_reader";

static const uint8_t DATA_CMD[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '/',  '#',  '!',  0x0D, 0x0A,
};

UH50Reader::UH50Reader(uart::UARTComponent *uart_in, uint32_t update_interval_ms)
    : PollingComponent(update_interval_ms), UARTDevice(uart_in) {}

void UH50Reader::setup() {
  this->send_data_cmd_();
  // Keep initial behavior close to legacy implementation: first full read shortly after boot.
  this->set_timeout(60000, [this]() { this->read_meter(); });
}

void UH50Reader::update() { this->read_meter(); }

float UH50Reader::get_setup_priority() const { return setup_priority::DATA; }

void UH50Reader::read_meter() {
  this->send_data_cmd_();
  this->read_telegram_();
}

void UH50Reader::send_data_cmd_() {
  if (this->uart_out_ == nullptr) {
    ESP_LOGW(TAG, "TX UART not configured, cannot send data command");
    return;
  }

  for (auto b : DATA_CMD) {
    this->uart_out_->write_byte(b);
  }
  this->uart_out_->flush();
  ESP_LOGI(TAG, "data cmd sent");
}

void UH50Reader::publish_sensors_(OBISData *od, int count) {
  for (int i = 0; i < count; i++) {
    if (!strcmp(od[i].obis_code, "6.8") && this->cumulative_active_import_sensor_ != nullptr) {
      this->cumulative_active_import_sensor_->publish_state(atof(od[i].value));
    } else if (!strcmp(od[i].obis_code, "6.26") && this->cumulative_volume_sensor_ != nullptr) {
      this->cumulative_volume_sensor_->publish_state(atof(od[i].value));
    } else if (!strcmp(od[i].obis_code, "6.4") && this->current_power_sensor_ != nullptr) {
      this->current_power_sensor_->publish_state(atof(od[i].value));
    } else if (!strcmp(od[i].obis_code, "6.27") && this->flow_rate_sensor_ != nullptr) {
      this->flow_rate_sensor_->publish_state(atof(od[i].value));
    } else if (!strcmp(od[i].obis_code, "6.29") && this->temperature_flow_sensor_ != nullptr) {
      this->temperature_flow_sensor_->publish_state(atof(od[i].value));
    } else if (!strcmp(od[i].obis_code, "6.28") && this->temperature_return_sensor_ != nullptr) {
      this->temperature_return_sensor_->publish_state(atof(od[i].value));
    } else if (!strcmp(od[i].obis_code, "6.30") && this->temperature_diff_sensor_ != nullptr) {
      this->temperature_diff_sensor_->publish_state(atof(od[i].value));
    }
  }
}

void UH50Reader::read_telegram_() {
  OBISData obis_data[MAX_OBIS_CODES];

  uint8_t b = 0x00;
  while (this->available() && b != 0x02) {
    b = this->read();
  }

  while (int len = this->available()) {
    if (len >= static_cast<int>(sizeof(this->buffer_))) {
      len = static_cast<int>(sizeof(this->buffer_) - 1);
    }

    ESP_LOGD(TAG, "Got %d bytes available to read", len);
    if (!this->read_array(reinterpret_cast<uint8_t *>(this->buffer_), len)) {
      ESP_LOGW(TAG, "read_array() returned false, meter reading may be incomplete");
    }
    this->buffer_[len] = '\0';
    ESP_LOGD(TAG, "Read %s", this->buffer_);

    int count = 0;
    parse_obis(this->buffer_, obis_data, &count);
    print_parsed_data(obis_data, count);
    this->publish_sensors_(obis_data, count);

    memset(this->buffer_, 0, sizeof(this->buffer_));
  }
}

}  // namespace uh50_reader
}  // namespace esphome