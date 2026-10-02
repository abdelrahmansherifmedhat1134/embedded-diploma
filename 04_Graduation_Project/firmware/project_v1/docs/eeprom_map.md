# EEPROM map — external 24C08

Phase 1 design. **Status: waiting for approval.** Implements REQ-EEP-01, 02, 03 and stores the data of REQ-SEC-02, 03, 04, 09 and REQ-HTR-03, 04.

Chip: 24C08, I²C address 0x50, 1024 bytes, 16-byte pages. The design uses the first 208 bytes (pages 0–12, all inside block 0); the rest stays free.

---

## 1. Chip facts the layout is built on

| Fact | Value | Consequence |
|---|---|---|
| Blocks | 4 x 256 bytes; address bits 9:8 travel in the slave address (0x50–0x53) | all data is in block 0: slave address always 0x50, one word-address byte |
| Page | 16 bytes; a page write must stay inside one page | every record is exactly one page and starts on a page boundary |
| Write cycle | up to 5 ms (data sheet), **10 ms in the Proteus model** (`TD_WRITE`) | during the cycle the chip does not answer its address -> ACK polling, time-out 50 ms |
| Endurance | 1 000 000 writes per page | a page is written only when a byte in it really changed |

---

## 2. Byte-level map

| Address | Size | Page | `#define` | Content |
|---|---|---|---|---|
| 0x00 | 1 | 0 | `ESTORE_ADDR_MAGIC` | `ESTORE_MAGIC_VALUE` = 0xA5: the EEPROM holds valid data |
| 0x01 | 1 | 0 | `ESTORE_ADDR_VERSION` | `ESTORE_VERSION_VALUE` = 0x01: layout version |
| 0x02–0x0F | 14 | 0 | — | reserved, 0xFF |
| 0x10 | 1 | 1 | `ESTORE_ADDR_HEATER_SET` | heater set temperature in °C (35, 40, … 75) |
| 0x11–0x1F | 15 | 1 | — | reserved, 0xFF (room for future settings) |
| 0x20–0x27 | 8 | 2 | `ESTORE_ADDR_ADMIN` + `USERDB_NAME_OFFSET` | admin user name |
| 0x28–0x2F | 8 | 2 | `ESTORE_ADDR_ADMIN` + `USERDB_PASS_OFFSET` | admin password |
| 0x30–0x7F | 5 x 16 | 3–7 | `ESTORE_ADDR_REMOTE_USERS` | remote user slots 0–4 |
| 0x80–0xCF | 5 x 16 | 8–12 | `ESTORE_ADDR_KEYPAD_USERS` | keypad user slots 0–4 |
| 0xD0–0x3FF | 816 | 13–63 | — | not used, never touched |

Slot *i* of a list is at `list address + i * USERDB_RECORD_SIZE`:

| Remote slot | Address | Keypad slot | Address |
|---|---|---|---|
| 0 | 0x30 | 0 | 0x80 |
| 1 | 0x40 | 1 | 0x90 |
| 2 | 0x50 | 2 | 0xA0 |
| 3 | 0x60 | 3 | 0xB0 |
| 4 | 0x70 | 4 | 0xC0 |

Why three separate pages at the start: the page with the magic byte is written **only on first boot**. The set temperature (written at run time) lives in its own page, so a power failure during that write can never damage the magic byte and trigger a factory reset of the accounts.

---

## 3. Account record (admin, remote user, keypad user — one format, 16 bytes = 1 page)

| Offset | Size | `#define` | Content |
|---|---|---|---|
| +0 | 8 | `USERDB_NAME_OFFSET`, `USERDB_NAME_SIZE` | user name, ASCII, padded with 0x00. **Not** terminated when it is 8 characters long (code always limits to 8) |
| +8 | 8 | `USERDB_PASS_OFFSET`, `USERDB_PASS_SIZE` | password, same format |

| Rule | Remote user | Keypad user | Admin |
|---|---|---|---|
| Name | 1–8 printable characters (0x21–0x7E, no space) | 1–8 digits | as remote |
| Password | 4–8 printable characters | 4–8 digits | as remote |
| Empty slot | first name byte is 0x00 or 0xFF | same | never empty (repaired at boot, see 4) |
| Unique | inside its list, and not equal to the admin name | inside its list | — |

Names and passwords are case sensitive and stored as plain text (architecture decision D-13). The first name byte doubles as the "slot used" flag, so adding or removing a user is **one page write** and can never be half done (decision D-6).

---

## 4. First boot and repair (REQ-SEC-09, REQ-HTR-03)

At every boot `ESTORE_voidInit` reads the 208 bytes into RAM, then:

| Check | Action |
|---|---|
| `MAGIC != 0xA5` or `VERSION != 0x01` | **factory defaults** (table below); all 13 pages marked for writing |
| magic valid, but the admin name's first byte is 0x00 / 0xFF or not printable | only the admin record is set back to the default (`USERDB_voidInit`) |
| magic valid, but `HEATER_SET` is not one of 35, 40, … 75 | the heater uses 60 and stores it |
| the chip does not answer | defaults in RAM only, nothing is written, status `ESTORE_FAULT` (the terminal prints a warning after login) |

