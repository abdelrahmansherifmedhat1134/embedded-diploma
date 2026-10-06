# Specification — ATmega32 Smart Home + Water Heater

Requirements, engineering rules, coding conventions and design decisions of the project.
The original course brief is `04_Graduation_Project/docs/course_brief_graduation_projects.pdf` (Project 1: Smart Home, Project 2: Electric Water Heater); where it is ambiguous, this document records the decision taken (Section 12).
The detailed design is in the other files of `docs/` (architecture, pin map, EEPROM map, UART protocol, test plan).

---

## 1. What this project is

One firmware image for an **ATmega32** that merges two course projects into a single smart home:

- **Smart Home (Project 1):** remote control over UART (Bluetooth/TTL), local keypad + LCD control, admin/user login stored in EEPROM, 5 on/off lamps, 1 dimmable lamp, servo door, temperature-controlled AC fan, alarm on 3 failed logins.
- **Electric Water Heater (Project 2):** Up/Down/ON-OFF buttons, 2-digit 7-segment display, water temperature sensing every 100 ms with a 10-sample average, heating and cooling elements with a ±5 °C band, set temperature saved in external EEPROM.
- **Integration ("best of both"):** the heater is one more appliance of the smart home. It keeps its own physical panel, and its status and set temperature are also reachable from the remote terminal and shown on the LCD status screen.

Design goal: **clean embedded software engineering**, not just "it works" —
layered architecture (MCAL / HAL / SERVICE / APP), use of timers and interrupts instead of blocking delays, persistent storage in EEPROM, and two communication protocols (UART + I²C).

Target: simulation in **Proteus**. Build system: **PlatformIO** (bare-metal avr-gcc / avr-libc). Host OS: Windows.

---

## 2. Build, run, verify

- Build: `pio run -e app` (run from the project root). Every module is built with `-Wall` and zero warnings.
- Output for Proteus: `.pio/build/<env>/firmware.hex`.
- `F_CPU`: read it from `platformio.ini` (`board_build.f_cpu`). Never assume a clock. All timer, UART baud and tick calculations must be derived from `F_CPU` and must be written as compile-time calculations or clearly commented constants.
- Proteus: the ATmega32 clock in Proteus must equal `F_CPU`.
- `platformio.ini` defines one environment for the application (`app`) and one per layer test (`test_<layer>`).
- Build a specific target with `pio run -e <env>` (`app` = real firmware, `test_<layer>` = layer test). The project is bare-metal: no Arduino framework.
- There is no hardware-in-the-loop here. "Verified" means: it compiles cleanly, the logic has been traced by hand against the spec, and a Proteus test step exists for it in `docs/test_plan.md`.

---

## 3. Development process

The firmware was built in gated phases; each phase ended with a review and a merge.

| Phase | Content | Result |
|---|---|---|
| 0 | Study of the existing course drivers, conventions (Section 4), test infrastructure | `test_base` smoke test of the existing drivers |
| 1 | Design: architecture, pin map, EEPROM map, UART protocol, test plan, open decisions | `docs/` design set, decisions in Section 12 |
| 2 | Bottom-up implementation, one layer at a time: MCAL → HAL → SERVICE → APP | one test program per layer: `test_mcal`, `test_hal`, `test_service`, `test_app` |
| 3 | Integration: `src/APP/main.c` (boot order + super-loop), final documentation | `app` firmware, all six builds with zero warnings |

Every layer was verified in Proteus before the next one started; the APP layer test is recorded step by step in `docs/test_app_partA_sequence.pdf` and `docs/test_app_partB_sequence.pdf`.

### General rules
- Small, reviewable changes. A working existing driver is never rewritten just to restyle it.
- When an existing driver is edited, only what is needed changes, in the author's style, and its public API is kept.
- Every implemented requirement is traceable: its ID from Section 6 is in a comment where it is implemented (e.g. `/* REQ-HTR-07 */`).
- Where the specification is ambiguous, the decision of Section 12 is used and marked in the code with `/* ASSUMPTION: ... */`.
- Git: one branch per phase or layer, one commit per finished module, one pull request per layer.

