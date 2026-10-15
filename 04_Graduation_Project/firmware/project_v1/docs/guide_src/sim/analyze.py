"""Turn the simavr logs into the numbers used in the guide (results.json)."""
import csv, json, sys, statistics as st
from collections import defaultdict

C = 16000.0          # cycles per ms
TASK = {0: '5 ms', 1: '10 ms', 2: '100 ms', 3: '500 ms', 4: '1 s'}


def load(run):
    d = {}
    d['slots'] = [(int(t), int(s), int(u)) for t, s, u in csv.reader(open(f'{run}/slots.csv'))]
    d['isr'] = [(int(v), int(s), int(u)) for v, s, u in csv.reader(open(f'{run}/isr.csv'))]
    d['twi'] = []
    for x in csv.reader(open(f'{run}/twi.csv')):
        d['twi'].append(dict(s=int(x[0]), e=int(x[1]), a=int(x[2]), n=int(x[3]), b=[int(v) for v in x[4:]]))
    d['pins'] = [(int(c), p, int(v)) for c, p, v in csv.reader(open(f'{run}/pins.csv'))]
    d['uart'] = [(int(l.split('\t')[0]), int(l.split('\t')[1])) for l in open(f'{run}/uart.txt')]
    d['ev'] = []
    for l in open(f'{run}/events.csv'):
        c, w = l.rstrip('\n').split(',', 1)
        d['ev'].append((int(c), w))
    return d


def ev_time(d, text):
    for c, w in d['ev']:
        if text in w:
            return c
    raise KeyError(text)


def first(seq, cond, after):
    for x in seq:
        if x[0] >= after and cond(x):
            return x
    return None


def slot_stats(d, skip_ms=0):
    out = {}
    for k in TASK:
        durs = [u for t, s, u in d['slots'] if t == k and s > skip_ms * C]
        if durs:
            out[TASK[k]] = dict(n=len(durs), min_us=min(durs) / 16, mean_us=st.mean(durs) / 16,
                                p99_us=sorted(durs)[int(0.99 * (len(durs) - 1))] / 16, max_us=max(durs) / 16)
    return out


def isr_stats(d):
    out = {}
    for v in (4, 13, 14):
        durs = [u for vv, s, u in d['isr'] if vv == v]
        if durs:
            out[v] = dict(n=len(durs), min_cyc=min(durs), max_cyc=max(durs), mean_cyc=st.mean(durs))
    return out


def passes(d):
    """contiguous runs of slots (gap < 20 us) = one loop pass doing work"""
    sl = sorted(d['slots'], key=lambda x: x[1])
    res = []
    cur = None
    for t, s, u in sl:
        if cur and s - cur[1] < 320:
            cur = (cur[0], s + u, cur[2] + [t])
        else:
            if cur:
                res.append(cur)
            cur = (s, s + u, [t])
    if cur:
        res.append(cur)
    return [(a, b - a, ts) for a, b, ts in res]


def utilisation(d, t0_ms, t1_ms):
    a, b = t0_ms * C, t1_ms * C
    busy = sum(min(s + u, b) - max(s, a) for t, s, u in d['slots'] if s + u > a and s < b)
    # ISR time outside slots
    sl = sorted((s, s + u) for t, s, u in d['slots'])
    import bisect
    starts = [x[0] for x in sl]
    isr_out = 0
    for v, s, u in d['isr']:
        if s < a or s > b:
            continue
        i = bisect.bisect_right(starts, s) - 1
        if i >= 0 and sl[i][0] <= s < sl[i][1]:
            continue
        isr_out += u
    return (busy + isr_out) / (b - a)


def twi_types(d):
    groups = defaultdict(list)
    for x in d['twi']:
        a7 = x['a'] >> 1
        rw = x['a'] & 1
        name = {0x27: 'LCD (0x27)', 0x20: 'Lamps (0x20)', 0x21: '7-seg tens (0x21)', 0x22: '7-seg units (0x22)'}.get(a7)
        if name is None:
            if rw:
                name = 'EEPROM read (0x50, R)'
            else:
                name = 'EEPROM (0x50, W)'
        groups[(name, x['n'])].append((x['e'] - x['s']) / 16)
    return {f'{k[0]} {k[1]} B': dict(n=len(v), min_us=min(v), max_us=max(v)) for k, v in sorted(groups.items())}