Factory defaults:

| Item | `#define` | Value |
|---|---|---|
| Magic, version | `ESTORE_MAGIC_VALUE`, `ESTORE_VERSION_VALUE` | 0xA5, 0x01 |
| Heater set temperature | `ESTORE_DEFAULT_HEATER_SET` | 60 |
| Admin user name | `ESTORE_DEFAULT_ADMIN_NAME` | `admin` |
| Admin password | `ESTORE_DEFAULT_ADMIN_PASS` | `1234` |
| Remote users, keypad users | — | all 10 slots empty (0x00) |
| Reserved bytes | — | 0xFF |

Pages are written from the **highest page down to page 0**, so the magic byte is written last: if power fails during the first-boot write, the magic is still missing and the defaults are written again next time.

The same defaults are loaded by the admin command "Factory reset" (`ESTORE_voidLoadDefaults`).

---

## 5. `ESTORE_cfg.h` (the map as code)

```c
/* chip area used : pages 0..12 of block 0 */
#define ESTORE_PAGE_SIZE             16
#define ESTORE_PAGE_COUNT            13
#define ESTORE_IMAGE_SIZE            (ESTORE_PAGE_SIZE * ESTORE_PAGE_COUNT)     /* 208 = 0xD0 */

/* page 0 : header (written on first boot only) */
#define ESTORE_ADDR_MAGIC            0x00
#define ESTORE_ADDR_VERSION          0x01
#define ESTORE_MAGIC_VALUE           0xA5
#define ESTORE_VERSION_VALUE         0x01

/* page 1 : settings */
#define ESTORE_ADDR_HEATER_SET       0x10

/* page 2 : admin record , pages 3..7 : remote users , pages 8..12 : keypad users */
#define ESTORE_ADDR_ADMIN            0x20
#define ESTORE_ADDR_REMOTE_USERS     0x30
#define ESTORE_ADDR_KEYPAD_USERS     0x80

/* account record : one page */
#define USERDB_RECORD_SIZE           16
#define USERDB_NAME_OFFSET           0
#define USERDB_NAME_SIZE             8
#define USERDB_PASS_OFFSET           8
#define USERDB_PASS_SIZE             8
#define USERDB_REMOTE_SLOTS          5
#define USERDB_KEYPAD_SLOTS          5

/* factory defaults */
#define ESTORE_DEFAULT_HEATER_SET    60
#define ESTORE_DEFAULT_ADMIN_NAME    "admin"
#define ESTORE_DEFAULT_ADMIN_PASS    "1234"

/* write-behind */
#define ESTORE_POLL_LIMIT            5       /* 5 x 10 ms : longest wait for one write cycle */
```

---

## 6. How the firmware reads and writes (REQ-EEP-03)

- **Reads never touch the chip.** The 208 bytes are read once at boot (one sequential read, 19 ms, before the scheduler starts) into a RAM image; login checks and status screens read the image.
- **Writes are write-behind.** A write changes the image and sets the dirty bit of its page (13 bits in a `u16`). Every 10 ms `ESTORE_voidUpdate` does one small thing: start one page write (1.7 ms on the bus), or make one ACK poll (0.12 ms). No delay, no waiting loop.
- A byte written with the value it already has changes nothing and costs no write cycle.
- Typical times: saving the set temperature or adding a user = 1 page = about 20 ms in the background; first boot = 13 pages = about 0.4 s in the background (the system is already usable from the RAM image).
- If the chip stops answering (5 polls = 50 ms), ESTORE stops writing, keeps working from RAM and reports `ESTORE_FAULT`.

Who may write what:

| Area | Written through | Allowed for |
|---|---|---|
| Header (page 0) | `ESTORE` itself | first boot and factory reset only |
| Heater set temperature | `HEATER` | panel buttons, admin, users (REQ-HTR-14) |
| Accounts (pages 2–12) | `USERDB`, only while its write gate is open | admin only (REQ-SEC-03, 07) |

The lockdown state is **not** stored: a reset always clears it (REQ-SEC-05).

---

## 7. Proteus notes

- NM24C08 part: A2 = GND (address 0x50), **WP = GND** (it is unconnected in the committed netlist, pin_map C-7), page size 16, `TD_WRITE = 10 ms`.
- A real power cycle for the firmware = the **RESET push button** on the ATmega32: the MCU restarts and the EEPROM model keeps its content. This is how the persistence tests are run.
- Whether the model keeps its content when the whole simulation is stopped and started again depends on the part's settings and is checked in the HAL test; the test plan does not depend on it.
- The I²C debugger shows a write as `S A0 A <addr> A <16 data bytes> P`, then polls `S A0 N P` until `S A0 A P`.
