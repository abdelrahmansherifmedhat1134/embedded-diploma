# simavr timing cross-check

Cycle-accurate runs of the unchanged v1 firmware, used for the "measured in simavr" figures in chapter 7 of the Technical Guide. Linux only.

| File | Purpose |
|---|---|
| `harness.c` | simavr host: PCF8574 ×4, 24C08 (optional write-cycle delay), keypad, buttons, LM35 inputs, UART; logs slots, ISRs, I²C transactions, pins, UART bytes, events |
| `syms.h` / `mksyms.sh` | ELF addresses the harness hooks (ISRs, `SCHED_u8IsTaskDue`, SCHED globals); regenerate after any firmware change |
| `main.txt`, `stress.txt` | full-system scenario (30 s) and back-to-back stress scenario (16 s) |
| `fb0.txt`, `fb5.txt`, `warm.txt` | boot only: first boot without / with a 5 ms write cycle, warm boot |
| `analyze.py` | reads the run folders and writes `results.json` (copy it to `../data/`) |
| `simavr_v1.6_twi_adc.patch` | fixes to simavr v1.6: TWINT write-1-clear, SLA+W ACK status 0x18, ADIF write-1-clear, TWI bit time from TWBR |

## Steps

```sh
# 1. simavr v1.6 with the patch
git clone --depth 1 --branch v1.6 https://github.com/buserror/simavr ext/simavr
(cd ext/simavr && git apply ../../simavr_v1.6_twi_adc.patch && make libsimavr RELEASE=1)
S=ext/simavr/simavr

# 2. firmware ELF (same flags as PlatformIO; run in firmware/project_v1)
avr-gcc -mmcu=atmega32 -DF_CPU=16000000L -Os -Wall -std=gnu11 \
  -ffunction-sections -fdata-sections -flto \
  $(find lib src -type d | sed 's/^/-I/') $(find lib src -name '*.c') \
  -Wl,--gc-sections -o app.elf

# 3. harness
sh mksyms.sh app.elf > syms.h
gcc -O2 -o harness harness.c -I$S/sim $S/obj-x86_64-linux-gnu/libsimavr.a -lelf -lm

# 4. runs (each writes a folder of CSVs), then the summary
#    every run writes the final EEPROM to ee_out.bin; warm.txt ("eeprom keep") loads ee.bin
for r in main stress fb0 fb5; do mkdir -p $r; ./harness app.elf $r.txt $r; done
cp ee_out.bin ee.bin; mkdir -p warm; ./harness app.elf warm.txt warm
python3 analyze.py        # run in the folder that holds the run folders
```

Limits: the I²C model has ideal edges and no clock stretching, and a START is modelled as one SCL period. The parts answer instantly apart from the 24C08 write cycle. See §7.1 of the guide.