def main():
    R = {}
    m = load('main')
    s = load('stress')
    R['boot_first_slot_ms'] = {r: int(open(f'{r}/events.csv').read().split(',')[0]) / C for r in ('fb0', 'fb5', 'warm')}
    R['slots_main'] = slot_stats(m, skip_ms=400)
    R['slots_stress'] = slot_stats(s, skip_ms=400)
    R['isr_main'] = isr_stats(m)
    R['isr_stress'] = isr_stats(s)
    pm = passes(m) + passes(s)
    longest = sorted(pm, key=lambda x: -x[1])[:8]
    R['longest_passes'] = [dict(t_ms=a / C, dur_us=u / 16, tasks=[TASK[t] for t in ts]) for a, u, ts in longest]
    R['util_main_idle_ms'] = utilisation(m, 400, 900)     # nobody logged in , status screen
    R['util_main_whole'] = utilisation(m, 400, 25000)
    R['util_stress_6_10s'] = utilisation(s, 6000, 10500)
    # worst 1-second window in stress
    best = max(((utilisation(s, t, t + 1000), t) for t in range(1000, 15000, 250)))
    R['util_stress_worst_1s'] = dict(util=best[0], t_ms=best[1])
    # fraction of time the I2C bus is busy (transaction + one SCL period for its START), after boot
    R['bus_busy'] = {}
    for name, d, (a, b) in (('main', m, (1000, 30000)), ('stress', s, (1000, 16000))):
        busy = sum(min(x['e'], b * C) - max(x['s'], a * C) + 160 for x in d['twi'] if x['e'] > a * C and x['s'] < b * C)
        R['bus_busy'][name] = busy / ((b - a) * C)
    R['twi_types'] = twi_types(m)
    R['twi_types_stress'] = twi_types(s)

    # ---- latency chains (main run) ----
    L = {}
    # 1. UART "1 3\r" : Enter received -> lamp write starts -> reply finished
    t_cmd = ev_time(m, '6000 uart 1 3')
    t_enter = t_cmd + 3 * 16640 + 16640          # 4 bytes injected 1 frame apart ; the CR is complete one frame after injection
    lamp = first([(x['s'], x) for x in m['twi']], lambda y: y[1]['a'] == 0x40, t_cmd)
    # reply "[OK] Lamp 3 is now ON\r\n" : last '\n' after the lamp write
    ok = [c for c, ch in m['uart'] if c > lamp[0]]
    txt = ''
    end_reply = None
    for c, ch in m['uart']:
        if c > lamp[0]:
            txt += chr(ch)
            if txt.endswith('ON\r\n'):
                end_reply = c
                break
    L['uart_lamp'] = dict(first_byte_ms=t_cmd / C, enter_complete_ms=t_enter / C, lamp_write_ms=lamp[0] / C,
                          cmd_to_lamp_ms=(lamp[0] - t_enter) / C, reply_done_ms=(end_reply - t_enter) / C if end_reply else None)
    # 2. keypad '2' in the lamp screen : key down -> lamp write -> LCD writes that change row 2
    t_key = ev_time(m, '12500 keys 2')
    lamp2 = first([(x['s'], x) for x in m['twi']], lambda y: y[1]['a'] == 0x40, t_key)
    lcd_after = [x for x in m['twi'] if x['a'] == 0x4e and x['s'] > lamp2[0]][:4]
    L['key_lamp'] = dict(key_down_ms=t_key / C, lamp_write_after_ms=(lamp2[0] - t_key) / C,
                         lcd_writes_after_ms=[(x['s'] - t_key) / C for x in lcd_after])
    info = None
    txt = ''
    for c, ch in m['uart']:
        if c > lamp2[0]:
            txt += chr(ch)
            if 'is now ON\r\n' in txt:
                info = c
                break
    L['key_lamp']['terminal_info_done_ms'] = (info - t_key) / C if info else None
    # 3. heater Up in RUN : press -> first 7-seg write (set temperature) ; blink intervals
    t_up = ev_time(m, '17000 btn up')
    seg = [x for x in m['twi'] if (x['a'] >> 1) in (0x21, 0x22) and x['s'] > t_up]
    L['btn_up_7seg'] = dict(first_write_after_ms=(seg[0]['s'] - t_up) / C if seg else None)
    # blink : 7-seg writes between 17.1 s and the timeout
    t_dn = ev_time(m, '18000 btn down')
    w = [x for x in m['twi'] if (x['a'] >> 1) == 0x21 and t_up < x['s'] < t_dn + 5200 * C]
    L['blink_tens_writes_ms'] = [round((x['s'] - t_up) / C, 1) for x in w][:30]
    # set-mode timeout : last press (down) -> display shows water again (tens digit pattern of water) ; no save (value unchanged)
    # 4. heater ON release -> heating relay ON
    t_on = ev_time(m, '15000 btn onoff')
    t_rel = t_on + 150 * C
    pb6 = first(m['pins'], lambda p: p[1] == 'PB6' and p[2] == 1, t_on)
    L['heater_on_to_heating'] = dict(release_ms=t_rel / C, relay_on_after_release_ms=(pb6[0] - t_rel) / C)
    # LED blink while heating
    pa3 = [c for c, p, v in m['pins'] if p == 'PA3' and t_rel < c < t_rel + 4000 * C]
    L['heater_led_toggle_ms'] = [round((c - t_rel) / C, 1) for c in pa3][:10]
    # 5. water step to 80 C -> cooling relay
    t_w = ev_time(m, '20000 adc 1 800')
    pb7 = first(m['pins'], lambda p: p[1] == 'PB7' and p[2] == 1, t_w)
    L['water_step_to_cooling_ms'] = (pb7[0] - t_w) / C if pb7 else None
    t_w2 = ev_time(m, '23000 adc 1 600')
    pb6b = first(m['pins'], lambda p: p[1] == 'PB6' and p[2] == 1, t_w2)
    L['water_step_to_heating_ms'] = (pb6b[0] - t_w2) / C if pb6b else None
    # 6. "5 65" : Enter -> EEPROM page write -> ACK poll ok
    t5 = ev_time(m, '7000 uart 5 65')
    t5e = t5 + 4 * 16640 + 16640
    pw = first([(x['s'], x) for x in m['twi']], lambda y: y[1]['a'] == 0xa0 and y[1]['n'] == 17, t5)
    polls = [x for x in m['twi'] if x['a'] == 0xa0 and x['n'] == 0 and pw[0] < x['s'] < pw[0] + 80 * C]
    L['set_temp_eeprom'] = dict(enter_to_page_write_ms=(pw[0] - t5e) / C, page_write_us=(pw[1]['e'] - pw[1]['s']) / 16,
                                page_bytes=pw[1]['b'][:3], polls_after_ms=[round((x['s'] - pw[0]) / C, 2) for x in polls])
    # 7. lockdown : 3rd Enter -> buzzer on
    t_l = ev_time(m, '26900 uart y')
    t_le = t_l + 2 * 16640
    pd3 = [c for c, p, v in m['pins'] if p == 'PD3' and c > t_l]
    L['lockdown'] = dict(enter_to_buzzer_ms=(pd3[0] - t_le) / C if pd3 else None,
                         buzzer_toggles_ms=[round((c - t_le) / C, 1) for c in pd3[:6]])
    # 8. AC : ambient step 25 -> 30 C at 22 s -> [INFO] AC is now ON on the terminal
    t_a = ev_time(m, '22000 adc 0 300')
    txt = ''
    acon = None
    for c, ch in m['uart']:
        if c > t_a:
            txt += chr(ch)
            if 'AC is now ON' in txt:
                acon = c
                break
    L['ambient_step_to_ac_info_ms'] = (acon - t_a) / C if acon else None
    R['latency'] = L
    # ---- release jitter : slot start - start of the tick ISR that set its flag ----
    import bisect
    J = {}
    for name, d in (('main', m), ('stress', s)):
        ticks = sorted(st_ for v, st_, u in d['isr'] if v == 4)
        jj = defaultdict(list)
        for t, s0, u in d['slots']:
            i = bisect.bisect_right(ticks, s0) - 1
            if i >= 0:
                jj[TASK[t]].append((s0 - ticks[i]) / 16)
        J[name] = {k: dict(min_us=min(v), max_us=max(v), mean_us=st.mean(v)) for k, v in jj.items()}
    R['release_latency'] = J
    # menu size in bytes (admin menu as printed)
    u = ''.join(chr(ch) for c, ch in m['uart'])
    i = u.find('---------- MENU'); j = u.find('admin> ', i)
    R['admin_menu_bytes'] = j - i
    R['uart_total_bytes_main'] = len(m['uart'])
    json.dump(R, open('results.json', 'w'), indent=1)
    print(json.dumps(R, indent=1)[:6000])


main()
