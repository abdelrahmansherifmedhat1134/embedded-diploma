# Pin map — ATmega32 Smart Home + Water Heater

**Status: final (v1.0), verified in Proteus.**
This is the final pin assignment. It is specification.md Section 7 with one approved change: **the dimmer and the AC fan swapped pins** (dimmer on PB3, fan on PD4; architecture decision D-17).

Checked against the netlist `simulation/design/design_netlist_phase1.SDF` (exported 2026-10-02). Proteus does not export simulation-only parts (keypad, 7-segment display, push buttons, motors, servo, instruments), so pins that go only to those parts look open in the netlist. Those rows say "not visible in the netlist"; they are confirmed by the layer tests in Phase 2.

---

## 1. Pins

Pin numbers are for the DIP-40 package. "Owner" is the only file that may name the pin.

### PORTA

| Pin | DIP | Dir | Function | Active | Owner | Proteus part and wiring |
|---|---|---|---|---|---|---|
| PA0 | 40 | analog in | LM35 ambient (AC) — ADC0 | 10 mV/°C | `LM35_cfg.h` | LM35 "AMBIENT AC", VOUT -> PA0 |
| PA1 | 39 | analog in | LM35 water (heater) — ADC1 | 10 mV/°C | `LM35_cfg.h` | LM35 "WATER HEATER", VOUT -> PA1 |
| PA2 | 38 | analog in | LDR — ADC2, **reserved, not used in v1** | — | — | TORCH_LDR + R1 10 kΩ divider |
| PA3 | 37 | out | Heater LED | high | `LED_cfg.h` | D1 LED -> R2 220 Ω -> GND. The layer test programs use the same LED as their status LED |
| PA4 | 36 | in, pull-up | Keypad row A | low | `KPAD_cfg.h` `KPAD_ROW_PIN0` | KEYPAD-SMALLCALC row A (not visible in the netlist) |
| PA5 | 35 | in, pull-up | Keypad row B | low | `KPAD_ROW_PIN1` | row B |
| PA6 | 34 | in, pull-up | Keypad row C | low | `KPAD_ROW_PIN2` | row C |
| PA7 | 33 | in, pull-up | Keypad row D | low | `KPAD_ROW_PIN3` | row D |

### PORTB

| Pin | DIP | Dir | Function | Active | Owner | Proteus part and wiring |
|---|---|---|---|---|---|---|
| PB0 | 1 | out | Keypad column 1 | low while scanned | `KPAD_COL_PIN0` | keypad column 1 (not visible in the netlist) |
| PB1 | 2 | out | Keypad column 2 | low while scanned | `KPAD_COL_PIN1` | column 2 |
| PB2 | 3 | out | Keypad column 3 | low while scanned | `KPAD_COL_PIN2` | column 3 |
| PB3 | 4 | out, OC0 | **Dimmer PWM** (Timer0, 7.8 kHz) | high | `DIMMER_cfg.h` | R6 10 kΩ -> C1 10 µF to GND (RC filter) -> Q4 2N2222 base; lamp "DIMMER" between +12 V and the collector |
| PB4 | 5 | out | Keypad column 4 | low while scanned | `KPAD_COL_PIN3` | column 4 |
| PB5 | 6 | in, pull-up | Heater Down button | low | `BUTTON_cfg.h` | push button to GND (not visible in the netlist) |
| PB6 | 7 | out | Heating element | high | `RELAY_cfg.h` | LED-RED "HEATING" -> R5 220 Ω -> GND (stands for the SSR) |
| PB7 | 8 | out | Cooling element | high | `RELAY_cfg.h` | LED-BLUE "COOLING" -> R4 220 Ω -> GND |

### PORTC

