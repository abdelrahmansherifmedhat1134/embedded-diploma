"""figs_mpl.py - waveforms, pin map, budgets, sequence diagrams and timing charts (matplotlib -> SVG).
Numbers come from numbers.py (computed) and data/results.json (simavr)."""
import os, json, csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Rectangle
import gnum as NUM

N = NUM.N
OUT = os.environ.get('FIG_OUT', os.path.join(os.path.dirname(__file__), 'out', 'images', 'guide'))
SIMDIR = os.environ.get('SIM_DIR', os.path.join(os.path.dirname(__file__), 'data'))
os.makedirs(OUT, exist_ok=True)

APP, SVC, HAL, MCAL = '#2B6CB0', '#2F855A', '#C05621', '#6B46C1'
APP_L, SVC_L, HAL_L, MCAL_L = '#E8F1FB', '#E9F6EF', '#FFF5E6', '#F3EEFB'
INK, MUTED, GRID, RED, SPARE = '#1F2933', '#52606D', '#C9D1D9', '#C53030', '#9AA5B1'

plt.rcParams.update({
    'font.family': 'Inter', 'font.size': 8.5, 'axes.edgecolor': MUTED, 'axes.labelcolor': INK,
    'xtick.color': MUTED, 'ytick.color': MUTED, 'axes.titlesize': 9.5, 'axes.titleweight': 'semibold',
    'axes.titlecolor': INK, 'axes.spines.top': False, 'axes.spines.right': False, 'svg.fonttype': 'none',
    'axes.grid': False, 'grid.color': GRID, 'grid.linewidth': 0.5, 'legend.frameon': False, 'figure.dpi': 100,
})


def save(fig, name):
    fig.savefig(os.path.join(OUT, name + '.svg'), bbox_inches='tight', pad_inches=0.04)
    plt.close(fig)


def wave(ax, xs, ys, y0, h=0.7, color=INK, lw=1.3):
    """digital waveform from transition list"""
    X, Y = [], []
    for i, (x, y) in enumerate(zip(xs, ys)):
        if i:
            X.append(x); Y.append(Y[-1])
        X.append(x); Y.append(y0 + h * y)
    ax.plot(X, Y, color=color, lw=lw, solid_joinstyle='miter')


def box(ax, x, y, w, h, text, fc, ec, fs=8, tc=INK, lw=1.0, r=0.02, ha='center', weight='normal'):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle=f'round,pad=0,rounding_size={r}', fc=fc, ec=ec, lw=lw))
    ax.text(x + (w / 2 if ha == 'center' else 0.01), y + h / 2, text, ha=ha, va='center', fontsize=fs, color=tc, weight=weight)


# ---------------------------------------------------------------- chapter 1
PINS = {  # DIP-40 : pin -> (name, function, layer colour)
    1: ('PB0', 'KPAD col 1', HAL), 2: ('PB1', 'KPAD col 2', HAL), 3: ('PB2', 'KPAD col 3', HAL), 4: ('PB3', 'OC0 dimmer PWM', MCAL),
    5: ('PB4', 'KPAD col 4', HAL), 6: ('PB5', 'Down button', HAL), 7: ('PB6', 'heating element', HAL), 8: ('PB7', 'cooling element', HAL),
    9: ('RESET', 'reset button', INK), 10: ('VCC', '+5 V', INK), 11: ('GND', '0 V', INK), 12: ('XTAL2', '16 MHz', INK), 13: ('XTAL1', '16 MHz', INK),
    14: ('PD0', 'RXD', MCAL), 15: ('PD1', 'TXD', MCAL), 16: ('PD2', 'INT0 PIR (reserved)', SPARE), 17: ('PD3', 'buzzer', HAL),
    18: ('PD4', 'OC1B fan PWM', MCAL), 19: ('PD5', 'OC1A door servo', MCAL), 20: ('PD6', 'ON/OFF button', HAL),
    21: ('PD7', 'Up button', HAL), 22: ('PC0', 'SCL', SVC), 23: ('PC1', 'SDA', SVC), 24: ('PC2', 'spare (TCK)', SPARE),
    25: ('PC3', 'spare (TMS)', SPARE), 26: ('PC4', 'spare (TDO)', SPARE), 27: ('PC5', 'spare (TDI)', SPARE), 28: ('PC6', 'spare', SPARE),
    29: ('PC7', 'spare', SPARE), 30: ('AVCC', '+5 V', INK), 31: ('GND', '0 V', INK), 32: ('AREF', 'open (2.56 V int.)', INK),
    33: ('PA7', 'KPAD row D', HAL), 34: ('PA6', 'KPAD row C', HAL), 35: ('PA5', 'KPAD row B', HAL), 36: ('PA4', 'KPAD row A', HAL),
    37: ('PA3', 'heater LED', HAL), 38: ('PA2', 'ADC2 LDR (reserved)', SPARE), 39: ('PA1', 'ADC1 LM35 water', MCAL), 40: ('PA0', 'ADC0 LM35 ambient', MCAL),
}


def pinmap():
    fig, ax = plt.subplots(figsize=(7.2, 6.2))
    ax.set_xlim(0, 10); ax.set_ylim(-0.6, 20.8); ax.axis('off')
    ax.add_patch(FancyBboxPatch((3.7, -0.2), 2.6, 20.6, boxstyle='round,pad=0,rounding_size=0.15', fc='#F5F7FA', ec=INK, lw=1.2))
    ax.add_patch(matplotlib.patches.Wedge((5, 20.4), 0.35, 180, 360, fc='white', ec=INK, lw=1))
    ax.text(5, 10.1, 'ATmega32  ·  DIP-40', ha='center', va='center', fontsize=10, weight='semibold', color=MUTED, rotation=90)
    for p in range(1, 41):
        name, fn, col = PINS[p]
        if p <= 20:
            y = 20 - p + 0.0; xp = 3.7
            ax.plot([3.35, 3.7], [y, y], color=INK, lw=1)
            ax.text(3.82, y, f'{p}', fontsize=6.5, va='center', color=MUTED)
            ax.text(4.12, y, name, fontsize=7.5, va='center', color=INK, weight='semibold')
            ax.add_patch(FancyBboxPatch((0.05, y - 0.38), 3.25, 0.76, boxstyle='round,pad=0,rounding_size=0.12', fc='white', ec=col, lw=1.1))
            ax.text(3.2, y, fn, ha='right', va='center', fontsize=7.4, color=col if col != INK else MUTED)
        else:
            y = p - 21
            ax.plot([6.3, 6.65], [y, y], color=INK, lw=1)
            ax.text(6.18, y, f'{p}', fontsize=6.5, va='center', ha='right', color=MUTED)
            ax.text(5.88, y, name, fontsize=7.5, va='center', ha='right', color=INK, weight='semibold')
            ax.add_patch(FancyBboxPatch((6.7, y - 0.38), 3.25, 0.76, boxstyle='round,pad=0,rounding_size=0.12', fc='white', ec=col, lw=1.1))
            ax.text(6.8, y, fn, ha='left', va='center', fontsize=7.4, color=col if col != INK else MUTED)
    handles = [Rectangle((0, 0), 1, 1, fc='white', ec=c, lw=1.4) for c in (MCAL, HAL, SVC, SPARE, INK)]
    ax.legend(handles, ['peripheral pin (timer, ADC, UART)', 'GPIO used by a HAL driver', 'I²C bus', 'spare / reserved', 'power, clock, reset'],
              loc='lower center', bbox_to_anchor=(0.5, -0.07), ncol=3, fontsize=7)
    save(fig, 'pinmap')


