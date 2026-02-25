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
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '/', '#', '!', 0x0D, 0x0A,
};

static const uint32_t START_TIMEOUT_MS = 3000;
static const uint32_t FRAME_TIMEOUT_MS = 6000;
static const uint32_t INTERBYTE_TIMEOUT_MS = 200;

UH50Reader::UH50Reader(uart::UARTComponent *uart_in, uint32_t update_interval_ms)
    : PollingComponent(update_interval_ms), UARTDevice(uart_in) {}

void UH50Reader::setup() {
  this->send_data_cmd_();
  this->set_timeout(60000, [this]() { this->read_meter(); });
}

void UH50Reader::update() { this->read_meter(); }

void UH50Reader::dump_config() {
  ESP_LOGCONFIG(TAG, "UH50 Reader:");
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", this->get_update_interval());
  ESP_LOGCONFIG(TAG, "  Read button: %s", this->has_read_button_ ? "enabled" : "disabled");
  ESP_LOGCONFIG(TAG, "  TX UART configured: %s", this->uart_out_ != nullptr ? "yes" : "no");

  LOG_SENSOR("  ", "cumulative_active_import", this->cumulative_active_import_sensor_);
  LOG_SENSOR("  ", "cumulative_volume", this->cumulative_volume_sensor_);
  LOG_SENSOR("  ", "current_power", this->current_power_sensor_);
  LOG_SENSOR("  ", "flow_rate", this->flow_rate_sensor_);
  LOG_SENSOR("  ", "temperature_flow", this->temperature_flow_sensor_);
  LOG_SENSOR("  ", "temperature_return", this->temperature_return_sensor_);
  LOG_SENSOR("  ", "temperature_diff", this->temperature_diff_sensor_);
}

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

  bool found_stx = false;
  uint32_t start = millis();
  while (millis() - start < START_TIMEOUT_MS) {
    if (this->available()) {
      const uint8_t b = this->read();
      if (b == 0x02) {
        found_stx = true;
        break;
      }
    }
    yield();
  }

  if (!found_stx) {
    ESP_LOGW(TAG, "Timed out waiting for STX");
    return;
  }

  size_t pos = 0;
  const uint32_t read_start = millis();
  uint32_t last_byte = millis();

  while (millis() - read_start < FRAME_TIMEOUT_MS && pos < sizeof(this->buffer_) - 1) {
    if (this->available()) {
      this->buffer_[pos++] = static_cast<char>(this->read());
      last_byte = millis();
      continue;
    }

    if (millis() - last_byte > INTERBYTE_TIMEOUT_MS) {
      break;
    }
    yield();
  }

  if (pos == 0) {
    ESP_LOGW(TAG, "No payload received after STX");
    return;
  }

  this->buffer_[pos] = '\0';
  ESP_LOGD(TAG, "Read %u bytes", static_cast<unsigned>(pos));

  int count = 0;
  parse_obis(this->buffer_, obis_data, &count);
  if (count == 0) {
    ESP_LOGW(TAG, "No OBIS values parsed from telegram");
  }

  print_parsed_data(obis_data, count);
  this->publish_sensors_(obis_data, count);
  memset(this->buffer_, 0, sizeof(this->buffer_));
}

}  // namespace uh50_reader
}  // namespace esphome
