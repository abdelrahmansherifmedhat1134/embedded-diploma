# CLAUDE.md — ATmega32 Smart Home + Water Heater

This file is the single source of truth for this project. Read it fully at the start of every session.
The original course specs are in `04_Graduation_Project/docs/Graduation_Projects.pdf` (Project 1: Smart Home, Project 2: Electric Water Heater).
The approved design is in `docs/` (architecture, pin map, EEPROM map, UART protocol, test plan). This file states the rules and requirements; the docs hold the details.
If this file and the PDF disagree, follow this file and point out the difference.

---

## 1. What this project is

One firmware image for an **ATmega32** that merges two course projects into a single smart home:

- **Smart Home (Project 1):** remote control over UART (Bluetooth/TTL), local keypad + LCD control, admin/user login stored in EEPROM, 5 on/off lamps, 1 dimmable lamp, servo door, temperature-controlled AC fan, alarm on 3 failed logins.
- **Electric Water Heater (Project 2):** Up/Down/ON-OFF buttons, 2-digit 7-segment display, water temperature sensing every 100 ms with a 10-sample average, heating and cooling elements with a ±5 °C band, set temperature saved in external EEPROM.
- **Integration ("best of both"):** the heater is one more appliance of the smart home. It keeps its own physical panel, and its status and set temperature are also reachable from the remote terminal and shown on the LCD status screen.

The project is graded on **clean embedded software engineering**, not just "it works":
layered architecture (MCAL / HAL / SERVICE / APP), use of timers and interrupts instead of blocking delays, persistent storage in EEPROM, and two communication protocols (UART + I²C).

Target: simulation in **Proteus**. Build system: **PlatformIO** (bare-metal avr-gcc / avr-libc). Host OS: Windows.

---

## 2. Build, run, verify

- Build: `pio run` (run from the project root). Build after **every** module you add or change, and fix all errors and warnings before moving on.
- Output for Proteus: `.pio/build/<env>/firmware.hex`.
- `F_CPU`: read it from `platformio.ini` (`board_build.f_cpu`). Never assume a clock. All timer, UART baud and tick calculations must be derived from `F_CPU` and must be written as compile-time calculations or clearly commented constants.
- Proteus clock: remind the user that the ATmega32 clock in Proteus must equal `F_CPU`.
- Do **not** change `platformio.ini` (board, framework, clock, flags) without asking. The only exception is adding `[env:test_*]` environments (see Section 3).
- Build a specific target with `pio run -e <env>` (`app` = real firmware, `test_<layer>` = layer test). If the framework is `arduino`, stop and ask — this project is meant to be bare-metal.
- There is no hardware-in-the-loop here. "Verified" means: it compiles cleanly, the logic has been traced by hand against the spec, and a Proteus test step exists for it in `docs/test_plan.md`.

---

## 3. How to work in this repo (mandatory workflow)

### Phase 0 — Learn the existing code + set up tests (no driver changes)
**Status: DONE (2026-10-01).** Merged in PR #1 and PR #2. Next: Phase 1.
1. Read every existing file (drivers, headers, `main.c`, `platformio.ini`, folder layout).
2. Fill in **Section 4 (Detected conventions)** of this file with concrete examples taken from the code.
3. List every existing driver, what it supports, and what is missing for this project (e.g. "DIO: ok", "LCD: 8-bit only, needs 4-bit or I²C", "Timer: no CTC mode").
4. Set up the test infrastructure described in "Test program per layer" below: add `[env:app]` and `[env:test_base]` to `platformio.ini`, create `test_mains/`, and write `test_mains/test_base.c`, a smoke test of the **existing** drivers only. You may edit `platformio.ini` for this.
5. Build `pio run -e test_base` and `pio run -e app`, then report back and wait.

### Phase 1 — Design (no code changes except `docs/`)
**Status: DONE (2026-10-02).** PR #3. All decisions are recorded in Section 12 and in `docs/architecture.md` Section 9. Next: Phase 2, MCAL.
Produce these files and wait for the user's approval before writing firmware:
- `docs/architecture.md` — layer diagram, module list, which module calls which, the scheduler design, and each APP state machine (states, events, transitions).
- `docs/pin_map.md` — final pin table (start from Section 7 and adapt it to the existing drivers).
- `docs/eeprom_map.md` — byte-level layout (start from Section 9).
- `docs/uart_protocol.md` — every prompt and command (start from Section 10).
- `docs/test_plan.md` — numbered Proteus test steps, one or more per requirement ID in Section 6.
- A list of the open decisions from Section 12 with your recommended answer for each.