def pinbudget():
    """32 I/O pins as a port x bit grid, coloured by use (from PINS / pin_map.md)"""
    port_pins = {'A': [40, 39, 38, 37, 36, 35, 34, 33], 'B': [1, 2, 3, 4, 5, 6, 7, 8], 'C': [22, 23, 24, 25, 26, 27, 28, 29], 'D': [14, 15, 16, 17, 18, 19, 20, 21]}
    fig, ax = plt.subplots(figsize=(7.2, 2.2))
    for r, port in enumerate('ABCD'):
        y = 3 - r
        ax.text(-0.15, y + 0.4, f'PORT{port}', ha='right', va='center', fontsize=7.5, weight='semibold', color=INK)
        for bit in range(8):
            name, fn, col = PINS[port_pins[port][bit]]
            ax.add_patch(FancyBboxPatch((bit + 0.04, y + 0.06), 0.92, 0.74, boxstyle='round,pad=0,rounding_size=0.06', fc='white', ec=col, lw=1.2))
            ax.text(bit + 0.5, y + 0.6, name, ha='center', va='center', fontsize=6.4, weight='semibold', color=INK)
            SHORT = {'ADC0 LM35 ambient': 'LM35 ambient', 'ADC1 LM35 water': 'LM35 water', 'ADC2 LDR (reserved)': 'LDR (resv.)', 'OC0 dimmer PWM': 'OC0 dimmer', 'INT0 PIR (reserved)': 'PIR (resv.)', 'OC1B fan PWM': 'OC1B fan', 'OC1A door servo': 'OC1A servo', 'ON/OFF button': 'ON/OFF btn', 'Down button': 'Down btn', 'Up button': 'Up btn', 'heating element': 'heating', 'cooling element': 'cooling'}
            ax.text(bit + 0.5, y + 0.3, SHORT.get(fn, fn.replace('KPAD ', 'kp ')), ha='center', va='center', fontsize=5.4, color=col if col != INK else MUTED)
    for bit in range(8):
        ax.text(bit + 0.5, 4.0, f'bit {bit}', ha='center', fontsize=6, color=MUTED)
    ax.set_xlim(-1.1, 8.1); ax.set_ylim(-0.05, 4.25); ax.axis('off')
    used = sum(1 for p in sum(port_pins.values(), []) if PINS[p][2] not in (SPARE,))
    ax.text(8.1, -0.02, f'{used} pins in use · 2 reserved (LDR, PIR) · 6 spare (PC2–PC7, freed by D-20)', ha='right', va='top', fontsize=6.6, color=MUTED)
    save(fig, 'pinbudget')


def memory():
    R = N['SIM']
    fig, axs = plt.subplots(2, 1, figsize=(7.2, 2.1), gridspec_kw={'hspace': 1.2})
    flash = 16344; ram_static = 733
    for ax, total, parts, title in (
        (axs[0], 32768, [('application code + text (pio run)', flash, APP), ('free', 32768 - flash, '#F5F7FA')], 'Flash 32 768 B'),
        (axs[1], 2048, [('.data', 42, HAL), ('.bss (static variables)', 691, SVC), ('stack + free', 2048 - 733, '#F5F7FA')], 'SRAM 2 048 B (static RAM = .data + .bss = 733 B, 35.8 %)')):
        x = 0
        for lab, n, c in parts:
            ax.barh(0, n, left=x, color=c, edgecolor=MUTED, lw=0.6, height=0.6)
            if n / total > 0.1:
                ax.text(x + n / 2, 0, f'{lab}\n{n} B ({n / total * 100:.1f} %)', ha='center', va='center', fontsize=6.6,
                        color='white' if c not in ('#F5F7FA',) else INK)
            else:
                ax.annotate(f'{lab} B', (x + n / 2, 0.3), (x + n / 2 + total * 0.02, 0.75), fontsize=6.3, color=INK,
                            arrowprops=dict(arrowstyle='-', lw=0.5, color=MUTED), annotation_clip=False)
            x += n
        ax.set_xlim(0, total); ax.set_yticks([]); ax.set_title(title, loc='left', fontsize=8.5)
        ax.spines['left'].set_visible(False)
    save(fig, 'memory')


def boot_timeline():
    """boot phases measured in simavr (data/warm : I2C trace + first tick ISR)"""
    tw = [(int(x[0]) / 16000, int(x[1]) / 16000, int(x[2]), int(x[3])) for x in csv.reader(open(os.path.join(SIMDIR, 'warm', 'twi.csv')))]
    tick1 = min(int(x[1]) for x in csv.reader(open(os.path.join(SIMDIR, 'warm', 'isr.csv'))) if x[0] == '4') / 16000
    first_slot = int(open(os.path.join(SIMDIR, 'warm', 'events.csv')).read().split(',')[0]) / 16000
    lcd_first = tw[0][0]; lcd_last = [t for t in tw if t[2] == 0x4e][6][1]
    rd0 = [t for t in tw if t[2] == 0xa0][0][0]; rd1 = [t for t in tw if t[2] == 0xa1][0][1]
    light = [t for t in tw if t[2] == 0x40 and t[0] > rd1][:5]
    sched = tick1 - 1.0
    phases = [('outputs safe, TERM', 0, lcd_first - 40, HAL),
              ('CLCD init: 40 + 5 + 1 ms delays, 4 nibbles,\n3 commands, 2 ms after Clear', lcd_first - 40, lcd_last + 2.0, HAL),
              ('LAMP, SEVEN_SEG, KPAD, BUTTON, LM35', lcd_last + 2.0, rd0, HAL),
              ('ESTORE: 208-byte image read', rd0, rd1, SVC),
              ('USERDB, EVQ, ALARM, SEC, LIGHT\n(5 lamp writes), DOOR … UIREM', rd1, sched, APP),
              ('SCHED start → first 5 ms slot', sched, first_slot, MUTED)]
    fig, ax = plt.subplots(figsize=(7.2, 1.95))
    for i, (lab, a, b, c) in enumerate(phases):
        ax.barh(0, b - a, left=a, color=c, alpha=0.85 if c != MUTED else 0.35, edgecolor='white', height=0.5)
        ypos, dx = [(0.8, 2), (-0.85, 0), (0.8, -12), (-0.85, -6), (0.8, 4), (-0.85, 6)][i]
        ax.annotate(f'{lab}\n{a:.1f}–{b:.1f} ms', ((a + b) / 2, 0.25 if ypos > 0 else -0.25), ((a + b) / 2 + dx, ypos), ha='center', va='center', fontsize=6.2,
                    color=INK, arrowprops=dict(arrowstyle='-', color=MUTED, lw=0.6))
    ax.set_xlim(-2, 84); ax.set_ylim(-1.4, 1.4); ax.set_yticks([]); ax.set_xlabel('time after reset [ms] (simavr, EEPROM with valid data)')
    ax.spines['left'].set_visible(False)
    save(fig, 'boot_timeline')


def t2_ctc():
    fig, (a1, a2) = plt.subplots(2, 1, figsize=(7.2, 2.2), sharex=True, gridspec_kw={'height_ratios': [3, 1], 'hspace': 0.15})
    t = []; y = []
    for k in range(4):
        t += [k, k + 0.9999]; y += [0, 249]
    a1.plot(t, y, color=MCAL, lw=1.4)
    a1.axhline(249, color=MUTED, lw=0.7, ls='--'); a1.text(3.98, 255, 'OCR2 = 249', ha='right', fontsize=7, color=MUTED)
    a1.set_ylabel('TCNT2'); a1.set_ylim(-10, 290); a1.set_yticks([0, 125, 249])
    a1.set_title('Timer2 CTC: 250 kHz timer clock, 250 counts = 1.000 ms, compare match → TIMER2_COMP ISR', loc='left')
    for k in range(1, 4):
        a2.annotate('', (k, 0.1), (k, 0.9), arrowprops=dict(arrowstyle='->', color=APP, lw=1.2))
        a2.text(k + 0.03, 0.55, 'ISR: tick++, 5 down-counters', fontsize=6.5, color=APP)
    a2.set_ylim(0, 1); a2.set_yticks([]); a2.spines['left'].set_visible(False)
    a2.set_xlabel('time [ms]'); a2.set_xlim(-0.05, 4)
    save(fig, 't2_ctc')


def t0_pwm():
    fig, axs = plt.subplots(3, 1, figsize=(7.2, 2.4), sharex=True, gridspec_kw={'hspace': 0.35})
    for ax, (p, o) in zip(axs, ((10, 25), (50, 127), (90, 229))):
        xs, ys = [], []
        for k in range(3):
            xs += [k * 256, k * 256 + o + 1]; ys += [1, 0]
        xs.append(768); ys.append(0)
        wave(ax, xs, ys, 0, 1, MCAL)
        ax.set_yticks([]); ax.spines['left'].set_visible(False)
        ax.text(-12, 0.5, f'{p} %\nOCR0 = {o}', ha='right', va='center', fontsize=7)
        ax.set_ylim(-0.2, 1.3)
    axs[-1].set_xticks([0, 256, 512, 768]); axs[-1].set_xticklabels(['0', '128 µs', '256 µs', '384 µs'])
    axs[0].set_title(f'Timer0 fast PWM on OC0 (PB3): F_CPU/8 = 2 MHz, 256 counts → {N["T0_FREQ"]:.1f} Hz, high for OCR0 + 1 counts', loc='left')
    save(fig, 't0_pwm')


