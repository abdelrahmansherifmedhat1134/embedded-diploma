# Pin map — ATmega32 Smart Home + Water Heater

Phase 1 design. **Status: waiting for approval.**
The pin assignment is CLAUDE.md Section 7, taken as decided: **no pin was moved**. This document adds the direction, active level, owning config file and Proteus part of every pin, the I²C addresses, and the real conflicts found (Section 4).

Checked against the committed netlist `simulation/project_v1_phase0.SDF` (exported 2026-10-01). Your working `.pdsprj` has newer, uncommitted changes, so parts marked "not in the netlist yet" may already be placed.

---

## 1. Pins

Pin numbers are for the DIP-40 package. "Owner" is the only file that may name the pin.

### PORTA

| Pin | DIP | Dir | Function | Active | Owner | Proteus part and wiring |
|---|---|---|---|---|---|---|
| PA0 | 40 | analog in | LM35 ambient (AC) — ADC0 | 10 mV/°C | `LM35_cfg.h` | LM35 "AMBIENT AC", VOUT -> PA0 |
| PA1 | 39 | analog in | LM35 water (heater) — ADC1 | 10 mV/°C | `LM35_cfg.h` | LM35 "WATER HEATER", VOUT -> PA1 |
| PA2 | 38 | analog in | LDR — ADC2, **reserved, not used in v1** | — | — | TORCH_LDR + R1 10 kΩ divider (already placed) |
| PA3 | 37 | out | Heater LED | high | `LED_cfg.h` | D1 LED -> R2 220 Ω -> GND. The layer test programs use the same LED as their status LED |
| PA4 | 36 | in, pull-up | Keypad row A | low | `KPAD_cfg.h` `KPAD_ROW_PIN0` | KEYPAD-SMALLCALC row A |
| PA5 | 35 | in, pull-up | Keypad row B | low | `KPAD_ROW_PIN1` | row B |
| PA6 | 34 | in, pull-up | Keypad row C | low | `KPAD_ROW_PIN2` | row C |
| PA7 | 33 | in, pull-up | Keypad row D | low | `KPAD_ROW_PIN3` | row D |

### PORTB

| Pin | DIP | Dir | Function | Active | Owner | Proteus part and wiring |
|---|---|---|---|---|---|---|
| PB0 | 1 | out | Keypad column 1 | low while scanned | `KPAD_COL_PIN0` | keypad column 1 |
| PB1 | 2 | out | Keypad column 2 | low while scanned | `KPAD_COL_PIN1` | column 2 |
| PB2 | 3 | out | Keypad column 3 | low while scanned | `KPAD_COL_PIN2` | column 3 |
| PB3 | 4 | out, OC0 | AC fan PWM (Timer0, 976 Hz) | high | `FAN_cfg.h` | R3 1 kΩ -> Q1 2N2222 base; DC MOTOR between +V and the collector, 1N4007 across the motor |
| PB4 | 5 | out | Keypad column 4 | low while scanned | `KPAD_COL_PIN3` | column 4 |
| PB5 | 6 | in, pull-up | Heater Down button | low | `BUTTON_cfg.h` | push button to GND |
| PB6 | 7 | out | Heating element | high | `RELAY_cfg.h` | LED-RED "HEATING" -> R5 220 Ω -> GND (stands for the SSR) |
| PB7 | 8 | out | Cooling element | high | `RELAY_cfg.h` | LED-BLUE "COOLING" -> R4 220 Ω -> GND |

### PORTC