### Phase 2 — Implement bottom-up
**Phase 2 MCAL: DONE (2026-10-02).** Branch `phase2-mcal`, `test_mcal` added.
**Phase 2 HAL: DONE (2026-10-03).** Branch `phase2-hal`, `test_hal` added; as-built notes, Proteus findings and the 7-segment story are in `docs/hal_summary.md`.
**Phase 2 SERVICE: DONE (2026-10-03).** Branch `phase2-service`, `test_service` added (static RAM 564 bytes, flash 22 536 bytes); as-built notes are in `docs/architecture.md` Section 3.3.
**Phase 2 APP: part A done (2026-10-03).** Branch `phase2-app`, `test_app` added with LIGHT, DOOR, CLIMATE, HEATER, ALARM, SEC (static RAM 628 bytes, flash 13 000 bytes). Next: part B (UILOC, UIREM).
Order: MCAL → HAL → SERVICE → APP → `main.c`. One module at a time:
write → `pio run` → fix → short summary of what changed → next module.
Stop for review after finishing each layer.

### Phase 3 — Integrate and document
Wire everything in `main.c`, run the full build, update `docs/test_plan.md` and `docs/architecture.md` to match the real code.

### General rules
- Prefer small, reviewable changes. Never rewrite a working existing driver just to restyle it.
- When editing an existing driver, change only what is needed, in the author's style, and keep its public API. If an API change is unavoidable, ask first.
- Never delete or rename existing files without asking.
- Every requirement you implement must be traceable: put the requirement ID from Section 6 in a comment where it is implemented (e.g. `/* REQ-HTR-07 */`).
- If a spec point is ambiguous, use the default in Section 12, mark it in the code with `/* ASSUMPTION: ... */`, and mention it in your summary. Do not silently invent behavior.
- Git: one branch per phase or layer. Commit once per finished module with a clear message. Open a PR when the layer is done and stop for review.

### Test program per layer (mandatory)
- `src/APP/main.c` is the real application. It is only rewritten in Phase 3; until then it stays as it is.
- Phase 0 ends with `test_mains/test_base.c` (existing drivers). Every Phase 2 layer (MCAL, HAL, SERVICE, APP) ends with its own test program:
  `test_mains/test_<layer>.c` plus a matching `[env:test_<layer>]` in `platformio.ini`.
- Each test environment uses `build_src_filter` to exclude `src/APP/main.c` and include only its own test file (plus all driver sources). Adding a test environment does not need permission; any other `platformio.ini` change still does. Already approved: adding `Service` to `lib_deps` when the SERVICE layer gets its first `.c` file (and the same line for `[env:app]` in Phase 3).
- A test program exercises **every module of its layer** and reports:
  - over UART: one line per check, `[PASS] <module>: <what>` or `[FAIL] <module>: <what>`, then a final summary line;
  - on a status LED: blinking = test running, solid ON = all passed, fast blink = at least one failure.
  - If UART is not yet tested/available, use LEDs only.
- Checks that need the user (keypad, buttons, sensors, servo angle, PWM duty) print what to do in Proteus and what the user should see, and use the scheduler/tick rather than long delays once it exists.
- Each test file starts with a header comment: Proteus parts needed, their wiring (matching `docs/pin_map.md`), and the expected result.
- A layer is only **done** when `pio run -e test_<layer>` builds with zero warnings, and `test_base` and the earlier `test_<layer>` environments still do.
- Until Phase 3, `pio run -e app` is **not** a compile check for new modules: `src/APP/main.c` includes no drivers, so nothing from `lib/` is built there. The per-module compile check is `pio run -e test_base` (it builds all of MCAL and HAL with `-Wall`), and `pio run -e test_<layer>` once that layer's test exists. `app` becomes the real check when `main.c` is rewritten in Phase 3.
- End each layer with exactly 3 lines for the user: the build command, the `.hex` path to load in Proteus (`.pio/build/test_<layer>/firmware.hex`), and what to watch for.

---

## 4. Detected conventions (Claude fills this in during Phase 0)

> Match these exactly in all new and edited code. The goal is that new files look like the same person wrote them.

> Reference style = the course author's drivers (DIO, ADC, EXTI, GIE, TIMER0, USART, CLCD, KPAD, SSG).
> TWI was restyled to this standard (`TWI.c/.h/_cfg.h`, `u8`, `reg_def.h`, `TWI_u8SendStartCondition`). One unified style across all layers — no `<avr/io.h>`/`stdint.h` in project code.

