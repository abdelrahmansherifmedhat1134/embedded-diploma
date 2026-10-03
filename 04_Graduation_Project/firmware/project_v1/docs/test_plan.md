# Test plan — Proteus

Phase 1 design. **Status: approved by the user on 2026-10-02.** 78 numbered steps; every requirement ID of CLAUDE.md Section 6 appears at least once (coverage table in Section 4).
Command numbers and message texts: [uart_protocol.md](uart_protocol.md). Pins and parts: [pin_map.md](pin_map.md). EEPROM addresses: [eeprom_map.md](eeprom_map.md).

"Verified" in this project means: it builds with zero warnings, the logic was traced by hand against the requirement, and the step below was run in Proteus. Phase 2 and 3 keep this file in line with the real code.

---

## 1. Setup (once)

| # | Do |
|---|---|
| S1 | `pio run -e app`, load `.pio/build/app/firmware.hex` into the ATmega32 |
| S2 | ATmega32 *Clock Frequency* = **16 MHz** (must equal `F_CPU` in `platformio.ini`) |
| S3 | Wire all parts as in `pin_map.md` Sections 1, 2 and 5, including the RESET push button |
| S4 | Virtual Terminal on PD0/PD1: 9600, 8, NONE, 1, echo off. I²C debugger on SCL/SDA. Oscilloscope: A = PB3 (dimmer), B = PD4 (fan), C = PD5 (servo) |
| S5 | Start values: ambient LM35 = 25 °C, water LM35 = 40 °C |
| S6 | "RESET" in a step means: press the RESET push button (the MCU restarts, the EEPROM keeps its content) |
| S7 | "Blank EEPROM" means: start the simulation with the 24C08 content cleared (all 0xFF) |

Standard accounts used by the steps (created in T-03): remote user `bob` / `5678`, keypad user `1111` / `2222`.
Wrong logins add up per source until a correct login on that source, and the 3rd one locks the system. After a step that fails a login on purpose, log in once correctly (or RESET) before the next step.

---

## 2. Layer test programs (Phase 2)

Each layer ends with `test_mains/test_<layer>.c` and `[env:test_<layer>]`. Output: one UART line per check, `[PASS] <module>: <what>` or `[FAIL] <module>: <what>`, `MANUAL:` lines for things you must do or look at, then a summary line. Status LED on PA3: slow blink = running, solid = all passed, fast blink = a failure.

