# UART protocol — remote terminal

Phase 1 design. **Status: waiting for approval.** Implements REQ-RUI-01..04 and the remote side of REQ-SEC, REQ-DOR-01, REQ-HTR-14.
The state machine behind it is `UIREM` in [architecture.md](architecture.md) Section 5.8. All text is stored in flash.

---

## 1. Link

| Item | Value |
|---|---|
| Format | **9600 baud, 8 data bits, no parity, 1 stop bit** |
| Baud value | `UBRR = F_CPU / (16 * 9600) - 1 = 103` at 16 MHz -> 9615 baud, error +0.16 %. U2X is not needed |
| Pins | PD0 = RXD, PD1 = TXD |
| Peer | Proteus Virtual Terminal, or HC-05 Bluetooth / USB-TTL adapter with any serial terminal app |
| Sent line ending | `\r\n` |
| Accepted Enter | `\r`, `\n` or `\r\n` |
| Echo | the MCU echoes each accepted character. Password input is echoed as `*`. (`TERM_ECHO_DEFAULT` can switch the echo off for phone apps that echo locally) |
| Editing | Backspace (0x08) or Delete (0x7F) removes the last character |
| Line length | max 16 characters; a longer line is rejected with `[ERR] Input too long.` |
| Case | names and passwords are case sensitive |
| Buffers | RX 32 bytes, TX 128 bytes, both interrupt driven. Nothing in the main loop ever waits for the UART |

Proteus Virtual Terminal settings: 9600 / 8 / NONE / 1, and **"Echo Typed Characters" off** (the MCU echoes).

---

## 2. Accounts

| Role | Where it can log in | Created by |
|---|---|---|
| Admin | UART only (REQ-SEC-01) | first boot: **`admin` / `1234`** (REQ-SEC-09). Change it with command 15 |
| Remote user | UART only | admin, command 10 (max 5) |
| Keypad user | keypad only (REQ-SEC-02) | admin, command 12 (max 5) |

Name: 1–8 characters, no space. Password: 4–8 characters. Keypad users: digits only for both.

---

## 3. Message kinds

| Prefix | Meaning |
|---|---|
| none | banner, prompts, menu, status text |
| `[OK]` | the command was done |
| `[ERR]` | the input was rejected; the menu is printed again (REQ-RUI-04) |
| `[INFO]` | something changed without a command from this terminal (keypad, heater buttons, automatic AC) (REQ-RUI-02) |
| `[WARN]` | storage problem |
| `[ALERT]` | lockdown |

---

## 4. Login (REQ-SEC-01, 05)

```
==== SMART HOME + WATER HEATER ====
Hey, please enter your username:
> admin
Please enter your password:
> ****
Welcome admin. You are logged in as ADMIN.
```

Both fields are always asked, then the pair is checked once. One checked pair = one attempt.

| Case | Reply | Then |
|---|---|---|
| Admin pair correct | `Welcome admin. You are logged in as ADMIN.` | admin menu |
| Remote-user pair correct | `Welcome bob. You are logged in as USER.` | user menu |
| Wrong pair, 1st time | `[ERR] Wrong username or password. 2 attempts left.` | asks for the username again |
| Wrong pair, 2nd time | `[ERR] Wrong username or password. 1 attempt left.` | asks for the username again |
| Wrong pair, 3rd time | `[ALERT] 3 wrong logins. SYSTEM LOCKED. Reset to restart.` | locked: no further input is answered |
| EEPROM not answering (after a good login) | `[WARN] EEPROM not responding. Changes will not be saved.` | menu |

A correct login sets the counter back to 3 attempts. The counter of the keypad is separate.

---

## 5. Menus (REQ-RUI-03)

User menu:

```
---------- MENU ----------
 1  Lamp on/off (1-5)
 2  Dimmer level
 3  AC status
 4  Heater status
 5  Heater set temperature
 6  System status
 0  Logout
 ?  Show this menu
bob>
```

Admin menu (the same, plus 7–16):

```
---------- MENU ----------
 1  Lamp on/off (1-5)
 2  Dimmer level
 3  AC status
 4  Heater status
 5  Heater set temperature
 6  System status
 7  Open the door
 8  Close the door
 9  Keypad control: allow / block
10  Add remote user
11  Remove remote user
12  Add keypad user
13  Remove keypad user
14  List users
15  Change admin password
16  Factory reset
 0  Logout
 ?  Show this menu
admin>
```