- Folder / file layout: `lib/MCAL/<MOD>/`, `lib/HAL/<MOD>/`, `lib/Service/` (shared `std_types.h`, `Bit_math.h`), `lib/MCAL/reg_def.h` (all register addresses), app in `src/APP/main.c`. One folder per module. New SERVICE modules go in `lib/Service/<MOD>/`, new APP modules in `src/APP/<MOD>/`.
- File naming: `DIO.c` / `DIO.h` / `CLCD_cfg.h` (module name in caps, `_cfg.h` for config). Older SSG uses `SSG_prog.c` / `SSG_int.h` / `SSG_CFG.h` — new modules follow the `MOD.c / MOD.h / MOD_cfg.h` form.
- Function naming: `MODULE_<rettype><Name>` — `DIO_voidSetPinValue`, `DIO_u8GetPinValue`, `ADC_u16StartConversion`, `CLCD_voidSendString`. Params `Copy_u8PortID`, locals `Local_u8data`. Some setters drop the type (`TIMER0_SetCallBack_OV`, `ADC_SetCallBack`); prefer the typed form. Constants `MODULE_NAME` macros (`DIO_PORTA`, `TIMER0_DIV_64`, `EXTI_FALLING_EDGE`).
- Types: `u8/u16/u32/s8/.../c8/f32` from `lib/Service/std_types.h` (`u32`/`s32` = `unsigned/signed long` = 32 bit, fixed in Phase 0; `NULL` defined there). No `<stdint.h>` in author code.
- Register access: `SET_BIT/CLR_BIT/GET_BIT/TOG_BIT` from `Bit_math.h` on registers defined as `*(volatile u8 *)(addr)` in `reg_def.h` with bit-name macros (`ADCSRA_ADEN`, `UCSRB_RXEN`). No `<avr/io.h>`. Masks written as binary literals (`ADMUX &= 0b11100000`). New registers (Timer1, Timer2, TWI) get added to `reg_def.h` the same way.
- Return values / error handling: `void` functions; argument range checked with `if(...) {...} else { //error }`; getters return the value. No enums. Where a bus can fail (TWI, and HAL drivers on top of it) the function returns a `u8` status using `#define`d codes (`TWI_OK`, `TWI_ERR_SLA_NACK`, …).
- Config style: pre-build `#define`s in `MOD_cfg.h` (pins as `DIO_PORTx`/`DIO_PIN_n`, mode selection via `#if CLCD_MODE == ...`). SSG uses a config struct `SSG_t` passed by pointer.
- Header guards / includes / comments: guards like `HAL_CLCD_CLCD_H_` or `DIO_H_`; Eclipse header block (`/* * FILE.h * Created on: ... * Author: eslam */`). `.c` include order: `../../Service/std_types.h`, `../../Service/Bit_math.h`, lower-layer headers, `../reg_def.h`, own `.h`, own `_cfg.h`. Headers do not include `std_types.h` themselves. Step comments like `/*1. Select ref */`.
- Interrupt style: `void __vector_N () __attribute__ ((signal));` prototype in the `.h`, body in the `.c`; ISR calls a user callback through a global function pointer (`void (*TIMER0_ov_ptr)(void) = NULL;`, set via `..._SetCallBack...`), with a `!= NULL` check (EXTI lacks it). Global interrupt via `GIE_voidEnableGlobalInterrupt()` (`__asm("SEI")`).
  **Required for new ISRs:** declare them `__attribute__ ((signal, used, externally_visible))`. With only `signal`, PlatformIO's LTO discards the ISR at link time (found in Phase 0: ADC/TIMER0/EXTI handlers were dropped, so `test_base` builds with `build_unflags = -flto` until they are fixed).
- Delay usage: `<util/delay.h>`. CLCD in I²C mode (the selected mode): 40/5/1 ms one-time init delays, 2 ms after clear/home, no per-pulse delay (each I²C write already takes ~0.3 ms). The parallel CLCD modes (unused) still have a 10 ms enable pulse. ADC sync, USART send/receive, KPAD (waits for key release) and TWI (~90 µs per byte, no timeout) all busy-wait.
- Indentation, brace style: tabs, K&R braces on the same line, `switch` with `case X: stmt; break ;` on one line, space before `;` in `break ;` / `return x ;`. Files are CRLF. No fixed line-width limit.
- Strings in flash: `<avr/pgmspace.h>` cannot be used, because it pulls in `<avr/io.h>`, which clashes with `reg_def.h`. Use GCC's `__flash` instead: `const __flash c8 *` parameters, plus a `FLASH_STR("...")` macro (statement expression with a `static const __flash c8[]`), from `lib/Service/flash_str.h` (moved there in Phase 2 SERVICE; include it after `std_types.h`), so all layers share it.
- Test programs: `test_mains/test_<layer>.c`, functions `TEST_voidName`, checks through `TEST_voidCheck(ok, FLASH_STR("MOD"), FLASH_STR("what"))`, status LED on PA3, Proteus wiring in the header comment.

