#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/gpio.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <SDI12.h>

namespace esphome {
namespace windsonic {

class WindSonicComponent : public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void set_data_pin(InternalGPIOPin *pin) { this->data_pin_ = pin; }
  void set_power_pin(GPIOPin *pin) { this->power_pin_ = pin; }
  void set_address(const std::string &address) { this->address_ = address; }
  void set_timeout(uint32_t timeout_ms) { this->timeout_ms_ = timeout_ms; }

  void set_raw_response_sensor(text_sensor::TextSensor *sensor) { this->raw_response_sensor_ = sensor; }
  void set_status_sensor(binary_sensor::BinarySensor *sensor) { this->status_sensor_ = sensor; }
  void set_direction_sensor(sensor::Sensor *sensor) { this->direction_sensor_ = sensor; }
  void set_speed_sensor(sensor::Sensor *sensor) { this->speed_sensor_ = sensor; }

 protected:
  void power_on();
  void power_off();
  bool read_response(String &response);
  bool request_measurement(const char *measurement, String &response);
  bool parse_measurement_response(const String &response);

  InternalGPIOPin *data_pin_{nullptr};
  GPIOPin *power_pin_{nullptr};
  SDI12 *sdi12_{nullptr};
  std::string address_ = "0";
  uint32_t timeout_ms_{500};
  text_sensor::TextSensor *raw_response_sensor_{nullptr};
  binary_sensor::BinarySensor *status_sensor_{nullptr};
  sensor::Sensor *direction_sensor_{nullptr};
  sensor::Sensor *speed_sensor_{nullptr};
};

}  // namespace windsonic
}  // namespace esphome