def t1_pwm():
    fig, axs = plt.subplots(3, 1, figsize=(7.2, 2.5), sharex=True, gridspec_kw={'hspace': 0.4})
    T = 20
    t = []; y = []
    for k in range(3):
        t += [k * T, k * T + T * 0.9999]; y += [0, 39999]
    axs[0].plot(t, y, color=MCAL, lw=1.3); axs[0].axhline(3000, color=APP, lw=0.7, ls='--')
    axs[0].text(59.5, 4500, 'OCR1A = 3000 (90°)', ha='right', fontsize=6.5, color=APP)
    axs[0].set_yticks([0, 39999]); axs[0].set_ylabel('TCNT1', fontsize=7); axs[0].set_ylim(-1500, 44000)
    wave(axs[1], [0, 1.5, 20, 21.5, 40, 41.5, 60], [1, 0, 1, 0, 1, 0, 0], 0, 1, APP)
    axs[1].text(-1, 0.5, 'OC1A servo\n1.5 ms', ha='right', va='center', fontsize=7)
    wave(axs[2], [0, 60], [1, 1], 0, 1, HAL)
    axs[2].text(-1, 0.5, 'OC1B fan\n100 % (= TOP)', ha='right', va='center', fontsize=7)
    for a in axs[1:]:
        a.set_yticks([]); a.spines['left'].set_visible(False); a.set_ylim(-0.2, 1.3)
    axs[2].set_xlabel('time [ms]')
    axs[0].set_title('Timer1 mode 14 (fast PWM, TOP = ICR1 = 39 999): 0.5 µs per count, 20.000 ms period', loc='left')
    save(fig, 't1_pwm')


def uart_frame():
    fig, ax = plt.subplots(figsize=(7.2, 1.35))
    bits = [1, 0] + [(0x41 >> i) & 1 for i in range(8)] + [1, 1]
    bt = 1e6 / N['BAUD_REAL']
    xs = [i * bt for i in range(len(bits))]
    wave(ax, xs + [len(bits) * bt], bits + [1], 0, 1, MCAL)
    labels = ['idle', 'start'] + [f'D{i}' for i in range(8)] + ['stop', 'idle']
    for i, l in enumerate(labels):
        ax.text(xs[i] + bt / 2, 1.25, l, ha='center', fontsize=6.5, color=MUTED)
        ax.axvline(xs[i], color=GRID, lw=0.5, zorder=0)
    ax.set_yticks([]); ax.spines['left'].set_visible(False); ax.set_ylim(-0.25, 1.5)
    ax.set_xticks([bt, 11 * bt]); ax.set_xticklabels(['0', f'{10 * bt / 1000:.3f} ms'])
    ax.set_title(f"8N1 frame of 'A' (0x41), LSB first: bit = {bt:.1f} µs at {N['BAUD_REAL']:.1f} baud, 10 bits per byte", loc='left')
    save(fig, 'uart_frame')


def twi_txn():
    fig, ax = plt.subplots(figsize=(7.2, 1.6))
    seq = [('S', 1, MUTED), ('SLA+W\n0x4E', 9, HAL), ('byte 1\nE=1', 9, APP), ('byte 2\nE=0', 9, APP), ('byte 3\nE=1', 9, APP), ('byte 4\nE=0', 9, APP), ('P', 0.6, MUTED)]
    x = 0
    for lab, w, c in seq:
        ax.add_patch(Rectangle((x, 0), w, 1, fc='white', ec=c, lw=1.2))
        ax.text(x + w / 2, 0.5, lab, ha='center', va='center', fontsize=7, color=c)
        if w == 9:
            ax.text(x + 8.5, 1.1, 'A', fontsize=6, color=MUTED, ha='center')
        x += w
    ax.set_xlim(-1, 48); ax.set_ylim(-0.9, 1.5); ax.axis('off')
    ax.text(0, -0.45, f'one LCD byte (CLCD, I²C mode) = START + 5 × 9 SCL periods\n= {N["TWI_TX"]["lcd_byte"]:.0f} µs computed · {N["SIM"]["twi_types"]["LCD (0x27) 4 B"]["min_us"] + 10:.0f} µs simavr · ≈ 480 µs measured in Proteus (hal_summary.md)', fontsize=7, color=INK, va='center')
    ax.set_title('I²C transaction anatomy at 100 kHz (10 µs per SCL period, 90 µs per byte incl. ACK)', loc='left')
    save(fig, 'twi_txn')


def adc_timing():
    fig, ax = plt.subplots(figsize=(7.2, 1.3))
    ax.add_patch(Rectangle((0, 0.2), 1.5, 0.6, fc=MCAL_L, ec=MCAL)); ax.text(0.75, 1.05, 'sample & hold (1.5 clk)', ha='center', va='center', fontsize=6.3, color=MCAL)
    ax.add_patch(Rectangle((1.5, 0.2), 11.5, 0.6, fc='white', ec=MCAL)); ax.text(7.25, 0.5, 'successive approximation, 10 bits', ha='center', va='center', fontsize=7)
    for i in range(14):
        ax.plot([i, i], [0.85, 0.95], color=MUTED, lw=0.6)
    ax.annotate('ADSC = 1', (0, 0.2), (0, -0.25), ha='center', fontsize=6.5, arrowprops=dict(arrowstyle='->', lw=0.6))
    ax.annotate('ADIF = 1, ADC = result', (13, 0.2), (13, -0.25), ha='center', fontsize=6.5, arrowprops=dict(arrowstyle='->', lw=0.6))
    ax.set_xlim(-1.5, 15); ax.set_ylim(-0.45, 1.25); ax.axis('off')
    ax.set_title(f'ADC: 16 MHz / 128 = {N["ADC_CLK_KHZ"]:.0f} kHz; 13 ADC clocks = {N["ADC_CONV_US"]:.0f} µs (first: 25 = {N["ADC_FIRST_US"]:.0f} µs)', loc='left')
    save(fig, 'adc_timing')


# ---------------------------------------------------------------- chapter 3 (HAL)
def pcf_lcd():
    fig, ax = plt.subplots(figsize=(7.2, 1.7))
    bits = ['P7\nD7', 'P6\nD6', 'P5\nD5', 'P4\nD4', 'P3\nBL', 'P2\nE', 'P1\nRW', 'P0\nRS']
    cols = [APP] * 4 + [MUTED, HAL, MUTED, HAL]
    for i, (b, c) in enumerate(zip(bits, cols)):
        ax.add_patch(Rectangle((i, 0), 1, 1, fc='white', ec=c, lw=1.2)); ax.text(i + 0.5, 0.5, b, ha='center', va='center', fontsize=7.5, color=c)
    ax.text(8.3, 0.5, "'A' = 0x41, RS = 1:\n  0x4D (hi nibble 4, E=1) → 0x49 (E=0)\n  0x1D (lo nibble 1, E=1) → 0x19 (E=0)\nsent as ONE transaction: S 4E 4D 49 1D 19 P", va='center', fontsize=7, family='DejaVu Sans Mono')
    ax.set_xlim(-0.2, 15.5); ax.set_ylim(-0.2, 1.2); ax.axis('off')
    ax.set_title('PCF8574 0x27 port bits → LCD pins (CLCD_cfg.h); BL is always written 1, RW always 0', loc='left')
    save(fig, 'pcf_lcd')


def lcd_buf():
    fig, ax = plt.subplots(figsize=(7.2, 1.9))
    want = 'Lamps:1-3-- D70%AC:ON   Room:29C'
    shown = 'Lamps:1---- D70%AC:OFF  Room:29C'
    for r, (lab, s) in enumerate((('wanted', want), ('shown', shown))):
        y = 1.3 - r * 0.9
        ax.text(-0.3, y + 0.3, lab, ha='right', va='center', fontsize=7.5, color=MUTED)
        for i, ch in enumerate(s):
            diff = want[i] != shown[i]
            ax.add_patch(Rectangle((i, y), 1, 0.6, fc='#FDECEC' if diff else 'white', ec=RED if diff else GRID, lw=0.8))
            ax.text(i + 0.5, y + 0.3, ch, ha='center', va='center', fontsize=7, family='DejaVu Sans Mono')
    ax.plot([16, 16], [0.3, 2.0], color=MUTED, lw=0.8)
    ax.text(15.8, 2.02, 'row 1', ha='right', va='bottom', fontsize=6.5, color=MUTED); ax.text(16.2, 2.02, 'row 2', ha='left', va='bottom', fontsize=6.5, color=MUTED)
    ax.text(0, 0.1, va='top', s= 'each LCD_BUF_voidUpdate (5 ms) finds the next differing cell (round robin) and sends ONE thing: a set-cursor command if the cursor\n'
                      'is elsewhere, else the character. Here: cursor→8, "3", cursor→20, "N", " " … 3 differing cells + 2 cursor moves = 5 calls = 25 ms.',
            fontsize=6.8, color=INK)
    ax.set_xlim(-2.5, 32.5); ax.set_ylim(-0.95, 2.3); ax.axis('off')
    save(fig, 'lcd_buf')