### Known driver issues after Phase 0 (fix in Phase 2, bottom-up)
Fixed in Phase 0: USART UCSRC read-modify-write (now one write), KPAD transposed key table and column left LOW, SPI stub missing `return`, `u32`/`s32` were 16 bit.
- **FIXED (Phase 2 MCAL, 2026-10-02)** · **ISRs (ADC, TIMER0, EXTI):** missing `used, externally_visible`, so LTO drops them (see Interrupt style). Fix, then remove `build_unflags = -flto` from `[env:test_base]`.
- **FIXED (Phase 2 MCAL, 2026-10-02)** · **TIMER0:** `TIMER0_GeneratePWM` has the COM bits swapped (NONINVERTED sets 11), and 100 % duty gives `OCR0 = 256` → 0. `test_base` reports both as `[FAIL]`.
- **FIXED (Phase 2 MCAL, 2026-10-02)** · **EXTI:** `EXTI_voidINTx_callBack` is not declared in `EXTI.h`, and the ISRs call the callback without a `NULL` check.
- **FIXED (Phase 2 MCAL, 2026-10-02)** · **ADC:** no function to switch `ADIE` off; after one async conversion the sync `ADC_u16StartConversion` hangs (the ISR clears `ADIF`). The comment in `ADC_voidInit` says AVCC, but the code selects the internal 2.56 V reference (which is what we want for the LM35: 4 steps per °C).
- **FIXED (Phase 2 MCAL, 2026-10-02)** · **USART:** baud value hard-coded (`UBRRL = 103`, correct only for 16 MHz) — must be computed from `F_CPU`. TX/RX are blocking; Section 5 needs RX interrupt + ring buffers.
- **FIXED (Phase 2 HAL, 2026-10-02)** · **KPAD:** blocks until the key is released; needs a non-blocking, debounced scan for the scheduler.
- **FIXED (Phase 2 MCAL, 2026-10-02)** · **TWI:** blocking, with no timeout (a stuck bus hangs the loop). The 24C08 needs non-blocking ACK polling (EEP-03).
- **FIXED (Phase 2 HAL, 2026-10-02)** · **CLCD:** `CLCD_voidClearDisp` exists but is not declared in `CLCD.h`; no text-from-flash function. `CLCD_voidInit` switches the cursor and blink ON (must be off for the status screen). One LCD byte costs 4 I²C transactions (~1.2 ms); it becomes one transaction (~0.5 ms).
- **FIXED (Phase 2 HAL, 2026-10-02)** · **KPAD (RAM):** `KPAD_MAT` and the pin arrays are not `const`, so they sit in RAM (24 bytes).
- **main.c:** includes `../lib/service/Std_Types.h` with the wrong letter case (works on Windows only). Rewritten in Phase 3.
- The exact fix for every item is in `docs/architecture.md` Section 2.
- **FIXED (Phase 2 HAL, 2026-10-02)** · **SSG:** writes raw segments to a whole port; does not fit the 2-digit design → new SEVEN_SEG driver (two PCF8574 chips since D-20; the first 7447 + multiplexed version was dropped because it was unreliable in Proteus).

### Proteus notes found in Phase 0
- A single read of the shared UBRRH/UCSRC address seems to return UCSRC in Proteus (a real ATmega32 returns UBRRH), so UBRRH cannot be verified in simulation.
- Parts powered from DC generators do not appear as VCC in the `.SDF` netlist; check the schematic before calling a pin "unconnected".
- Keypad wiring: keypad rows A–D → PA4–PA7, columns 1–4 → PB0, PB1, PB2, PB4.
- The `.SDF` netlist does not export simulation-only parts (keypad, 7-segment display, push buttons, motors, servo, instruments). A pin that goes only to such a part looks open in the netlist; that does not mean it is unconnected.
- Reading back a PCF8574 whose pins drive 7-segment LEDs returns a different value than the one written, while the display shows the right digit (found with `hal_testing_v2`). Do not read-back-check the segment chips; the `test_hal` 7-segment steps are manual.
- The Proteus 24C08 model has **no write-cycle delay**: it ACKs its address right after a write (`test_hal` reports "ready at once" as `[INFO]`, not a failure). The real chip needs about 5 ms, so ACK polling in `ESTORE` must still be written for it. (An earlier note here said the model had a 10 ms `TD_WRITE`; that was wrong.)
- The Proteus buzzer ("DC Buzzer with Sound") is silent with its defaults (`VNOM=5V`, `LOAD=12`) on PD3. It works directly on PD3 with Operating Voltage = 3 V and Load Resistance = 150 Ω. **Real hardware still needs an NPN driver stage** (pin_map C-9): PD3 -> 1k -> base, buzzer between +5 V and the collector, flyback diode.
- The Proteus servo model maps a 1-2 ms pulse to -90..+90 degrees by default. Set its Min/Max Angle properties to 0 / 180 so 1 ms = 0 degrees and 2 ms = 180 degrees.

