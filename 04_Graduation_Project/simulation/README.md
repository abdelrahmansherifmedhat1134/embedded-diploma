# Proteus simulation

Open the projects with Proteus 8.x. In every project the ATmega32 **Clock Frequency must be 16 MHz** (equal to `F_CPU`).

## Final system

| File | Load into the ATmega32 |
|---|---|
| [`final/smart_home_water_heater_final.pdsprj`](final/smart_home_water_heater_final.pdsprj) | `firmware/project_v1/.pio/build/app/firmware.hex` (or the `firmware.hex` from the GitHub release) |

Virtual Terminal: 9600 baud, 8N1, "Echo Typed Characters" off. Default login: `admin` / `1234`.

## Layer tests

One schematic per layer test program (`firmware/project_v1/test_mains/`). Each test file's header comment lists the parts and wiring it needs.

| File | Firmware (`pio run -e …`) |
|---|---|
| [`layer_tests/1_mcal_test.pdsprj`](layer_tests/1_mcal_test.pdsprj) | `test_mcal` |
| [`layer_tests/2_hal_test.pdsprj`](layer_tests/2_hal_test.pdsprj) | `test_hal` |
| [`layer_tests/3_service_test.pdsprj`](layer_tests/3_service_test.pdsprj) | `test_service` |
| [`layer_tests/4_app_test_partA.pdsprj`](layer_tests/4_app_test_partA.pdsprj) | `test_app` (part A: LIGHT, DOOR, CLIMATE, HEATER, ALARM, SEC) |
| [`layer_tests/5_app_test_partB.pdsprj`](layer_tests/5_app_test_partB.pdsprj) | `test_app` (complete APP layer with both user interfaces) |

## Design references

| File | Content |
|---|---|
| `design/base_wiring_phase0.pdsprj` | wiring of the original course drivers, used by `test_base` |
| `design/base_wiring_phase0_netlist.SDF`, `design/design_netlist_phase1.SDF` | netlist exports used to check `docs/pin_map.md` |

## Experiments

| File | Content |
|---|---|
| `experiments/7seg_v1_7447_multiplexed_failed.pdsprj` | first 7-segment design (7447 decoder, multiplexed digits); unreliable in Proteus — see `docs/hal_summary.md` Section 3 |
| `experiments/7seg_v2_i2c_expanders.pdsprj` | replacement design: one PCF8574 per digit (decision D-20), used in the final system |
| `experiments/buzzer_test.pdsprj` | buzzer model settings bench |
| `experiments/led_switching_base_setup.pdsprj` | the course's starting project |