| Pin | DIP | Dir | Function | Active | Owner | Proteus part and wiring |
|---|---|---|---|---|---|---|
| PC0 | 22 | SCL | I²C clock | — | TWI hardware | 4.7 kΩ pull-up "SCL" to +5 V |
| PC1 | 23 | SDA | I²C data | — | TWI hardware | 4.7 kΩ pull-up "SDA" to +5 V |
| PC2 | 24 | — | **spare** (was 7447 A) | — | — | free since D-20; leave unconnected (JTAG TCK on a real chip, see C-3) |
| PC3 | 25 | — | **spare** (was 7447 B) | — | — | free since D-20 |
| PC4 | 26 | — | **spare** (was 7447 C) | — | — | free since D-20 |
| PC5 | 27 | — | **spare** (was 7447 D) | — | — | free since D-20 |
| PC6 | 28 | — | **spare** (was digit 1 enable) | — | — | free since D-20 |
| PC7 | 29 | — | **spare** (was digit 2 enable) | — | — | free since D-20 |

**7-segment display (D-20):** two common-anode digits, common pins on +5 V, each digit behind its own PCF8574 on the I²C bus (tens 0x21, units 0x22, see Section 2); the 7447 and the PNP drivers are removed. The PC2–PC7 rows above are spare.

### PORTD

| Pin | DIP | Dir | Function | Active | Owner | Proteus part and wiring |
|---|---|---|---|---|---|---|
| PD0 | 14 | in, RXD | UART receive | — | USART hardware | Virtual Terminal TXD (HC-05 TXD on real hardware) |
| PD1 | 15 | out, TXD | UART transmit | — | USART hardware | Virtual Terminal RXD (HC-05 RXD through a 5 V -> 3.3 V divider) |
| PD2 | 16 | in, INT0 | PIR — **reserved, not used in v1** | high | — | test_base / test_mcal put a push button here for the EXTI check |
| PD3 | 17 | out | Buzzer | high | `BUZZER_cfg.h` | **Proteus (works):** BUZ1 directly between PD3 and GND with Operating Voltage = 3 V and Load Resistance = 150 Ω. **Real hardware (TODO, see C-9):** needs a transistor driver |
| PD4 | 18 | out, OC1B | **AC fan PWM** (Timer1, 50 Hz) | high | `FAN_cfg.h` | R3 1 kΩ -> Q1 2N2222 base; DC MOTOR between its supply and the collector, 1N4007 across the motor (motor not visible in the netlist) |
| PD5 | 19 | out, OC1A | Door servo (Timer1, 50 Hz) | 1–2 ms pulse | `SERVO_cfg.h` | MOTOR-PWMSERVO control pin (not visible in the netlist) |
| PD6 | 20 | in, pull-up | Heater ON/OFF button | low | `BUTTON_cfg.h` | push button to GND (not visible in the netlist) |
| PD7 | 21 | in, pull-up | Heater Up button | low | `BUTTON_cfg.h` | push button to GND (not visible in the netlist) |

### Other pins

| Pin | DIP | Connection |
|---|---|---|
| RESET | 9 | R19 10 kΩ to +5 V + push button to GND. Needed by the test plan (restart without losing the EEPROM content; leaving lockdown) |
| VCC / GND | 10 / 11, 31 | 5 V / 0 V |
| AVCC | 30 | +5 V |
| AREF | 32 | open (or 100 nF to GND). **Never to VCC** (C-5) |
| XTAL1/2 | 13 / 12 | 16 MHz crystal + 2 x 22 pF on real hardware. Proteus: part property *Clock Frequency* = 16 MHz |

All 32 I/O pins are assigned (PA2 and PD2 are reserved for the optional LDR and PIR).

---

## 2. I²C bus (100 kHz)

`TWBR = ((F_CPU / 100000) - 16) / 2 = 72` at 16 MHz, prescaler 1 (`TWI_cfg.h`, already in the code).

