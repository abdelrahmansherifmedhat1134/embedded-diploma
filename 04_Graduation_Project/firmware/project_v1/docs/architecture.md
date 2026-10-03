# Architecture — ATmega32 Smart Home + Water Heater

Phase 1 design. **Status: design decisions approved by the user on 2026-10-02 (Section 9). No firmware exists for this design yet.**
Clock: `F_CPU = 16 000 000` (from `platformio.ini`, `board_build.f_cpu`). Every timing value below is derived from `F_CPU`.

Related documents: [pin_map.md](pin_map.md), [eeprom_map.md](eeprom_map.md), [uart_protocol.md](uart_protocol.md), [test_plan.md](test_plan.md), [hal_summary.md](hal_summary.md) (HAL as built).

Contents: 1 Layers · 2 FIX modules · 3 NEW modules (API + cfg) · 4 Scheduler · 5 State machines · 6 Data ownership · 7 RAM/flash budget · 8 Phase 2 order · 9 Design decisions

---

## 1. Layers and modules

Legend: **[E]** existing, used as it is · **[F]** existing, needs a fix or an addition (Section 2) · **[N]** new (Section 3).

```
+-----------------------------------------------------------------------------------------------+
| src/APP/main.c [F, rewritten in Phase 3]   init -> GIE -> super-loop on SCHED flags           |
+-----------------------------------------------------------------------------------------------+
| APP  (src/APP/<MOD>/)                                                                         |
|   UI      : UIREM [N] (UART terminal)        UILOC [N] (LCD + keypad)                         |
|   feature : LIGHT [N]  DOOR [N]  CLIMATE [N]  HEATER [N]                                      |
|   safety  : SEC [N] (login, roles, lockdown counter)   ALARM [N] (latched lockdown)           |
+-----------------------------------------------------------------------------------------------+
| SERVICE  (lib/Service/<MOD>/)                                                                 |
|   system    : SCHED [N]  TERM [N]  ESTORE [N]  USERDB [N]  EVQ [N]                            |
|   utilities : RINGBUF [N]  MAVG [N]  FMT [N]  flash_str.h [N]  std_types.h [E]  Bit_math.h [E]|
+-----------------------------------------------------------------------------------------------+
| HAL  (lib/HAL/<MOD>/)                                                                         |
|   display : CLCD [F]  LCD_BUF [N]  SEVEN_SEG [N]            (SSG [E] not used)                |
|   input   : KPAD [F]  BUTTON [N]  LM35 [N]                                                    |
|   output  : LAMP [N]  DIMMER [N]  SERVO [N]  FAN [N]  RELAY [N]  LED [N]  BUZZER [N]          |
|   chips   : PCF8574 [F]  EXT_EEPROM [N]                                                       |
+-----------------------------------------------------------------------------------------------+
| MCAL  (lib/MCAL/<MOD>/)                                                                       |
|   DIO [F]  GIE [E]  ADC [F]  TIMER0 [F]  TIMER1 [N]  TIMER2 [N]  USART [F]  TWI [F]  EXTI [F] |
|   reg_def.h [F]                                              (SPI [E] stub, not used)         |
+-----------------------------------------------------------------------------------------------+
```

42 modules: 3 existing and unused or untouched (`GIE`, `SPI`, `SSG`), 9 fixed, 30 new; plus the shared headers (`reg_def.h` fixed, `flash_str.h` new).

### 1.1 Dependency rules

1. **APP** includes SERVICE and HAL headers only. No MCAL header, no register, no port or pin number.
2. **SERVICE / system** modules sit above HAL. Two of them talk to MCAL directly because no chip sits in between: `SCHED -> TIMER2`, `TERM -> USART` (a pass-through HAL wrapper would add nothing). See decision D-2.
3. **SERVICE / utilities** (`RINGBUF`, `MAVG`, `FMT`, the type headers) touch no hardware and may be included from any layer, exactly like `std_types.h` today.
4. **HAL** includes MCAL headers. A HAL module may use a lower HAL module when one chip sits behind another: `CLCD -> PCF8574` (already so), `LCD_BUF -> CLCD`, `LAMP -> PCF8574`.
5. **MCAL** includes nothing above it. Interrupts reach upper layers only through the author's callback style (`MOD_SetCallBack...`).
6. Inside APP there is one direction only: **UI -> SEC -> ALARM -> feature**, and **UI -> feature**. Feature modules never call a UI; they report changes by posting to `EVQ`.

### 1.2 Call graph

```
UIREM  -> TERM, EVQ, FMT, SEC, USERDB, ESTORE(status only), LIGHT, DOOR, CLIMATE, HEATER, ALARM(IsActive)
UILOC  -> LCD_BUF, KPAD, FMT, SEC, LIGHT, CLIMATE, HEATER, ALARM(IsActive)        (no DOOR: SEC-06)
SEC    -> USERDB, ALARM, EVQ
ALARM  -> HEATER(ForceOff), CLIMATE(ForceOff), BUZZER, EVQ
LIGHT  -> LAMP, DIMMER, EVQ                  DOOR    -> SERVO
CLIMATE-> LM35, MAVG, FAN, EVQ               HEATER  -> LM35, MAVG, BUTTON, SEVEN_SEG, RELAY, LED, ESTORE, EVQ
USERDB -> ESTORE -> EXT_EEPROM -> TWI        TERM    -> USART, RINGBUF          EVQ -> RINGBUF      ESTORE -> EVQ (fault)
SCHED  -> TIMER2                             LCD_BUF -> CLCD -> PCF8574 -> TWI  LAMP -> PCF8574 -> TWI  SEVEN_SEG -> PCF8574 -> TWI
KPAD, BUTTON, RELAY, LED, BUZZER -> DIO      LM35 -> ADC      DIMMER -> TIMER0, DIO      SERVO, FAN -> TIMER1, DIO
```

---

## 2. FIX modules — exact changes

Rule: change only what is listed, in the author's style, public API kept. Every item comes from "Known driver issues after Phase 0" or from something this design needs.

| Module | Change | Why |
|---|---|---|
| **reg_def.h** | Add, in the same style: `TCCR1A/B`, `TCNT1H/L`, `OCR1AH/L`, `OCR1BH/L`, `ICR1H/L` (0x4F..0x46) + bit names; `TCCR2` (0x45), `TCNT2` (0x44), `OCR2` (0x43) + bit names; `TIMSK_OCIE2/TOIE2/TICIE1/OCIE1A/OCIE1B/TOIE1`, matching `TIFR` bits; `MCUCSR_JTD` (bit 7) | Timer1, Timer2, JTAG disable |
| **DIO** | Add `void DIO_voidDisableJTAG();` (writes `MCUCSR_JTD` twice, as the datasheet requires) | PC2–PC5 are JTAG pins (pin_map C-3). Since D-20 nothing needs it at boot; the function stays in MCAL (tested by `test_mcal`) |
| **ADC** | 1. ISR prototype -> `__attribute__ ((signal, used, externally_visible))`. 2. `ADC_u16StartConversion` clears `ADIE` first, so a previous async conversion can no longer steal `ADIF` and hang it. 3. Add `void ADC_voidDisableInterrupt();`. 4. Fix the wrong comment (`AVCC` -> internal 2.56 V). 5. Globals `ADC_Ptr` / `data` become `static` (`data` is too generic a global name) | LTO drops ISR; sync-after-async hang |
| **TIMER0** | 1. Both ISR prototypes get `used, externally_visible`. 2. `TIMER0_GeneratePWM`: NONINVERTED = `COM01=1, COM00=0`, INVERTED = `COM01=1, COM00=1` (they are swapped today). 3. `OCR0 = ((u16)DutyCycle * 255) / 100` (today 100 % gives 256 -> 0). 4. `TIMER0_voidInit` case label `TIMER0_CTC_DISCONNECTED` -> `TIMER0_NORMAL` (same value 0, wrong name) | test_base `[FAIL]`s |
| **EXTI** | 1. Three ISR prototypes get `used, externally_visible`. 2. Declare `EXTI_voidINT0/1/2_callBack` in `EXTI.h`. 3. `NULL` check before calling the callback | crash on unset callback |
| **USART** | 1. `USART_cfg.h` (new): `USART_BAUD_RATE 9600UL`, `USART_UBRR_VALUE ((F_CPU / (16UL * USART_BAUD_RATE)) - 1)` = 103 (real 9615 baud, error +0.16 %, U2X not needed); init writes `UBRRH` then `UBRRL` from it. 2. New non-blocking API, author's callback style: `USART_voidEnableRxInterrupt()`, `USART_voidEnableTxInterrupt()` / `USART_voidDisableTxInterrupt()` (UDRIE), `USART_voidSetCallBack_RX(void (*ptr)(u8))` (ISR reads `UDR` and passes the byte), `USART_voidSetCallBack_TX(void (*ptr)(void))` (UDRE ISR), `USART_voidWriteData(u8)` (writes `UDR`, no wait). 3. `__vector_13` (RXC) and `__vector_14` (UDRE) with the full attribute list and `NULL` checks. 4. The blocking `USART_voidSend / u8Recieve / voidSendString` stay (test_base uses them). The RX/TX **ring buffers live in `TERM`** (SERVICE), so MCAL stays free of upper-layer code | hard-coded baud; blocking RX/TX |
| **TWI** | 1. `TWI_voidSendCommand` waits at most `TWI_TIMEOUT_LOOPS` (new in `TWI_cfg.h`, about 1 ms, normal wait is 90 µs) and returns a status; all callers pass it up as new code `TWI_ERR_TIMEOUT 6`. 2. On timeout the bus is released (`TWSTO`, then `TWEN` off/on). 3. Add `u8 TWI_u8ProbeAddress(u8 Copy_u8SlaveAddress);` = START + SLA+W + STOP, returns `TWI_OK` (ACK) or `TWI_ERR_SLA_NACK`: the building block for 24C08 ACK polling and for "is the chip there" checks. Stays polled (no TWI interrupt): the longest transfer is bounded at ~1.7 ms (Section 4.3) | stuck bus hangs the loop; EEP-03 |
| **PCF8574** | Add `u8 PCF8574_u8WriteBytes(u8 Copy_u8Address, const u8 * Copy_pu8Data, u8 Copy_u8Length);` (several port values in one I²C transaction) | CLCD speed-up below |
| **CLCD** | 1. Declare `CLCD_voidClearDisp` in `CLCD.h`. 2. I²C mode sends one LCD byte as **one** transaction of 4 port bytes (hi-nibble E=1, E=0, lo-nibble E=1, E=0) instead of 4 transactions: ~0.5 ms per character instead of ~1.2 ms. E pulse = one byte time (90 µs), far above the 450 ns the LCD needs. 3. Display-control command comes from new `CLCD_DISPLAY_CTRL` in `CLCD_cfg.h`, default `0b00001100` (display on, cursor off, blink off): today the cursor and blink are on, which looks wrong on a status screen. 4. Add `void CLCD_voidSendFlashString(const __flash c8 * Copy_pc8Str);` 5. Add `u8 CLCD_u8GetStatus();` = result of the last I²C write (`TWI_OK` or a TWI error; always 0 in the parallel modes), so `LCD_BUF` can see a failed write | missing prototype; speed; no flash text; error visibility |
| **KPAD** | Add non-blocking API next to the old one: `void KPAD_voidUpdate();` (one matrix scan, call every 10 ms; a key is accepted after `KPAD_DEBOUNCE_SCANS` = 2 equal scans) and `u8 KPAD_u8GetKey();` (returns each accepted key **once**, then `KPAD_NO_KEY` 0xFF until all keys are released). `KPAD_MAT` and the two pin arrays become `static const __flash` (plain `const` would still be copied to RAM on AVR; `__flash` really frees the 24 B). `KPAD_u8GetKeyPressed` (blocking) stays for test_base | blocks until release |
| **main.c** | Rewritten in Phase 3 only (today: hello-world with wrong-case include paths) | — |