| Program | Modules | Checks |
|---|---|---|
| `test_base` (exists) | Phase 0 drivers | must be all `[PASS]` after the MCAL fixes, built **with** LTO |
| `test_mcal` | DIO, GIE, ADC, TIMER0, TIMER1, TIMER2, USART, TWI, EXTI | JTAG disable bit set · ADC sync read works **after** an async read · Timer0 PWM 0 / 50 / 100 % on PB3 at 7.8 kHz (register check + scope) · Timer1: `ICR1` = 39999, 20 ms period, OC1A 1.0 / 1.5 / 2.0 ms, OC1B 0 / 50 / 100 % · Timer2: 1000 compare interrupts in 1.000 s (counted against Timer1) · USART: `UBRR` = 103, RX callback receives typed keys, a 300-byte burst leaves through the TX interrupt without a lost byte · TWI: ACK from 0x20, 0x27, 0x50, `TWI_ERR_SLA_NACK` from 0x60, `TWI_ERR_TIMEOUT` within 2 ms when SDA is held low (MANUAL) · EXTI: callback on a PD2 button, no crash with no callback set |
| `test_hal` | PCF8574, CLCD, LCD_BUF, KPAD, BUTTON, SEVEN_SEG, LM35, RELAY, LED, BUZZER, LAMP, FAN, SERVO, DIMMER, EXT_EEPROM | LCD text through LCD_BUF, cursor off, longest `LCD_BUF_voidUpdate` call measured < 1 ms · each keypad key reported exactly once per press · button press and release events, a held button gives one event · 7-seg (manual, slow): chips 0x21 and 0x22 ACK (no I²C error), then by eye: blank (3 s), 00 11 .. 99 (1 s each), 37, 73, 10, 99 (1 s each), 150 shows 99, counting 00–99 (500 ms up to 55, then 300 ms), 88 (3 s), Disable blank (3 s), Enable again (the chip ports are not read back: Proteus returns other values than the lit digit) · both LM35 printed in °C · lamps 1–5 chase, read-back equals written · relays, LED, buzzer on/off · fan 0–100 % ramp · servo 0 / 90 / 180° · dimmer 0–100 % in steps of 10 · EEPROM: page write, `EXT_EEPROM_u8IsReady` = 0 right after it and 1 within 50 ms, read back equal, value still there after RESET |
| `test_service` | RINGBUF, MAVG, FMT, SCHED, TERM, EVQ, ESTORE, USERDB | RINGBUF empty / full / wrap · MAVG: average of 10 known samples, "full" only after 10, reset · FMT: 0, 9, 10, 255, 65535, text -> number rejects letters · SCHED: over 10 s the five flags are served 2000 / 1000 / 100 / 20 / 10 times, `SCHED_u16GetOverruns()` = 0 · TERM: echo, masked echo, backspace, too-long line, CR / LF / CRLF · EVQ: order kept, drop when full, mute · ESTORE: defaults on a blank chip (the test blanks page 0 itself), 13 page writes with the magic page last, byte change -> exactly one page write, unchanged byte -> no write, a write during the write cycle -> page written once more, factory reset, longest `ESTORE_voidUpdate` < 2.5 ms, fault when the chip is removed (MANUAL: SDA-to-GND switch, `[SKIP]` after 15 s), `ESTORE_OK` + data kept after RESET (MANUAL: the run after the RESET is a short second run) · USERDB: add, duplicate, full, bad name, bad password, digits-only rule, 8 + 8 characters, remove, admin password, admin repair, write gate closed -> `USERDB_ERR_READ_ONLY`. **As built:** RINGBUF, MAVG, FMT, EVQ and SCHED are printed with the blocking USART functions, everything after `TERM_voidInit` through TERM; the TERM steps are MANUAL (type `abc`, `1234`, `abx` Backspace `c`, 20 characters, Enter alone, `a` Ctrl+J, `b` Enter Ctrl+J; 20 s each, then `[SKIP]`); the 24C08 model answers at once after a write, reported as `[INFO]` |
| `test_app` | ALARM, SEC, LIGHT, DOOR, CLIMATE, HEATER, UILOC, UIREM | automatic part (PASS/FAIL): 3-strike counter per source and reset on success, permission table, dimmer rounding, set-temperature clamp · interactive part: the steps of Section 3 with a reduced super-loop. **As built, part A** (LIGHT, DOOR, CLIMATE, HEATER, ALARM, SEC): the automatic part presses the heater buttons itself (pins pulled low) and feeds temperatures through `CLIMATE_voidFeedSample` / `HEATER_voidFeedSample`, so it also checks both hysteresis bands, "no decision before 10 samples", the clamp at 35 / 75, the save after 5 s, the ALARM latch and the keypad arbitration; no status LED (PA3 is the heater LED). The live part prints `[INFO] <event> <arg>` and a `[STAT]` line every 5 s for Sections 3.6 and 3.7 (T-46..T-69); typed lines `1..5`, `+`, `-`, `o`, `c`, `35..75`, `a` stand in for the UIs (T-71, T-73). T-70 and T-72 wait for part B |

A layer is done only when `pio run -e test_<layer>` and `pio run -e app` both build with zero warnings.

---

## 3. Requirement steps (full application)

