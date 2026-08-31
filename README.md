# ESPHome WindSonic SDI-12 component

External ESPHome component for a Gill WindSonic ultrasonic anemometer using the EnviroDIY Arduino SDI-12 library.

## Hardware

The WindSonic requires a separate 9.6-16 V supply. Do not power it from a 3.3 V GPIO.

Use an SDI-12 interface/transceiver that supports the bus voltage and bidirectional data line. Connect:

- WindSonic `V+` to the protected 12 V supply.
- WindSonic `0V` to the supply ground and ESP32 ground.
- WindSonic `DATA` through the SDI-12 interface to the configured ESP32 data GPIO.
- The optional `power_pin` to the enable/control input of a suitable high-side power switch.

The XIAO ESP32-S3 example uses GPIO9 for data and GPIO10 for the optional power-switch control. Change these pins to match your wiring.

## ESPHome configuration

For a local checkout, use the example directly:

```yaml
external_components:
	- source:
			type: local
			path: ../components
		components: [windsonic]

windsonic:
	id: windsonic_bus
	data_pin: GPIO9
	power_pin: GPIO10
	update_interval: 30s
	timeout: 500ms
	direction:
		name: WindSonic Direction
	speed:
		name: WindSonic Speed
	u:
		name: WindSonic U Component
	v:
		name: WindSonic V Component
	status:
		name: WindSonic Status
	raw_response:
		name: WindSonic Raw Response
```

For a Git-based external component, replace the local source with the repository URL and a pinned revision:

```yaml
external_components:
	- source:
			type: git
			url: https://github.com/YOUR_ACCOUNT/esphome-windsonic
			ref: main
		components: [windsonic]
```

The component sends the SDI-12 `M!` command, waits for the sensor's ready response, then requests `D0!`. The first four values are exposed as direction in degrees, speed in m/s, and the U/V wind-vector components in m/s.

## Troubleshooting

- Confirm the WindSonic has its required supply voltage and shares ground with the ESP32.
- Use an appropriate SDI-12 physical interface; a direct 3.3 V GPIO connection is not a substitute for bus protection and level handling.
- Keep the data line short during initial testing and verify the sensor address is `0` unless it was changed.
- Enable `raw_response` and inspect the ESPHome logs when diagnosing wiring or protocol problems.
- A `false` status means the component did not receive a valid measurement response during the configured timeout.

The component pins EnviroDIY `SDI-12` library version `2.3.2` through ESPHome code generation.
