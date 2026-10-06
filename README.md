# Embedded Systems Diploma

Coursework and graduation project from my embedded systems diploma (C, AVR ATmega32, FreeRTOS).
Toolchain: PlatformIO + Antigravity IDE, simulated in Proteus.

| Folder | Contents |
|---|---|
| `01_C_Programming/` | C assignments, classwork, practice (Codeforwin, HackerRank) and `my_c_lib` (array, string, bit-math helpers) |
| `02_AVR_ATmega32/` | `drivers_reference/`: the current MCAL/HAL/Service driver set. `Nasr99/`: an earlier AVR project with FreeRTOS |
| `03_RTOS/` | FreeRTOS kernel sources and notes |
| `04_Graduation_Project/` | Smart home + electric water heater on one ATmega32 (both course projects merged): `firmware/project_v1` (main PlatformIO project, see its README), `firmware/experiments/`, `simulation/` (Proteus; final system = `project_v1_phase3_final.pdsprj`), `docs/` (course specs) |
| `Course_Material/notes/` | Lecture notes |

## Building the graduation project

```bash
cd 04_Graduation_Project/firmware/project_v1
pio run -e app
```