### 3.1 Security and login

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-01 | SEC-09, EEP-02 | Blank EEPROM, start. Terminal: `admin` / `1234` | `Welcome admin. You are logged in as ADMIN.` and the admin menu (items 1–16) |
| T-02 | SEC-01 | Keypad: try ID `1234` with any PIN (no keypad user exists yet) | `Wrong ID or PIN`. The keypad has digits only and checks only the keypad list, so the admin can never log in there |
| T-03 | SEC-03 | Admin: `10` -> `bob` / `5678`; `12` -> `1111` / `2222`; `14` | both `[OK] ... added.`; the list shows `bob` under remote users and `1111` under keypad users, no passwords |
| T-04 | SEC-02 | Logout. Terminal: `1111` / `2222`. Keypad: ID `5678` PIN `5678` | terminal: `[ERR] Wrong username or password. 2 attempts left.` Keypad: `Wrong ID or PIN`. The two lists are separate |
| T-05 | SEC-03 | Admin: `10` six times with different names; `10` with `bob` again; `12` with ID `12ab`; `10` with password `12` | 6th: `[ERR] The user list is full (5).` · `[ERR] This username already exists.` · `[ERR] Keypad users need digits only.` · `[ERR] Password must be 4 to 8 characters.` |
| T-06 | SEC-03 | Admin: `11 bob`, logout, log in as `bob` | `[OK] Remote user bob removed.`, then the login fails (add `bob` again afterwards) |
| T-07 | SEC-04, EEP-02 | With `bob` and `1111` stored: RESET. Log in as `bob` on the terminal and as `1111` on the keypad | both logins work after the restart |
| T-08 | SEC-05, ALM-01 | Terminal: three wrong pairs in a row | `2 attempts left`, `1 attempt left`, then `[ALERT] 3 wrong logins. SYSTEM LOCKED...`; buzzer beeps; LCD `SYSTEM LOCKED`; a correct login typed now is not answered |
| T-09 | SEC-05 | RESET. Keypad: three wrong ID/PIN pairs | `2 tries left`, `1 tries left`, then LCD `SYSTEM LOCKED`, buzzer, and the terminal prints the `[ALERT]` line |
| T-10 | SEC-05 | RESET. Terminal: 2 wrong, 1 correct, logout, 2 wrong | no lockdown: the counter went back to 3 after the good login |
| T-11 | SEC-05 | RESET. Terminal: 2 wrong pairs. Keypad: 2 wrong pairs | no lockdown: the counters are per source |
| T-12 | SEC-05, ALM-01 | Cause a lockdown, then RESET | normal start, buzzer silent, login works |
| T-13 | SEC-06, DOR-01 | Log in as `bob`. Type `7`, `8`, `9`, `10`, `14`, `16` | each: `[ERR] Not allowed for your role.`; the servo does not move |
| T-14 | SEC-06, LUI-02 | Keypad user: look through the LCD menu | Lamps, Dimmer, AC, Heater, Exit — no door entry |
| T-15 | SEC-07 | Remote user `bob`: try `10`–`16`. Then `5 50` | account commands refused (accounts are read-only in user mode); the set temperature is accepted (decision D-12) |
| T-16 | SEC-08 | `bob` logged in on the terminal. Keypad: log in as `1111`, toggle lamp 2 | keypad login works; terminal shows `[INFO] Keypad user logged in` and `[INFO] Lamp 2 is now ON` |
| T-17 | SEC-08 | Admin logged in on the terminal. Press a keypad key | LCD: `Keypad blocked` / `by admin` for 2 s, then the status screen |
| T-18 | SEC-08 | Admin: `9`. Keypad: log in. Admin: `9` again | `[OK] Keypad control is now ALLOWED`, keypad login works; second `9`: `BLOCKED` and the LCD drops back to the status screen |
| T-19 | SEC-08 | Keypad user logged in. Terminal: admin logs in | the keypad session ends at once (status screen) |
| T-20 | SEC-08 | Admin logged in, send nothing for 120 s | `[INFO] No input for 120 s. Logged out.`; the keypad works again |
| T-21 | SEC-09 | Admin: `15` -> `abcd`; logout; log in with `1234`, then with `abcd` | old password refused, new one accepted. Then `16` + `YES`: `admin` / `1234` works again and the user lists are empty (create `bob` and `1111` again for the following steps) |

