"""
numbers.py - every derived number in the guide.

Inputs: constants read from the firmware headers (parsed, not typed in) and the
simavr results (sim/results.json). Output: dict N used by build.py ({{N.key}} in the
chapter files). Labels: "computed" = derived here from F_CPU and the cfg values;
"simavr" = measured in the patched simavr run of app.elf.
"""
import json, os, re

ROOT = os.environ.get('FW_ROOT', os.path.join(os.path.dirname(__file__), '..'))   # firmware/project_v1
SIM = os.environ.get('SIM_RESULTS', os.path.join(os.path.dirname(__file__), 'data', 'results.json'))


def define(rel, name):
    """value of a #define in a firmware header (integer literals and 0b.. only)"""
    txt = open(os.path.join(ROOT, rel), encoding='latin-1').read()
    m = re.search(r'#define\s+' + name + r'\s+([0-9xXa-fA-FbB]+)(UL|U|L)?', txt)
    if not m:
        raise KeyError(name)
    v = m.group(1)
    return int(v, 0) if not v.startswith(('0b', '0B')) else int(v[2:], 2)


def f_cpu():
    txt = open(os.path.join(ROOT, 'platformio.ini')).read()
    return int(re.search(r'board_build\.f_cpu\s*=\s*(\d+)', txt).group(1))


F = f_cpu()
N = {}
N['F_CPU'] = F

# ---------------- Timer2 : 1 ms tick ----------------
pre2 = define('lib/Service/SCHED/SCHED_cfg.h', 'SCHED_TIMER_PRESCALER')
tick_hz = define('lib/Service/SCHED/SCHED_cfg.h', 'SCHED_TICK_HZ')
ocr2 = F // (pre2 * tick_hz) - 1
N['T2_PRE'] = pre2
N['T2_CLK_KHZ'] = F / pre2 / 1000
N['T2_OCR'] = ocr2
N['T2_PERIOD_US'] = (ocr2 + 1) * pre2 / F * 1e6
N['T2_ERR_PPM'] = ((ocr2 + 1) * pre2 / F * tick_hz - 1) * 1e6

# ---------------- Timer0 : dimmer ----------------
N['T0_PRE'] = 8
N['T0_FREQ'] = F / (8 * 256)
N['T0_PERIOD_US'] = 256 * 8 / F * 1e6
dim = []
for p in range(0, 101, 10):
    if p == 0:
        dim.append((p, '—', 'pin low (output disconnected)', 0.0))
    elif p == 100:
        dim.append((p, 255, 'constant high', 100.0))
    else:
        o = p * 255 // 100
        dim.append((p, o, f'{(o + 1) * 0.5:.1f} µs high of 128 µs', (o + 1) / 256 * 100))
N['DIM_TABLE'] = dim

# ---------------- Timer1 : servo + fan ----------------
pre1 = define('lib/MCAL/TIMER1/TIMER1_cfg.h', 'TIMER1_PRESCALER')
f1 = define('lib/MCAL/TIMER1/TIMER1_cfg.h', 'TIMER1_PWM_FREQ_HZ')
top = F // (pre1 * f1) - 1
tpm = F // (pre1 * 1000)
N['T1_PRE'] = pre1
N['T1_TOP'] = top
N['T1_FREQ'] = F / (pre1 * (top + 1))
N['T1_TICK_US'] = pre1 / F * 1e6
N['T1_TICKS_PER_MS'] = tpm
smin = define('lib/HAL/SERVO/SERVO_cfg.h', 'SERVO_MIN_PULSE_US')
smax = define('lib/HAL/SERVO/SERVO_cfg.h', 'SERVO_MAX_PULSE_US')
sang = define('lib/HAL/SERVO/SERVO_cfg.h', 'SERVO_MAX_ANGLE')
servo = []
for a in (0, 45, 90, 135, 180):
    us = smin + (smax - smin) * a // sang
    o = us * tpm // 1000
    servo.append((a, us, o, (o + 1) * pre1 / F * 1e6))
N['SERVO_TABLE'] = servo
fan = []
for p in (0, 10, 50, 90, 100):
    if p == 0:
        fan.append((p, '—', 'pin low'))
    elif p == 100:
        fan.append((p, top, 'constant high'))
    else:
        o = p * (top + 1) // 100
        fan.append((p, o, f'{(o + 1) * pre1 / F * 1e3:.4f} ms of 20 ms'))
