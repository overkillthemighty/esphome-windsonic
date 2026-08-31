#include "windsonic.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include <cmath>
#include <cstdlib>

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

bool WindSonicComponent::request_measurement(const char *measurement, String &response) {
  if (this->sdi12_ == nullptr) {
    return false;
  }

  std::string command = this->address_ + measurement + "!";
  ESP_LOGVV(TAG, "Sending command: %s", command.c_str());
  this->sdi12_->clearBuffer();
  this->sdi12_->sendCommand(command.c_str(), 0);
  String acknowledgement;
  if (!this->read_response(acknowledgement) || acknowledgement.length() < 4 ||
      acknowledgement[0] != this->address_[0]) {
    ESP_LOGVV(TAG, "Complete reply: %s", acknowledgement.c_str());
    return false;
  }
  ESP_LOGVV(TAG, "Complete reply: %s", acknowledgement.c_str());

  const uint32_t wait_ms = static_cast<uint32_t>(acknowledgement.substring(1, 4).toInt()) * 1000U;
  if (wait_ms > 0) {
    delay(wait_ms);
  } else {
    delay(30);
  }

  command = this->address_ + "D0!";
  ESP_LOGVV(TAG, "Sending command: %s", command.c_str());
  response = "";
  this->sdi12_->clearBuffer();
  this->sdi12_->sendCommand(command.c_str(), 0);
  const bool received = this->read_response(response);
  ESP_LOGVV(TAG, "Complete reply: %s", response.c_str());
  return received;
}

bool WindSonicComponent::parse_measurement_response(const String &response, float &first, float &second, int &status) {
  if (response.length() < 2 || response[0] != this->address_[0]) {
    return false;
  }

  const char *cursor = response.c_str() + 1;
  char *end = nullptr;
  first = strtof(cursor, &end);
  if (end == cursor) {
    return false;
  }
  cursor = end;
  second = strtof(cursor, &end);
  if (end == cursor) {
    return false;
  }
  cursor = end;
  char *status_end = nullptr;
  const long parsed_status = strtol(cursor, &status_end, 10);
  if (status_end == cursor || parsed_status < 0 || parsed_status > 255) {
    return false;
  }
  while (*status_end == '\r' || *status_end == '\n' || *status_end == ' ') {
    ++status_end;
  }
  if (*status_end != '\0' || !std::isfinite(first) || !std::isfinite(second)) {
    return false;
  }
  status = static_cast<int>(parsed_status);
  return true;
}

void WindSonicComponent::publish_failure(bool vector_measurement, const String &response, int status) {
  if (vector_measurement) {
    if (this->u_sensor_ != nullptr) {
      this->u_sensor_->publish_state(NAN);
    }
    if (this->v_sensor_ != nullptr) {
      this->v_sensor_->publish_state(NAN);
    }
    if (this->raw_vector_sensor_ != nullptr) {
      this->raw_vector_sensor_->publish_state(response.c_str());
    }
  } else {
    if (this->direction_sensor_ != nullptr) {
      this->direction_sensor_->publish_state(NAN);
    }
    if (this->speed_sensor_ != nullptr) {
      this->speed_sensor_->publish_state(NAN);
    }
    if (this->raw_polar_sensor_ != nullptr) {
      this->raw_polar_sensor_->publish_state(response.c_str());
    }
  }
  if (this->status_code_sensor_ != nullptr) {
    this->status_code_sensor_->publish_state(status);
  }
  if (this->status_sensor_ != nullptr) {
    this->status_sensor_->publish_state(false);
  }
}

void WindSonicComponent::update() {
  if (this->transaction_active_) {
    return;
  }

  const uint32_t now = millis();
  const bool vector_enabled = this->u_sensor_ != nullptr || this->v_sensor_ != nullptr ||
                              this->raw_vector_sensor_ != nullptr;
  const bool polar_due = !this->polar_updated_ || now - this->last_polar_update_ >= this->polar_update_interval_ms_;
  const bool vector_due = vector_enabled &&
                          (!this->vector_updated_ || now - this->last_vector_update_ >= this->vector_update_interval_ms_);
  if (!polar_due && !vector_due) {
    return;
  }

  this->transaction_active_ = true;
  this->power_on();

  if (polar_due) {
    this->last_polar_update_ = now;
    this->polar_updated_ = true;
    String response;
    float direction = NAN;
    float speed = NAN;
    int status = -1;
    const bool parsed = this->request_measurement("M", response) &&
                        this->parse_measurement_response(response, direction, speed, status);
    if (this->raw_response_sensor_ != nullptr) {
      this->raw_response_sensor_->publish_state(parsed ? response.c_str() : "NO_RESPONSE");
    }
    if (!parsed || status != 0 || direction == 999.99f || speed == 999.99f) {
      this->publish_failure(false, response, parsed ? status : -1);
    } else {
      if (this->raw_polar_sensor_ != nullptr) {
        this->raw_polar_sensor_->publish_state(response.c_str());
      }
      if (this->direction_sensor_ != nullptr) {
        this->direction_sensor_->publish_state(direction);
      }
      if (this->speed_sensor_ != nullptr) {
        this->speed_sensor_->publish_state(speed);
      }
      if (this->status_code_sensor_ != nullptr) {
        this->status_code_sensor_->publish_state(status);
      }
      if (this->status_sensor_ != nullptr) {
        this->status_sensor_->publish_state(true);
      }
    }
  }

  if (vector_due) {
    this->last_vector_update_ = now;
    this->vector_updated_ = true;
    String response;
    float u = NAN;
    float v = NAN;
    int status = -1;
    const bool parsed = this->request_measurement("M1", response) &&
                        this->parse_measurement_response(response, u, v, status);
    if (!parsed || status != 0 || u == 999.99f || v == 999.99f) {
      this->publish_failure(true, response, parsed ? status : -1);
    } else {
      if (this->raw_vector_sensor_ != nullptr) {
        this->raw_vector_sensor_->publish_state(response.c_str());
      }
      if (this->u_sensor_ != nullptr) {
        this->u_sensor_->publish_state(u);
      }
      if (this->v_sensor_ != nullptr) {
        this->v_sensor_->publish_state(v);
      }
      if (this->status_code_sensor_ != nullptr) {
        this->status_code_sensor_->publish_state(status);
      }
      if (this->status_sensor_ != nullptr) {
        this->status_sensor_->publish_state(true);
      }
    }
  }

  this->power_off();
  this->transaction_active_ = false;
}

void WindSonicComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "WindSonic SDI-12 sensor");
  ESP_LOGCONFIG(TAG, "  Address: %s", this->address_.c_str());
  ESP_LOGCONFIG(TAG, "  Data pin: %d", this->data_pin_ != nullptr ? this->data_pin_->get_pin() : -1);
  ESP_LOGCONFIG(TAG, "  Response timeout: %ums", static_cast<unsigned int>(this->timeout_ms_));
  ESP_LOGCONFIG(TAG, "  Polar update interval: %ums", static_cast<unsigned int>(this->polar_update_interval_ms_));
  ESP_LOGCONFIG(TAG, "  Vector update interval: %ums", static_cast<unsigned int>(this->vector_update_interval_ms_));
}

}  // namespace windsonic
}  // namespace esphome