### 3.2 Local UI (LCD + keypad)

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-22 | LUI-01 | Compare what the keypad can log in as | keypad users only; no admin functions anywhere on the LCD |
| T-23 | LUI-02 | Keypad: `1111` `=` `2222` `=` | menu `1Lamps  2Dimmer` / `3AC 4Heat C=Exit` |
| T-24 | LUI-02, LGT-01 | `1`, then `1`..`5` one by one, then `C` | each key toggles that lamp (LED and LCD mark); `C` returns to the menu |
| T-25 | LUI-02, LGT-02 | `2`, then `+` three times, `-` once, `C` | 10, 20, 30, 20 % on the LCD; the lamp brightness follows |
| T-26 | LUI-02, AC-01 | `3`; change the ambient LM35 from 25 to 27 | room temperature on the LCD follows within about 1 s |
| T-27 | LUI-02, HTR-14 | `4`; press `+`, then `-` | water temperature, state and set temperature shown; set goes 60 -> 65 -> 60 |
| T-28 | LUI-02 | At the menu press `C` | logged out, status screen |
| T-29 | LUI-03 | Nobody logged in on the keypad; lamps 1 and 3 on, dimmer 70 %, ambient 30 °C, heater on | the LCD alternates every 3 s: `Lamps:1-3-- D70%` / `AC:ON   Room:30C`, then `Heater:...` / `Water:..C Set:..` |
| T-30 | LUI-03 | Log in on the keypad, press nothing for 30 s | automatic logout, status screen |
| T-31 | LUI-04 | Type the PIN | every digit appears as `*`; the ID is shown in clear |
| T-32 | LUI-02 | While typing the ID press `*`; with nothing typed press `C` | `*` deletes one digit; `C` goes back to the status screen |

### 3.3 Remote UI (UART)

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-33 | RUI-01 | Start the simulation; type in the terminal | banner and `Hey, please enter your username:`; typed characters are echoed; clean text at 9600 baud in both directions |
| T-34 | RUI-02 | Logged in on the terminal: toggle a lamp on the keypad, press the heater ON/OFF button, raise the ambient temperature above 28 | one `[INFO]` line for each, followed by the prompt |
| T-35 | RUI-02 | Type `1` (no Enter yet), then press the heater ON/OFF button, then Enter and `3` | the `[INFO]` line does not appear while the line is half typed; it is printed after the command is finished |
| T-36 | RUI-03 | Compare the menu of `admin` and of `bob` | admin: 1–16; user: 1–6. `?` prints the menu again |
| T-37 | RUI-03 | `1 3`, `2 70`, `5 65`, `6` | `[OK]` replies; the status block shows lamp 3 ON, dimmer 70 %, set 65 |
| T-38 | RUI-04 | Type `99`, `abc`, a 20-character line, `1 9`, `2 55`, `5 62` | `[ERR] Unknown command.` (x2), `[ERR] Input too long.`, `[ERR] Lamp number must be 1 to 5.`, `[ERR] Dimmer level must be 0 to 100 in steps of 10.`, `[ERR] Temperature must be 35 to 75 in steps of 5.` — the menu follows each |
| T-39 | RUI-01 | Type a password with a typing error and Backspace | the `*` is removed and the corrected password is accepted |

### 3.4 Lighting

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-40 | LGT-01 | Start | all 5 lamps off |
| T-41 | LGT-01 | Terminal: `1 1` … `1 5`, then again | each lamp turns on, then off; the others do not change; the I²C debugger shows one write to 0x40 per command (a lamp that is ON has its bit at 0) |
| T-42 | LGT-02 | `2 0`, `2 10`, `2 50`, `2 100`; scope channel A (PB3) | duty 0 % (flat low), 10 %, 50 %, 100 % (flat high) at about 7.8 kHz; the lamp gets brighter with each step and is off at 0 % |
| T-43 | LGT-02 | `2 55`, `2 110` | both rejected with the dimmer error |