| Device | Part | 7-bit address | On the wire (write / read) | Address pins | Notes |
|---|---|---|---|---|---|
| LCD 16x2 | LM016L behind PCF8574 "LCD I2C EXPANDER" | **0x27** | 0x4E / 0x4F | A2 A1 A0 = 1 1 1 | P0 = RS, P1 = RW, P2 = E, P3 = backlight (not connected), P4–P7 = D4–D7 (`CLCD_cfg.h`) |
| Lamps 1–5 | PCF8574 "LEDS I2C EXPANDER" | **0x20** | 0x40 / 0x41 | A2 A1 A0 = 0 0 0 | P0–P4 = lamp 1–5 = LEDs D6, D2, D3, D4, D5. **Active-low**: +5 V -> 220 Ω -> LED -> pin. P5–P7 unused (kept high) |
| 7-segment tens | PCF8574 "SEG TENS EXPANDER" | **0x21** | 0x42 / 0x43 | A2 A1 A0 = 0 0 1 | P0..P6 = segments a..g, P7 = dp (not connected or off). Common-anode digit, common pin on +5 V, each segment pin -> 220 Ω -> segment (**active low**) |
| 7-segment units | PCF8574 "SEG UNITS EXPANDER" | **0x22** | 0x44 / 0x45 | A2 A1 A0 = 0 1 0 | same wiring as the tens chip |
| EEPROM | NM24C08 "U2" | **0x50** (blocks 0x50–0x53) | 0xA0 / 0xA1 | A2 = GND | Only block 0 is used. The Proteus model has `TD_WRITE = 10 ms` (write cycle), page size 16 |

The PCF8574 is a 100 kHz part, so the bus is not run at 400 kHz. Only the main loop uses the bus (no interrupt touches it), so transactions can never interleave.

---

## 3. Peripherals

| Resource | Used for | Pins |
|---|---|---|
| Timer0, fast PWM, 7.8 kHz | dimmer | PB3 (OC0) |
| Timer1, fast PWM mode 14, 50 Hz | door servo, AC fan | PD5 (OC1A), PD4 (OC1B) |
| Timer2, CTC 1 ms + interrupt | system tick | — |
| USART, RX and UDRE interrupts | remote terminal | PD0, PD1 |
| TWI, polled | LCD, lamps, EEPROM | PC0, PC1 |
| ADC, internal 2.56 V reference, /128 | two LM35 | PA0, PA1 |
| INT0 | optional PIR (not in v1) | PD2 |

---

## 4. Conflicts found and how they were closed

"Proteus" = matters in simulation, "hardware" = matters only on a real board.

| ID | Where | Problem | Status |
|---|---|---|---|
| **C-1** | Dimmer + servo on Timer1 (Proteus + hardware) | One TOP value for both: the dimmer would have been 50 Hz PWM, too slow to filter into a usable 0–5 V level | **Closed.** Dimmer moved to PB3 / Timer0 (7.8 kHz), fan moved to PD4 / Timer1 (50 Hz is fine for an on/off fan). Seen in the netlist: PB3 -> R6 -> C1 / Q4, PD4 -> R3 -> Q1 |
| **C-2** | Lamp LEDs on the PCF8574 (hardware) | A PCF8574 can sink 25 mA but source only ~0.1 mA, and it powers up with all pins high | **Closed.** LEDs rewired active-low (seen in the netlist); `LAMP_ON_LEVEL 0`. Power-up = all lamps off |
| **C-3** | PC2–PC5 = JTAG pins (hardware) | On a real ATmega32 the JTAGEN fuse is programmed from the factory, so these pins do not work as I/O | **No longer relevant (D-20)**: PC2–PC7 are spare and unused. `DIO_voidDisableJTAG()` stays in MCAL but is not called at boot |
| **C-4** | PB5, PB6, PB7 = ISP pins MOSI, MISO, SCK (hardware) | During in-circuit programming the heating/cooling outputs toggle and the Down button can disturb the programmer | **Accepted.** Nothing in firmware; on a real board use 1 kΩ series resistors or a jumper |
| **C-5** | AREF (Proteus + hardware) | The ADC uses the internal 2.56 V reference; AREF tied to 5 V would short it and make every temperature read about half | **Closed.** AREF is open in the netlist, AVCC is on +5 V |
| **C-6** | Dimmer lamp driver (Proteus) | The lamp was wired straight to a port pin | **Closed.** RC filter (R6 10 kΩ, C1 10 µF) -> Q4 -> lamp on +12 V. See the note below |
| **C-7** | 24C08 pin 7, write protect (Proteus) | If the pin is high, every write is ignored | **Wired to GND in the schematic; cannot be seen in the netlist** (the export does not list this pin). The HAL EEPROM test proves that writes are accepted |
| **C-9** | Buzzer on PD3 (Proteus + hardware) | With the model defaults (5 V, 12 Ω = 0.4 A) the direct connection was silent. In Proteus it works with Operating Voltage 3 V and Load 150 Ω (about 20–33 mA from the pin). That is still at or above the 20 mA an AVR pin should give, and a real buzzer is not this model | **Open for the real board.** Add a driver: PD3 -> 1 kΩ -> base of an NPN (2N2222 / BC547), emitter GND, buzzer between +5 V and the collector, 1N4148 across the buzzer (cathode to +5 V); use an active buzzer rated for 5 V. No firmware change (`BUZZER_ON_LEVEL` stays HIGH) |
| **C-8** | PC6 / PC7 digit drivers (Proteus) | PNP drivers: a digit is on when the pin is low; the emitters had no supply | **Obsolete (D-20).** The PNP drivers and the 7447 were replaced by two PCF8574 chips; both digits stay lit without multiplexing |