After the ISR fixes, `build_unflags = -flto` is removed from `[env:test_base]` and test_base must report all `[PASS]`.

---

## 3. NEW modules — public API and `_cfg.h`

Style: `MODULE_<type><Name>`, `Copy_` parameters, `u8` status codes as `#define`s, pre-build `#define` configuration, files `MOD.c / MOD.h / MOD_cfg.h`. Bodies are Phase 2 work; the prototypes below are the contract.

### 3.1 MCAL

#### TIMER1 — 16-bit PWM (servo + fan)
```c
/* TIMER1.h */
#define TIMER1_PWM_DISCONNECTED   0
#define TIMER1_PWM_INVERTED       1
#define TIMER1_PWM_NONINVERTED    2
void TIMER1_voidInit();                               /* mode 14 : fast PWM , TOP = ICR1 , from TIMER1_cfg.h */
void TIMER1_voidSetOCRA(u16 Copy_u16Value);           /* written high byte first */
void TIMER1_voidSetOCRB(u16 Copy_u16Value);
void TIMER1_voidGeneratePWM_A(u8 Copy_u8mode);        /* OC1A = PD5 */
void TIMER1_voidGeneratePWM_B(u8 Copy_u8mode);        /* OC1B = PD4 */

/* TIMER1_cfg.h */
#define TIMER1_PRESCALER          8UL                 /* CS11 */
#define TIMER1_PWM_FREQ_HZ        50UL                /* 20 ms period for the servo */
#define TIMER1_TOP_VALUE          ((u16)((F_CPU / (TIMER1_PRESCALER * TIMER1_PWM_FREQ_HZ)) - 1))   /* 39999 */
#define TIMER1_TICKS_PER_MS       ((u16)(F_CPU / (TIMER1_PRESCALER * 1000UL)))                      /* 2000 */
```
`TIMER1_voidInit` may be called twice (SERVO and FAN both call it), like `PCF8574_voidInit` today. No interrupt is used.

#### TIMER2 — 8-bit CTC tick
```c
/* TIMER2.h  (Timer2 prescaler codes differ from Timer0) */
#define TIMER2_OFF        0
#define TIMER2_NO_PRE     1
#define TIMER2_DIV_8      2
#define TIMER2_DIV_32     3
#define TIMER2_DIV_64     4
#define TIMER2_DIV_128    5
#define TIMER2_DIV_256    6
#define TIMER2_DIV_1024   7
#define TIMER2_NORMAL     0
#define TIMER2_CTC        1
void TIMER2_voidInit(u8 Copy_u8PreScalare, u8 Copy_u8Mode);
void TIMER2_voidSetOCR(u8 Copy_u8OCR_Val);
void TIMER2_voidEnableOCInterrupt();
void TIMER2_voidDisableOCInterrupt();
void TIMER2_voidSetCallBack_OC(void (*ptr)(void));
void __vector_4 () __attribute__ ((signal, used, externally_visible)) ;
```
No `_cfg.h`: the tick values live in `SCHED_cfg.h`, where they are used.

### 3.2 HAL

