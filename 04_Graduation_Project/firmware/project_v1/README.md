# ATmega32 Smart Home + Electric Water Heater

One firmware image for an ATmega32 that merges the two course projects (Smart Home and Electric Water Heater). The heater is one more appliance of the smart home: it keeps its own panel (Up / Down / ON-OFF buttons, 2-digit 7-segment display, LED) and is also reachable from the UART terminal and the keypad/LCD.
Bare-metal avr-gcc, built with PlatformIO, simulated in Proteus.

## Features (requirement groups, see CLAUDE.md Section 6)

| Group | What it does |
|---|---|
| SEC | admin (UART only) and users (UART or keypad), separate user lists, accounts in EEPROM, 3rd wrong login in a row locks the system |
| LUI | keypad + 16x2 LCD menu (user mode): lamps, dimmer, AC status, heater; `*` for password digits; status screen when idle |
| RUI | UART terminal (9600 8N1), numbered menu, admin-only options, every state change is printed |
| LGT | 5 on/off lamps (PCF8574, active low) and 1 dimmable lamp (Timer0 PWM, 0-100 % in 10 % steps) |
| DOR | servo door (Timer1 PWM, 0° / 90°), admin only |
| AC | LM35 room temperature, fan on above 28 °C, off below 21 °C (PWM fan) |
| HTR | set temperature 35-75 °C in steps of 5, saved in EEPROM; 100 ms sampling, 10-sample average, heat / cool with a ±5 °C band; display and LED blink rules |
| ALM | buzzer on lockdown until reset |
| EEP | external 24C08 over I²C, write-behind with ACK polling (no blocking) |

Not built in v1 (decision 15): AC fan speed ramp, PIR alarm, LDR. Their pins stay reserved.

## Architecture

- Layers: **MCAL** (DIO, GIE, ADC, TIMER0/1/2, USART, TWI, EXTI) → **HAL** (LCD, keypad, buttons, LM35, 7-segment, lamps, relays, servo, fan, ...) → **SERVICE** (SCHED, TERM, ESTORE, USERDB, EVQ, utilities) → **APP** (SEC, ALARM, LIGHT, DOOR, CLIMATE, HEATER, UILOC, UIREM). A layer calls only the one below it.
- Scheduler: Timer2 gives a 1 ms tick; the ISR only sets task flags (5 ms, 10 ms, 100 ms, 500 ms, 1 s). `main()` runs the super-loop; every APP module is a non-blocking state machine.
- UART RX and TX are interrupt driven through ring buffers (`TERM`); nothing waits for a byte.
- I²C bus (PC0 SCL, PC1 SDA): 24C08 EEPROM `0x50`, lamps PCF8574 `0x20`, 7-segment tens `0x21`, 7-segment units `0x22`, LCD PCF8574 `0x27`.
- Strings live in flash; integer maths only. Final size of `app`: flash 16 344 bytes (49.9 %), static RAM 733 bytes (35.8 %).

## Build and run

```
pio run -e app
```

Hex file: `.pio/build/app/firmware.hex`. Load it in the ATmega32 of the final Proteus project [`project_v1_phase3_final.pdsprj`](../../simulation/project_v1_phase3_final.pdsprj) (full system wiring, matches `docs/pin_map.md`) and set the ATmega32 **clock to 16 MHz** (it must equal `F_CPU` in `platformio.ini`). Virtual Terminal on PD0 / PD1: 9600, 8N1.

Default login on first boot (blank EEPROM): **admin / 1234** (remote terminal only). Change it with admin command 15.

Keypad (calculator pad): digits enter the ID and PIN, `=` Enter, `*` or `C` back (delete a digit / leave the screen), `+` / `-` step a value (dimmer ±10 %, heater ±5 °C). Keypad usernames and PINs are numeric.

## Proteus projects (`../../simulation/`)

| File | Use |
|---|---|
| `project_v1_phase3_final.pdsprj` | **final system**: load `.pio/build/app/firmware.hex` |
| `project_v1_phase2_APP_partA.pdsprj`, `project_v1_phase2_APP_partB.pdsprj` | APP layer tests (`test_app`) |
| `project_v1_phase2_HAL.pdsprj`, `project_v1_phase2_MCAL.pdsprj`, `project_v1_phase2_services.pdsprj` | layer tests (`test_hal`, `test_mcal`, `test_service`) |
| `project_v1_phase0.pdsprj` | Phase 0 base wiring |
| `buzzer_test`, `hal_test_7seg_v1_fail` / `v2`, `led_switching` | small experiments (see `docs/hal_summary.md`) |

## Documents (`docs/`)

| File | Content |
|---|---|
| `architecture.md` | layers, module APIs, scheduler, super-loop, boot order, state machines, RAM/flash numbers, decisions |
| `pin_map.md` | final pin table and Proteus wiring |
| `eeprom_map.md` | byte layout of the 24C08 |
| `uart_protocol.md` | every prompt and command, example session, default accounts |
| `test_plan.md` | numbered Proteus steps for every requirement ID |
| `hal_summary.md` | HAL as built, Proteus findings, the 7-segment story |
| `test_app_partA_sequence.pdf`, `test_app_partB_sequence.pdf` | recorded Proteus results of the APP layer test |

`CLAUDE.md` holds the project rules, conventions and decisions.

## Test evidence

Each layer has its own test program (`test_mains/`, run with `pio run -e test_<layer>`, hex in `.pio/build/test_<layer>/firmware.hex`): `test_base`, `test_mcal`, `test_hal`, `test_service`, `test_app`. They print `[PASS]` / `[FAIL]` lines on the UART. All six environments (`app` and the five tests) build with zero warnings. The APP layer test passed 130 steps in Proteus: 48 in part A and 82 in part B (see the two PDFs above). `test_app` runs the same super-loop as `app` after its automatic part.