### Expertise level
Write code at the same level as the existing drivers: plain C, readable, well commented, no clever tricks.
Do not introduce things the existing code does not use (RTOS, function-pointer dispatch tables, heavy macro metaprogramming, dynamic memory, `printf`) unless it is clearly needed — and if so, explain it in one short comment and in your summary.

---

## 5. Architecture

```
APP       : SEC (security), ALARM, LIGHT, DOOR, CLIMATE, HEATER, UILOC (LCD + keypad), UIREM (UART terminal)
SERVICE   : system    = SCHED (time base + task flags), TERM (UART rings + line editor), ESTORE (EEPROM image + write-behind), USERDB (accounts), EVQ (event queue)
            utilities = RINGBUF, MAVG (moving average), FMT (number <-> text), flash_str.h, std_types.h, Bit_math.h
HAL       : CLCD, LCD_BUF, KPAD, BUTTON, LM35, SEVEN_SEG, LAMP, RELAY, LED, DIMMER, SERVO, FAN, BUZZER, EXT_EEPROM (24C08), PCF8574
MCAL      : DIO, GIE, ADC, TIMER0, TIMER1, TIMER2, USART, TWI (I²C), EXTI
```
The full module list with the API of every module is in `docs/architecture.md`.

Rules:
- A layer only calls the layer directly below it (APP may also use SERVICE). MCAL never includes HAL or APP headers.
- Two approved exceptions: `SCHED` and `TERM` (SERVICE) call MCAL directly (`TIMER2`, `USART`), because no chip sits in between. SERVICE utilities (`RINGBUF`, `MAVG`, `FMT`, the type headers) touch no hardware and may be included from any layer.
- A HAL module may use a lower HAL module when one chip sits behind another (`CLCD → PCF8574`, `LCD_BUF → CLCD`, `LAMP → PCF8574`).
- Inside APP: UI modules call SEC and the feature modules; feature modules never call a UI, they post events to `EVQ`.
- HAL modules hide pins and chips. APP code never touches a register or a port/pin number.
- All pin assignments live in HAL/MCAL config headers, never in APP code.
- `main.c` only does: init all modules, enable global interrupts, run the super-loop that dispatches scheduler tasks.

### Timing model (no blocking delays)
- One hardware timer produces a **1 ms system tick** (see Section 8). The ISR only increments a tick counter and sets task flags. It does no real work at all (the 7-segment display has its own PCF8574 chips and needs no refresh, decision D-20).
- The super-loop checks the flags and runs tasks: 5 ms (LCD update), 10–20 ms (keypad scan, button debounce), 100 ms (temperature sampling), 500 ms / 1 s (blinking, timeouts).
- Every APP module is a **non-blocking state machine**: it never waits in a loop; it keeps its state and returns.
- Allowed delays: microsecond-level delays required by a chip's timing inside a HAL driver (e.g. LCD enable pulse), and one-time delays during init before the scheduler starts (e.g. LCD power-up). Nothing longer than ~2 ms after init.
- UART RX uses the RX-complete interrupt into a ring buffer. UART TX should use a TX ring buffer with the UDRE interrupt so long messages never block the loop.
- Variables shared between an ISR and the main loop are `volatile`. Multi-byte shared variables are read with interrupts briefly disabled (or `ATOMIC_BLOCK`).

### Memory limits (ATmega32: 32 KB flash, 2 KB RAM, 1 KB internal EEPROM)
- All UART and LCD text constants go in flash with `__flash` / `FLASH_STR("...")` (see Section 4, "Strings in flash") and are sent by functions that take a `const __flash c8 *`. Many prompt strings in RAM would overflow the 2 KB.
- No dynamic memory. Keep buffers small and sized with named constants.

---

## 6. Requirements (with IDs)