### Test program per layer
- Every layer has its own test program `test_mains/test_<layer>.c` with a matching `[env:test_<layer>]` in `platformio.ini`; it excludes `src/APP/main.c` and builds only its own test file plus the driver libraries.
- A test program exercises every module of its layer and reports over UART, one line per check (`[PASS] <module>: <what>` / `[FAIL] ...`) and a summary line; a status LED shows running / all passed / failure.
- Checks that need a person (keypad, buttons, sensors, servo angle, PWM duty) print what to do in Proteus and what should be seen.
- Each test file starts with a header comment: Proteus parts, wiring (matching `docs/pin_map.md`) and the expected result.
- A layer is done when its test passes in Proteus and every environment still builds with zero warnings.

---

## 4. Coding conventions

> All new and edited code follows the conventions of the course drivers, so the whole code base reads as one style.

> Reference style = the course author's drivers (DIO, ADC, EXTI, GIE, TIMER0, USART, CLCD, KPAD, SSG).
> TWI was restyled to this standard (`TWI.c/.h/_cfg.h`, `u8`, `reg_def.h`, `TWI_u8SendStartCondition`). One unified style across all layers — no `<avr/io.h>`/`stdint.h` in project code.

- Folder / file layout: `lib/MCAL/<MOD>/`, `lib/HAL/<MOD>/`, `lib/Service/` (shared `std_types.h`, `Bit_math.h`), `lib/MCAL/reg_def.h` (all register addresses), app in `src/APP/main.c`. One folder per module. New SERVICE modules go in `lib/Service/<MOD>/`, new APP modules in `src/APP/<MOD>/`.
- File naming: `DIO.c` / `DIO.h` / `CLCD_cfg.h` (module name in caps, `_cfg.h` for config). Older SSG uses `SSG_prog.c` / `SSG_int.h` / `SSG_CFG.h` — new modules follow the `MOD.c / MOD.h / MOD_cfg.h` form.
- Function naming: `MODULE_<rettype><Name>` — `DIO_voidSetPinValue`, `DIO_u8GetPinValue`, `ADC_u16StartConversion`, `CLCD_voidSendString`. Params `Copy_u8PortID`, locals `Local_u8data`. Some setters drop the type (`TIMER0_SetCallBack_OV`, `ADC_SetCallBack`); prefer the typed form. Constants `MODULE_NAME` macros (`DIO_PORTA`, `TIMER0_DIV_64`, `EXTI_FALLING_EDGE`).
- Types: `u8/u16/u32/s8/.../c8/f32` from `lib/Service/std_types.h` (`u32`/`s32` = `unsigned/signed long` = 32 bit, fixed in Phase 0; `NULL` defined there). No `<stdint.h>` in author code.
- Register access: `SET_BIT/CLR_BIT/GET_BIT/TOG_BIT` from `Bit_math.h` on registers defined as `*(volatile u8 *)(addr)` in `reg_def.h` with bit-name macros (`ADCSRA_ADEN`, `UCSRB_RXEN`). No `<avr/io.h>`. Masks written as binary literals (`ADMUX &= 0b11100000`). New registers (Timer1, Timer2, TWI) get added to `reg_def.h` the same way.
- Return values / error handling: `void` functions; argument range checked with `if(...) {...} else { //error }`; getters return the value. No enums. Where a bus can fail (TWI, and HAL drivers on top of it) the function returns a `u8` status using `#define`d codes (`TWI_OK`, `TWI_ERR_SLA_NACK`, …).
- Config style: pre-build `#define`s in `MOD_cfg.h` (pins as `DIO_PORTx`/`DIO_PIN_n`, mode selection via `#if CLCD_MODE == ...`). SSG uses a config struct `SSG_t` passed by pointer.
- Header guards / includes / comments: guards like `HAL_CLCD_CLCD_H_` or `DIO_H_`; Eclipse header block (`/* * FILE.h * Created on: ... * Author: ... */`). `.c` include order: `../../Service/std_types.h`, `../../Service/Bit_math.h`, lower-layer headers, `../reg_def.h`, own `.h`, own `_cfg.h`. Headers do not include `std_types.h` themselves. Step comments like `/*1. Select ref */`.
- Interrupt style: `void __vector_N () __attribute__ ((signal));` prototype in the `.h`, body in the `.c`; ISR calls a user callback through a global function pointer (`void (*TIMER0_ov_ptr)(void) = NULL;`, set via `..._SetCallBack...`), with a `!= NULL` check (EXTI lacks it). Global interrupt via `GIE_voidEnableGlobalInterrupt()` (`__asm("SEI")`).
  **Required for new ISRs:** declare them `__attribute__ ((signal, used, externally_visible))`. With only `signal`, PlatformIO's LTO discards the ISR at link time (found in Phase 0: ADC/TIMER0/EXTI handlers were dropped, so `test_base` builds with `build_unflags = -flto` until they are fixed).