### 3.5 Door

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-44 | DOR-01 | Admin: `7`, `7`, `8`, `8` | `[OK] Door is now OPEN`, `Door is already open.`, `[OK] Door is now CLOSED`, `Door is already closed.`; the servo moves twice |
| T-45 | DOR-02 | Scope channel C (PD5) while closed and while open | period 20 ms (50 Hz); pulse 1.0 ms closed (0°), 1.5 ms open (90°). If the Proteus servo shows other angles, tune `SERVO_MIN_PULSE_US` / `SERVO_MAX_PULSE_US` |

### 3.6 Air conditioning

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-46 | AC-01 | Ambient 25 °C. Terminal: `3` | `AC is OFF. Room temperature 25C.` |
| T-47 | AC-02 | Raise ambient to 28 °C, wait 2 s; then 29 °C | at 28: still OFF (the rule is "higher than 28"); at 29: fan runs within about 1 s, `[INFO] AC is now ON` |
| T-48 | AC-02 | Lower to 25 °C, then 21 °C, then 20 °C | ON at 25 and at 21 (hysteresis); OFF at 20, `[INFO] AC is now OFF` |
| T-49 | AC-02 | From 20 °C raise to 25 °C | stays OFF between 21 and 28 |
| T-50 | AC-03 | Scope channel B (PD4) with the AC on, then off | on: 50 Hz PWM at the configured speed (100 % = flat high), the motor turns; off: flat low, the motor stops |

### 3.7 Water heater

Set water LM35 by hand. "set" starts at 60. Heating = red LED on PB6, cooling = blue LED on PB7, heater LED = PA3.

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-51 | HTR-05, HTR-06 | Start | 7-segment blank, heater LED off, both elements off |
| T-52 | HTR-05 | Press and hold ON/OFF for 2 s, then release | nothing while held; on **release** the display shows the water temperature. Press and release again: everything off |
| T-53 | HTR-06 | Heater OFF: press Up, press Down | nothing happens (decision D-10) |
| T-54 | HTR-01, HTR-11 | Heater ON: press Up once | the display shows `60` blinking; the value has **not** changed |
| T-55 | HTR-02, HTR-11 | Press Up, Up, Down | 65, 70, 65 — each shown immediately |
| T-56 | HTR-03 | Keep pressing Up; then keep pressing Down | stops at 75; stops at 35 |
| T-57 | HTR-11 | In setting mode: the I²C debugger (writes to 0x21 / 0x22 every 0.5 s) or a stopwatch | the digits are on 0.5 s and off 0.5 s (1 s period) |
| T-58 | HTR-12 | Stop pressing | 5 s after the last press the display stops blinking and shows the water temperature |
| T-59 | HTR-12 | In setting mode press Up every 3 s, four times | the mode stays active (each press restarts the 5 s) |
| T-60 | HTR-04, EEP-01 | Set 50, wait for the mode to end; watch the I²C debugger | one page write to 0xA0, word address 0x10, first data byte 0x32 |
| T-61 | HTR-04 | RESET, heater ON, press Up once | blinking `50`: the value came back from the EEPROM |
| T-62 | HTR-03 | Blank EEPROM, start, heater ON, press Up once | blinking `60` |
| T-63 | HTR-07, HTR-10 | Heater ON, change the water LM35 from 40 to 50 in one step | the display climbs from 40 to 50 over about 1 s (ten 100 ms samples enter the average) |
| T-64 | HTR-08 | Heater OFF, water 40, set 60. Turn ON and watch the red LED | the heating element switches on about 1 s after turn-on, not at once (no decision before 10 samples) |
| T-65 | HTR-09 | set = 60. Water 50 -> 54 -> 56 -> 64 | 50: heating ON, cooling OFF. 54: the same. 56 and 64: unchanged (still heating) |
| T-66 | HTR-09 | Water 66 -> 62 -> 56 -> 54 | 66: heating OFF, cooling ON. 62 and 56: unchanged (still cooling). 54: cooling OFF, heating ON. The two LEDs are never on together |
| T-67 | HTR-13 | Heating element on | heater LED blinks, 0.5 s on / 0.5 s off |
| T-68 | HTR-13 | Cooling element on | heater LED steadily on |
| T-69 | HTR-06, HTR-13 | Press ON/OFF while heating | both elements, the heater LED and the display go off |
| T-70 | HTR-14 | Terminal: `4`; `5 65`; `5 62`; `5 80` | status line; `[OK] ... 65C` and the page write appears on the I²C debugger; both others rejected |
| T-71 | HTR-14 | Panel in setting mode; terminal: `5 45` | the blinking value becomes 45 and the 5 s restart |
| T-72 | HTR-14 | Admin logged in on the terminal; press the heater ON/OFF button | the heater switches; `[INFO] Heater is now ON` |

