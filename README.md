# Embedded Systems Diploma — Abdelrahman Sherif Medhat

![C](https://img.shields.io/badge/C-bare--metal-blue) ![ATmega32](https://img.shields.io/badge/MCU-ATmega32-informational) ![FreeRTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green) ![PlatformIO](https://img.shields.io/badge/build-PlatformIO-orange) ![License: MIT (own code)](https://img.shields.io/badge/license-MIT%20%28own%20code%29-lightgrey)

Coursework and graduation project from the **AMIT Learning embedded systems diploma** (onsite, 14 Sep 2025 – 8 Apr 2026, instructor **Eng. Eslam Hefny**): C programming, AVR ATmega32 drivers, FreeRTOS, and a bare-metal graduation project.

## Graduation project — ATmega32 Smart Home + Electric Water Heater

One ATmega32 firmware that merges the two course projects into a single smart home: remote control over UART, a keypad/LCD menu, admin and user accounts in EEPROM, lamps, a dimmer, a servo door, temperature-controlled AC, and a complete electric water heater with its own panel — on a layered MCAL / HAL / SERVICE / APP architecture with a 1 ms scheduler and no blocking delays.

**→ [Project page and documentation](04_Graduation_Project/)** · **[Presentation (PDF)](04_Graduation_Project/presentation/Smart_Home_Water_Heater_Presentation.pdf)** · **[Download firmware + Proteus project (latest release)](https://github.com/abdelrahmansherifmedhat1134/embedded-diploma/releases/latest)**

## Repository layout

| Folder | Contents |
|---|---|
| [`04_Graduation_Project/`](04_Graduation_Project/) | Graduation project: firmware (PlatformIO), Proteus simulation, design documents, test evidence |
| [`01_C_Programming/`](01_C_Programming/) | C assignments, classwork, practice (Codeforwin, HackerRank) and `my_c_lib` (array, string, bit-math helpers) |
| [`02_AVR_ATmega32/`](02_AVR_ATmega32/) | `drivers_reference/`: the course MCAL / HAL / Service driver set. `Nasr99/`: an earlier AVR project with FreeRTOS |
| [`03_RTOS/`](03_RTOS/) | FreeRTOS kernel sources and notes |
| [`Course_Material/`](Course_Material/) | Lecture notes |

## Build the graduation project

```bash
cd 04_Graduation_Project/firmware/project_v1
pio run -e app          # firmware: .pio/build/app/firmware.hex
```

Toolchain: PlatformIO (bare-metal avr-gcc), Antigravity IDE, Proteus 8 for simulation.

> **Demo login:** on first boot (blank EEPROM) the firmware creates the admin account `admin` / `1234`. This is a simulation default, documented in [`uart_protocol.md`](04_Graduation_Project/firmware/project_v1/docs/uart_protocol.md); change it with admin command 15.

## License

My own code and documentation are under the [MIT License](LICENSE). FreeRTOS (GPLv2 with the FreeRTOS exception) and the AMIT course material (drivers by Eng. Eslam Hefny, course briefs) keep their own terms; see [NOTICE](NOTICE) for the full list.