- Delay usage: `<util/delay.h>`. CLCD in I²C mode (the selected mode): 40/5/1 ms one-time init delays, 2 ms after clear/home, no per-pulse delay (each I²C write already takes ~0.3 ms). The parallel CLCD modes (unused) still have a 10 ms enable pulse. ADC sync, USART send/receive, KPAD (waits for key release) and TWI (~90 µs per byte, no timeout) all busy-wait.
- Indentation, brace style: tabs, K&R braces on the same line, `switch` with `case X: stmt; break ;` on one line, space before `;` in `break ;` / `return x ;`. Files are CRLF. No fixed line-width limit.
- Strings in flash: `<avr/pgmspace.h>` cannot be used, because it pulls in `<avr/io.h>`, which clashes with `reg_def.h`. Use GCC's `__flash` instead: `const __flash c8 *` parameters, plus a `FLASH_STR("...")` macro (statement expression with a `static const __flash c8[]`), from `lib/Service/flash_str.h` (moved there in Phase 2 SERVICE; include it after `std_types.h`), so all layers share it.
- Test programs: `test_mains/test_<layer>.c`, functions `TEST_voidName`, checks through `TEST_voidCheck(ok, FLASH_STR("MOD"), FLASH_STR("what"))`, status LED on PA3, Proteus wiring in the header comment.

### Fixes made to the course drivers
Fixed in Phase 0: USART UCSRC read-modify-write (now one write), KPAD transposed key table and column left LOW, SPI stub missing `return`, `u32`/`s32` were 16 bit.
- **Fixed (Phase 2 MCAL, 2026-10-02)** · **ISRs (ADC, TIMER0, EXTI):** missing `used, externally_visible`, so LTO drops them (see Interrupt style). Fix, then remove `build_unflags = -flto` from `[env:test_base]`.
- **Fixed (Phase 2 MCAL, 2026-10-02)** · **TIMER0:** `TIMER0_GeneratePWM` has the COM bits swapped (NONINVERTED sets 11), and 100 % duty gives `OCR0 = 256` → 0. `test_base` reports both as `[FAIL]`.
- **Fixed (Phase 2 MCAL, 2026-10-02)** · **EXTI:** `EXTI_voidINTx_callBack` is not declared in `EXTI.h`, and the ISRs call the callback without a `NULL` check.
- **Fixed (Phase 2 MCAL, 2026-10-02)** · **ADC:** no function to switch `ADIE` off; after one async conversion the sync `ADC_u16StartConversion` hangs (the ISR clears `ADIF`). The comment in `ADC_voidInit` says AVCC, but the code selects the internal 2.56 V reference (which is what we want for the LM35: 4 steps per °C).
- **Fixed (Phase 2 MCAL, 2026-10-02)** · **USART:** baud value hard-coded (`UBRRL = 103`, correct only for 16 MHz) — must be computed from `F_CPU`. TX/RX are blocking; Section 5 needs RX interrupt + ring buffers.
- **Fixed (Phase 2 HAL, 2026-10-02)** · **KPAD:** blocks until the key is released; needs a non-blocking, debounced scan for the scheduler.
- **Fixed (Phase 2 MCAL, 2026-10-02)** · **TWI:** blocking, with no timeout (a stuck bus hangs the loop). The 24C08 needs non-blocking ACK polling (EEP-03).
- **Fixed (Phase 2 HAL, 2026-10-02)** · **CLCD:** `CLCD_voidClearDisp` exists but is not declared in `CLCD.h`; no text-from-flash function. `CLCD_voidInit` switches the cursor and blink ON (must be off for the status screen). One LCD byte costs 4 I²C transactions (~1.2 ms); it becomes one transaction (~0.5 ms).
- **Fixed (Phase 2 HAL, 2026-10-02)** · **KPAD (RAM):** `KPAD_MAT` and the pin arrays are not `const`, so they sit in RAM (24 bytes).
- **Fixed (Phase 3, 2026-10-04)** · **main.c:** included `../lib/service/Std_Types.h` with the wrong letter case. Rewritten: correct includes, boot order and super-loop of architecture 4.6 / 4.2.
- The exact fix for every item is in `docs/architecture.md` Section 2.
- **Fixed (Phase 2 HAL, 2026-10-02)** · **SSG:** writes raw segments to a whole port; does not fit the 2-digit design → new SEVEN_SEG driver (two PCF8574 chips since D-20; the first 7447 + multiplexed version was dropped because it was unreliable in Proteus).