def keypad():
    fig, ax = plt.subplots(figsize=(6.2, 2.6))
    K = [['7', '8', '9', '/'], ['4', '5', '6', '*'], ['1', '2', '3', '-'], ['C', '0', '=', '+']]
    rows = ['PA4', 'PA5', 'PA6', 'PA7']; cols = ['PB0', 'PB1', 'PB2', 'PB4']
    for r in range(4):
        ax.plot([0.5, 4.6], [3 - r, 3 - r], color=HAL, lw=1)
        ax.text(0.4, 3 - r, f'row {"ABCD"[r]} · {rows[r]}\n(input, pull-up)', ha='right', va='center', fontsize=6.5, color=HAL)
    for c in range(4):
        ax.plot([1 + c, 1 + c], [-0.5, 3.4], color=MCAL, lw=1)
        ax.text(1 + c, 3.55, f'col {c + 1}\n{cols[c]}', ha='center', fontsize=6.5, color=MCAL)
    for r in range(4):
        for c in range(4):
            ax.add_patch(FancyBboxPatch((0.72 + c, 2.72 - r), 0.56, 0.56, boxstyle='round,pad=0,rounding_size=0.08', fc='white', ec=INK, lw=0.8))
            ax.text(1 + c, 3 - r, K[r][c], ha='center', va='center', fontsize=8, weight='semibold')
    ax.text(5.0, 1.5, 'scan (KPAD_voidUpdate, every 10 ms):\nfor each column: drive LOW, wait 5 µs,\nread the 4 rows; a LOW row = key at\n(row, column); drive the column HIGH.\nAccepted after 2 equal scans (20 ms);\nreported once (KPAD_u8GetKey).', fontsize=6.8, va='center')
    ax.set_xlim(-1.4, 9.3); ax.set_ylim(-0.7, 4.1); ax.axis('off')
    save(fig, 'keypad')


def debounce_timing():
    fig, ax = plt.subplots(figsize=(7.2, 1.9))
    # raw with bounce
    raw_x = [0, 12, 13, 14.5, 15, 16, 75, 76, 77, 140]; raw_y = [1, 0, 1, 0, 1, 0, 1, 0, 1, 1]
    wave(ax, raw_x, raw_y, 2.2, 0.6, MUTED)
    ax.text(-3, 2.5, 'raw pin', ha='right', va='center', fontsize=7)
    samples = list(range(2, 140, 10))
    for s in samples:
        ax.plot([s, s], [0, 3], color=GRID, lw=0.5, zorder=0)
    ax.text(2, 3.05, '10 ms samples', fontsize=6, color=MUTED)
    wave(ax, [0, 42, 102, 140], [1, 0, 1, 1], 1.1, 0.6, HAL)
    ax.text(-3, 1.4, 'debounced', ha='right', va='center', fontsize=7)
    ax.annotate('PressEvent\n(3rd low sample)', (42, 1.1), (42, 0.35), ha='center', fontsize=6.5, arrowprops=dict(arrowstyle='->', lw=0.6))
    ax.annotate('ReleaseEvent → heater ON/OFF acts here', (102, 1.7), (102, 0.35), ha='center', fontsize=6.5, arrowprops=dict(arrowstyle='->', lw=0.6))
    ax.set_xlim(-12, 145); ax.set_ylim(0, 3.3); ax.set_yticks([]); ax.spines['left'].set_visible(False); ax.set_xlabel('time [ms]')
    ax.set_title('BUTTON: 3 equal samples at 10 ms → a change is accepted 20–30 ms after the contact settles', loc='left')
    save(fig, 'debounce')


def sevenseg():
    fig, ax = plt.subplots(figsize=(7.2, 1.8))
    segs = {'a': ((0.2, 2.0), (0.8, 2.0)), 'b': ((0.85, 1.95), (0.85, 1.05)), 'c': ((0.85, 0.95), (0.85, 0.05)), 'd': ((0.2, 0.0), (0.8, 0.0)),
            'e': ((0.15, 0.95), (0.15, 0.05)), 'f': ((0.15, 1.95), (0.15, 1.05)), 'g': ((0.2, 1.0), (0.8, 1.0))}
    table = [0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90]
    for d in range(10):
        ox = d * 1.45
        for i, s in enumerate('abcdefg'):
            on = not ((table[d] >> i) & 1)
            (x0, y0), (x1, y1) = segs[s]
            ax.plot([ox + x0, ox + x1], [y0, y1], color=RED if on else '#EEE', lw=3.2, solid_capstyle='round')
        ax.text(ox + 0.5, -0.45, f'0x{table[d]:02X}', ha='center', fontsize=6.8, family='DejaVu Sans Mono')
    ax.text(-0.2, -0.95, 'SEVEN_SEG_TABLE: bit 0 = a … bit 6 = g, bit 7 = dp; a 0 bit lights the segment (common anode, PCF8574 sinks). 0xFF = blank.',
            fontsize=6.8, color=INK)
    ax.set_xlim(-0.4, 14.6); ax.set_ylim(-1.2, 2.3); ax.axis('off')
    save(fig, 'sevenseg')


def ee_poll():
    fig, ax = plt.subplots(figsize=(7.2, 1.7))
    def blk(x, w, y, h, fc, ec, lab, above):
        ax.add_patch(Rectangle((x, y), w, h, fc=fc, ec=ec))
        ax.text(x + w / 2, y + h + 0.12 if above else y - 0.12, lab, ha='center', va='bottom' if above else 'top', fontsize=6.4, color=ec)
    blk(0, 1.63, 1.0, 0.45, SVC_L, SVC, 'page write\n1.63 ms (18 bytes)', True)
    ax.add_patch(Rectangle((1.63, 0.35), 5.0, 0.35, fc='white', ec=MUTED, hatch='////'))
    ax.text(4.1, 0.2, 'internal write cycle ≤ 5 ms: chip NACKs its address', ha='center', va='top', fontsize=6.4, color=MUTED)
    blk(10, 0.12, 1.0, 0.45, SVC, SVC, 'ACK poll 0.1 ms\n→ ACK → IDLE', True)
    blk(20, 1.63, 1.0, 0.45, SVC_L, SVC, 'next dirty page', True)
    for t in (0, 10, 20, 30):
        ax.axvline(t, color=GRID, lw=0.6, zorder=0); ax.text(t, -0.45, f'{t} ms\n10 ms slot', ha='center', fontsize=6, color=MUTED, va='top')
    ax.set_xlim(-1, 31); ax.set_ylim(-1.15, 2.2); ax.axis('off')
    ax.set_title('ESTORE write-behind on the bus: one step per 10 ms slot, never a wait', loc='left')
    save(fig, 'ee_poll')


def lm35():
    fig, ax = plt.subplots(figsize=(6.4, 2.3))
    T = list(range(0, 101, 5))
    ax.plot(T, [t * 4 for t in T], color=MCAL, lw=1.5)
    for t, c in ((21, APP), (28, APP), (35, HAL), (75, HAL)):
        ax.plot([t], [t * 4], 'o', ms=3, color=c); ax.text(t + 2, t * 4 - 16, f'{t} °C → {t * 4}', fontsize=6.5, color=c)
    ax.set_xlabel('temperature [°C]'); ax.set_ylabel('ADC value = temp × 4')
    ax.set_title('LM35 (10 mV/°C) with Vref = 2.56 V: 2.5 mV per LSB = 0.25 °C', loc='left', fontsize=8.5)
    ax.grid(True)
    save(fig, 'lm35')


# ---------------------------------------------------------------- chapter 4 (SERVICE)
def sched_offsets():
    fig, ax = plt.subplots(figsize=(7.2, 2.0))
    tasks = [('5 ms', 5, 0, APP), ('10 ms', 10, 2, SVC), ('100 ms', 100, 1, HAL), ('500 ms', 500, 3, MCAL), ('1 s', 1000, 4, INK)]
    t0, t1 = 995, 1016
    for i, (lab, p, off, c) in enumerate(tasks):
        y = 4 - i
        ax.text(t0 - 0.6, y, lab, ha='right', va='center', fontsize=7.5, color=c)
        first = p + off
        k = first
        while k <= t1:
            if k >= t0:
                ax.add_patch(Rectangle((k, y - 0.3), 0.8, 0.6, fc=c, ec=c, alpha=0.85))
            k += p
    for t in range(t0, t1 + 1):
        ax.axvline(t, color=GRID, lw=0.4, zorder=0)
        ax.text(t + 0.4, -0.75, str(t), ha='center', fontsize=5.8, color=MUTED)
    ax.set_xlim(t0 - 3.5, t1 + 1); ax.set_ylim(-1, 4.6); ax.axis('off')
    ax.text(t0, 4.75, 'flags set by the tick ISR around t = 1 s (tick numbers below): first due on 5, 12, 101, 503, 1004 → no two flags in the same tick',
            fontsize=7, color=INK)
    save(fig, 'sched_offsets')


