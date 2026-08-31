#include "windsonic.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace windsonic {

static const char *const TAG = "windsonic";

void WindSonicComponent::setup() {
  if (this->data_pin_ == nullptr) {
    ESP_LOGE(TAG, "Data pin not configured");
    return;
  }

  if (this->sdi12_ == nullptr) {
    this->sdi12_ = new SDI12();
  }

  this->sdi12_->setDataPin(this->data_pin_->get_pin());
  this->sdi12_->begin();

  if (this->power_pin_ != nullptr) {
    this->power_pin_->setup();
    this->power_pin_->digital_write(false);
  }
}

void WindSonicComponent::power_on() {
  if (this->power_pin_ != nullptr) {
    this->power_pin_->digital_write(true);
    delay(50);
  }
}

void WindSonicComponent::power_off() {
  if (this->power_pin_ != nullptr) {
    this->power_pin_->digital_write(false);
  }
}

bool WindSonicComponent::read_response(String &response) {
  if (this->sdi12_ == nullptr) {
    return false;
  }

  bool received_data = false;
  const uint32_t start = millis();
  while (millis() - start < this->timeout_ms_) {
    while (this->sdi12_->available() > 0) {
      const char c = static_cast<char>(this->sdi12_->read());
      if (c == '\r' || c == '\n') {
        if (received_data) {
          return true;
        }
      } else {
        response += c;
        received_data = true;
      }
    }
    yield();
  }
  return received_data;
}

bool WindSonicComponent::request_measurement(String &response) {
  if (this->sdi12_ == nullptr) {
    return false;
  }

  std::string command = this->address_ + "M!";
  this->sdi12_->clearBuffer();
  this->sdi12_->sendCommand(command.c_str(), 0);
  if (!this->read_response(response) || response.length() < 4) {
    return false;
  }

  const uint32_t wait_ms = static_cast<uint32_t>(response.substring(1, 4).toInt()) * 1000U;
  if (wait_ms > 0) {
    delay(wait_ms);
  }

  command = this->address_ + "D0!";
  response = "";
  this->sdi12_->clearBuffer();
  this->sdi12_->sendCommand(command.c_str(), 0);
  return this->read_response(response);
}

bool WindSonicComponent::parse_measurement_response(const String &response) {
  if (response.length() == 0) {
    return false;
  }

  if (response[0] != this->address_[0]) {
    return false;
  }

  std::vector<float> values;
  String current = "";
  for (size_t i = 1; i < response.length(); ++i) {
    const char c = response[i];
    if (c == '+' || c == '-') {
      if (current.length() > 0) {
        values.push_back(current.toFloat());
      }
      current = String(c);
    } else if (c == '\r' || c == '\n') {
      continue;
    } else if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E') {
      current += c;
    }
  }

  if (current.length() > 0) {
    values.push_back(current.toFloat());
  }

  if (values.size() < 2) {
    return false;
  }

  const float direction = values[0];
  const float speed = values[1];
  const float u = values.size() > 2 ? values[2] : 0.0f;
  const float v = values.size() > 3 ? values[3] : 0.0f;

  if (this->direction_sensor_ != nullptr) {
    this->direction_sensor_->publish_state(direction);
  }
  if (this->speed_sensor_ != nullptr) {
    this->speed_sensor_->publish_state(speed);
  }
  if (this->u_sensor_ != nullptr) {
    this->u_sensor_->publish_state(u);
  }
  if (this->v_sensor_ != nullptr) {
    this->v_sensor_->publish_state(v);
  }
  return true;
}

void WindSonicComponent::update() {
  this->power_on();

  String response;
  if (!this->request_measurement(response)) {
    if (this->raw_response_sensor_ != nullptr) {
      this->raw_response_sensor_->publish_state("NO_RESPONSE");
    }
    if (this->status_sensor_ != nullptr) {
      this->status_sensor_->publish_state(false);
    }
    this->power_off();
    return;
  }

  if (this->raw_response_sensor_ != nullptr) {
    this->raw_response_sensor_->publish_state(response.c_str());
  }

  const bool ok = this->parse_measurement_response(response);
  if (this->status_sensor_ != nullptr) {
    this->status_sensor_->publish_state(ok);
  }

  this->power_off();
}

void WindSonicComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "WindSonic SDI-12 sensor");
  ESP_LOGCONFIG(TAG, "  Address: %s", this->address_.c_str());
  ESP_LOGCONFIG(TAG, "  Data pin: %d", this->data_pin_ != nullptr ? this->data_pin_->get_pin() : -1);
  ESP_LOGCONFIG(TAG, "  Response timeout: %ums", static_cast<unsigned int>(this->timeout_ms_));
}

}  // namespace windsonic
}  // namespace esphome
