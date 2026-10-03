# HAL layer — as built (Phase 2 HAL, 2026-10-03)

What was built, where it differs from the Phase 1 design, what was found in Proteus, and how the 7-segment display ended up on two PCF8574 chips. API details stay in [architecture.md](architecture.md) Section 3.2; wiring in [pin_map.md](pin_map.md); test steps in [test_plan.md](test_plan.md).

Branch `phase2-hal`, PR #5. Test program: `test_mains/test_hal.c`, `pio run -e test_hal` -> `.pio/build/test_hal/firmware.hex`.

---

## 1. Modules

| Module | State | Notes |
|---|---|---|
| PCF8574 | fixed | `PCF8574_u8WriteBytes` = several port values in one I²C transaction |
| CLCD | fixed | one I²C transaction of 4 port bytes per LCD byte (~0.48 ms measured, was ~1.2 ms); `CLCD_DISPLAY_CTRL` in cfg (cursor and blink off); `CLCD_voidSendFlashString`; `CLCD_voidClearDisp` declared; **new** `CLCD_u8GetStatus()` = result of the last I²C write, used by LCD_BUF |
| LCD_BUF | new | wanted/shown buffers, `LCD_BUF_voidUpdate` sends at most one byte (longest call measured 0.49 ms); after an I²C error it waits `LCD_BUF_RETRY_UPDATES` calls |
| KPAD | fixed | non-blocking `KPAD_voidUpdate` / `KPAD_u8GetKey`, 2 equal scans = accepted, a held key is reported once; tables are `static const __flash` (plain `const` would still be copied to RAM on AVR) |
| BUTTON | new | 3 buttons, 3 x 10 ms debounce, press and release events cleared by reading |
| SEVEN_SEG | new, **redesigned (D-20)** | see Section 3 |
| LM35 | new | raw ADC value = temperature in 0.25 °C (2.56 V reference); unknown sensor ID reads 0 |
| RELAY, LED, BUZZER | new | level first, then output direction, so nothing switches on at start-up |
| LAMP | new | RAM copy of the PCF8574 port; the copy changes only after a successful write; `LAMP_ERR_NUMBER` (0xFF) for a lamp number outside 1..5 |
| FAN, SERVO, DIMMER | new | 0 % = compare output disconnected and pin driven low, 100 % = compare value equals TOP; values above 100 % / 180° are limited; FAN and SERVO share Timer1, a second `TIMER1_voidInit` changes nothing |
| EXT_EEPROM | new | block read split at 256-byte borders (24C08 block = I²C address), page write inside one 16-byte page, one-shot ACK poll `EXT_EEPROM_u8IsReady` |
| SSG | untouched | old driver, replaced by SEVEN_SEG, not used |

Compile check until Phase 3: `pio run -e test_base` / `test_<layer>` (the `app` environment builds only the old `main.c`, which includes no driver).

## 2. Test program `test_hal`

Own 1 ms tick from Timer2 (SCHED does not exist yet); the main loop serves `LCD_BUF_voidUpdate` every 5 ms and `KPAD_voidUpdate` + `BUTTON_voidUpdate` every 10 ms; Timer1 is a stop watch. Output: `[PASS]` / `[FAIL]` / `[SKIP]` / `[INFO]` lines, `MANUAL:` lines, a summary; status LED on PA3 (slow blink = running, solid = all passed, fast blink = failure).

Last full Proteus run before the 7-segment redesign: 70 passed, 4 failed (the 4 were the 7-segment read-back checks, see Section 4).

Manual steps: LCD text, relays/LED/buzzer, lamp chase, **7-segment** (below), LM35 sliders, keypad (hold, 3 taps, corner keys), 3 buttons, servo 0/90/180°, fan ramp, dimmer ramp, EEPROM persistence (press RESET and run again).

7-segment steps (all by eye, one short `look: NN` line per step): blank 3 s · 00 11 … 99, 1 s each · 37 73 10 99, 1 s each · 150 shows 99 · counting 00–99, 500 ms per step up to 55 then 300 ms · 88 (every segment) 3 s · Disable blank 3 s · Enable, 88 back. Only "no I²C error" is checked by the program.

## 3. 7-segment display history (D-20)

1. **First design:** one 7447 BCD decoder, two digits multiplexed from the 1 ms tick (`SEVEN_SEG_voidRefresh`), PNP digit drivers on PC6 / PC7.
2. **Bench `experiments/hal_testing_v1`** (7447 version): direct pin test (A1-A3) and driver test (B1-B5). Findings: the pins followed the port (readback OK, JTAG disabled, MCUCSR = 128), the generated code was correct (checked in the disassembly). A first sampling check reported a false error because it read the digit pins and the BCD pins one by one while the tick interrupt switched digits (fixed: one port snapshot). Symptom in Proteus: 37 shown as 33, the counter showed the same strange symbol on both digits — both digits conducted at the same time (slow PNP). A 1 ms dead time (`SEVEN_SEG_BLANK_TICKS`) was added but the 7447 version stayed unreliable in Proteus. Schematic: `simulation/hal_test_7seg_v1_fail.pdsprj`.
3. **Decision D-20 (approved 2026-10-03):** two PCF8574 expanders, one per common-anode digit (tens 0x21, units 0x22; P0..P6 = segments a..g, P7 = dp, active low, 0xFF = blank). No multiplexing, no tick hook, no JTAG handling, PC2–PC7 spare.
4. **Bench `experiments/hal_testing_v2`** (I²C version, project_v1 `lib/` copied, with the LCD and the lamp chip on the same bus): bus scan, raw 22-pattern port test per chip, digit patterns, all 100 numbers, driver behaviour, bus sharing, stress, timing, looks. Schematic: `simulation/hal_test_7seg_v2.pdsprj`. Result: **the display shows the right digits**; the chip port read-back returns other values than the lit digit, so read-back checks were dropped from `test_hal` and the display is checked by eye.
5. `project_v1`: driver and docs moved to D-20; schematic `simulation/project_v1_phase2_HAL.pdsprj`.

## 4. Proteus findings (also in CLAUDE.md "Proteus notes")

- The 24C08 model has no write-cycle delay (it ACKs right after a write): `test_hal` reports `[INFO]`, not a failure. Real chip ~5 ms, so `ESTORE` still polls.
- The servo model maps a 1–2 ms pulse to -90..+90° by default: set Min/Max Angle to 0 / 180.
- Reading back a PCF8574 whose pins drive segment LEDs returns another value than the one written (the display itself is right): do not read-back-check segment chips.
- A multiplexed 7447 display with PNP drivers did not work reliably (D-20).
- The buzzer produced no sound when wired straight to PD3. The Proteus model ("DC Buzzer with Sound") has `LOAD=12` and `VNOM=5V`, i.e. it wants 0.4 A, far more than an AVR pin can give (and a real board needs a transistor anyway). Fix: PD3 -> 1 kΩ -> NPN (2N2222) -> buzzer; `test_hal` now buzzes 3 s with a steady level, then 3 s with a 500 Hz square wave as a debugging aid. Status: waiting for confirmation in Proteus.

## 5. Experiment folders (not part of the firmware)

| Folder | What | Build |
|---|---|---|
| `experiments/hal_testing_v1` | 7447 + multiplexing bench (failed design, kept for documentation) | `pio run` in the folder |
| `experiments/hal_testing_v2` | I²C 7-segment bench with a copy of `project_v1/lib` | `pio run` in the folder |

Each folder has its own `platformio.ini`; the first line of its `src/main.c` header explains wiring and expected output.
