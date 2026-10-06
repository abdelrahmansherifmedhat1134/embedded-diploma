# ATmega32 Smart Home + Electric Water Heater

**Graduation project — AMIT Learning Embedded Systems Diploma**

| | |
|---|---|
| Author | **Abdelrahman Sherif Medhat** (individual project — the course project is designed for teams of up to six) |
| Course | Embedded Systems Diploma, AMIT Learning (onsite), 14 Sep 2025 – 8 Apr 2026 |
| Instructor | Eng. Eslam Hefny |
| Target | ATmega32 @ 16 MHz, bare-metal C (avr-gcc, PlatformIO), simulated in Proteus 8 |
| Release | [**v1.0** — firmware `.hex` + Proteus project](https://github.com/abdelrahmansherifmedhat1134/embedded-diploma/releases/latest) |

The course offered two separate projects — a **Smart Home** and an **Electric Water Heater**. This project builds **both on one ATmega32**: the water heater becomes one appliance of the smart home. It keeps its own panel (Up / Down / ON-OFF buttons, two-digit display, status LED), and its state and set temperature are also reachable from the UART terminal and the keypad/LCD menu.

<!-- DEMO: replace this comment with the demo GIF or video link once recorded:
![Demo](firmware/project_v1/docs/images/demo.gif)
-->

## What it does

| Area | Behaviour |
|---|---|
| **Security** | Admin (terminal only) and users (terminal or keypad) with separate user lists; accounts stored in EEPROM; the 3rd wrong login in a row locks the system and sounds the alarm until reset |
| **Remote terminal (UART)** | 9600 8N1 menu over Bluetooth/USB-TTL: lamps, dimmer, AC, heater, door, user management; every change made elsewhere is reported live |
| **Keypad + LCD** | 16x2 LCD and 4x4 keypad menu for users; PIN shown as `*`; status pages when idle |
| **Lighting** | 5 on/off lamps on an I²C expander, 1 dimmable lamp (PWM → 0–5 V, 10 % steps) |
| **Door** | Servo, 0° / 90°, admin only |
| **Air conditioning** | LM35 room temperature; fan on above 28 °C, off below 21 °C |
| **Water heater** | Set temperature 35–75 °C in 5 °C steps, saved in EEPROM; sampled every 100 ms, decisions on a 10-sample average with a ±5 °C band; blinking set-mode display and heater LED as specified |
| **Storage** | External 24C08 EEPROM over I²C with a RAM image and non-blocking write-behind |

## Architecture

![Firmware architecture](firmware/project_v1/docs/images/architecture.svg)

- **Layered:** MCAL → HAL → SERVICE → APP; a layer only calls the one below it, and APP code never touches a register or pin.
- **No blocking:** Timer2 produces a 1 ms tick whose ISR only sets task flags; `main()` runs a super-loop of non-blocking state machines (longest loop pass ≈ 2.2 ms).
- **Interrupt-driven UART** through ring buffers; **one I²C bus** for the LCD, lamps, both display digits and the EEPROM — which is how ~41 needed I/O lines fit on 32 pins.
- **Small:** flash 16.3 KB (49.9 %), static RAM 733 B (35.8 %); integer maths only, all text in flash.

<!-- SCHEMATIC: replace this comment with the full Proteus schematic screenshot:
![Proteus schematic](firmware/project_v1/docs/images/schematic.png)
-->

## Verification

| Evidence | Where |
|---|---|
| Requirement traceability — every requirement ID mapped to its test steps | [`test_plan.md` § 4 Coverage](firmware/project_v1/docs/test_plan.md#4-coverage) |
| 78-step Proteus test plan for the full system | [`test_plan.md` § 3](firmware/project_v1/docs/test_plan.md#3-requirement-steps-full-application) |
| One test program per layer, `[PASS]` / `[FAIL]` over UART | [`test_mains/`](firmware/project_v1/test_mains/) |
| APP layer verified step by step in Proteus — 130 / 130 steps passed | [Part A (48 steps)](firmware/project_v1/docs/test_app_partA_sequence.pdf) · [Part B (82 steps)](firmware/project_v1/docs/test_app_partB_sequence.pdf) |
| Requirement IDs in the code (`/* REQ-HTR-09 */`) | 126 tags across `src/` and `lib/` |
| All six builds (`app` + five layer tests) with `-Wall` | zero warnings |

## Run it

**Without building:** download `firmware.hex` and the Proteus project from the [latest release](https://github.com/abdelrahmansherifmedhat1134/embedded-diploma/releases/latest).

**From source:**
```bash
cd firmware/project_v1
pio run -e app                   # -> .pio/build/app/firmware.hex
```
Open [`simulation/final/smart_home_water_heater_final.pdsprj`](simulation/final/smart_home_water_heater_final.pdsprj), load the `.hex` into the ATmega32, set its clock to **16 MHz**, and start. Terminal: 9600 8N1. First-boot login: **`admin` / `1234`**. More in [`simulation/README.md`](simulation/README.md).

## Repository map

| Path | Contents |
|---|---|
| [`firmware/project_v1/`](firmware/project_v1/) | The firmware (PlatformIO project) — start with its [README](firmware/project_v1/README.md) |
| [`firmware/project_v1/docs/`](firmware/project_v1/docs/) | Specification, architecture, pin map, EEPROM map, UART protocol, test plan, test records |
| [`simulation/`](simulation/) | Proteus projects: final system, one per layer test, experiments |
| [`firmware/experiments/`](firmware/experiments/) | Test benches (7-segment redesign) and the original course baseline |
| [`docs/`](docs/) | Course brief and pre-design notes |

## Design highlights

- **Two projects, one MCU, 32 pins.** Moving the LCD, five lamps and both display digits onto I²C expanders freed enough pins for everything else.
- **A design that failed, and why it was replaced.** The first two-digit display (7447 decoder, multiplexed from the 1 ms tick) was unreliable in Proteus; a test bench isolated the cause and the display moved to one I²C expander per digit (decision D-20, [`hal_summary.md`](firmware/project_v1/docs/hal_summary.md) § 3).
- **EEPROM that never blocks.** The 24C08 needs up to 5 ms per write; a RAM image plus a write-behind state machine with ACK polling keeps every loop pass short.
- **Course drivers kept and fixed, not rewritten.** Bugs found in the original drivers (swapped PWM mode bits, ISRs dropped by link-time optimisation, a blocking keypad, a fixed baud value) were fixed in their own style — listed in [`specification.md` § 4](firmware/project_v1/docs/specification.md).

## Tools

PlatformIO (bare-metal avr-gcc) · Antigravity IDE · Proteus 8 · Git / GitHub · Claude Code (AI coding assistant)

---

Course drivers (MCAL/HAL base set) by Eng. Eslam Hefny, AMIT Learning; project code and documentation by Abdelrahman Sherif Medhat.