def ringbuf():
    fig, ax = plt.subplots(figsize=(7.2, 1.5))
    data = ['', '', 'h', 'e', 'l', 'p', '', '', '']
    for i, d in enumerate(data):
        ax.add_patch(Rectangle((i, 0), 1, 0.7, fc=SVC_L if d else 'white', ec=SVC, lw=1))
        ax.text(i + 0.5, 0.35, d, ha='center', va='center', fontsize=8, family='DejaVu Sans Mono')
        ax.text(i + 0.5, -0.25, str(i), ha='center', fontsize=6, color=MUTED)
    ax.annotate('Tail (consumer)\nnext byte to read', (2.5, 0.7), (2.5, 1.35), ha='center', fontsize=6.5, arrowprops=dict(arrowstyle='->', lw=0.7))
    ax.annotate('Head (producer)\nnext free slot', (6.5, 0.7), (6.5, 1.35), ha='center', fontsize=6.5, arrowprops=dict(arrowstyle='->', lw=0.7))
    ax.text(9.4, 0.35, 'count = Head − Tail = 4\nfull when Head + 1 == Tail\n(one slot always empty:\ncapacity = Size − 1)', fontsize=6.8, va='center')
    ax.set_xlim(-0.3, 13.5); ax.set_ylim(-0.5, 1.9); ax.axis('off')
    save(fig, 'ringbuf')


def term_path():
    fig, ax = plt.subplots(figsize=(7.2, 2.1))
    box(ax, 0.0, 1.2, 1.3, 0.55, 'PD0 RXD', MCAL_L, MCAL)
    box(ax, 1.7, 1.2, 1.6, 0.55, 'USART RXC ISR\n(__vector_13)', MCAL_L, MCAL, fs=7)
    box(ax, 3.7, 1.2, 1.8, 0.55, 'RX ring 32 B\n(RINGBUF)', SVC_L, SVC, fs=7)
    box(ax, 5.9, 1.2, 2.2, 0.55, 'TERM_u8GetLine\nline editor, echo (5 ms)', SVC_L, SVC, fs=7)
    box(ax, 8.5, 0.7, 1.4, 0.55, 'UIREM', APP_L, APP)
    box(ax, 5.9, 0.15, 2.2, 0.55, 'TERM_voidPut…\n(drops when full)', SVC_L, SVC, fs=7)
    box(ax, 3.7, 0.15, 1.8, 0.55, 'TX ring 128 B', SVC_L, SVC, fs=7)
    box(ax, 1.7, 0.15, 1.6, 0.55, 'USART UDRE ISR\n(__vector_14)', MCAL_L, MCAL, fs=7)
    box(ax, 0.0, 0.15, 1.3, 0.55, 'PD1 TXD', MCAL_L, MCAL)
    arr = dict(arrowstyle='->', color=MUTED, lw=0.9)
    for a, b in (((1.3, 1.47), (1.7, 1.47)), ((3.3, 1.47), (3.7, 1.47)), ((5.5, 1.47), (5.9, 1.47)), ((8.1, 1.47), (8.7, 1.25)),
                 ((8.7, 0.7), (8.1, 0.42)), ((5.9, 0.42), (5.5, 0.42)), ((3.7, 0.42), (3.3, 0.42)), ((1.7, 0.42), (1.3, 0.42))):
        ax.annotate('', b, a, arrowprops=arr)
    ax.text(3.75, 1.85, 'ISR = producer · main loop = consumer', fontsize=6.5, color=MUTED)
    ax.text(3.75, -0.12, 'main loop = producer · ISR = consumer (UDRIE on while bytes wait)', fontsize=6.5, color=MUTED)
    ax.set_xlim(-0.1, 10.1); ax.set_ylim(-0.3, 2.05); ax.axis('off')
    save(fig, 'term_path')