### 3.8 Alarm

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-73 | ALM-01, SEC-05 | Heater heating, AC on, lamps 1 and 2 on, dimmer 50 %, door open. Cause a lockdown (T-08) | buzzer beeps; heating and cooling off; fan off; 7-segment blank; lamps, dimmer and door unchanged |
| T-74 | ALM-01 | In lockdown: keypad keys, terminal input, heater buttons | all ignored; the buzzer continues until RESET |
| T-75 | ALM-02 | Optional PIR alarm | **not built in v1** (decision D-16); no step |

### 3.9 Storage and timing

| # | Requirements | Step | Expected |
|---|---|---|---|
| T-76 | EEP-01, EEP-02 | Blank EEPROM, start, wait 1 s, pause; open the 24C08 memory window | 0x00 = A5, 0x01 = 01, 0x10 = 3C, 0x20 = `admin`, 0x28 = `1234`, 0x30–0xCF = 00. The I²C debugger shows the pages written from 0xC0 down to 0x00 |
| T-77 | EEP-03 | Admin: add three users one after the other while the heater display is blinking; watch the I²C debugger and the display | one 16-byte page write per user, followed by short `A0 N` polls until `A0 A`; the blinking and the keypad never stall |
| T-78 | EEP-03 | Remove the 24C08 from the schematic, start, log in | the system starts normally (no hang), `[WARN] EEPROM not responding...` after login, `admin` / `1234` works from RAM |

---

## 4. Coverage

| Requirement | Steps | Requirement | Steps |
|---|---|---|---|
| SEC-01 | T-01, T-02 | HTR-01 | T-54 |
| SEC-02 | T-04 | HTR-02 | T-55 |
| SEC-03 | T-03, T-05, T-06 | HTR-03 | T-56, T-62 |
| SEC-04 | T-07 | HTR-04 | T-60, T-61 |
| SEC-05 | T-08 – T-12, T-73 | HTR-05 | T-51, T-52 |
| SEC-06 | T-13, T-14 | HTR-06 | T-51, T-53, T-69 |
| SEC-07 | T-15 | HTR-07 | T-63 |
| SEC-08 | T-16 – T-20 | HTR-08 | T-64 |
| SEC-09 | T-01, T-21 | HTR-09 | T-65, T-66 |
| LUI-01 | T-22 | HTR-10 | T-63 |
| LUI-02 | T-23 – T-28, T-32 | HTR-11 | T-54, T-55, T-57 |
| LUI-03 | T-29, T-30 | HTR-12 | T-58, T-59 |
| LUI-04 | T-31 | HTR-13 | T-67, T-68, T-69 |
| RUI-01 | T-33, T-39 | HTR-14 | T-27, T-70, T-71, T-72 |
| RUI-02 | T-34, T-35 | ALM-01 | T-08, T-12, T-73, T-74 |
| RUI-03 | T-36, T-37 | ALM-02 | T-75 (optional, not built) |
| RUI-04 | T-38 | EEP-01 | T-60, T-76 |
| LGT-01 | T-24, T-40, T-41 | EEP-02 | T-01, T-07, T-76 |
| LGT-02 | T-25, T-42, T-43 | EEP-03 | T-77, T-78 |
| DOR-01 | T-13, T-44 | AC-01 | T-26, T-46 |
| DOR-02 | T-45 | AC-02 | T-47, T-48, T-49 |
| | | AC-03 | T-50 |