Note on C-6, to check in the HAL test (not a blocker): with R6 = 10 kΩ the base current of Q4 is at most (5 − 0.7) V / 10 kΩ = 0.43 mA, and the lamp (12 V, 24 Ω) needs 0.5 A for full brightness. Unless the transistor gain is above 1000 the lamp will stay dim even at 100 %. If that shows in the simulation, either lower R6 to 1 kΩ (the filter still has 10 ms, plenty at 7.8 kHz) or set the lamp's LOAD to about 240 Ω. No firmware change either way.

Not conflicts, noted so they are not mistaken for one:
- PA3 is both the heater LED and the test-program status LED: the test programs run alone, never together with the application.
- PD2 has a push button in the test programs and is the PIR pin in the pin plan: the PIR is not built in v1.

---

## 5. Proteus parts

| Part | Library name | Qty | In the netlist |
|---|---|---|---|
| Microcontroller | ATMEGA32 (CLOCK = 16 MHz, CKSEL = 1111) | 1 | yes |
| Temperature sensor | LM35 | 2 | yes (PA0, PA1) |
| LCD | LM016L | 1 | yes |
| I/O expander | PCF8574 | 4 | 2 in the netlist (0x27, 0x20); **add 0x21 and 0x22** (7-segment digits) |
| EEPROM | NM24C08 | 1 | yes |
| ~~BCD decoder~~ | ~~7447 + 7 x 220 Ω~~ | removed (D-20) | — |
| Display | 2 x 7SEG-COM-ANODE (one per digit, each with 7 x 220 Ω) | 2 | not exported |
| ~~Digit drivers~~ | ~~2N3906 + 1 kΩ~~ | removed (D-20) | — |
| Keypad | KEYPAD-SMALLCALC | 1 | not exported (rows A–D -> PA4–PA7, columns 1–4 -> PB0, PB1, PB2, PB4) |
| Push buttons | BUTTON | 3 + reset | not exported (PD6, PD7, PB5, RESET); the RESET pull-up R19 is there |
| Lamp LEDs | LED-AQUA + 220 Ω | 5 | yes, active-low |
| Dimmer | LAMP 12 V + R6 10 kΩ + C1 10 µF + 2N2222 | 1 | yes |
| Heater LED | LED + 220 Ω | 1 | yes (PA3) |
| Heating / cooling indicator | LED-RED, LED-BLUE + 220 Ω | 2 | yes (PB6, PB7) |
| Fan | MOTOR (DC) + 2N2222 + 1 kΩ + 1N4007 | 1 | transistor yes, motor not exported |
| Door | MOTOR-PWMSERVO | 1 | not exported (PD5) |
| Buzzer | BUZZER ("DC Buzzer with Sound", set to **3 V, 150 Ω**, 500 Hz) | 1 | yes (PD3, direct) |
| LDR | TORCH_LDR + 10 kΩ | 1 | yes (PA2, unused by the firmware) |
| I²C pull-ups | RES 4.7 kΩ | 2 | yes |
| Instruments | VIRTUAL TERMINAL (9600, 8N1), I2C DEBUGGER, OSCILLOSCOPE | — | not exported |

Proteus settings to keep: ATmega32 clock = **16 MHz** (must equal `F_CPU`), AREF open, 24C08 WP = GND.