The prompt is the user name followed by `>`. The full menu is printed after login, after every `[ERR]`, and for `?` or an empty line; after a successful command only the prompt comes back (decision D-8).

---

## 6. Commands

Commands with one argument accept it on the same line (`1 3`, `2 70`, `5 65`, `11 bob`, `13 1111`). Without it the terminal asks.

| # | Role | Prompt for the argument | Valid | Reply | Error replies |
|---|---|---|---|---|---|
| 1 | all | `Lamp number (1-5):` | 1–5 | `[OK] Lamp 3 is now ON` / `OFF` (toggles) | `[ERR] Lamp number must be 1 to 5.` |
| 2 | all | `Dimmer level in % (0-100, step 10):` | 0, 10, … 100 | `[OK] Dimmer is now 70%` | `[ERR] Dimmer level must be 0 to 100 in steps of 10.` |
| 3 | all | — | — | `AC is ON. Room temperature 29C.` / `AC is OFF. Room temperature 24C.` / `AC: waiting for the first reading.` | — |
| 4 | all | — | — | `Heater is OFF. Water 41C, set 60C.` / `Heater is ON (HEATING). Water 52C, set 60C.` (`HEATING`, `COOLING`, `IDLE`) | — |
| 5 | all | `Set temperature (35-75, step 5):` | 35, 40, … 75 | `[OK] Heater set temperature is now 65C` | `[ERR] Temperature must be 35 to 75 in steps of 5.` |
| 6 | all | — | — | status block (Section 7) | — |
| 7 | admin | — | — | `[OK] Door is now OPEN` / `Door is already open.` | — |
| 8 | admin | — | — | `[OK] Door is now CLOSED` / `Door is already closed.` | — |
| 9 | admin | — | — | `[OK] Keypad control is now ALLOWED` / `BLOCKED` (toggles) | — |
| 10 | admin | `New remote username (1-8 characters):` then `Password (4-8 characters):` (echo `*`) | see Section 2 | `[OK] Remote user bob added.` | `[ERR] Username must be 1 to 8 characters, no spaces.` · `[ERR] Password must be 4 to 8 characters.` · `[ERR] This username already exists.` · `[ERR] The user list is full (5).` |
| 11 | admin | `Remote username to remove:` | existing name | `[OK] Remote user bob removed.` | `[ERR] User not found.` |
| 12 | admin | `New keypad ID (1-8 digits):` then `PIN (4-8 digits):` (echo `*`) | digits | `[OK] Keypad user 1111 added.` | as 10, plus `[ERR] Keypad users need digits only.` |
| 13 | admin | `Keypad ID to remove:` | existing ID | `[OK] Keypad user 1111 removed.` | `[ERR] User not found.` |
| 14 | admin | — | — | user list (Section 8) | — |
| 15 | admin | `New admin password (4-8 characters):` (echo `*`) | 4–8 characters | `[OK] Admin password changed.` | `[ERR] Password must be 4 to 8 characters.` |
| 16 | admin | `This erases all users and settings. Type YES to confirm:` | `YES` | `[OK] Factory reset done. Admin is admin / 1234.` then logout | anything else: `Cancelled.` |
| 0 | all | — | — | `Goodbye bob.` then the banner and the username prompt | — |
| ? | all | — | — | the menu | — |

General errors:

| Input | Reply |
|---|---|
| unknown command, letters where a number is expected | `[ERR] Unknown command.` + menu |
| 7–16 typed by a remote user | `[ERR] Not allowed for your role.` + menu (REQ-SEC-06) |
| more than 16 characters | `[ERR] Input too long.` + menu |
| lamp / EEPROM bus error | `[ERR] Device not responding.` |

Commands 15 and 16, the one-line form and `?` are extras beyond the course text (decision D-9).

---

## 7. Status block (command 6)

```
---- System status ----
Lamps : 1=ON 2=OFF 3=ON 4=OFF 5=OFF
Dimmer: 70%
AC    : ON, room 29C
Heater: ON (HEATING), water 52C, set 60C
Door  : CLOSED
Keypad: ALLOWED, user logged in
```

The last line is printed for the admin only (`ALLOWED` / `BLOCKED`, `user logged in` / `nobody logged in`).

---

## 8. User list (command 14)

```
Remote users (2 of 5):
  bob
  alice
Keypad users (1 of 5):
  1111
```

Passwords are never printed.

---

## 9. Events (REQ-RUI-02)