### Security and login — REQ-SEC
- **SEC-01** Two roles: **admin** and **user**. Admin logs in **only remotely** (UART). Users log in via keypad/LCD or remotely.
- **SEC-02** Keypad users and remote users are **separate lists** with separate usernames. Keypad usernames are numeric (the keypad has digits only).
- **SEC-03** Admin can add and remove users (both lists) from the remote terminal.
- **SEC-04** Usernames and passwords survive power-off (EEPROM).
- **SEC-05** The **3rd wrong login in a row** (username or password, admin or user, counted per login source) → the system **locks down and sounds the alarm until reset**. See Section 12 for exactly what "locks down" means. (The course text says "more than 3"; the user decided on the 3rd.)
- **SEC-06** Admin and users can control everything, **except users cannot open/close the door**.
- **SEC-07** EEPROM access: admin = read/write, user = read-only. This applies to the **accounts**: only the admin can add, remove or change them (a write gate in `USERDB`, open only while the admin is logged in). The heater set temperature is not covered by this rule (HTR-14).
- **SEC-08** Keypad users can control the system while a remote user is logged in. While the **admin** is logged in remotely, keypad control is blocked until the admin allows it (see Section 12).
- **SEC-09** Default admin credentials are written on first boot when the EEPROM has no valid data (detected by a magic byte). Show them in `docs/uart_protocol.md`.

### Local UI (LCD + keypad) — REQ-LUI
- **LUI-01** Used for emergency / non-mobile control, **user mode only**.
- **LUI-02** Menu-driven: login → main menu → lamps / dimmer / AC status / heater (status and set temperature) → logout. Clear, short LCD screens.
- **LUI-03** When the keypad/LCD is not in use (not logged in, or after an idle timeout), the LCD shows a **status screen of running devices** (lamps on, dimmer %, AC on/off + room temp, heater state + water temp).
- **LUI-04** Password digits are shown as `*`.

### Remote UI (UART) — REQ-RUI
- **RUI-01** Two-way communication with a PC/mobile via HC-05 Bluetooth or a USB-TTL adapter (Proteus: Virtual Terminal).
- **RUI-02** Every action and state change prints a message on the remote screen, e.g. `Hey, please enter your username:`.
- **RUI-03** Menu/command interface for all features; admin sees admin-only options (user management, door, allow-local-control).
- **RUI-04** Invalid commands get a clear error message and the menu is shown again. (After a successful command only the prompt is printed; `?` shows the menu.)

### Lighting — REQ-LGT
- **LGT-01** 5 on/off lamps through isolating transistor + relay (LEDs acceptable in simulation).
- **LGT-02** 1 dimmable lamp: a dimmer circuit (transistor + solid-state relay) controlled by a 0–5 V signal → generated by PWM (optionally RC-filtered). Level set in steps (e.g. 0–100 % in 10 % steps).

### Door — REQ-DOR
- **DOR-01** Servo motor opens/closes the door, **admin only**, by remote command "open door" / "close door".
- **DOR-02** Servo driven by hardware PWM at 50 Hz (Timer1), closed = 0°, open = 90° (tune pulse widths as constants).

### Air conditioning — REQ-AC
- **AC-01** LM35 reads **ambient** temperature.
- **AC-02** If temperature > 28 °C → AC (DC motor fan) ON. If temperature < 21 °C → AC OFF. Between 21 and 28 → keep the current state (hysteresis).
- **AC-03** Fan driven by PWM through the NPN driver stage. Optional enhancement after the spec is done: speed increases with temperature above 28 °C.

### Water heater — REQ-HTR
- **HTR-01** Up/Down buttons change the set temperature. The **first** press of either only enters **setting mode** (no change).
- **HTR-02** In setting mode, each Up press = +5 °C, each Down press = −5 °C.
- **HTR-03** Set temperature range 35–75 °C (clamp). Initial value 60 °C when EEPROM has no valid value.
- **HTR-04** Set temperature is saved to external EEPROM once set, and restored at power-up.
- **HTR-05** At power-up the heater is **OFF**. Releasing ON/OFF toggles OFF ↔ ON (act on **release**, not press).
- **HTR-06** In OFF state: all heater displays off (7-seg blank, heater LED off) and both elements off.
- **HTR-07** Water temperature is sampled **every 100 ms** (separate LM35 from the ambient one).
- **HTR-08** Heating/cooling decisions use the **average of the last 10 readings**. No decision until 10 samples exist after turning ON.
- **HTR-09** avg < set − 5 → heater ON, cooler OFF. avg > set + 5 → heater OFF, cooler ON. Between → keep current states.
- **HTR-10** 7-seg shows the **current water temperature** by default.
- **HTR-11** In setting mode, the 7-seg shows the **set temperature and blinks with a 1 s period**; every change is shown immediately.
- **HTR-12** Setting mode exits after **5 s** without Up/Down presses.
- **HTR-13** Heater LED: heating element ON → LED blinks with a 1 s period. Cooling element ON → LED steady ON. Neither → LED off.
- **HTR-14** Integration: remote terminal and keypad can show heater state, water temp, set temp, and (for admin, remote users and keypad users) change the set temperature with the same rules (35–75, steps of 5, saved to EEPROM). The heater's physical ON/OFF button always works.