| Pin | DIP | Dir | Function | Active | Owner | Proteus part and wiring |
|---|---|---|---|---|---|---|
| PC0 | 22 | SCL | I²C clock | — | TWI hardware | 4.7 kΩ pull-up "SCL" to VCC |
| PC1 | 23 | SDA | I²C data | — | TWI hardware | 4.7 kΩ pull-up "SDA" to VCC |
| PC2 | 24 | out | 7447 input A (BCD bit 0) | high | `SEVEN_SEG_cfg.h` | 7447 pin 7 |
| PC3 | 25 | out | 7447 input B | high | `SEVEN_SEG_cfg.h` | 7447 pin 1 |
| PC4 | 26 | out | 7447 input C | high | `SEVEN_SEG_cfg.h` | 7447 pin 2 |
| PC5 | 27 | out | 7447 input D (BCD bit 3) | high | `SEVEN_SEG_cfg.h` | 7447 pin 6 |
| PC6 | 28 | out | 7-seg digit 1 (tens) enable | **low** | `SEVEN_SEG_cfg.h` | R11 1 kΩ -> Q3 2N3906 base (PNP high-side switch to the digit's common anode) |
| PC7 | 29 | out | 7-seg digit 2 (units) enable | **low** | `SEVEN_SEG_cfg.h` | R12 1 kΩ -> Q2 2N3906 base |

7447: LT, BI/RBO, RBI (pins 3, 4, 5) tied high; outputs a–g through 7 x 220 Ω to the segment pins of a 2-digit common-anode display (7SEG-MPX2-CA).

### PORTD

| Pin | DIP | Dir | Function | Active | Owner | Proteus part and wiring |
|---|---|---|---|---|---|---|
| PD0 | 14 | in, RXD | UART receive | — | USART hardware | Virtual Terminal TXD (HC-05 TXD on real hardware) |
| PD1 | 15 | out, TXD | UART transmit | — | USART hardware | Virtual Terminal RXD (HC-05 RXD through a 5 V -> 3.3 V divider) |
| PD2 | 16 | in, INT0 | PIR — **reserved, not used in v1** | high | — | test_base / test_mcal put a push button here for the EXTI check |
| PD3 | 17 | out | Buzzer | high | `BUZZER_cfg.h` | BUZ1 (active buzzer) to GND |
| PD4 | 18 | out, OC1B | Dimmer PWM (Timer1, 50 Hz) | high | `DIMMER_cfg.h` | RC filter -> driver transistor -> lamp L1 (see C-1 and C-6) |
| PD5 | 19 | out, OC1A | Door servo (Timer1, 50 Hz) | 1–2 ms pulse | `SERVO_cfg.h` | MOTOR-PWMSERVO control pin |
| PD6 | 20 | in, pull-up | Heater ON/OFF button | low | `BUTTON_cfg.h` | push button to GND |
| PD7 | 21 | in, pull-up | Heater Up button | low | `BUTTON_cfg.h` | push button to GND |

### Other pins

| Pin | DIP | Connection |
|---|---|---|
| RESET | 9 | 10 kΩ to VCC + **push button to GND**. Needed by the test plan (reset without losing the EEPROM content; leaving lockdown) |
| VCC / GND | 10 / 11, 31 | 5 V / 0 V |
| AVCC | 30 | 5 V (through 10 µH + 100 nF on real hardware) |
| AREF | 32 | **100 nF to GND only. Never to VCC** (C-5) |
| XTAL1/2 | 13 / 12 | 16 MHz crystal + 2 x 22 pF on real hardware. Proteus: part property *Clock Frequency* = 16 MHz |

All 32 I/O pins are assigned (PA2 and PD2 are reserved for the optional LDR and PIR).

---

## 2. I²C bus (100 kHz)

`TWBR = ((F_CPU / 100000) - 16) / 2 = 72` at 16 MHz, prescaler 1 (`TWI_cfg.h`, already in the code).

| Device | Part | 7-bit address | On the wire (write / read) | Address pins | Notes |
|---|---|---|---|---|---|
| LCD 16x2 | LM016L behind PCF8574 "LCD I2C EXPANDER" | **0x27** | 0x4E / 0x4F | A2 A1 A0 = 1 1 1 | P0 = RS, P1 = RW, P2 = E, P3 = backlight (not connected), P4–P7 = D4–D7 (`CLCD_cfg.h`) |
| Lamps 1–5 | PCF8574 "LEDS I2C EXPANDER" | **0x20** | 0x40 / 0x41 | A2 A1 A0 = 0 0 0 | P0–P4 = lamp 1–5 (LEDs D5, D4, D3, D2, D6 in the schematic), P5–P7 unused |
| EEPROM | NM24C08 "U2" | **0x50** (blocks 0x50–0x53) | 0xA0 / 0xA1 | A2 = GND | Only block 0 is used. The Proteus model has `TD_WRITE = 10 ms` (write cycle), page size 16 |

The PCF8574 is a 100 kHz part, so the bus is not run at 400 kHz. Only the main loop uses the bus (no interrupt touches it), so transactions can never interleave.

---

## 3. Peripherals

| Resource | Used for | Pins |
|---|---|---|
| Timer0, fast PWM | AC fan | PB3 (OC0) |
| Timer1, fast PWM mode 14, 50 Hz | door servo, dimmer | PD5 (OC1A), PD4 (OC1B) |
| Timer2, CTC 1 ms + interrupt | system tick, 7-segment multiplexing | — |
| USART, RX and UDRE interrupts | remote terminal | PD0, PD1 |
| TWI, polled | LCD, lamps, EEPROM | PC0, PC1 |
| ADC, internal 2.56 V reference, /128 | two LM35 | PA0, PA1 |
| INT0 | optional PIR (not in v1) | PD2 |

---

## 4. Conflicts and risks found

Only real ones. "Proteus" = matters in simulation, "hardware" = matters only on a real board.

| ID | Where | Problem | What the design does / what I need from you |
|---|---|---|---|
| **C-1** | PD4 dimmer + PD5 servo (Proteus + hardware) | Both are on Timer1, which has one TOP value. The servo needs a 20 ms period, so the dimmer PWM is also **50 Hz**. The course asks for a 0–5 V control voltage; filtering 50 Hz needs a slow RC: 10 kΩ + 47 µF (ripple ~50 mV, but ~2 s to settle). | **Default: keep Section 7** and use that RC filter. **Recommended alternative:** swap the two PWM users — dimmer on PB3/OC0 (Timer0 at 7.8 kHz, RC 10 kΩ + 1 µF, ripple ~15 mV, settles in 50 ms), fan on PD4/OC1B (50 Hz is fine for an on/off motor). It is two wires in Proteus and two `_cfg.h` files; nothing else changes. Your call. |
| **C-2** | Lamp LEDs on the PCF8574 (hardware) | The schematic wires the LEDs **active-high** (pin -> LED -> 220 Ω -> GND). The Proteus model drives this, but a real PCF8574 sources only ~0.1 mA (it can sink 25 mA), and its power-up state is all pins high = all lamps ON until the firmware runs. | `LAMP_ON_LEVEL` is a macro. Default = 1 to match your schematic. For real hardware rewire active-low (VCC -> 220 Ω -> LED -> pin) and set the macro to 0. |
| **C-3** | PC2–PC5 = JTAG pins (hardware) | On a real ATmega32 the JTAGEN fuse is programmed from the factory, so these four pins do not work as I/O. | The firmware sets `JTD` at start-up (`DIO_voidDisableJTAG`). Proteus is not affected. |
| **C-4** | PB5, PB6, PB7 = ISP pins MOSI, MISO, SCK (hardware) | While the chip is programmed in-circuit, the heating/cooling outputs toggle and the Down button can disturb the programmer. | Nothing in firmware. On a real board: 1 kΩ series resistors or a jumper. Proteus is not affected. |
| **C-5** | AREF (Proteus + hardware) | The ADC uses the **internal 2.56 V reference** (4 steps per °C). If AREF is wired to 5 V the reference is shorted and every temperature reads half. | Keep AREF unconnected in Proteus (it is, in the netlist) or a 100 nF capacitor to GND. |
| **C-6** | PD4 -> lamp L1 (Proteus + hardware) | L1 (12 V, 24 Ω) is connected directly to PD4 in the netlist: a 5 V pin cannot drive it (200 mA) and there is no 0–5 V control stage. | Add the dimmer stage: PD4 -> RC filter -> NPN (emitter follower or the SSR input) -> lamp from its own supply. |
| **C-7** | 24C08 pin 7, write protect (Proteus) | Not connected in the netlist. If the model reads it as high, every write is ignored. | Tie WP to GND. |
| **C-8** | PC6 / PC7 digit drivers (Proteus) | Q2/Q3 are PNP, so a digit is on when the pin is **low**. The emitters are tied together but do not show a VCC connection in the netlist. | `SEVEN_SEG_DIGIT_ON_LEVEL DIO_PIN_LOW` (macro). Check that both emitters go to VCC. |

Not conflicts, noted so they are not mistaken for one:
- PA3 is both the heater LED and the test-program status LED: the test programs run alone, never together with the application.
- PD2 has a push button in the test programs and is the PIR pin in the pin plan: the PIR is not built in v1.

---

## 5. Proteus parts

| Part | Library name | Qty | State in the committed netlist |
|---|---|---|---|
| Microcontroller | ATMEGA32 (CLOCK = 16 MHz, CKSEL = 1111) | 1 | placed |
| Temperature sensor | LM35 | 2 | placed (PA0, PA1) |
| LCD | LM016L | 1 | placed |
| I/O expander | PCF8574 | 2 | placed (0x27, 0x20) |
| EEPROM | NM24C08 | 1 | placed, WP open (C-7) |
| BCD decoder | 7447 | 1 | placed, outputs open |
| 2-digit display | 7SEG-MPX2-CA + 7 x 220 Ω | 1 | **not in the netlist yet** |
| Digit drivers | 2N3906 + 1 kΩ | 2 | placed (C-8) |
| Keypad | KEYPAD-SMALLCALC | 1 | **not in the netlist yet** (rows A–D -> PA4–PA7, columns 1–4 -> PB0, PB1, PB2, PB4) |
| Push buttons | BUTTON | 3 + reset | **not in the netlist yet** (PD6, PD7, PB5, RESET) |
| Lamp LEDs | LED-AQUA + 220 Ω | 5 | placed on the lamp expander (C-2) |
| Dimmer lamp | LAMP (12 V) + RC filter + NPN | 1 | lamp placed, driver stage missing (C-6) |
| Heater LED | LED + 220 Ω | 1 | placed (PA3) |
| Heating / cooling indicator | LED-RED, LED-BLUE + 220 Ω | 2 | placed (PB6, PB7) |
| Fan | MOTOR (DC) + 2N2222 + 1 kΩ + 1N4007 | 1 | transistor placed, motor **not in the netlist yet** |
| Door | MOTOR-PWMSERVO | 1 | **not in the netlist yet** (PD5) |
| Buzzer | BUZZER (active) | 1 | placed (PD3) |
| LDR | TORCH_LDR + 10 kΩ | 1 | placed (PA2, unused by the firmware) |
| I²C pull-ups | RES 4.7 kΩ | 2 | placed |
| Instruments | VIRTUAL TERMINAL (9600, 8N1), I2C DEBUGGER, OSCILLOSCOPE | — | instruments are not exported to the netlist |

Proteus settings to keep: ATmega32 clock = **16 MHz** (must equal `F_CPU`), AREF open, 24C08 WP = GND.