While somebody is logged in on the terminal, every change made elsewhere is reported. An event is printed only when the terminal is at the prompt and nothing has been typed yet, so it never cuts into a half-typed line; the prompt is printed again after it. Changes caused by the terminal's own command are answered with `[OK]`, not repeated as an event.

| Cause | Text |
|---|---|
| lamp toggled on the keypad | `[INFO] Lamp 2 is now ON` |
| dimmer changed on the keypad | `[INFO] Dimmer is now 30%` |
| AC switched by temperature | `[INFO] AC is now ON` / `[INFO] AC is now OFF` |
| heater ON/OFF button | `[INFO] Heater is now ON` / `OFF` |
| heater element changed | `[INFO] Heater: HEATING` / `COOLING` / `IDLE` |
| set temperature changed on the panel or keypad | `[INFO] Heater set temperature is now 65C` |
| keypad user logged in / out | `[INFO] Keypad user logged in` / `logged out` |
| EEPROM stopped answering | `[WARN] EEPROM not responding. Changes will not be saved.` |
| lockdown caused on the keypad | `[ALERT] 3 wrong logins. SYSTEM LOCKED. Reset to restart.` (printed even when nobody is logged in) |
| 120 s without input | `[INFO] No input for 120 s. Logged out.` then the banner |

---

## 10. Lockdown (REQ-SEC-05, REQ-ALM-01)

After the 3rd wrong login in a row on the terminal **or** on the keypad: the alert line is printed once, then every received byte is ignored. The buzzer sounds, the heater elements and the AC fan are switched off, the LCD shows `SYSTEM LOCKED`. Only an MCU reset ends it.

---

## 11. Example session

`>` marks what the person types (the MCU echo); everything else is sent by the MCU.

```
==== SMART HOME + WATER HEATER ====
Hey, please enter your username:
> admin
Please enter your password:
> ****
Welcome admin. You are logged in as ADMIN.
---------- MENU ----------
 1  Lamp on/off (1-5)
 2  Dimmer level
 3  AC status
 4  Heater status
 5  Heater set temperature
 6  System status
 7  Open the door
 8  Close the door
 9  Keypad control: allow / block
10  Add remote user
11  Remove remote user
12  Add keypad user
13  Remove keypad user
14  List users
15  Change admin password
16  Factory reset
 0  Logout
 ?  Show this menu
admin> 12
New keypad ID (1-8 digits):
> 1111
PIN (4-8 digits):
> ****
[OK] Keypad user 1111 added.
admin> 10
New remote username (1-8 characters):
> bob
Password (4-8 characters):
> ****
[OK] Remote user bob added.
admin> 1
Lamp number (1-5):
> 3
[OK] Lamp 3 is now ON
admin> 2 70
[OK] Dimmer is now 70%
admin> 7
[OK] Door is now OPEN
admin> 5 62
[ERR] Temperature must be 35 to 75 in steps of 5.
---------- MENU ----------
 ... (menu again) ...
admin> 5 65
[OK] Heater set temperature is now 65C
admin> 9
[OK] Keypad control is now ALLOWED
[INFO] Keypad user logged in
admin> 
[INFO] Lamp 1 is now ON
admin> 
[INFO] Heater is now ON
admin> 
[INFO] Heater: HEATING
admin> 6
---- System status ----
Lamps : 1=ON 2=OFF 3=ON 4=OFF 5=OFF
Dimmer: 70%
AC    : OFF, room 24C
Heater: ON (HEATING), water 41C, set 65C
Door  : OPEN
Keypad: ALLOWED, user logged in
admin> 0
Goodbye admin.
==== SMART HOME + WATER HEATER ====
Hey, please enter your username:
> bob
Please enter your password:
> ****
Welcome bob. You are logged in as USER.
---------- MENU ----------
 1  Lamp on/off (1-5)
 2  Dimmer level
 3  AC status
 4  Heater status
 5  Heater set temperature
 6  System status
 0  Logout
 ?  Show this menu
bob> 7
[ERR] Not allowed for your role.
---------- MENU ----------
 ... (menu again) ...
bob> 4
Heater is ON (HEATING). Water 47C, set 65C.
bob> 0
Goodbye bob.
==== SMART HOME + WATER HEATER ====
Hey, please enter your username:
> bob
Please enter your password:
> *****
[ERR] Wrong username or password. 2 attempts left.
Hey, please enter your username:
```
