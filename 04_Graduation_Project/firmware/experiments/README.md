# Firmware experiments

Side projects kept for reference. None of them is part of the final firmware in `../project_v1`.

| Folder | What it is |
|---|---|
| `led_switching/` | The starting point of the graduation project: the course drivers (Eng. Eslam Hefny) with a UART/LED smoke test. Kept unchanged as the original baseline; its `platformio.ini` still uses the Arduino framework and the old folder layout. `project_v1` replaced it with a bare-metal build. |
| `hal_testing_v1/` | Test bench for the first 7-segment design (7447 BCD decoder, two multiplexed digits). It showed the design was unreliable in Proteus, which led to decision D-20. Schematic: `../../simulation/experiments/7seg_v1_7447_multiplexed_failed.pdsprj`. |
| `hal_testing_v2/` | Test bench for the replacement design (one PCF8574 per digit on the I²C bus), used in the final system. Schematic: `../../simulation/experiments/7seg_v2_i2c_expanders.pdsprj`. |

The full story of the 7-segment redesign is in `../project_v1/docs/hal_summary.md` Section 3.
