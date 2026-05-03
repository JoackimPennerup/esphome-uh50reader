#include "uh50_reader.h"

#include <cstdlib>

#include "esphome/core/log.h"

namespace esphome {
namespace uh50_reader {

static const char *const TAG = "uh50_reader";

static const uint8_t DATA_CMD[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '/', '#', '!', 0x0D, 0x0A,
};

static const uint32_t START_TIMEOUT_MS = 5000;
static const uint32_t FRAME_TIMEOUT_MS = 6000;
static const uint32_t INTERBYTE_TIMEOUT_MS = 200;

UH50Reader::UH50Reader(uart::UARTComponent *uart_in, uint32_t update_interval_ms)
    : PollingComponent(update_interval_ms), UARTDevice(uart_in) {}

void UH50Reader::setup() {
  this->set_timeout(this->startup_read_delay_ms_, [this]() { this->request_read_(); });
}

void UH50Reader::update() { this->request_read_(); }

void UH50Reader::loop() {
  switch (this->read_state_) {
    case ReadState::IDLE:
      if (this->read_requested_) {
        this->start_read_();
      }
      break;
    case ReadState::WAITING_FOR_STX:
      this->process_waiting_for_stx_();
      break;
    case ReadState::READING_FRAME:
      this->process_reading_frame_();
      break;
  }
}

void UH50Reader::dump_config() {
  ESP_LOGCONFIG(TAG, "UH50 Reader:");
  ESP_LOGCONFIG(TAG, "  Update interval: %u ms", this->get_update_interval());
  ESP_LOGCONFIG(TAG, "  Startup read delay: %u ms", this->startup_read_delay_ms_);
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

void UH50Reader::read_meter() { this->request_read_(); }

void UH50Reader::request_read_() {
  if (this->read_requested_ || this->read_state_ != ReadState::IDLE) {
    ESP_LOGD(TAG, "Read request ignored because a transaction is already in progress");
    return;
  }

  this->read_requested_ = true;
}

void UH50Reader::start_read_() {
  if (this->uart_out_ == nullptr) {
    ESP_LOGW(TAG, "TX UART not configured, cannot send data command");
    this->read_requested_ = false;
    return;
  }

  while (this->available()) {
    this->read();
  }

  this->buffer_pos_ = 0;
  this->buffer_[0] = '\0';
  this->read_requested_ = false;
  this->read_state_ = ReadState::WAITING_FOR_STX;
  this->read_started_at_ = millis();
  this->frame_started_at_ = 0;
  this->last_byte_at_ = this->read_started_at_;

  this->send_data_cmd_();
}

void UH50Reader::send_data_cmd_() {
  for (auto b : DATA_CMD) {
    this->uart_out_->write_byte(b);
  }
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

void UH50Reader::reset_read_state_() {
  this->read_state_ = ReadState::IDLE;
  this->buffer_pos_ = 0;
  this->buffer_[0] = '\0';
  this->read_started_at_ = 0;
  this->frame_started_at_ = 0;
  this->last_byte_at_ = 0;
}

void UH50Reader::process_waiting_for_stx_() {
  while (this->available()) {
    if (this->read() == 0x02) {
      this->read_state_ = ReadState::READING_FRAME;
      this->frame_started_at_ = millis();
      this->last_byte_at_ = this->frame_started_at_;
      return;
    }
  }

  if (millis() - this->read_started_at_ >= START_TIMEOUT_MS) {
    ESP_LOGW(TAG, "Timed out waiting for STX");
    this->reset_read_state_();
  }
}

void UH50Reader::process_reading_frame_() {
  while (this->available()) {
    const uint8_t b = this->read();
    if (b == 0x03) {
      this->finish_read_();
      return;
    }

    if (this->buffer_pos_ >= sizeof(this->buffer_) - 1) {
      ESP_LOGW(TAG, "Telegram exceeded buffer size, truncating");
      this->finish_read_();
      return;
    }

    this->buffer_[this->buffer_pos_++] = static_cast<char>(b);
    this->last_byte_at_ = millis();
  }

  if (millis() - this->frame_started_at_ >= FRAME_TIMEOUT_MS ||
      millis() - this->last_byte_at_ > INTERBYTE_TIMEOUT_MS) {
    this->finish_read_();
  }
}

void UH50Reader::finish_read_() {
  if (this->buffer_pos_ == 0) {
    ESP_LOGW(TAG, "No payload received after STX");
    this->reset_read_state_();
    return;
  }

  this->buffer_[this->buffer_pos_] = '\0';
  ESP_LOGD(TAG, "Read %u bytes", static_cast<unsigned>(this->buffer_pos_));

  OBISData obis_data[MAX_OBIS_CODES];
  int count = 0;
  parse_obis(this->buffer_, obis_data, &count);
  if (count == 0) {
    ESP_LOGW(TAG, "No OBIS values parsed from telegram");
  }

  print_parsed_data(obis_data, count);
  this->publish_sensors_(obis_data, count);
  this->reset_read_state_();
}

}  // namespace uh50_reader
}  // namespace esphome