def mavg():
    fig, ax = plt.subplots(figsize=(7.2, 2.0))
    s = [160] * 3 + [200] * 14
    avg = []
    win = [160] * 10
    for i, x in enumerate(s):
        win.append(x); win = win[-10:]
        avg.append((sum(win) + len(win) // 2) // len(win))
    t = [i * 100 for i in range(len(s))]
    ax.step(t, [x / 4 for x in s], where='post', color=MUTED, lw=1, label='sample (40 → 50 °C step)')
    ax.plot(t, [a / 4 for a in avg], 'o-', color=SVC, ms=3, lw=1.3, label='10-sample average (MAVG)')
    ax.axhline(45, color=HAL, ls='--', lw=0.7); ax.text(1620, 45.3, 'e.g. set − 5', fontsize=6.5, color=HAL, ha='right')
    ax.set_xlabel('time [ms] (one sample per 100 ms task)'); ax.set_ylabel('°C'); ax.legend(loc='lower right', fontsize=7); ax.grid(True)
    ax.set_title('Step response of the 10-sample moving average: full effect after 10 samples = 1.0 s', loc='left')
    save(fig, 'mavg')


def eeprom_map():
    fig, ax = plt.subplots(figsize=(7.2, 2.6))
    pages = [('magic A5 · version 01 · 14 × FF', MUTED, 'page 0  0x00'), ('heater set temperature (1 B) · 15 × FF', HAL, 'page 1  0x10'),
             ('admin: name[8] + password[8]', RED, 'page 2  0x20')] + \
            [(f'remote user {i}: name[8] + password[8]', APP, f'page {3 + i}  0x{0x30 + 16 * i:02X}') for i in range(5)] + \
            [(f'keypad user {i}: ID[8] + PIN[8] (digits)', SVC, f'page {8 + i}  0x{0x80 + 16 * i:02X}') for i in range(5)]
    for i, (lab, c, a) in enumerate(pages):
        y = 12 - i
        ax.add_patch(Rectangle((0, y), 16, 0.82, fc='white', ec=c, lw=1))
        ax.text(8, y + 0.41, lab, ha='center', va='center', fontsize=6.6, color=c)
        ax.text(-0.3, y + 0.41, a, ha='right', va='center', fontsize=6.4, family='DejaVu Sans Mono', color=INK)
    ax.text(16.4, 6.4, 'pages 13–63 (0xD0–0x3FF)\nnot used\n\nwritten from page 12\ndown to page 0:\nthe magic byte last', fontsize=6.6, va='center', color=MUTED)
    ax.set_xlim(-5.5, 22); ax.set_ylim(-0.3, 13); ax.axis('off')
    save(fig, 'eeprom_map')


# ---------------------------------------------------------------- chapter 5 extras
def climate_hyst():
    fig, ax = plt.subplots(figsize=(6.4, 2.1))
    ax.plot([15, 28, 28], [0, 0, 1], color=APP, lw=1.6)
    ax.plot([35, 21, 21], [1, 1, 0], color=HAL, lw=1.6, ls='--')
    ax.annotate('', (28, 0.6), (28, 0.4), arrowprops=dict(arrowstyle='->', color=APP))
    ax.annotate('', (21, 0.4), (21, 0.6), arrowprops=dict(arrowstyle='->', color=HAL))
    ax.text(28.3, 0.5, 'avg > 28 °C → ON', fontsize=7, color=APP); ax.text(20.7, 0.5, 'avg < 21 °C\n→ OFF', fontsize=7, color=HAL, ha='right')
    ax.set_yticks([0, 1]); ax.set_yticklabels(['fan off', 'fan on (100 %)']); ax.set_xlabel('room temperature, 10-sample average [°C]')
    ax.set_xlim(15, 35); ax.set_ylim(-0.2, 1.3)
    ax.set_title('CLIMATE hysteresis (REQ-AC-02)', loc='left')
    save(fig, 'climate_hyst')


def heater_band():
    fig, ax = plt.subplots(figsize=(7.2, 2.2))
    import math
    t = [i * 0.1 for i in range(0, 300)]
    water = [40 + 30 * (1 - math.exp(-x / 8)) for x in t]
    water = [w if x < 18 else w + (x - 18) * 0.9 for x, w in zip(t, water)]
    ax.plot(t, water, color=INK, lw=1.2, label='water (illustrative)')
    ax.axhline(60, color=MUTED, lw=0.8); ax.text(0.3, 60.6, 'set = 60', fontsize=7, color=MUTED)
    ax.axhspan(55, 65, color='#F5F7FA'); ax.axhline(55, color=RED, ls='--', lw=0.8); ax.axhline(65, color=APP, ls='--', lw=0.8)
    ax.text(29.8, 54.2, 'avg < 55 → heating ON, cooling OFF', ha='right', va='top', fontsize=6.8, color=RED)
    ax.text(0.3, 65.8, 'avg > 65 → heating OFF, cooling ON', fontsize=6.8, color=APP)
    ax.text(29.8, 61.2, 'between 55 and 65: keep the element', ha='right', fontsize=6.8, color=MUTED)
    ax.set_xlabel('time [s]'); ax.set_ylabel('°C'); ax.set_xlim(0, 30); ax.set_ylim(38, 80)
    ax.set_title('HEATER control band (REQ-HTR-09), illustrative water curve', loc='left')
    save(fig, 'heater_band')


def heater_outputs():
    fig, axs = plt.subplots(4, 1, figsize=(7.2, 2.6), sharex=True, gridspec_kw={'hspace': 0.5})
    T = 4000
    # 500 ms phase
    ph_x = list(range(0, T + 1, 500)); ph = [(i + 1) % 2 for i in range(len(ph_x))]
    wave(axs[0], ph_x, ph, 0, 1, MUTED); axs[0].text(-60, 0.5, 'blink phase\n(500 ms task)', ha='right', va='center', fontsize=6.5)
    wave(axs[1], ph_x, ph, 0, 1, RED); axs[1].text(-60, 0.5, 'heater LED\nwhile HEATING', ha='right', va='center', fontsize=6.5)
    wave(axs[2], [0, T], [1, 1], 0, 1, APP); axs[2].text(-60, 0.5, 'heater LED\nwhile COOLING', ha='right', va='center', fontsize=6.5)
    wave(axs[3], ph_x, ph, 0, 1, HAL); axs[3].text(-60, 0.5, '7-seg in SET\n(set temp / blank)', ha='right', va='center', fontsize=6.5)
    for a in axs:
        a.set_yticks([]); a.spines['left'].set_visible(False); a.set_ylim(-0.2, 1.3)
    axs[-1].set_xlabel('time [ms]')
    axs[0].set_title('HEATER outputs (REQ-HTR-11, HTR-13): one blink period = 2 × 500 ms = 1 s', loc='left')
    save(fig, 'heater_outputs')


# ---------------------------------------------------------------- sequence diagrams (chapter 6)
def sequence(name, lanes, msgs, title, height=None, notes=()):
    """lanes: list of (label, colour). msgs: (t, from, to, text[, style]) ; t in vertical units"""
    tmax = max(m[0] for m in msgs) + 1
    top = 1.55
    h = height or (0.75 + (tmax + top) * 0.27)
    fig, ax = plt.subplots(figsize=(7.2, h))
    n = len(lanes)
    xs = [i * (10 / (n - 1)) for i in range(n)]
    for (lab, c), x in zip(lanes, xs):
        box(ax, x - 0.72, -0.05, 1.44, 1.0, lab, 'white', c, fs=6.2, tc=c)
        ax.plot([x, x], [0.95, tmax + top], color=c, lw=0.8, ls=(0, (3, 2)))
    for m in msgs:
        t, a, b, txt = m[:4]
        st = m[4] if len(m) > 4 else '-'
        y = t + top
        xa, xb = xs[a], xs[b]
        if a == b:
            ax.plot([xa, xa + 0.3, xa + 0.3, xa + 0.05], [y - 0.15, y - 0.15, y + 0.15, y + 0.15], color=MUTED, lw=0.8)
            ax.annotate('', (xa + 0.02, y + 0.15), (xa + 0.12, y + 0.15), arrowprops=dict(arrowstyle='->', color=MUTED, lw=0.8))
            ax.text(xa + 0.38, y, txt, fontsize=6.2, va='center', color=INK,
                    bbox=dict(fc='white', ec='none', pad=0.6))
        else:
            ax.annotate('', (xb, y), (xa, y), arrowprops=dict(arrowstyle='->', color=INK if st == '-' else MUTED, lw=0.9, ls='--' if st == '--' else '-'))
            ax.text((xa + xb) / 2, y - 0.06, txt, ha='center', va='bottom', fontsize=6.2, color=INK,
                    bbox=dict(fc='white', ec='none', pad=0.4))
    for (t, txt) in notes:
        ax.text(-1.0, t + top, txt, fontsize=6.0, color=APP, ha='right', va='center', weight='semibold')
    ax.set_xlim(-2.0, 11.6); ax.set_ylim(tmax + top + 0.1, -0.1)
    ax.axis('off'); ax.set_title(title, loc='left')
    save(fig, name)


def seq_remote():
    L = N['SIM']['latency']['set_temp_eeprom']
    lanes = [('Terminal', MUTED), ('USART ISR\n+ TERM', MCAL), ('UIREM\n(5 ms)', APP), ('SEC /\nUSERDB', APP), ('HEATER', APP), ('ESTORE\n(10 ms)', SVC), ('24C08\n(I²C)', HAL)]
    msgs = [(0, 0, 1, '"admin\\r" (6 B, 6.2 ms)'), (0.9, 1, 2, 'TERM_u8GetLine → READY'), (1.6, 2, 0, 'password prompt', '--'),
            (2.4, 0, 1, '"1234\\r"'), (3.1, 1, 2, 'line'), (3.8, 2, 3, 'SEC_u8Login(REMOTE)'), (4.5, 3, 3, 'CheckAdmin: 8+8 bytes vs RAM image'),
            (5.3, 3, 2, 'SEC_LOGIN_ADMIN · write gate open'), (6.0, 2, 0, 'Welcome … + menu (408 B, one line per 5 ms step)', '--'),
            (7.0, 0, 1, '"5 65\\r"'), (7.7, 1, 2, 'line "5 65"'), (8.4, 2, 4, 'EVQ mute · HEATER_u8SetSetTemp(65)'),
            (9.1, 4, 5, 'ESTORE_voidWriteByte(0x10, 65) → page 1 dirty'), (9.8, 2, 0, '"[OK] Heater set temperature is now 65C"', '--'),
            (10.6, 5, 6, f'next 10 ms slot: page write S A0 10 41 FF… P ({L["page_write_us"]:.0f} µs)'),
            (11.4, 6, 6, 'write cycle ≤ 5 ms'), (12.3, 5, 6, 'next slot: ACK poll S A0 P → ACK → IDLE')]
    notes = [(7.0, 'Enter = 0'), (10.6, f'+{L["enter_to_page_write_ms"]:.1f} ms'), (12.3, f'+{L["enter_to_page_write_ms"] + L["polls_after_ms"][0]:.1f} ms')]
    sequence('seq_remote', lanes, msgs, 'Scenario 1 — remote login → set heater temperature 65 °C → EEPROM write-behind', notes=notes)


def seq_keypad():
    K = N['SIM']['latency']['key_lamp']
    lanes = [('Keypad', MUTED), ('KPAD\n(10 ms)', HAL), ('UILOC\n(10 ms)', APP), ('LIGHT', APP), ('LAMP /\nPCF8574', HAL), ('LCD_BUF\n(5 ms)', HAL), ('EVQ →\nUIREM', SVC)]
    msgs = [(0, 0, 1, 'key "2" pressed (row C, col 2)'), (0.8, 1, 1, 'scan 1: candidate "2"'), (1.6, 1, 1, 'scan 2 (10 ms later): accepted, pending = "2"'),
            (2.4, 2, 1, 'KPAD_u8GetKey() = "2" (same pass)'), (3.2, 2, 3, 'LIGHT_u8ToggleLamp(2)'), (4.0, 3, 4, 'LAMP_u8SetState → S 40 xx P (190 µs)'),
            (4.8, 3, 6, 'EVQ_voidPost(EVQ_LAMP, 2)'), (5.6, 2, 5, 'UILOC_voidDraw: "*" in row 2 (RAM only)'),
            (6.4, 5, 5, '5 ms: set cursor (0.46 ms)'), (7.2, 5, 5, '5 ms: write "*" (0.46 ms)'), (8.0, 6, 6, 'at the prompt: "[INFO] Lamp 2 is now ON"')]
    notes = [(0, 't = 0'), (4.0, f'+{K["lamp_write_after_ms"]:.1f} ms'), (6.4, f'+{K["lcd_writes_after_ms"][0]:.1f} ms'), (7.2, f'+{K["lcd_writes_after_ms"][1]:.1f} ms'),
             (8.0, f'+{K["terminal_info_done_ms"]:.1f} ms')]
    sequence('seq_keypad', lanes, msgs, 'Scenario 2 — keypad user toggles lamp 2 (times: simavr run)', notes=notes)


def seq_lockdown():
    Lk = N['SIM']['latency']['lockdown']
    lanes = [('Terminal', MUTED), ('UIREM', APP), ('SEC', APP), ('ALARM', APP), ('HEATER /\nCLIMATE', APP), ('BUZZER', HAL), ('UILOC', APP)]
    msgs = [(0, 0, 1, '3rd wrong pair "x" / "y"'), (0.8, 1, 2, 'SEC_u8Login(REMOTE)'), (1.6, 2, 2, 'no match: ++fail[REMOTE] = 3'),
            (2.4, 2, 3, 'ALARM_voidTrigger()'), (3.2, 3, 4, 'HEATER_voidForceOff · CLIMATE_voidForceOff'), (4.0, 3, 5, 'BUZZER_voidOn'),
            (4.8, 2, 1, 'SEC_LOGIN_LOCKED'), (5.6, 1, 0, 'next 5 ms: "[ALERT] 3 wrong logins. SYSTEM LOCKED."', '--'),
            (6.4, 6, 6, 'next 10 ms: ALARM active → LOCKED screen'), (7.2, 3, 5, 'every 500 ms: toggle (0.5 s / 0.5 s)')]
    notes = [(0, 't = 0'), (4.0, f'+{Lk["enter_to_buzzer_ms"]:.1f} ms')]
    sequence('seq_lockdown', lanes, msgs, 'Scenario 3 — third wrong login → lockdown', notes=notes)


# ---------------------------------------------------------------- chapter 7 (timing)
def load_slots(run):
    p = os.path.join(SIMDIR, run, 'slots.csv')
    return [(int(t), int(s), int(u)) for t, s, u in csv.reader(open(p))]


def gantt_100ms():
    """one 100 ms window of the simavr stress run , slots drawn at their real start and length"""
    sl = load_slots('stress')
    isr = [(int(v), int(s), int(u)) for v, s, u in csv.reader(open(os.path.join(SIMDIR, 'stress', 'isr.csv')))]
    # pick the window that contains the longest 10 ms slot
    t_long = max((x for x in sl if x[0] == 1), key=lambda x: x[2])[1]
    w0 = (t_long // 16000 - 30) * 16000
    w1 = w0 + 100 * 16000
    cols = {0: APP, 1: SVC, 2: HAL, 3: MCAL, 4: INK}
    names = {0: '5 ms', 1: '10 ms', 2: '100 ms', 3: '500 ms', 4: '1 s'}
    fig, (ax, ax2) = plt.subplots(2, 1, figsize=(7.2, 2.9), gridspec_kw={'height_ratios': [3, 1.2], 'hspace': 0.55})
    for t, s, u in sl:
        if w0 <= s < w1:
            ax.add_patch(Rectangle(((s - w0) / 16000, 4 - t - 0.35), max(u / 16000, 0.12), 0.7, fc=cols[t], ec=cols[t]))
    for v, s, u in isr:
        if w0 <= s < w1 and v != 4:
            ax.plot([(s - w0) / 16000], [-0.6], '|', color=MCAL, ms=5)
    ax.set_yticks([4, 3, 2, 1, 0, -0.6]); ax.set_yticklabels(['5 ms', '10 ms', '100 ms', '500 ms', '1 s', 'UART ISR'])
    ax.set_xlim(0, 100); ax.set_ylim(-1.1, 4.6); ax.set_xlabel('time in the window [ms]')
    ax.set_title(f'One 100 ms window of the simavr stress run (t = {w0 / 16e6:.2f} s)', loc='left')
    for k in range(0, 101, 5):
        ax.axvline(k, color=GRID, lw=0.4, zorder=0)
    # zoom on the longest slot
    z0 = t_long - 1 * 16000; z1 = t_long + 4 * 16000
    for t, s, u in sl:
        if z0 <= s < z1:
            ax2.add_patch(Rectangle(((s - z0) / 16, 0.15), u / 16, 0.7, fc=cols[t], ec='white'))
            ax2.text((s - z0) / 16 + u / 32, 0.5, f'{names[t]} slot\n{u / 16:.0f} µs', ha='center', va='center', fontsize=6.2,
                     color='white' if u > 3000 else INK)
    for v, s, u in isr:
        if z0 <= s < z1 and v == 4:
            ax2.plot([(s - z0) / 16, (s - z0) / 16], [0.0, 1.0], color=RED, lw=0.8)
    ax2.text(5000, 1.15, 'red = tick ISR (≈ 16 µs each)', ha='right', fontsize=6.2, color=RED)
    ax2.set_xlim(0, 5000); ax2.set_ylim(0, 1.3); ax2.set_yticks([]); ax2.set_xlabel('µs (zoom on the longest slot of the run)')
    ax2.spines['left'].set_visible(False)
    save(fig, 'gantt_100ms')


def i2c_breakdown():
    T = N['TWI_TX']; S = N['SIM']['twi_types']; SS = N['SIM']['twi_types_stress']
    rows = [('LCD byte (CLCD, 4 port bytes)', T['lcd_byte'], S['LCD (0x27) 4 B']['max_us'] + 10, 480, 5),
            ('LCD init nibble (2 port bytes)', T['lcd_nibble'], S['LCD (0x27) 2 B']['max_us'] + 10, None, 3),
            ('lamp expander write', T['lamp'], S['Lamps (0x20) 1 B']['max_us'] + 10, None, 2),
            ('7-seg digit write', T['seg_digit'], SS['7-seg tens (0x21) 1 B']['max_us'] + 10, None, 2),
            ('EEPROM page write (16 B)', T['ee_page'], S['EEPROM (0x50, W) 17 B']['max_us'] + 10, None, 18),
            ('EEPROM ACK poll', T['ee_poll'], S['EEPROM (0x50, W) 0 B']['max_us'] + 10, None, 1)]
    fig, ax = plt.subplots(figsize=(7.2, 2.4))
    y = list(range(len(rows)))[::-1]
    ax.barh([v + 0.18 for v in y], [r[1] for r in rows], height=0.34, color=SVC, label='computed: START + n × 90 µs')
    ax.barh([v - 0.18 for v in y], [r[2] for r in rows], height=0.34, color=HAL, label='simavr (TWBR-timed model)')
    for v, r in zip(y, rows):
        ax.text(r[2] + 25, v - 0.18, f'{r[2]:.0f}', va='center', fontsize=6.5, color=HAL)
        ax.text(r[1] + 25, v + 0.18, f'{r[1]:.0f}', va='center', fontsize=6.5, color=SVC)
        if r[3]:
            ax.plot([r[3]], [v], 'D', color=MCAL, ms=4)
            ax.text(r[3] + 25, v + 0.45, 'Proteus ≈ 480 (hal_summary.md)', fontsize=6, color=MCAL)
    ax.set_yticks(y); ax.set_yticklabels([f'{r[0]}\n({r[4]} bytes incl. address)' for r in rows], fontsize=6.8)
    ax.set_xlabel('bus time per transaction [µs]'); ax.legend(loc='lower right', fontsize=7)
    ax.set_title('I²C transaction durations at 100 kHz', loc='left')
    save(fig, 'i2c_breakdown')


def latency_waterfall():
    L = N['SIM']['latency']
    chains = [
        ('UART "1 3⏎" → lamp on', [('Enter received', 0, MUTED), ('wait for 5 ms slot', L['uart_lamp']['cmd_to_lamp_ms'], APP)], L['uart_lamp']['cmd_to_lamp_ms']),
        ('UART "1 3⏎" → "[OK] …" sent', [('to lamp', L['uart_lamp']['cmd_to_lamp_ms'], APP), ('24 B at 1.04 ms', L['uart_lamp']['reply_done_ms'] - L['uart_lamp']['cmd_to_lamp_ms'], MCAL)], L['uart_lamp']['reply_done_ms']),
        ('key "2" → lamp on', [('2 debounce scans', L['key_lamp']['lamp_write_after_ms'], HAL)], L['key_lamp']['lamp_write_after_ms']),
        ('key "2" → LCD shows "*"', [('to lamp', L['key_lamp']['lamp_write_after_ms'], HAL), ('cursor + char, 5 ms each', L['key_lamp']['lcd_writes_after_ms'][1] - L['key_lamp']['lamp_write_after_ms'], APP)], L['key_lamp']['lcd_writes_after_ms'][1]),
        ('"5 65⏎" → page write starts', [('to 10 ms slot', L['set_temp_eeprom']['enter_to_page_write_ms'], SVC)], L['set_temp_eeprom']['enter_to_page_write_ms']),
        ('Up press → 7-seg written', [('debounce + next 100 ms task', L['btn_up_7seg']['first_write_after_ms'], HAL)], L['btn_up_7seg']['first_write_after_ms']),
        ('ambient 25→30 °C → [INFO] AC ON', [('average crosses 28 °C (7 samples)', L['ambient_step_to_ac_info_ms'], SVC)], L['ambient_step_to_ac_info_ms']),
        ('water 40→80 °C → cooling on', [('average > set + 5', L['water_step_to_cooling_ms'], SVC)], L['water_step_to_cooling_ms']),
        ('ON/OFF release → heating on', [('10 samples after turn-on', L['heater_on_to_heating']['relay_on_after_release_ms'], RED)], L['heater_on_to_heating']['relay_on_after_release_ms']),
    ]
    fig, ax = plt.subplots(figsize=(7.2, 3.0))
    for i, (lab, parts, tot) in enumerate(chains):
        y = len(chains) - 1 - i
        x = 0
        for pl, d, c in parts:
            if d > 0:
                ax.barh(y, d, left=x, color=c, height=0.55, edgecolor='white')
            x += d
        ax.text(tot * 1.12 + 0.3, y, f'{tot:.1f} ms', va='center', fontsize=6.8)
    ax.set_xscale('symlog', linthresh=10); ax.set_xlim(0, 4000)
    ax.set_yticks(range(len(chains))[::-1]); ax.set_yticklabels([c[0] for c in chains], fontsize=6.8)
    ax.set_xlabel('latency from the stimulus [ms] (log scale above 10 ms)'); ax.grid(True, axis='x')
    ax.set_title('Latency chains measured in simavr (main scenario)', loc='left')
    save(fig, 'latency_waterfall')


def slot_hist():
    R = N['SIM']
    fig, ax = plt.subplots(figsize=(7.2, 2.1))
    names = ['5 ms', '10 ms', '100 ms', '500 ms', '1 s']
    cols = [APP, SVC, HAL, MCAL, INK]
    for i, (n, c) in enumerate(zip(names, cols)):
        a = R['slots_main'][n]; b = R['slots_stress'][n]
        mx = max(a['max_us'], b['max_us'])
        ax.plot([a['min_us'], mx], [i, i], color=c, lw=1)
        ax.plot([a['mean_us']], [i], 'o', color=c, ms=4)
        ax.plot([mx], [i], '|', color=c, ms=10, mew=2)
        ax.text(mx * 1.08, i, f'max {mx:.0f} µs', va='center', fontsize=6.8, color=c)
    ax.set_xscale('log'); ax.set_yticks(range(5)); ax.set_yticklabels(names); ax.invert_yaxis()
    ax.set_xlim(5, 6000); ax.set_xlabel('slot duration [µs] (min — mean ● — max |), main + stress runs, ISR time included'); ax.grid(True, axis='x')
    ax.axvline(1000, color=RED, lw=0.7, ls='--'); ax.text(1000, -0.75, '1 tick', color=RED, fontsize=6.5, ha='center')
    ax.set_title('Slot execution times measured in simavr', loc='left')
    save(fig, 'slot_hist')


def cpu_load():
    R = N['SIM']
    vals = [('tick ISR alone (computed)', 0.0163 * 100 / 1.0 if False else 245 / 16000 * 100),
            ('idle, status screen (simavr)', R['util_main_idle_ms'] * 100),
            ('main scenario, 25 s (simavr)', R['util_main_whole'] * 100),
            ('stress, 6–10.5 s (simavr)', R['util_stress_6_10s'] * 100),
            ('stress, worst 1 s window (simavr)', R['util_stress_worst_1s']['util'] * 100)]
    fig, ax = plt.subplots(figsize=(7.2, 1.7))
    for i, (l, v) in enumerate(vals):
        ax.barh(len(vals) - 1 - i, v, color=SVC if i else MCAL, height=0.55)
        ax.text(v + 0.15, len(vals) - 1 - i, f'{v:.1f} %', va='center', fontsize=7)
    ax.set_yticks(range(len(vals))[::-1]); ax.set_yticklabels([v[0] for v in vals], fontsize=7)
    ax.set_xlim(0, 10); ax.set_xlabel('CPU busy [%]')
    ax.set_title('CPU utilisation (busy = slots + ISRs; the rest is the idle polling of SCHED_u8IsTaskDue)', loc='left')
    save(fig, 'cpu_load')


def blink_corner():
    fig, ax = plt.subplots(figsize=(7.2, 1.9))
    # ticks : 100 ms tasks at x01 , 500 ms at x03 (relative ms)
    ax.annotate('Up pressed', (0, 0.6), (0, 2.0), ha='center', fontsize=6.3, arrowprops=dict(arrowstyle='->', lw=0.6))
    xs = [0, 73.7, 75.6, 575.6, 1075.6, 1500]
    ys = [0, 1, 0, 1, 0, 0]
    wave(ax, xs, ys, 0.6, 0.6, HAL)
    ax.text(-20, 0.9, '7-seg', ha='right', va='center', fontsize=7)
    ax.annotate('100 ms task shows the set value', (73.7, 1.2), (180, 1.5), fontsize=6.3, arrowprops=dict(arrowstyle='->', lw=0.6))
    ax.annotate('500 ms task 2 ms later: phase → 0, blank', (75.6, 0.6), (180, 0.2), fontsize=6.3, arrowprops=dict(arrowstyle='->', lw=0.6))
    ax.set_xlim(-60, 1500); ax.set_ylim(0, 2.3); ax.set_yticks([]); ax.spines['left'].set_visible(False); ax.set_xlabel('ms after the press (simavr main run, t = 17 s)')
    ax.set_title('Corner case: a press just before a 500 ms toggle is shown for only ≈ 2 ms', loc='left')
    save(fig, 'blink_corner')


def trace_matrix():
    req = ['SEC-01', 'SEC-02', 'SEC-03', 'SEC-04', 'SEC-05', 'SEC-06', 'SEC-07', 'SEC-08', 'SEC-09', 'LUI-01', 'LUI-02', 'LUI-03', 'LUI-04',
           'RUI-01', 'RUI-02', 'RUI-03', 'RUI-04', 'LGT-01', 'LGT-02', 'DOR-01', 'DOR-02', 'AC-01', 'AC-02', 'AC-03', 'HTR-01', 'HTR-02',
           'HTR-03', 'HTR-04', 'HTR-05', 'HTR-06', 'HTR-07', 'HTR-08', 'HTR-09', 'HTR-10', 'HTR-11', 'HTR-12', 'HTR-13', 'HTR-14', 'ALM-01',
           'ALM-02', 'EEP-01', 'EEP-02', 'EEP-03']
    mods = ['SEC', 'ALARM', 'LIGHT', 'DOOR', 'CLIMATE', 'HEATER', 'UILOC', 'UIREM', 'USERDB', 'ESTORE', 'TERM', 'SCHED', 'MAVG']
    import re, glob
    root = NUM.ROOT
    files = {m: [] for m in mods}
    for m in mods:
        files[m] = glob.glob(os.path.join(root, 'src', 'APP', m, '*.c')) + glob.glob(os.path.join(root, 'lib', '*', m, '*.c'))
    M = [[0] * len(req) for _ in mods]
    for i, m in enumerate(mods):
        txt = ''.join(open(f, encoding='latin-1').read() for f in files[m])
        for j, r in enumerate(req):
            M[i][j] = len(re.findall('REQ-' + r + r'\b', txt))
    fig, ax = plt.subplots(figsize=(7.2, 2.6))
    for i in range(len(mods)):
        for j in range(len(req)):
            if M[i][j]:
                ax.add_patch(Rectangle((j, len(mods) - 1 - i), 0.92, 0.85, fc=APP, alpha=min(1, 0.35 + 0.15 * M[i][j])))
    ax.set_xlim(0, len(req)); ax.set_ylim(0, len(mods))
    ax.set_xticks([j + 0.46 for j in range(len(req))]); ax.set_xticklabels(req, rotation=90, fontsize=5.6)
    ax.set_yticks([i + 0.42 for i in range(len(mods))]); ax.set_yticklabels(mods[::-1], fontsize=6.5)
    ax.set_title('REQ-… tags in the code, per module (darker = more tags)', loc='left')
    for s in ('left', 'bottom'):
        ax.spines[s].set_visible(False)
    ax.tick_params(length=0)
    save(fig, 'trace_matrix')
    return M, req, mods


def all_figs():
    for f in (pinmap, pinbudget, memory, boot_timeline, t2_ctc, t0_pwm, t1_pwm, uart_frame, twi_txn, adc_timing, pcf_lcd, lcd_buf, keypad,
              debounce_timing, sevenseg, ee_poll, lm35, sched_offsets, ringbuf, term_path, mavg, eeprom_map, climate_hyst, heater_band,
              heater_outputs, seq_remote, seq_keypad, seq_lockdown, gantt_100ms, i2c_breakdown, latency_waterfall, slot_hist, cpu_load,
              blink_corner, trace_matrix):
        f()


if __name__ == '__main__':
    all_figs()
    print('mpl figures ->', OUT)