### Alarm — REQ-ALM
- **ALM-01** Buzzer sounds on lockdown (SEC-05) until MCU reset.
- **ALM-02** Optional enhancement: PIR motion sensor triggers the alarm when an "away/armed" mode is set by admin.

### Storage — REQ-EEP
- **EEP-01** External **24C08** I²C EEPROM (satisfies both specs; counts as the second protocol). Internal EEPROM only if the user decides so in Phase 1.
- **EEP-02** Data layout and a magic/version byte as in Section 9.
- **EEP-03** EEPROM writes must not block: use the 24C08 write-cycle **ACK polling** in a non-blocking way, or schedule the next write for a later tick — never a 5–10 ms delay in the loop.

---

## 7. Pin map (final — full detail in `docs/pin_map.md`)

The full feature list does **not** fit the 32 I/O pins if everything is wired directly (~41 pins needed). **Decided:** LCD and the 5 lamps on the I²C bus (PCF8574 expanders), 7-segments on two more PCF8574 expanders (one per digit, no multiplexing), and (Phase 1) the dimmer on Timer0 and the AC fan on Timer1.

| Pin | Function | Notes |
|---|---|---|
| PA0 | ADC0 — LM35 ambient (AC) | |
| PA1 | ADC1 — LM35 water (heater) | |
| PA2 | ADC2 — LDR | optional |
| PA3 | Heater LED | |
| PA4–PA7 | Keypad rows 1–4 | freed by LCD → I²C |
| PB0 | Keypad column 1 | freed by LCD → I²C |
| PB1 | Keypad column 2 | freed by LCD → I²C |
| PB2 | Keypad column 3 | freed by LCD → I²C |
| PB3 | OC0 — dimmer PWM | Timer0, 7.8 kHz, RC-filtered |
| PB4 | Keypad column 4 | |
| PB5 | Down button | |
| PB6 | Heating element (SSR) | |
| PB7 | Cooling element (SSR) | |
| PC0 | SCL | I²C bus |
| PC1 | SDA | I²C bus |
| PC2–PC7 | spare | free since D-20 (the 7447 and the multiplexing were removed). PC2–PC5 are JTAG pins on a real chip |
| PD0 | UART RXD | |
| PD1 | UART TXD | |
| PD2 | PIR (INT0) | optional |
| PD3 | Buzzer | |
| PD4 | OC1B — AC fan PWM | Timer1, 50 Hz |
| PD5 | OC1A — door servo | Timer1 |
| PD6 | Heater ON/OFF button | polling + debounce |
| PD7 | Heater Up button | |

I²C bus devices: 24C08 EEPROM (0x50), PCF8574 for the LCD (0x27), PCF8574 for lamps 1–5 on P0–P4 (0x20, **active-low**: lamp ON = bit 0), PCF8574 for the tens digit of the 7-segment display (0x21) and one for the units digit (0x22): P0..P6 = segments a..g, P7 = dp, common anode, **active-low**, 0xFF = blank.

---

## 8. Timer allocation (final)

| Timer | Mode | Use |
|---|---|---|
| Timer0 | Fast PWM, OC0 (PB3), prescaler 8 → 7.8 kHz | dimmer duty |
| Timer1 | Fast PWM mode 14, TOP = ICR1 → 20 ms period | OC1A = servo (1–2 ms pulse), OC1B = AC fan speed |
| Timer2 | CTC, compare-match interrupt | **1 ms system tick** (prescaler/OCR2 computed from `F_CPU`) |

If the existing timer drivers do not support a needed mode, extend them in their own style.

---

## 9. EEPROM layout (24C08 — final, byte addresses in `docs/eeprom_map.md`)

- 16-byte pages; every record is exactly one page. Only pages 0–12 (0x00–0xCF) are used.
- Page 0: magic byte + layout version (written on first boot only). If missing/wrong on boot → write defaults (admin `admin` / `1234`, heater set temp 60 °C, empty user lists).
- Page 1: heater set temperature (1 byte). Its own page, so saving it can never damage the magic byte.
- Page 2: admin record. Pages 3–7: 5 remote users. Pages 8–12: 5 keypad users.
- Record = name (8 bytes) + password (8 bytes), padded with 0x00. No separate valid flag: a first name byte of 0x00 or 0xFF means "empty slot".
- The whole area is mirrored in RAM (`ESTORE`); reads never touch the chip, writes go out one page per 10 ms slot with ACK polling.
- Sizes and offsets are `#define`s in `ESTORE_cfg.h`.
- Lockout state is **not** stored — lockdown lasts until reset.