#### LCD_BUF — non-blocking LCD (shadow buffer over CLCD)
```c
/* LCD_BUF.h */
void LCD_BUF_voidInit();                               /* CLCD_voidInit (blocking, boot only) + clear buffers */
void LCD_BUF_voidClear();                              /* RAM only : fills the wanted text with spaces */
void LCD_BUF_voidWriteChar  (u8 Copy_u8Row, u8 Copy_u8Col, c8 Copy_c8Char);
void LCD_BUF_voidWriteFlash (u8 Copy_u8Row, u8 Copy_u8Col, const __flash c8 * Copy_pc8Text);
void LCD_BUF_voidWriteRam   (u8 Copy_u8Row, u8 Copy_u8Col, const c8 * Copy_pc8Text);
void LCD_BUF_voidUpdate();                             /* call every 5 ms : sends at most ONE byte to the LCD */

/* LCD_BUF_cfg.h */
#define LCD_BUF_ROWS            2
#define LCD_BUF_COLS            16
#define LCD_BUF_RETRY_UPDATES   200     /* after an I2C error wait 200 x 5 ms = 1 s before trying again */
```
Two arrays of 32 bytes: *wanted* and *shown*. Writers only touch *wanted* (instant). `LCD_BUF_voidUpdate` finds the next differing cell; if the LCD cursor is not there it sends one set-cursor command, otherwise one data byte. Cost per call <= 0.5 ms; a full redraw takes ~35 calls = 175 ms. After every send it checks `CLCD_u8GetStatus()`: on an I²C error the byte stays "not shown", the cursor is marked unknown, and `LCD_BUF` waits `LCD_BUF_RETRY_UPDATES` calls before trying again. Without this a 16-character line would block the loop for 8 ms (and 20 ms with today's CLCD).

#### SEVEN_SEG — 2 digits, one PCF8574 per digit (D-20)
```c
/* SEVEN_SEG.h */
void SEVEN_SEG_voidInit();                             /* display blank */
void SEVEN_SEG_voidSetNumber(u8 Copy_u8Number);        /* 0..99 , bigger values show 99 (leading zero) */
void SEVEN_SEG_voidEnable();                           /* show the number */
void SEVEN_SEG_voidDisable();                          /* blank : 0xFF on both chips */
u8   SEVEN_SEG_u8GetStatus();                          /* status of the last write : TWI_OK or a TWI error (like LAMP) */

/* SEVEN_SEG_cfg.h */
#define SEVEN_SEG_TENS_ADDRESS     0x21                /* PCF8574 , A2 A1 A0 = 0 0 1 */
#define SEVEN_SEG_UNITS_ADDRESS    0x22                /* PCF8574 , A2 A1 A0 = 0 1 0 */
#define SEVEN_SEG_ON_LEVEL         0                   /* common anode : a segment lights when its pin is LOW */
```
Each common-anode digit has its own PCF8574: P0..P6 = segments a..g, P7 = dp (always off), through 220 Ω. Pattern table `static const __flash u8 SEVEN_SEG_TABLE[10]` = `C0 F9 A4 B0 99 92 82 F8 80 90`; `0xFF` = blank. `SetNumber`, `Enable` and `Disable` write the chips **immediately**, one I²C write per digit and only when that digit's pattern differs from what the chip shows (about 0.2 ms each). A failed write marks the digit "unknown", so the next call writes it again. No interrupt, no multiplexing, no `Refresh`; call from the main loop only (the I²C bus is polled and shared).

#### BUTTON — debounced push buttons
```c
/* BUTTON.h */
#define BUTTON_ONOFF    0
#define BUTTON_UP       1
#define BUTTON_DOWN     2
void BUTTON_voidInit();                                /* inputs with pull-up */
void BUTTON_voidUpdate();                              /* call every 10 ms */
u8   BUTTON_u8IsPressed(u8 Copy_u8ButtonID);           /* debounced level */
u8   BUTTON_u8GetPressEvent  (u8 Copy_u8ButtonID);     /* 1 once per press   , cleared by reading */
u8   BUTTON_u8GetReleaseEvent(u8 Copy_u8ButtonID);     /* 1 once per release , cleared by reading */

/* BUTTON_cfg.h */
#define BUTTON_COUNT              3
#define BUTTON_ONOFF_PORT         DIO_PORTD
#define BUTTON_ONOFF_PIN          DIO_PIN_6
#define BUTTON_UP_PORT            DIO_PORTD
#define BUTTON_UP_PIN             DIO_PIN_7
#define BUTTON_DOWN_PORT          DIO_PORTB
#define BUTTON_DOWN_PIN           DIO_PIN_5
#define BUTTON_PRESSED_LEVEL      DIO_PIN_LOW
#define BUTTON_DEBOUNCE_SAMPLES   3                    /* 3 x 10 ms = 30 ms */
```

#### LM35 — temperature sensors
```c
/* LM35.h */
#define LM35_AMBIENT    0
#define LM35_WATER      1
void LM35_voidInit();                                  /* ADC_voidInit */
u16  LM35_u16ReadTempX4(u8 Copy_u8SensorID);           /* one conversion (~110 us) , unit = 0.25 C */

/* LM35_cfg.h */
#define LM35_AMBIENT_CHANNEL   ADC_CHANNEL_0
#define LM35_WATER_CHANNEL     ADC_CHANNEL_1
/* 10 mV/C , ADC reference 2.56 V , 10 bit --> 2.5 mV per step --> 4 steps per C */
#define LM35_STEPS_PER_C       4
```
Integer only: the raw ADC value **is** the temperature in quarter degrees.

#### LAMP — 5 on/off lamps on a PCF8574
```c
/* LAMP.h */
#define LAMP_OFF    0
#define LAMP_ON     1
#define LAMP_ERR_NUMBER 0xFF                           /* returned for a lamp number outside 1..5 (TWI codes are 1..6) */
void LAMP_voidInit();                                  /* all lamps OFF */
u8   LAMP_u8SetState(u8 Copy_u8LampNumber, u8 Copy_u8State);   /* lamp 1..5 ; returns TWI_OK or a TWI error */
u8   LAMP_u8GetState(u8 Copy_u8LampNumber);

/* LAMP_cfg.h */
#define LAMP_COUNT          5
#define LAMP_I2C_ADDRESS    0x20                       /* PCF8574 , A2 A1 A0 = 0 0 0 */
#define LAMP_FIRST_BIT      0                          /* lamp 1 = P0 ... lamp 5 = P4 */
#define LAMP_ON_LEVEL       0                          /* active low : +5 V -> 220 R -> LED -> pin (pin_map.md C-2) */
```

#### RELAY, LED, BUZZER — plain outputs
```c
/* RELAY.h */                                          /* RELAY_cfg.h */
#define RELAY_HEATING   0                              #define RELAY_HEATING_PORT   DIO_PORTB
#define RELAY_COOLING   1                              #define RELAY_HEATING_PIN    DIO_PIN_6
void RELAY_voidInit();          /* both OFF */         #define RELAY_COOLING_PORT   DIO_PORTB
void RELAY_voidOn (u8 Copy_u8RelayID);                 #define RELAY_COOLING_PIN    DIO_PIN_7
void RELAY_voidOff(u8 Copy_u8RelayID);                 #define RELAY_ON_LEVEL       DIO_PIN_HIGH

/* LED.h */                                            /* LED_cfg.h */
#define LED_HEATER      0                              #define LED_HEATER_PORT      DIO_PORTA
void LED_voidInit();                                   #define LED_HEATER_PIN       DIO_PIN_3
void LED_voidOn (u8 Copy_u8LedID);                     #define LED_ON_LEVEL         DIO_PIN_HIGH
void LED_voidOff(u8 Copy_u8LedID);

/* BUZZER.h */                                         /* BUZZER_cfg.h */
void BUZZER_voidInit();                                #define BUZZER_PORT          DIO_PORTD
void BUZZER_voidOn();                                  #define BUZZER_PIN           DIO_PIN_3
void BUZZER_voidOff();                                 #define BUZZER_ON_LEVEL      DIO_PIN_HIGH
```

#### DIMMER, SERVO, FAN — PWM outputs
```c
/* DIMMER.h */
void DIMMER_voidInit();                                /* Timer0 fast PWM , level 0 */
void DIMMER_voidSetLevel(u8 Copy_u8Percent);           /* 0..100 */
/* DIMMER_cfg.h */
#define DIMMER_PORT              DIO_PORTB
#define DIMMER_PIN               DIO_PIN_3             /* OC0 */
#define DIMMER_TIMER_PRESCALER   TIMER0_DIV_8          /* F_CPU / 8 / 256 = 7812 Hz : easy to filter to 0-5 V */

/* SERVO.h */
void SERVO_voidInit();                                 /* TIMER1_voidInit , closed position */
void SERVO_voidSetAngle(u8 Copy_u8Angle);              /* 0..180 degrees */
/* SERVO_cfg.h */
#define SERVO_PORT            DIO_PORTD
#define SERVO_PIN             DIO_PIN_5                /* OC1A */
#define SERVO_MIN_PULSE_US    1000UL                   /* 0 degrees   */
#define SERVO_MAX_PULSE_US    2000UL                   /* 180 degrees */
#define SERVO_MAX_ANGLE       180                      /* bigger angles are limited to this */

/* FAN.h */
void FAN_voidInit();                                   /* TIMER1_voidInit , stopped */
void FAN_voidSetSpeed(u8 Copy_u8Percent);              /* 0..100 */
/* FAN_cfg.h */
#define FAN_PORT              DIO_PORTD
#define FAN_PIN               DIO_PIN_4                /* OC1B , 50 Hz (shares Timer1 with the servo) */
```
PWM corner cases, the same in all three: 0 % = compare output **disconnected** and the pin driven low (compare value 0 would leave a one-tick spike); 100 % = compare value equal to TOP (constant high).
Servo counts: `OCR1A = pulse_us * TIMER1_TICKS_PER_MS / 1000` -> 1.0 ms = 2000, 1.5 ms (90°) = 3000, 2.0 ms = 4000. Fan: `OCR1B = percent * (TIMER1_TOP_VALUE + 1) / 100` -> 10 % = 4000. Dimmer: `OCR0 = percent * 255 / 100`.

#### EXT_EEPROM — 24C08
```c
/* EXT_EEPROM.h */
#define EXT_EEPROM_OK       0
#define EXT_EEPROM_ERROR    1
void EXT_EEPROM_voidInit();                            /* TWI_voidMasterInit */
u8   EXT_EEPROM_u8ReadBlock(u16 Copy_u16Address, u8 * Copy_pu8Data, u8 Copy_u8Length);        /* blocking , 90 us per byte : boot only */
u8   EXT_EEPROM_u8WritePage(u16 Copy_u16Address, const u8 * Copy_pu8Data, u8 Copy_u8Length);  /* 1..16 bytes inside ONE page , returns after sending ; the chip is then busy */
u8   EXT_EEPROM_u8IsReady();                           /* ONE ACK poll : 1 = write cycle finished . never waits */

/* EXT_EEPROM_cfg.h */
#define EXT_EEPROM_I2C_ADDRESS   0x50                  /* 1010 A2 B1 B0 , A2 = GND ; B1 B0 = address bits 9:8 */
#define EXT_EEPROM_PAGE_SIZE     16
#define EXT_EEPROM_SIZE          1024
#define EXT_EEPROM_BLOCK_SIZE    256                   /* one I2C address (B1 B0) covers 256 bytes : a read is split at the borders */
```

### 3.3 SERVICE

#### flash_str.h
The `FLASH_STR("...")` macro moves here from `test_base.c` unchanged, so every layer shares one definition.

#### RINGBUF — byte ring buffer
```c
/* RINGBUF.h */
#define RINGBUF_OK       0
#define RINGBUF_FULL     1
#define RINGBUF_EMPTY    2
typedef struct{
	u8 * Buffer ;
	u8   Size ;                 /* capacity = Size - 1 */
	volatile u8 Head ;          /* written only by the producer */
	volatile u8 Tail ;          /* written only by the consumer */
}RINGBUF_t;
void RINGBUF_voidInit(RINGBUF_t * rb, u8 * Copy_pu8Storage, u8 Copy_u8Size);
u8   RINGBUF_u8Put(RINGBUF_t * rb, u8 Copy_u8Data);
u8   RINGBUF_u8Get(RINGBUF_t * rb, u8 * Copy_pu8Data);
u8   RINGBUF_u8GetCount(RINGBUF_t * rb);
u8   RINGBUF_u8GetFree (RINGBUF_t * rb);
```
One producer and one consumer, 8-bit indexes: safe between an ISR and the main loop **without** disabling interrupts. A struct passed by pointer follows the author's `SSG_t`. No `_cfg.h`.

#### MAVG — moving average (REQ-HTR-08)
```c
/* MAVG.h */
typedef struct{
	u16 Samples[MAVG_WINDOW] ;
	u16 Sum ;
	u8  Index ;
	u8  Count ;
}MAVG_t;
void MAVG_voidReset(MAVG_t * avg);
void MAVG_voidAddSample(MAVG_t * avg, u16 Copy_u16Sample);
u8   MAVG_u8IsFull(MAVG_t * avg);                      /* 1 when MAVG_WINDOW samples are stored */
u16  MAVG_u16GetAverage(MAVG_t * avg);                 /* rounded average of the samples stored so far */

/* MAVG_cfg.h */
#define MAVG_WINDOW    10
```

#### FMT — number to text (no `printf`)
```c
/* FMT.h */
u8 FMT_u8NumberToText(u16 Copy_u16Number, c8 * Copy_pc8Text);                      /* writes digits + '\0' , returns the length */
u8 FMT_u8TextToNumber(const c8 * Copy_pc8Text, u16 * Copy_pu16Number);             /* 1 = text was a valid number (digits only , <= 65535) */
```

#### SCHED — time base and task flags (Section 4)
```c
/* SCHED.h */
#define SCHED_TASK_5MS      0
#define SCHED_TASK_10MS     1
#define SCHED_TASK_100MS    2
#define SCHED_TASK_500MS    3
#define SCHED_TASK_1S       4
void SCHED_voidInit();                                 /* Timer2 CTC , callback ; interrupt still off */
void SCHED_voidStart();                                /* enable the tick interrupt */
u8   SCHED_u8IsTaskDue(u8 Copy_u8TaskID);              /* 1 once per period , clears the flag atomically */
u32  SCHED_u32GetTickMs();                             /* read with interrupts off */
u16  SCHED_u16GetOverruns();                           /* flags that were set again before being served */

/* SCHED_cfg.h */
#define SCHED_TIMER_PRESCALER     64UL                 /* TIMER2_DIV_64 */
#define SCHED_TICK_HZ             1000UL
#define SCHED_OCR_VALUE           ((u8)((F_CPU / (SCHED_TIMER_PRESCALER * SCHED_TICK_HZ)) - 1))    /* 249 */
#if (F_CPU % (SCHED_TIMER_PRESCALER * SCHED_TICK_HZ)) != 0
#error "F_CPU does not give an exact 1 ms tick with prescaler 64"
#endif
/* first tick each task becomes due on : all different , so two tasks never start in the same ms */
#define SCHED_OFFSET_5MS          0
#define SCHED_OFFSET_10MS         2
#define SCHED_OFFSET_100MS        1
#define SCHED_OFFSET_500MS        3
#define SCHED_OFFSET_1S           4
```

#### TERM — UART terminal I/O (ring buffers + line editor)
```c
/* TERM.h */
#define TERM_ECHO_OFF       0
#define TERM_ECHO_NORMAL    1
#define TERM_ECHO_MASKED    2                          /* echo '*' : passwords */
#define TERM_LINE_NONE      0
#define TERM_LINE_READY     1
#define TERM_LINE_TOO_LONG  2
void TERM_voidInit();                                  /* USART init , rings , callbacks , RX interrupt */
u8   TERM_u8TxFree();                                  /* free bytes in the TX ring */
void TERM_voidPutChar  (c8 Copy_c8Char);
void TERM_voidPutFlash (const __flash c8 * Copy_pc8Text);
void TERM_voidPutRam   (const c8 * Copy_pc8Text);
void TERM_voidPutNumber(u16 Copy_u16Number);
void TERM_voidNewLine();                               /* "\r\n" */
void TERM_voidSetEcho(u8 Copy_u8Mode);
u8   TERM_u8GetLine(c8 * Copy_pc8Line);                /* call every 5 ms ; copies the line when TERM_LINE_READY */
u8   TERM_u8IsLineEmpty();                             /* 1 = nothing typed yet (safe moment to print an event) */
u8   TERM_u8IsRxActive();                              /* 1 = a byte arrived since the last call (idle timer) */

/* TERM_cfg.h */
#define TERM_RX_BUFFER_SIZE    32
#define TERM_TX_BUFFER_SIZE    128
#define TERM_LINE_MAX          16                      /* typed characters per line */
#define TERM_ECHO_DEFAULT      TERM_ECHO_NORMAL        /* TERM_ECHO_OFF for phone apps that echo locally */
```
RX: `USART` RX callback -> `RINGBUF_u8Put`. TX: `TERM_voidPut...` -> ring -> `USART_voidEnableTxInterrupt()`; the UDRE callback takes one byte or disables the interrupt when the ring is empty. Output that does not fit is dropped, so callers check `TERM_u8TxFree()` first (Section 5.8).

#### EVQ — event queue (feature -> remote terminal)
```c
/* EVQ.h */
#define EVQ_LAMP              1     /* arg = lamp number (state is read from LIGHT) */
#define EVQ_DIMMER            2
#define EVQ_AC                3
#define EVQ_HEATER_POWER      4
#define EVQ_HEATER_ELEMENT    5
#define EVQ_HEATER_SET        6
#define EVQ_LOCAL_SESSION     7     /* arg = 1 login , 0 logout */
#define EVQ_STORAGE_FAULT     8
#define EVQ_LOCKDOWN          9
void EVQ_voidInit();
void EVQ_voidPost(u8 Copy_u8Event, u8 Copy_u8Arg);     /* dropped when full or muted */
u8   EVQ_u8Get(u8 * Copy_pu8Event, u8 * Copy_pu8Arg);  /* 1 = one event returned */
void EVQ_voidSetMute(u8 Copy_u8State);                 /* 1 = posts are dropped */

/* EVQ_cfg.h */
#define EVQ_MAX_EVENTS    16
```
Main-loop context only (never from an ISR). `UIREM` mutes the queue while it runs one of its own commands, because that change is already answered with `[OK]`; so the queue only ever holds changes made somewhere else (keypad, heater panel, automatic AC).

#### ESTORE — EEPROM image with write-behind (REQ-EEP-01..03)
```c
/* ESTORE.h */
#define ESTORE_OK            0
#define ESTORE_FIRST_BOOT    1      /* defaults were written at this boot */
#define ESTORE_FAULT         2      /* chip not answering : RAM image only */
void ESTORE_voidInit();                                /* blocking read of the image (boot only) ; defaults if magic/version wrong */
void ESTORE_voidUpdate();                              /* call every 10 ms : write-behind state machine */
u8   ESTORE_u8ReadByte (u8 Copy_u8Address);
void ESTORE_voidReadBlock (u8 Copy_u8Address, u8 * Copy_pu8Data, u8 Copy_u8Length);
void ESTORE_voidWriteByte (u8 Copy_u8Address, u8 Copy_u8Data);                      /* no effect and no wear if the value is unchanged */
void ESTORE_voidWriteBlock(u8 Copy_u8Address, const u8 * Copy_pu8Data, u8 Copy_u8Length);
void ESTORE_voidLoadDefaults();                        /* factory reset : defaults + all pages marked for writing */
u8   ESTORE_u8IsBusy();                                /* 1 = something still waits to be written */
u8   ESTORE_u8GetStatus();
```
`ESTORE_cfg.h` = the complete EEPROM map and defaults: see [eeprom_map.md](eeprom_map.md) Section 5.

#### USERDB — accounts (REQ-SEC-02, 03, 04, 07, 09)
```c
/* USERDB.h */
#define USERDB_LIST_REMOTE      0
#define USERDB_LIST_KEYPAD      1
#define USERDB_OK               0
#define USERDB_ERR_FULL         1
#define USERDB_ERR_EXISTS       2
#define USERDB_ERR_NOT_FOUND    3
#define USERDB_ERR_BAD_NAME     4      /* length , character set , digits-only rule for keypad users */
#define USERDB_ERR_BAD_PASS     5
#define USERDB_ERR_READ_ONLY    6      /* write access is closed (REQ-SEC-07) */
void USERDB_voidInit();                                                                 /* repairs an invalid admin record with the default */
u8   USERDB_u8CheckAdmin(const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass);             /* 1 = match */
u8   USERDB_u8CheckUser (u8 Copy_u8List, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass);
u8   USERDB_u8AddUser   (u8 Copy_u8List, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass);
u8   USERDB_u8RemoveUser(u8 Copy_u8List, const c8 * Copy_pc8Name);
u8   USERDB_u8GetUserName(u8 Copy_u8List, u8 Copy_u8Slot, c8 * Copy_pc8Name);           /* 1 = slot is used */
u8   USERDB_u8SetAdminPassword(const c8 * Copy_pc8Pass);
void USERDB_voidSetWriteAccess(u8 Copy_u8State);       /* 1 only while the admin is logged in */

/* USERDB_cfg.h */
#define USERDB_NAME_MIN    1
#define USERDB_NAME_MAX    8
#define USERDB_PASS_MIN    4
#define USERDB_PASS_MAX    8
```

### 3.4 APP

Each APP module has `MOD_voidInit()` plus the task functions the super-loop calls (Section 4.2) and getters for the UIs.

```c
/* SEC.h */
#define SEC_SOURCE_REMOTE    0
#define SEC_SOURCE_LOCAL     1
#define SEC_ROLE_NONE        0
#define SEC_ROLE_USER        1
#define SEC_ROLE_ADMIN       2
#define SEC_LOGIN_USER       1
#define SEC_LOGIN_ADMIN      2
#define SEC_LOGIN_FAILED     3
#define SEC_LOGIN_LOCKED     4
void SEC_voidInit();
u8   SEC_u8Login(u8 Copy_u8Source, const c8 * Copy_pc8Name, const c8 * Copy_pc8Pass);
void SEC_voidLogout(u8 Copy_u8Source);
u8   SEC_u8GetRole(u8 Copy_u8Source);
u8   SEC_u8GetAttemptsLeft(u8 Copy_u8Source);
u8   SEC_u8IsLocalAllowed();                           /* REQ-SEC-08 */
void SEC_voidSetLocalAllowed(u8 Copy_u8State);         /* effective only while the admin is logged in */
/* SEC_cfg.h */
#define SEC_MAX_FAILED_LOGINS    3                     /* the 3rd failure locks the system */

/* ALARM.h */
void ALARM_voidInit();
void ALARM_voidTrigger();                              /* latched until MCU reset */
u8   ALARM_u8IsActive();
void ALARM_voidTask500ms();                            /* buzzer pattern */
/* ALARM_cfg.h */
#define ALARM_BUZZER_BEEP    1                         /* 1 = 0.5 s on / 0.5 s off , 0 = continuous */

/* LIGHT.h */
void LIGHT_voidInit();
u8   LIGHT_u8ToggleLamp(u8 Copy_u8LampNumber);         /* returns the new state */
u8   LIGHT_u8GetLamp(u8 Copy_u8LampNumber);
void LIGHT_voidSetDimmer(u8 Copy_u8Percent);           /* rounded down to a multiple of LIGHT_DIMMER_STEP , max 100 */
u8   LIGHT_u8GetDimmer();
/* LIGHT_cfg.h */
#define LIGHT_DIMMER_STEP    10

/* DOOR.h */
#define DOOR_CLOSED    0
#define DOOR_OPEN      1
void DOOR_voidInit();                                  /* closed */
u8   DOOR_u8SetState(u8 Copy_u8State);                 /* 1 = it moved , 0 = it was already there */
u8   DOOR_u8GetState();
/* DOOR_cfg.h */
#define DOOR_CLOSED_ANGLE    0
#define DOOR_OPEN_ANGLE      90

/* CLIMATE.h */
void CLIMATE_voidInit();
void CLIMATE_voidTask100ms();
u8   CLIMATE_u8IsAcOn();
u8   CLIMATE_u8IsTempValid();                          /* 0 during the first second */
u8   CLIMATE_u8GetRoomTemp();                          /* whole degrees */
void CLIMATE_voidForceOff();
/* CLIMATE_cfg.h */
#define CLIMATE_AC_ON_ABOVE_C     28
#define CLIMATE_AC_OFF_BELOW_C    21
#define CLIMATE_FAN_SPEED         100                  /* percent while the AC is on */

/* HEATER.h */
#define HEATER_ELEMENT_NONE       0
#define HEATER_ELEMENT_HEATING    1
#define HEATER_ELEMENT_COOLING    2
void HEATER_voidInit();
void HEATER_voidTask10ms();                            /* buttons */
void HEATER_voidTask100ms();                           /* sampling , control , display , setting timeout */
void HEATER_voidTask500ms();                           /* blink phase */
u8   HEATER_u8IsOn();
u8   HEATER_u8GetElement();
u8   HEATER_u8GetWaterTemp();
u8   HEATER_u8GetSetTemp();
u8   HEATER_u8SetSetTemp(u8 Copy_u8Temp);              /* 1 = accepted and saved (REQ-HTR-14) */
void HEATER_voidForceOff();
/* HEATER_cfg.h */
#define HEATER_SET_MIN              35
#define HEATER_SET_MAX              75
#define HEATER_SET_STEP             5
#define HEATER_SET_DEFAULT          60
#define HEATER_BAND                 5
#define HEATER_SETTING_TIMEOUT      50                 /* 50 x 100 ms = 5 s */

/* UILOC.h */                                          /* UIREM.h */
void UILOC_voidInit();                                 void UIREM_voidInit();
void UILOC_voidTask10ms();                             void UIREM_voidTask5ms();
void UILOC_voidTask1s();                               void UIREM_voidTask1s();
/* UILOC_cfg.h */                                      /* UIREM_cfg.h */
#define UILOC_IDLE_TIMEOUT_S     30                    #define UIREM_IDLE_TIMEOUT_S     120
#define UILOC_MESSAGE_TIME       200  /* x 10 ms */    #define UIREM_TX_RESERVE         64   /* free TX bytes needed before a step prints */
#define UILOC_STATUS_PAGE_S      3                     #define UIREM_MENU_AFTER_COMMAND 0    /* 1 = reprint the whole menu after every command */
```

---

## 4. Scheduler

### 4.1 Tick

Timer2, CTC, prescaler 64: timer clock = 16 MHz / 64 = 250 kHz, `OCR2 = F_CPU / (64 * 1000) - 1 = 249` -> compare match every 250 counts = **1.000 ms exactly**. A compile-time `#error` fires if another `F_CPU` cannot give an exact millisecond.

**Inside the ISR** (`__vector_4` -> TIMER2 callback -> `SCHED` tick function), under 10 µs = 1 % CPU:
1. `tick++` (`volatile u32`).
2. Five down-counters; when one reaches zero it is reloaded with its period and its bit is set in `volatile u8` flags (if the bit was still set, `overruns++`).

Nothing else ever runs in an interrupt except the two USART ISRs, which only move one byte to/from a ring buffer.

### 4.2 Super-loop (`main.c`, Phase 3)

Explicit `if` per period, no dispatch table:

```c
while(1){
	if(SCHED_u8IsTaskDue(SCHED_TASK_5MS))  { LCD_BUF_voidUpdate(); UIREM_voidTask5ms(); }
	if(SCHED_u8IsTaskDue(SCHED_TASK_10MS)) { KPAD_voidUpdate(); BUTTON_voidUpdate();
	                                         HEATER_voidTask10ms(); UILOC_voidTask10ms(); ESTORE_voidUpdate(); }
	if(SCHED_u8IsTaskDue(SCHED_TASK_100MS)){ HEATER_voidTask100ms(); CLIMATE_voidTask100ms(); }
	if(SCHED_u8IsTaskDue(SCHED_TASK_500MS)){ HEATER_voidTask500ms(); ALARM_voidTask500ms(); }
	if(SCHED_u8IsTaskDue(SCHED_TASK_1S))   { UILOC_voidTask1s(); UIREM_voidTask1s(); }
}
```

| Period | First due at | Task | Work | Worst time |
|---|---|---|---|---|
| 1 ms (ISR) | — | tick | counters and flags only (nothing else runs in the tick ISR, D-20) | < 10 µs |
| 5 ms | 0 | `LCD_BUF_voidUpdate` | one LCD byte over I²C | 0.5 ms |
| | | `UIREM_voidTask5ms` | line editor, one command step, one output line | 0.3 ms |
| 10 ms | 2 | `KPAD_voidUpdate`, `BUTTON_voidUpdate` | matrix scan, 3 pins | 0.1 ms |
| | | `HEATER_voidTask10ms`, `UILOC_voidTask10ms` | button/key events, RAM only (a lamp toggle adds one 0.2 ms I²C write) | 0.3 ms |
| | | `ESTORE_voidUpdate` | one ACK poll (0.12 ms) **or** one page write | **1.7 ms** |
| 100 ms | 1 | `HEATER_voidTask100ms` | 1 ADC conversion, average, control law, display (I²C only when a digit changes) | 0.6 ms |
| | | `CLIMATE_voidTask100ms` | 1 ADC conversion, average, hysteresis | 0.2 ms |
| 500 ms | 3 | `HEATER_voidTask500ms`, `ALARM_voidTask500ms` | toggle blink phase / buzzer | 10 µs |
| 1 s | 4 | `UILOC_voidTask1s`, `UIREM_voidTask1s` | idle timers, status screen text (RAM) | 0.1 ms |

The offsets (0, 2, 1, 3, 4) are chosen so that no two periods ever become due in the same millisecond (5 ms slots fall on multiples of 5, 10 ms on x2, 100 ms on x01, 500 ms on x03, 1 s on x004). The worst loop pass is therefore one slot, about 2.2 ms (the 10 ms slot when an EEPROM page is written).

### 4.3 Blocking budget

Rule from CLAUDE.md: nothing longer than ~2 ms after init.

| Operation | Time | Where |
|---|---|---|
| ADC conversion (13 cycles at 125 kHz) | 104 µs | `LM35` |
| I²C byte at 100 kHz | 90 µs | `TWI` |
| Lamp write (address + 1 byte) | 0.2 ms | `LAMP` |
| 7-segment update (up to 2 chips, address + 1 byte each; only changed digits) | 0.4 ms | `SEVEN_SEG` |
| LCD byte (address + 4 bytes) | 0.5 ms | `CLCD` |
| EEPROM page write (address + word address + 16 bytes) | 1.7 ms | `EXT_EEPROM` |
| EEPROM ACK poll | 0.12 ms | `EXT_EEPROM` |
| Any I²C wait on a dead bus | <= 1 ms then `TWI_ERR_TIMEOUT` | `TWI` |

One-time boot delays (before `SCHED_voidStart`): LCD power-up 46 ms, EEPROM image read 208 bytes = 19 ms.

Timeouts are counted in calls of the task that owns them (5 s = 50 calls of the 100 ms task), never by comparing 32-bit ticks. A flag that is served late causes jitter, not drift, because the counters keep running in the ISR; `SCHED_u16GetOverruns()` lets the service test prove that no period is ever skipped.

### 4.4 Interrupt-shared data

| Data | ISR side | Main side | Protection |
|---|---|---|---|
| `tick` (u32) | writes | reads | read with interrupts off (`GIE` disable/enable) |
| task flags (u8) | sets bits | tests and clears | test-and-clear with interrupts off |
| RX ring | writes `Head` | writes `Tail` | none needed (one producer, one consumer, u8) |
| TX ring | writes `Tail` | writes `Head` | none needed |

Critical sections are used only in main-loop context, where interrupts are always enabled, so a plain disable/enable pair is correct.

### 4.5 Timers

| Timer | Mode | Numbers | Use |
|---|---|---|---|
| Timer0 | fast PWM, OC0 | /8 -> 7812 Hz; `OCR0 = percent * 255 / 100` | dimmer |
| Timer1 | mode 14, TOP = `ICR1` = 39999 | /8 -> 0.5 µs per count, 20 ms period | OC1A servo, OC1B AC fan |
| Timer2 | CTC, `OCR2` = 249 | /64 -> 1 ms | system tick |

### 4.6 Boot order (`main`)

1. Outputs to their safe state first: `RELAY`, `LED`, `BUZZER`, `FAN`, `SERVO`, `DIMMER` (no JTAG handling needed: PC2–PC7 are spare, D-20).
2. `TERM_voidInit`, `LCD_BUF_voidInit` (blocking 46 ms), `LAMP_voidInit`, `SEVEN_SEG_voidInit` (blank), `KPAD_voidInit`, `BUTTON_voidInit`, `LM35_voidInit`.
3. `ESTORE_voidInit` (blocking 19 ms read, or defaults), `USERDB_voidInit`, `EVQ_voidInit`.
4. APP inits: `ALARM`, `SEC`, `LIGHT`, `DOOR`, `CLIMATE`, `HEATER`, `UILOC`, `UIREM`.
5. `SCHED_voidInit`, `GIE_voidEnableGlobalInterrupt`, `SCHED_voidStart`, super-loop.

---

## 5. State machines

Notation: one row = one transition. `[guard]`, then the action, then the next state. Every machine runs to completion and returns; none waits.

### 5.1 SEC — sessions and lockdown counter (REQ-SEC-01, 02, 05, 06, 07, 08)

Per source (REMOTE, LOCAL) a session state, plus a fail counter per source. One login attempt = one (name, password) pair checked together.

| State | Event | Guard | Action | Next |
|---|---|---|---|---|
| any | `Login(src, name, pass)` | `ALARM_u8IsActive()` | return `SEC_LOGIN_LOCKED` | same |
| `LOGGED_OUT` | `Login(REMOTE, ..)` | admin record matches | fail[REMOTE] = 0; `USERDB_voidSetWriteAccess(1)`; local allowed = 0; force LOCAL to `LOGGED_OUT`; return `SEC_LOGIN_ADMIN` | `ADMIN` |
| `LOGGED_OUT` | `Login(REMOTE, ..)` | remote-user list matches | fail[REMOTE] = 0; return `SEC_LOGIN_USER` | `USER` |
| `LOGGED_OUT` | `Login(LOCAL, ..)` | `SEC_u8IsLocalAllowed()` and keypad-user list matches | fail[LOCAL] = 0; post `EVQ_LOCAL_SESSION(1)`; return `SEC_LOGIN_USER` | `USER` |
| `LOGGED_OUT` | `Login(src, ..)` | no match and `++fail[src] < 3` | return `SEC_LOGIN_FAILED` | `LOGGED_OUT` |
| `LOGGED_OUT` | `Login(src, ..)` | no match and `++fail[src] == 3` | `ALARM_voidTrigger()`; return `SEC_LOGIN_LOCKED` | `LOGGED_OUT` (system locked) |
| `USER` / `ADMIN` | `Logout(src)` | — | if it was the admin: `USERDB_voidSetWriteAccess(0)`, local allowed = 0; if LOCAL: post `EVQ_LOCAL_SESSION(0)` | `LOGGED_OUT` |
| `ADMIN` | `SetLocalAllowed(x)` | — | store x; x = 0 also forces LOCAL to `LOGGED_OUT` | `ADMIN` |

`SEC_u8IsLocalAllowed()` = not locked **and not** (remote role is ADMIN and local allowed is 0).
The admin record is only ever checked for the REMOTE source (SEC-01). A wrong name and a wrong password give the same result, so the reply never tells which half was wrong.

Permissions (checked by the UIs through `SEC_u8GetRole`):

| Action | Admin (remote) | Remote user | Keypad user |
|---|---|---|---|
| Lamps, dimmer | yes | yes | yes |
| View AC, heater, system status | yes | yes | yes |
| Change heater set temperature | yes | yes | yes (decision D-9) |
| Open / close door | yes | **no** | **no** (no menu entry) |
| Add / remove / list users, change admin password, factory reset, keypad allow/block | yes | no | no |

### 5.2 ALARM — latched lockdown (REQ-ALM-01, SEC-05)

| State | Event | Action | Next |
|---|---|---|---|
| `IDLE` | `ALARM_voidTrigger()` | `HEATER_voidForceOff()`; `CLIMATE_voidForceOff()`; `BUZZER_voidOn()`; post `EVQ_LOCKDOWN` | `ACTIVE` |
| `ACTIVE` | 500 ms task | toggle the buzzer (0.5 s on / 0.5 s off) | `ACTIVE` |
| `ACTIVE` | anything else | ignored. Only an MCU reset leaves this state | `ACTIVE` |

Lamps, dimmer and door keep their state (Section 12 #2). Both UIs poll `ALARM_u8IsActive()` and go to their own `LOCKED` state.

### 5.3 LIGHT — lamps and dimmer (REQ-LGT-01, 02)

No modes, only data: lamp mask (5 bits) and dimmer percent. Boot: all off, 0 %.

| Event | Guard | Action |
|---|---|---|
| `ToggleLamp(n)` | 1 <= n <= 5 | flip bit n; `LAMP_u8SetState`; post `EVQ_LAMP(n)` |
| `SetDimmer(p)` | — | p limited to 100 and rounded down to a multiple of 10; `DIMMER_voidSetLevel(p)`; if changed post `EVQ_DIMMER(p)` |

### 5.4 DOOR (REQ-DOR-01, 02)

| State | Event | Action | Next |
|---|---|---|---|
| `CLOSED` | `SetState(DOOR_OPEN)` | `SERVO_voidSetAngle(DOOR_OPEN_ANGLE)`; return 1 | `OPEN` |
| `OPEN` | `SetState(DOOR_CLOSED)` | `SERVO_voidSetAngle(DOOR_CLOSED_ANGLE)`; return 1 | `CLOSED` |
| either | `SetState(same)` | return 0 | same |

Only `UIREM` calls it, and only when `SEC_u8GetRole(SEC_SOURCE_REMOTE) == SEC_ROLE_ADMIN`. The door can only be moved from the terminal, so it posts no event.

### 5.5 CLIMATE — air conditioning (REQ-AC-01, 02, 03)

Every 100 ms: sample the ambient LM35 into a `MAVG_t`, then:

| State | Guard | Action | Next |
|---|---|---|---|
| `WAIT` (boot) | average has 10 samples | — | `AC_OFF` |
| `AC_OFF` | `avg_x4 > 28 * 4` | `FAN_voidSetSpeed(CLIMATE_FAN_SPEED)`; post `EVQ_AC(1)` | `AC_ON` |
| `AC_ON` | `avg_x4 < 21 * 4` | `FAN_voidSetSpeed(0)`; post `EVQ_AC(0)` | `AC_OFF` |
| `AC_OFF` / `AC_ON` | 21 <= avg <= 28 | nothing (hysteresis) | same |
| any | `CLIMATE_voidForceOff()` | fan 0 % | `FORCED_OFF` (stays until reset) |

### 5.6 HEATER — water heater (REQ-HTR-01..14)

Panel state:

| State | Event | Guard | Action | Next |
|---|---|---|---|---|
| (boot) | init | — | set = `ESTORE` byte if it is 35..75 and a multiple of 5, else 60; elements, LED, display off | `OFF` |
| `OFF` | ON/OFF **released** | — | set re-read from `ESTORE`; water average reset; display on; post `EVQ_HEATER_POWER(1)` | `RUN` |
| `OFF` | Up / Down pressed | — | ignored (decision D-10) | `OFF` |
| `RUN` | Up or Down pressed | — | **value not changed**; timer = 5 s; blink phase = on | `SET` |
| `SET` | Up pressed | — | `set = min(set + 5, 75)`; timer = 5 s; blink phase = on (the new value is visible at once) | `SET` |
| `SET` | Down pressed | — | `set = max(set - 5, 35)`; timer = 5 s; blink phase = on | `SET` |
| `SET` | 100 ms task | `--timer == 0` | if the value changed: save to `ESTORE`, post `EVQ_HEATER_SET` | `RUN` |
| `RUN` / `SET` | ON/OFF released | — | save if changed; both elements off; LED off; display blank; post `EVQ_HEATER_POWER(0)` | `OFF` |
| any | `HEATER_u8SetSetTemp(t)` (remote / keypad) | 35 <= t <= 75 and t % 5 == 0 | set = t; save now; post `EVQ_HEATER_SET`; in `SET` also restart the 5 s timer | same |
| any | `HEATER_voidForceOff()` | — | as "ON/OFF released" | `LOCKED` (buttons ignored until reset) |

Element control, every 100 ms in `RUN` and `SET`, **only when the average holds 10 samples** (HTR-07, 08, 09):

| Element state | Guard | Action | Next |
|---|---|---|---|
| any | `avg_x4 < (set - 5) * 4` | cooling off, then heating on | `HEATING` |
| any | `avg_x4 > (set + 5) * 4` | heating off, then cooling on | `COOLING` |
| any | between | nothing | same |

The water LM35 is sampled every 100 ms in every panel state (so the remote terminal and the LCD can show the water temperature while the heater is off); only the decisions are tied to `RUN`/`SET`.

Outputs (written by the 100 ms and 500 ms tasks; `phase` toggles every 500 ms, so one blink period is 1 s):

| Panel state | 7-segment (HTR-10, 11) | Heater LED (HTR-13) |
|---|---|---|
| `OFF`, `LOCKED` | blank (HTR-06) | off |
| `RUN` | water temperature = `(avg_x4 + 2) / 4`, limited to 99 | `HEATING`: follows `phase` · `COOLING`: on · `NONE`: off |
| `SET` | set temperature while `phase` = on, blank while off | same as `RUN` |

### 5.7 UILOC — LCD and keypad (REQ-LUI-01..04, SEC-08)

Keys of the fitted keypad (`KPAD_MAT`): digits, `=` Enter, `*` or `C` back (deletes the last digit, or leaves the screen when nothing is typed), `+` / `-` up / down.

| State | Event | Guard | Action | Next |
|---|---|---|---|---|
| `STATUS` | 1 s task | — | refresh the status page; switch page every 3 s | `STATUS` |
| `STATUS` | any key | `SEC_u8IsLocalAllowed()` | show `User ID:` (a digit key is kept as the first digit) | `ASK_ID` |
| `STATUS` | any key | not allowed | show `Keypad blocked` / `by admin` for 2 s | `MESSAGE` -> `STATUS` |
| `ASK_ID` | digit / back / `=` | 1..8 digits for `=` | edit the ID, shown in clear | `ASK_PIN` (on `=`), `STATUS` (back when empty) |
| `ASK_PIN` | digit / back | — | edit the PIN, shown as `*` (LUI-04) | `ASK_PIN`, `ASK_ID` (back when empty) |
| `ASK_PIN` | `=` | `SEC_u8Login` = USER | — | `MENU` |
| `ASK_PIN` | `=` | = FAILED | show `Wrong ID or PIN` / `n tries left` for 2 s | `MESSAGE` -> `ASK_ID` |
| `ASK_PIN` | `=` | = LOCKED | — | `LOCKED` |
| `MENU` | `1` / `2` / `3` / `4` | — | — | `LAMPS` / `DIMMER` / `AC` / `HEATER` |
| `MENU` | back | — | `SEC_voidLogout(LOCAL)` | `STATUS` |
| `LAMPS` | `1`..`5` | — | `LIGHT_u8ToggleLamp` | `LAMPS` |
| `DIMMER` | `+` / `-` | — | `LIGHT_voidSetDimmer(current ± 10)` | `DIMMER` |
| `AC` | 1 s task | — | refresh (view only) | `AC` |
| `HEATER` | `+` / `-` | — | `HEATER_u8SetSetTemp(current ± 5)` | `HEATER` |
| `LAMPS`, `DIMMER`, `AC`, `HEATER` | back | — | — | `MENU` |
| any logged-in state | 1 s task | 30 s without a key | logout | `STATUS` |
| any logged-in state | 10 ms task | `SEC_u8GetRole(LOCAL)` is NONE (admin logged in or blocked the keypad) | — | `STATUS` |
| any | 10 ms task | `ALARM_u8IsActive()` | show `SYSTEM LOCKED` / `Reset required` | `LOCKED` (keys ignored) |

Screens (16 x 2, texts final in Phase 2):

| Screen | Line 1 | Line 2 |
|---|---|---|
| Status page 1 (LUI-03) | `Lamps:1-3-- D70%` | `AC:ON   Room:29C` |
| Status page 2 | `Heater:HEATING` (or `OFF` / `COOLING` / `IDLE`) | `Water:55C Set:60` |
| Login | `User ID:` + digits | `PIN:` + `****` |
| Menu (LUI-02) | `1Lamps  2Dimmer` | `3AC 4Heat C=Exit` |
| Lamps | `Lamp 1 2 3 4 5` | `     * - * - -` |
| Dimmer | `Dimmer:  70%` | `+ / -     C=Back` |
| AC | `AC:ON  Room:29C` | `on>28 off<21` |
| Heater | `Water:55C HEAT` | `Set:60C  + / -` |

### 5.8 UIREM — UART terminal (REQ-RUI-01..04, SEC-01, 03, 06)

Texts, menu numbers and error messages: [uart_protocol.md](uart_protocol.md).

Output rule: the task prints at most one "step" per call and only when `TERM_u8TxFree() >= UIREM_TX_RESERVE` (64). Long outputs (banner, menu, status block, user list) are a **print sequence**: a sequence number and a step counter; each step prints one line. This is what keeps a 450-byte menu from ever blocking on a 128-byte TX ring.

| State | Event | Guard | Action | Next |
|---|---|---|---|---|
| `BANNER` | boot / after logout | — | print sequence: banner + `Hey, please enter your username:` | `ASK_NAME` |
| `ASK_NAME` | line | — | keep the name; echo masked; print the password prompt | `ASK_PASS` |
| `ASK_PASS` | line | `SEC_u8Login(REMOTE)` = ADMIN or USER | echo normal; welcome; storage warning if `ESTORE_FAULT` | `MENU_PRINT` |
| `ASK_PASS` | line | = FAILED | error + attempts left | `ASK_NAME` |
| `ASK_PASS` | line | = LOCKED | — | `LOCKED` |
| `MENU_PRINT` | sequence finished | — | print the prompt | `PROMPT` |
| `PROMPT` | no line, nothing typed | event waiting in `EVQ` | print one `[INFO]` line and the prompt again | `PROMPT` |
| `PROMPT` | line `?` or empty | — | — | `MENU_PRINT` |
| `PROMPT` | line `0` | — | goodbye; `SEC_voidLogout(REMOTE)` | `BANNER` |
| `PROMPT` | line `3`, `4`, `6`, `14` | role allows | print sequence (status / list) | `PROMPT` |
| `PROMPT` | line `7`, `8`, `9` | admin | do it; print the result | `PROMPT` |
| `PROMPT` | line `1`, `2`, `5`, `11`, `13` | role allows; no argument typed on the same line | print the argument prompt | `ARG` |
| `PROMPT` | the same with an argument on the line (`1 3`) | role allows | handle it as `ARG` at once | `PROMPT` |
| `PROMPT` | line `10` / `12` | admin | prompt for the name | `ADD_NAME` |
| `PROMPT` | line `15` | admin | echo masked; prompt | `NEW_PASS` |
| `PROMPT` | line `16` | admin | prompt `Type YES to confirm:` | `CONFIRM` |
| `PROMPT` | line `7`..`16` | role is USER | `[ERR] Not allowed for your role.` | `MENU_PRINT` |
| `PROMPT` | any other line / too long | — | `[ERR] ...` (RUI-04) | `MENU_PRINT` |
| `ARG` | line | valid | apply; `[OK] ...` | `PROMPT` |
| `ARG` | line | invalid | `[ERR] ...` | `MENU_PRINT` |
| `ADD_NAME` | line | name valid and free | keep it; echo masked; prompt for the password | `ADD_PASS` |
| `ADD_NAME` | line | invalid / exists / list full | `[ERR] ...` | `MENU_PRINT` |
| `ADD_PASS` | line | `USERDB_u8AddUser` = OK | echo normal; `[OK] ...` | `PROMPT` |
| `ADD_PASS` | line | error | echo normal; `[ERR] ...` | `MENU_PRINT` |
| `NEW_PASS` | line | valid | `USERDB_u8SetAdminPassword`; `[OK] ...` | `PROMPT` |
| `CONFIRM` | line `YES` | — | `ESTORE_voidLoadDefaults()`; message; logout | `BANNER` |
| `CONFIRM` | other line | — | `Cancelled.` | `PROMPT` |
| any logged-in state | 1 s task | 120 s without a received byte | timeout message; logout | `BANNER` |
| any | 5 ms task | `ALARM_u8IsActive()` | print the lock message once; received bytes are thrown away | `LOCKED` |

While nobody is logged in remotely the events in `EVQ` are read and discarded (except `EVQ_LOCKDOWN`, which is always printed).

### 5.9 Supporting state machines (SERVICE / HAL)

**ESTORE write-behind** (10 ms task, REQ-EEP-03):

| State | Guard | Action | Next |
|---|---|---|---|
| `IDLE` | a page is dirty (highest page number first) | clear its dirty bit; `EXT_EEPROM_u8WritePage(page * 16, &image[page * 16], 16)`; polls = 0 | `WRITING` (`FAULT` if the transfer failed) |
| `WRITING` | `EXT_EEPROM_u8IsReady()` = 1 | — | `IDLE` |
| `WRITING` | not ready and `++polls < 5` | — | `WRITING` |
| `WRITING` | not ready and `polls == 5` (50 ms) | post `EVQ_STORAGE_FAULT` | `FAULT` |
| `FAULT` | — | no more writes; `ESTORE_u8GetStatus()` = `ESTORE_FAULT` | `FAULT` |

A byte changed while its page is being written sets the dirty bit again, so the page is written once more. Reads always come from the RAM image and never wait.

**KPAD / BUTTON debounce:** a raw value becomes the accepted value after N equal consecutive samples (KPAD 2 x 10 ms, BUTTON 3 x 10 ms). A change of the accepted value produces one event; events are cleared by reading them.

**LCD_BUF:** described in Section 3.2.

---

## 6. Data flow — who owns what

Rule: one owner per piece of state, kept `static` in the owner's `.c`; everyone else uses the owner's functions. No shared global variables.

| State | Owner | Changed by (through the owner's API) | Read by |
|---|---|---|---|
| Lamp states (5 bits) | `LIGHT` | `UIREM`, `UILOC` | `UIREM`, `UILOC` |
| Dimmer level | `LIGHT` | `UIREM`, `UILOC` | `UIREM`, `UILOC` |
| Door state | `DOOR` | `UIREM` (admin only) | `UIREM` |
| Room temperature (average), AC on/off | `CLIMATE` | itself (100 ms task), `ALARM` (force off) | `UIREM`, `UILOC` |
| Water temperature (average) | `HEATER` | itself (100 ms task) | `UIREM`, `UILOC` |
| Heater panel state, element state | `HEATER` | itself (buttons, control law), `ALARM` (force off) | `UIREM`, `UILOC` |
| Heater set temperature | `HEATER` (working copy) + `ESTORE` (stored copy) | panel buttons, `UIREM`, `UILOC` | `UIREM`, `UILOC` |
| Login sessions, fail counters, keypad-allowed flag | `SEC` | `UIREM`, `UILOC` | `UIREM`, `UILOC` |
| Lockdown flag | `ALARM` | `SEC` | `UIREM`, `UILOC` |
| Accounts | `USERDB` (inside the `ESTORE` image) | `UIREM` (admin, write gate open) | `SEC` |
| EEPROM image, dirty pages, fault flag | `ESTORE` | `USERDB`, `HEATER` | `USERDB`, `HEATER`, `UIREM` (status) |
| Pending events | `EVQ` | `LIGHT`, `CLIMATE`, `HEATER`, `SEC`, `ALARM`, `ESTORE` | `UIREM` |
| Tick, task flags | `SCHED` | tick ISR | `main` |
| LCD text | `LCD_BUF` | `UILOC` | — |

```
 keypad --KPAD--> UILOC --+                                   +--> LAMP --> PCF8574 (I2C)
                          +--> SEC --> USERDB --> ESTORE --> EXT_EEPROM (I2C)
 UART --USART--> TERM --> UIREM --+        |                  +--> DIMMER (Timer0) / SERVO (Timer1)
                          |       |        v                  |
                          |       +--> ALARM --> BUZZER       |
                          +----------> LIGHT / DOOR ----------+
 LM35 ambient --> CLIMATE --> FAN (Timer1)      LIGHT, CLIMATE, HEATER, SEC, ALARM, ESTORE
 LM35 water + buttons --> HEATER --> RELAY, LED, SEVEN_SEG          |  EVQ_voidPost
                                                                    v
 UILOC --> LCD_BUF --> CLCD --> PCF8574 (I2C)          EVQ --> UIREM --> TERM --> UART "[INFO] ..."
```

---

## 7. RAM and flash budget

### 7.1 RAM (2048 bytes)

| Item | Bytes |
|---|---|
| `ESTORE` image (0x00–0xCF) + dirty mask + state | 208 + 5 |
| `TERM` RX ring 32 + TX ring 128 + line 17 + 2 ring structs + flags | 192 |
| `LCD_BUF` wanted 32 + shown 32 + cursor/retry | 67 |
| `EVQ` storage 32 + ring struct | 38 |
| `MAVG_t` x 2 (water, ambient) | 48 |
| `SCHED` tick, flags, counters, overruns | 16 |
| MCAL callback pointers (TIMER0 x2, TIMER2, ADC x2, EXTI x3, USART x2) | 20 |
| `KPAD`, `BUTTON`, `SEVEN_SEG`, `LAMP` state | 22 |
| `SEC` (counters, roles, flag, remote user name 9) | 15 |
| `LIGHT`, `DOOR`, `CLIMATE`, `ALARM`, `HEATER` state | 20 |
| `UILOC` (state, ID 9, PIN 9, timers) | 26 |
| `UIREM` (state, sequence, command, name 9, timers) | 22 |
| **Static total** | **~700 (34 %)** |
| Stack: deepest path `main -> UIREM -> USERDB -> ESTORE` with a 17-byte line buffer, plus one ISR frame with a callback (~40 bytes) | reserve 300 |
| **Free** | **~1050** |

All text is in flash. Today `KPAD` keeps 24 bytes of tables in RAM; the fix in Section 2 makes them `const`.

### 7.2 Flash (32 768 bytes)

Reference point: `test_base` (existing drivers + about 2 KB of test text, no LTO) is 7.4 KB today.

| Part | Estimate |
|---|---|
| MCAL (10 modules) | 2.6 KB |
| HAL (16 modules) | 3.4 KB |
| SERVICE (8 modules) | 3.3 KB |
| APP: `UIREM` 3.2, `UILOC` 2.4, `HEATER` 1.2, `SEC` 0.5, `CLIMATE` 0.4, `LIGHT` 0.4, `DOOR` 0.2, `ALARM` 0.2 | 8.5 KB |
| Text in flash: UART (~75 strings) 2.0 KB, LCD 0.4 KB | 2.4 KB |
| Vectors, startup, libgcc helpers (16/32-bit divide) | 0.7 KB |
| **Total** | **~21 KB (64 %)** |

No `float`, no `printf`, no dynamic memory. The real numbers are printed by `pio run` and recorded at the end of every layer; the alarm line is 28 KB.

---

## 8. Phase 2 implementation order

One module at a time: write -> `pio run` -> fix -> short summary. Each layer ends with its test program and stops for review.

| Step | Modules, in order | Ends with |
|---|---|---|
| **MCAL** | `reg_def.h` additions -> ISR attributes (ADC, TIMER0, EXTI) + remove `-flto` unflag -> TIMER0 PWM fix -> ADC fix -> EXTI fix -> DIO JTAG -> USART (baud + interrupts) -> TWI (timeout + probe) -> TIMER2 -> TIMER1 | `test_mains/test_mcal.c`, `[env:test_mcal]`. `test_base` must now be all `[PASS]` |
| **HAL** | PCF8574 multi-byte -> CLCD -> LCD_BUF -> KPAD -> BUTTON -> SEVEN_SEG -> LM35 -> RELAY, LED, BUZZER -> LAMP -> FAN -> SERVO -> DIMMER -> EXT_EEPROM | `test_mains/test_hal.c`, `[env:test_hal]` (uses TIMER2 directly for a 1 ms tick, because SCHED does not exist yet) |
| **SERVICE** | flash_str.h -> RINGBUF -> MAVG -> FMT -> SCHED -> TERM -> EVQ -> ESTORE -> USERDB | `test_mains/test_service.c`, `[env:test_service]` |
| **APP** | ALARM -> SEC -> LIGHT -> DOOR -> CLIMATE -> HEATER -> UILOC -> UIREM | `test_mains/test_app.c`, `[env:test_app]` (its own small super-loop) |
| **Phase 3** | `src/APP/main.c`, full build, docs updated to the real code | `pio run -e app` + the full `test_plan.md` |

What each test program checks is listed in [test_plan.md](test_plan.md) Section 2.

`platformio.ini`: each `[env:test_<layer>]` is added like `[env:test_base]` (allowed without asking). One further change is **approved** (2026-10-02) for when the SERVICE layer starts: `lib/Service` gets `.c` files for the first time, so `Service` must be added to `lib_deps`, and `[env:app]` will need the same `lib_deps` line in Phase 3.

---

## 9. Design decisions and assumptions

**All approved by the user on 2026-10-02.** Marked in code as `/* ASSUMPTION: ... */` where they are implemented. "CLAUDE" = CLAUDE.md.

| ID | Decision | Reason |
|---|---|---|
| D-1 | Modules added to CLAUDE Section 5: HAL `LCD_BUF`, `RELAY`, `LED`; SERVICE `TERM`, `ESTORE`, `EVQ`, `MAVG`, `FMT`, `flash_str.h` | I²C LCD is too slow to write synchronously; EEPROM reads must not wait for write cycles; UI and feature modules need a one-way link |
| D-2 | `SCHED` and `TERM` (SERVICE) call MCAL directly | CLAUDE Section 5 places the scheduler in SERVICE and the UART in MCAL but also says a layer only calls the layer directly below; an empty HAL wrapper would be the only alternative |
| D-3 | USART stays a thin MCAL driver with callbacks; the ring buffers are in `TERM` | keeps MCAL free of SERVICE code and matches the author's ISR -> callback style |
| D-4 | TWI stays polled (with timeout) instead of interrupt-driven | every transfer is bounded (<= 1.7 ms); an interrupt-driven master with a transaction queue is a much bigger and riskier module |
| D-5 | The whole EEPROM map is mirrored in RAM (208 bytes) | login must scan up to 11 records; reading them from the chip would block ~20 ms and would fail during a write cycle |
| D-6 | An account slot has no separate valid byte: a first name byte of `0x00` or `0xFF` means "empty" | keeps a record at exactly one 16-byte page with 8 + 8 characters, so every add/remove is one atomic page write (CLAUDE Section 9 suggested a flag byte) |
| D-7 | Keypad: `=` is Enter (the keypad has no `#`), `*` and `C` are back, `+`/`-` step values | `KPAD_MAT` is a calculator keypad; changing the driver table is not needed |
| D-8 | The full menu is printed after login, after an error and on `?`; after a successful command only the one-line prompt. `UIREM_MENU_AFTER_COMMAND 1` restores "menu after every command" | a 15-line menu after every command takes 0.5 s at 9600 baud and floods a phone screen |
| D-9 | Extras beyond the spec: one-line commands (`1 3`), admin "change password" and "factory reset", keypad users may change the heater set temperature with `+`/`-` | small, and they make the demo and the tests much easier; each can be dropped without touching the rest |
| D-10 | Heater Up/Down are ignored while the heater is OFF | HTR-06: all displays are off, so setting mode could not be shown |
| D-11 | Remote session ends after 120 s without input | otherwise an admin who walks away blocks the keypad for ever (SEC-08) |
| D-12 | SEC-07 is implemented as a write gate in `USERDB`, open only while the admin is logged in. The heater set temperature is not behind the gate | the course text ties "read/write in admin mode, read only in user mode" to user registration; HTR-14 lets users change the set temperature |
| D-13 | Passwords are stored and compared as plain text | simulation project; hashing is a possible later improvement |
| D-14 | Lamp, dimmer and door states are not stored in EEPROM (boot = off, closed) | not required |
| D-15 | Lockdown buzzer beeps 0.5 s / 0.5 s | recognisable as an alarm; one macro makes it continuous |
| D-16 | Not built in the first version: AC-03 fan speed ramp, ALM-02 PIR, LDR. Pins stay reserved | CLAUDE: optional, after the spec is done |
| D-17 | Dimmer and fan swapped against the first pin plan: dimmer on PB3 / OC0 (Timer0, 7.8 kHz), fan on PD4 / OC1B (Timer1, 50 Hz) | the dimmer needs a 0–5 V level; 7.8 kHz filters with a small RC and reacts fast, 50 Hz does not. 50 Hz is fine for an on/off fan (pin_map C-1) |
| D-18 | Lamps on the PCF8574 are active-low (`LAMP_ON_LEVEL 0`) | a PCF8574 can sink but not source LED current, and its power-up state (all high) then means all lamps off (pin_map C-2) |
| D-19 | `Service` is added to `lib_deps` when the SERVICE layer gets its first `.c` file, and `[env:app]` gets the same line in Phase 3 | without it the layer does not link |
| D-20 | **Approved 2026-10-03.** The 7-segment display is two PCF8574 expanders, one per common-anode digit (tens 0x21, units 0x22), instead of a 7447 + multiplexed digits. No `SEVEN_SEG_voidRefresh`, no tick hook, no JTAG handling | the 7447 + PNP multiplexed version was unreliable in Proteus (both digits showed the same number); it also frees PC2–PC7 and removes all work from the tick ISR. Cost: two more I²C devices (0x21, 0x22) and 0.2 ms of bus time per changed digit |