N['FAN_TABLE'] = fan

# ---------------- USART ----------------
baud = define('lib/MCAL/USART/USART_cfg.h', 'USART_BAUD_RATE')
ubrr = F // (16 * baud) - 1
real = F / (16 * (ubrr + 1))
N['BAUD'] = baud
N['UBRR'] = ubrr
N['BAUD_REAL'] = real
N['BAUD_ERR_PCT'] = (real / baud - 1) * 100
N['BYTE_MS'] = 10 / real * 1e3
rx = define('lib/Service/TERM/TERM_cfg.h', 'TERM_RX_BUFFER_SIZE')
tx = define('lib/Service/TERM/TERM_cfg.h', 'TERM_TX_BUFFER_SIZE')
N['RX_SIZE'] = rx
N['TX_SIZE'] = tx
N['RX_FILL_MS'] = rx * N['BYTE_MS']
N['TX_DRAIN_MS'] = tx * N['BYTE_MS']
# U2X alternative for comparison
ubrr2 = round(F / (8 * baud)) - 1
N['UBRR_U2X'] = ubrr2
N['BAUD_ERR_U2X_PCT'] = (F / (8 * (ubrr2 + 1)) / baud - 1) * 100

# ---------------- ADC ----------------
N['ADC_CLK_KHZ'] = F / 128 / 1000
N['ADC_CONV_US'] = 13 / (F / 128) * 1e6
N['ADC_FIRST_US'] = 25 / (F / 128) * 1e6
N['ADC_LSB_MV'] = 2560 / 1024

# ---------------- TWI ----------------
scl = define('lib/MCAL/TWI/TWI_cfg.h', 'TWI_SCL_FREQ')
twbr = (F // scl - 16) // 2
scl_real = F / (16 + 2 * twbr)
N['TWBR'] = twbr
N['SCL_KHZ'] = scl_real / 1000
N['SCL_PERIOD_US'] = 1e6 / scl_real
N['TWI_BYTE_US'] = 9e6 / scl_real
loops = F // 1000 // 6
N['TWI_TIMEOUT_LOOPS'] = loops
N['TWI_LOOP_CYC'] = 8               # from the disassembly of TWI_u8SendCommand (in, sbrs, rjmp, sbiw, brne)
N['TWI_TIMEOUT_MS'] = loops * 8 / F * 1e3


def twi_t(nbytes, start=1):
    """computed bus time of one transaction : START (1 SCL period, model) + bytes x 9 SCL periods"""
    return start * N['SCL_PERIOD_US'] + nbytes * N['TWI_BYTE_US']


N['TWI_TX'] = {
    'lcd_byte': twi_t(5), 'lcd_nibble': twi_t(3), 'lamp': twi_t(2), 'seg_digit': twi_t(2),
    'ee_page': twi_t(18), 'ee_poll': twi_t(1), 'ee_boot_read': twi_t(2) + twi_t(1 + 208),
}

# ---------------- boot ----------------
N['LCD_INIT_DELAYS_MS'] = 40 + 5 + 1 + 2
N['LCD_INIT_BUS_MS'] = (4 * twi_t(3) + 3 * twi_t(5)) / 1000
N['LCD_INIT_MS'] = N['LCD_INIT_DELAYS_MS'] + N['LCD_INIT_BUS_MS']

# ---------------- scheduler ----------------
N['OFFSETS'] = {k: define('lib/Service/SCHED/SCHED_cfg.h', 'SCHED_OFFSET_' + k) for k in ('5MS', '10MS', '100MS', '500MS', '1S')}
N['FIRST_DUE'] = {'5MS': 5 + N['OFFSETS']['5MS'], '10MS': 10 + N['OFFSETS']['10MS'], '100MS': 100 + N['OFFSETS']['100MS'],
                  '500MS': 500 + N['OFFSETS']['500MS'], '1S': 1000 + N['OFFSETS']['1S']}

# ---------------- simavr ----------------
try:
    R = json.load(open(SIM))
except FileNotFoundError:
    R = {}
N['SIM'] = R


def fmt(v, nd=2):
    if isinstance(v, float):
        s = f'{v:,.{nd}f}'
        return s.replace(',', ' ')
    if isinstance(v, int):
        return f'{v:,}'.replace(',', ' ')
    return str(v)


if __name__ == '__main__':
    for k, v in N.items():
        if k != 'SIM':
            print(k, v)