### Proteus simulation notes
- A single read of the shared UBRRH/UCSRC address seems to return UCSRC in Proteus (a real ATmega32 returns UBRRH), so UBRRH cannot be verified in simulation.
- Parts powered from DC generators do not appear as VCC in the `.SDF` netlist; check the schematic before calling a pin "unconnected".
- Keypad wiring: keypad rows A–D → PA4–PA7, columns 1–4 → PB0, PB1, PB2, PB4.
- The `.SDF` netlist does not export simulation-only parts (keypad, 7-segment display, push buttons, motors, servo, instruments). A pin that goes only to such a part looks open in the netlist; that does not mean it is unconnected.
- Reading back a PCF8574 whose pins drive 7-segment LEDs returns a different value than the one written, while the display shows the right digit (found with `hal_testing_v2`). Do not read-back-check the segment chips; the `test_hal` 7-segment steps are manual.
- The Proteus 24C08 model has **no write-cycle delay**: it ACKs its address right after a write (`test_hal` reports "ready at once" as `[INFO]`, not a failure). The real chip needs about 5 ms, so ACK polling in `ESTORE` must still be written for it. (An earlier note here said the model had a 10 ms `TD_WRITE`; that was wrong.)
- The Proteus buzzer ("DC Buzzer with Sound") is silent with its defaults (`VNOM=5V`, `LOAD=12`) on PD3. It works directly on PD3 with Operating Voltage = 3 V and Load Resistance = 150 Ω. **Real hardware still needs an NPN driver stage** (pin_map C-9): PD3 -> 1k -> base, buzzer between +5 V and the collector, flyback diode.
- The Proteus servo model maps a 1-2 ms pulse to -90..+90 degrees by default. Set its Min/Max Angle properties to 0 / 180 so 1 ms = 0 degrees and 2 ms = 180 degrees.

### Code level
Plain C at the level of the course drivers: readable, commented, no clever tricks. No RTOS, no dynamic memory, no `printf`, no function-pointer dispatch tables beyond the drivers' ISR callbacks.

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
- **SEC-05** The **3rd wrong login in a row** (username or password, admin or user, counted per login source) → the system **locks down and sounds the alarm until reset**. Section 12 defines "locks down". (The course text says "more than 3"; the 3rd failure was chosen, decision 2.)
- **SEC-06** Admin and users can control everything, **except users cannot open/close the door**.
- **SEC-07** EEPROM access: admin = read/write, user = read-only. This applies to the **accounts**: only the admin can add, remove or change them (a write gate in `USERDB`, open only while the admin is logged in). The heater set temperature is not covered by this rule (HTR-14).
- **SEC-08** Keypad users can control the system while a remote user is logged in. While the **admin** is logged in remotely, keypad control is blocked until the admin allows it (see Section 12).
- **SEC-09** Default admin credentials are written on first boot when the EEPROM has no valid data (detected by a magic byte). They are listed in `docs/uart_protocol.md`.

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
- **EEP-01** External **24C08** I²C EEPROM (satisfies both specs; counts as the second protocol). (The internal EEPROM is not used.)
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
- The full command list and an example session are in `docs/uart_protocol.md`.

---

## 11. Coding checklist (applied to every module)

- [ ] Compiles with zero warnings (`pio run -e app` and `pio run -e test_<layer>`).
- [ ] Matches Section 4 conventions exactly.
- [ ] No blocking delays beyond what Section 5 allows.
- [ ] No register access or pin numbers in APP code.
- [ ] Shared ISR variables are `volatile` and read atomically when multi-byte.
- [ ] String constants in flash (`__flash` / `FLASH_STR`).
- [ ] Integer math only (no `float`) — e.g. LM35 with 2.56 V internal reference: `temp_x4 = adc_value` (0.25 °C units), or with AVCC 5 V: `temp = (adc * 500UL) / 1024`.
- [ ] Requirement IDs commented where implemented.
- [ ] Assumptions marked with `/* ASSUMPTION: */`.
- [ ] `docs/test_plan.md` updated with the Proteus test steps for this module.
- [ ] The module is covered by its layer's test program in `test_mains/`, and `pio run -e test_<layer>` builds cleanly.

---

## 12. Decisions

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