---

## 10. UART protocol (final — full text in `docs/uart_protocol.md`)

- 9600 baud, 8N1 (confirm the baud error for the chosen `F_CPU`; enable U2X if it helps).
- Menu-driven, numbered options, human-readable prompts, terminated with `\r\n`. Works with the Proteus Virtual Terminal and a Bluetooth terminal app.
- Flow: welcome → `Hey, please enter your username:` → `Please enter your password:` → role-specific main menu → action → confirmation message (e.g. `[OK] Lamp 3 is now ON`) → prompt. The full menu is printed after login, after every error and on `?`.
- Admin menu adds: add/remove remote user, add/remove keypad user, list users, open/close door, allow/block local control, change admin password, factory reset.
- Commands with one argument also accept it on the same line (`1 3`, `5 65`).
- Every state change triggered from anywhere (keypad, buttons, auto AC, heater) is also printed to the remote terminal while someone is logged in remotely.
- Write the full command list and example session in `docs/uart_protocol.md`.

---

## 11. Coding checklist (check before calling any module done)

- [ ] Compiles with zero warnings (`pio run -e test_base`, or `-e test_<layer>` once it exists; `-e app` only from Phase 3, see "Test program per layer").
- [ ] Matches Section 4 conventions exactly.
- [ ] No blocking delays beyond what Section 5 allows.
- [ ] No register access or pin numbers in APP code.
- [ ] Shared ISR variables are `volatile` and read atomically when multi-byte.
- [ ] String constants in flash (`__flash` / `FLASH_STR`).
- [ ] Integer math only (no `float`) unless the user agrees — e.g. LM35 with 2.56 V internal reference: `temp_x4 = adc_value` (0.25 °C units), or with AVCC 5 V: `temp = (adc * 500UL) / 1024`.
- [ ] Requirement IDs commented where implemented.
- [ ] Assumptions marked with `/* ASSUMPTION: */`.
- [ ] `docs/test_plan.md` updated with the Proteus test steps for this module.
- [ ] The module is covered by its layer's test program in `test_mains/`, and `pio run -e test_<layer>` builds cleanly.

---

## 12. Decisions (all decided — Phase 1, 2026-10-02)

1. **SEC-08 remote/local arbitration:** keypad user control is allowed while a remote *user* is logged in; blocked while the *admin* is logged in until the admin sends "allow keypad". An admin login ends an active keypad session, and the allow flag resets at every admin login.
2. **SEC-05 lockdown:** the system locks right after the **3rd** failed login in a row. Counters are per login source (UART / keypad) and reset on a successful login. Safe state: heating element OFF, cooler OFF, fan OFF, lamps / dimmer / door unchanged, buzzer beeps 0.5 s on / 0.5 s off, LCD shows `SYSTEM LOCKED`, UART prints a lock message, all input ignored until MCU reset.
3. **EEPROM:** external 24C08 for everything.
4. **Blink timing (HTR-11, HTR-13):** 1 s period (500 ms on, 500 ms off).
5. **Heater in OFF state:** both elements forced OFF, displays off; sampling continues; the 10-sample average is reset at turn-on. Up/Down are ignored while OFF.
6. **Save moment for set temperature:** when leaving setting mode (5 s timeout), on turn-off if it changed, and at once on a remote or keypad change.
7. **Keypad:** the fitted keypad is a calculator pad (`7 8 9 /`, `4 5 6 *`, `1 2 3 -`, `C 0 = +`). Numeric ID then numeric PIN; `=` = Enter, `*` or `C` = back (delete last digit, or leave the screen when nothing is typed), `+` / `-` = step a value.
8. **Idle timeouts:** LCD session 30 s without keys → logout. Remote session 120 s without input → logout.
9. **Water temperature in Proteus:** the LM35 value is adjusted by hand.
10. **Spec typo:** "Cooling Element is OB" in the PDF means "is ON".
11. **Dimmer and fan pins:** dimmer on PB3 / OC0 (Timer0), fan on PD4 / OC1B (Timer1).
12. **Lamp polarity:** active-low on the PCF8574.
13. **Accounts:** default admin `admin` / `1234`; user lists start empty; passwords are plain text; lamp, dimmer and door states are not stored.
14. **Extras kept:** one-line commands, admin "change password" and "factory reset", keypad users may change the heater set temperature.
15. **Not built in v1:** AC-03 fan speed ramp, ALM-02 PIR, LDR. Pins stay reserved.
