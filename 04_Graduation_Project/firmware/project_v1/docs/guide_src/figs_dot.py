"""figs_dot.py - state machines, call graph and data-flow diagrams (Graphviz -> SVG).
Every state, guard and action below is taken from the source files named in each function."""
import os, subprocess

OUT = os.environ.get('FIG_OUT', os.path.join(os.path.dirname(__file__), 'out', 'images', 'guide'))
os.makedirs(OUT, exist_ok=True)

APP, SVC, HAL, MCAL = '#2B6CB0', '#2F855A', '#C05621', '#6B46C1'
APP_L, SVC_L, HAL_L, MCAL_L = '#E8F1FB', '#E9F6EF', '#FFF5E6', '#F3EEFB'
INK, MUTED, RED = '#1F2933', '#52606D', '#C53030'

HEAD = f'''digraph G {{
  graph [fontname="Inter", fontsize=11, bgcolor="white", pad=0.15, nodesep=0.35, ranksep=0.45, splines=true];
  node  [fontname="Inter", fontsize=10, shape=box, style="rounded,filled", color="{APP}", fillcolor="{APP_L}", fontcolor="{INK}", penwidth=1.2, margin="0.12,0.06"];
  edge  [fontname="Inter", fontsize=8.5, color="{MUTED}", fontcolor="{INK}", arrowsize=0.7, penwidth=1.0];
'''


def render(name, body, head=HEAD):
    src = head + body + '\n}\n'
    open(os.path.join(OUT, name + '.dot'), 'w').write(src)
    subprocess.run(['dot', '-Tsvg', '-o', os.path.join(OUT, name + '.svg')], input=src.encode(), check=True)


def start(n='init'):
    return f'  {n} [shape=circle, label="", width=0.16, style=filled, fillcolor="{INK}", color="{INK}"];\n'


# ---------------- APP state machines ----------------
def sm_sec():  # src/APP/SEC/SEC.c
    render('sm_sec', start() + f'''
  rankdir=LR;
  OUT  [label="LOGGED_OUT\\n(per source)"];
  USER [label="USER"];
  ADM  [label="ADMIN\\n(remote only)"];
  LOCK [label="system LOCKED\\n(ALARM active)", color="{RED}", fillcolor="#FDECEC"];
  init -> OUT;
  OUT -> ADM [label="Login(REMOTE)\\nadmin record matches\\nfail=0 · write gate open\\nlocal allowed=0 · end LOCAL"];
  OUT -> USER [label="Login(src)\\nlist of src matches\\n[LOCAL: IsLocalAllowed]\\nfail[src]=0"];
  OUT -> OUT [label="no match\\n++fail[src] < 3\\n→ LOGIN_FAILED"];
  OUT -> LOCK [label="no match\\n++fail[src] == 3\\nALARM_voidTrigger()", color="{RED}", fontcolor="{RED}"];
  USER -> OUT [label="Logout(src)\\nLOCAL: post EVQ_LOCAL_SESSION(0)"];
  ADM -> OUT [label="Logout(REMOTE) · 120 s idle\\nwrite gate closed · local allowed=0"];
  ADM -> ADM [label="SetLocalAllowed(x)\\nx=0 ends LOCAL session"];
''')


def sm_alarm():  # ALARM.c
    render('sm_alarm', start() + f'''
  rankdir=LR;
  IDLE [label="IDLE"];
  ACT [label="ACTIVE (latched)", color="{RED}", fillcolor="#FDECEC"];
  init -> IDLE [label="ALARM_voidInit\\nbuzzer off"];
  IDLE -> ACT [label="ALARM_voidTrigger()\\nHEATER_voidForceOff · CLIMATE_voidForceOff\\nBUZZER on · post EVQ_LOCKDOWN"];
  ACT -> ACT [label="500 ms task\\ntoggle buzzer (D-15)"];
  ACT -> ACT [label="Trigger again: ignored", fontcolor="{MUTED}"];
''')


def sm_climate():  # CLIMATE.c
    render('sm_climate', start() + f'''
  rankdir=TB; nodesep=1.1; ranksep=0.6;
  W [label="WAIT\\n(average < 10 samples)"];
  OFF [label="AC_OFF\\nfan 0 %"];
  ON [label="AC_ON\\nfan 100 %"];
  F [label="FORCED_OFF\\n(until reset)", color="{RED}", fillcolor="#FDECEC"];
  init -> W;
  W -> OFF [label="10th sample"];
  OFF -> ON [label="avg_x4 > 28·4 = 112\\nFAN_voidSetSpeed(100)\\npost EVQ_AC(1)"];
  ON -> OFF [label="avg_x4 < 21·4 = 84\\nFAN_voidSetSpeed(0)\\npost EVQ_AC(0)"];
  OFF -> OFF [label="84 ≤ avg_x4 ≤ 112\\n(hysteresis)", fontcolor="{MUTED}"];
  {{W OFF ON}} -> F [label="CLIMATE_voidForceOff()", color="{RED}", fontcolor="{RED}"];
''')


def sm_heater():  # HEATER.c panel states
    render('sm_heater', start() + f'''
  rankdir=TB; nodesep=0.9; ranksep=0.55;
  OFF [label="OFF\\nelements off · display blank\\nLED off"];
  RUN [label="RUN\\ndisplay = water temp"];
  SET [label="SET\\ndisplay = set temp, blinking\\ntimer = 50 × 100 ms"];
  LCK [label="LOCKED\\n(buttons ignored)", color="{RED}", fillcolor="#FDECEC"];
  init -> OFF [label="HEATER_voidInit\\nset = ESTORE byte or 60"];
  OFF -> RUN [label="ON/OFF released\\nset re-read · average reset\\npost HEATER_POWER(1)"];
  OFF -> OFF [label="Up / Down: ignored (D-10)", fontcolor="{MUTED}"];
  RUN -> SET [label="Up or Down pressed\\n(value NOT changed)\\ntimer = 50 · phase = 1"];
  SET -> SET [label="Up: set = min(set+5, 75)\\nDown: set = max(set−5, 35)\\ntimer = 50 · phase = 1"];
  SET -> RUN [label="100 ms task: --timer == 0\\nsave if changed"];
  RUN -> OFF [label="ON/OFF released\\nsave if changed · elements off\\npost HEATER_POWER(0)"];
  SET -> OFF [label="ON/OFF released"];
  {{OFF RUN SET}} -> LCK [label="HEATER_voidForceOff()", color="{RED}", fontcolor="{RED}"];
''')


def sm_heater_element():
    render('sm_heater_element', start() + f'''
  rankdir=LR;
  N [label="NONE\\n(IDLE)"];
  H [label="HEATING\\ncooler off, then heater on", color="{RED}", fillcolor="#FDECEC"];
  C [label="COOLING\\nheater off, then cooler on", color="{APP}"];
  init -> N [label="turn-on"];
  N -> H [label="avg_x4 < (set−5)·4"];
  N -> C [label="avg_x4 > (set+5)·4"];
  H -> C [label="avg_x4 > (set+5)·4"];
  C -> H [label="avg_x4 < (set−5)·4"];
  H -> H [label="otherwise: keep", fontcolor="{MUTED}"];
  C -> C [label="otherwise: keep", fontcolor="{MUTED}"];
  label="evaluated every 100 ms in RUN and SET, only when the average holds 10 samples (HTR-08); each change posts EVQ_HEATER_ELEMENT";
  labelloc=b; fontsize=9; fontcolor="{MUTED}";
''')


def sm_uiloc():  # UILOC.c
    render('sm_uiloc', start() + f'''
  rankdir=TB; nodesep=0.45; ranksep=0.5;
  ST [label="STATUS\\n2 pages, 3 s each"];
  ID [label="ASK_ID\\n\\"User ID:\\" + digits"];
  PIN [label="ASK_PIN\\n\\"PIN:\\" + ****"];
  MSG [label="MESSAGE\\n2 s (200 × 10 ms)"];
  MENU [label="MENU\\n1Lamps 2Dimmer 3AC 4Heat"];
  LA [label="LAMPS"]; DI [label="DIMMER"]; AC [label="AC (view)"]; HE [label="HEATER"];
  LK [label="LOCKED\\n\\"SYSTEM LOCKED\\"", color="{RED}", fillcolor="#FDECEC"];
  init -> ST;
  ST -> ID [label="any key\\n[IsLocalAllowed]\\n(digit kept)"];
  ST -> MSG [label="any key, blocked\\n\\"Keypad blocked\\""];
  ID -> PIN [label="= (≥1 digit)"];
  ID -> ST [label="back, empty"];
  PIN -> ID [label="back, empty"];
  PIN -> MENU [label="= → SEC_u8Login\\n= USER"];
  PIN -> MSG [label="FAILED\\n\\"n tries left\\""];
  MSG -> ST [label="2 s"]; MSG -> ID [label="2 s (after a failed login)", style=dashed];
  MENU -> LA [label="1"]; MENU -> DI [label="2"]; MENU -> AC [label="3"]; MENU -> HE [label="4"];
  {{LA DI AC HE}} -> MENU [label="* / C", style=dashed];
  MENU -> ST [label="* / C → logout"];
  {{rank=same; LA DI AC HE}}
  any [shape=plaintext, style="", label="any state:\\nALARM active → LOCKED\\n30 s no key → logout → STATUS\\nrole became NONE → STATUS", fontcolor="{MUTED}"];
  any -> LK [color="{RED}"];
''')


def sm_uirem():  # UIREM.c
    render('sm_uirem', start() + f'''
  rankdir=TB; nodesep=0.4; ranksep=0.45;
  AN [label="ASK_NAME"]; AP [label="ASK_PASS\\n(echo *)"]; PR [label="PROMPT\\n\\"name> \\""];
  ARG [label="ARG\\n(cmd 1, 2, 5, 11, 13)"]; ADN [label="ADD_NAME\\n(cmd 10, 12)"]; ADP [label="ADD_PASS\\n(echo *)"];
  NP [label="NEW_PASS\\n(cmd 15)"]; CF [label="CONFIRM\\n(cmd 16)"];
  LK [label="LOCKED\\ninput discarded", color="{RED}", fillcolor="#FDECEC"];
  init -> AN [label="queue: BANNER, ASK_NAME"];
  AN -> AN [label="Enter alone"];
  AN -> AP [label="line"];
  AP -> PR [label="SEC = ADMIN / USER\\nqueue WELCOME, [WARN], MENU, PROMPT"];
  AP -> AN [label="FAILED\\n[ERR] … attempts left"];
  PR -> PR [label="?, empty, 3, 4, 6, 14, 7, 8, 9\\nor 1/2/5/11/13 with argument"];
  PR -> ARG [label="1, 2, 5, 11, 13\\nno argument"];
  ARG -> PR [label="[OK] or [ERR] + menu"];
  PR -> ADN; ADN -> ADP [label="name valid"]; ADN -> PR [label="[ERR]"];
  ADP -> PR [label="USERDB result"];
  PR -> NP; NP -> PR;
  PR -> CF; CF -> PR [label="other: Cancelled."]; CF -> AN [label="YES: factory reset, logout"];
  PR -> AN [label="0 · 120 s idle\\nlogout"];
  any [shape=plaintext, style="", label="any state, every 5 ms:\\nALARM active → [ALERT] once → LOCKED", fontcolor="{MUTED}"];
  any -> LK [color="{RED}"];
''')


def sm_estore():  # ESTORE.c
    render('sm_estore', start() + f'''
  rankdir=LR;
  node [color="{SVC}", fillcolor="{SVC_L}"];
  I [label="IDLE"]; W [label="WRITING\\n(chip in write cycle)"]; F [label="FAULT\\nRAM image only", color="{RED}", fillcolor="#FDECEC"];
  init -> I [label="ESTORE_voidInit\\n(blocking 208-byte read)"];
  init -> F [label="read failed", style=dashed];
  I -> W [label="dirty page (highest first)\\nclear bit · WritePage(16 B)\\n≈ 1.7 ms on the bus"];
  I -> F [label="WritePage failed\\npost EVQ_STORAGE_FAULT"];
  W -> I [label="IsReady() = 1\\n(one ACK poll ≈ 0.1 ms)"];
  W -> W [label="not ready, ++polls < 5"];
  W -> F [label="polls == 5 (50 ms)\\npost EVQ_STORAGE_FAULT"];
  label="one step per call of ESTORE_voidUpdate (10 ms task); never waits";
  labelloc=b; fontsize=9; fontcolor="{MUTED}";
''')


def sm_light_door():
    render('sm_door', start() + f'''
  rankdir=LR;
  C [label="CLOSED\\nservo 0°"]; O [label="OPEN\\nservo 90°"];
  init -> C [label="DOOR_voidInit"];
  C -> O [label="SetState(OPEN)\\nreturn 1"];
  O -> C [label="SetState(CLOSED)\\nreturn 1"];
  C -> C [label="SetState(CLOSED): return 0", fontcolor="{MUTED}"];
  O -> O [label="SetState(OPEN): return 0", fontcolor="{MUTED}"];
''')


def sm_debounce():
    render('sm_debounce', start() + f'''
  rankdir=LR;
  node [color="{HAL}", fillcolor="{HAL_L}"];
  R [label="released\\n(debounced)"]; P [label="pressed\\n(debounced)"];
  init -> R;
  R -> R [label="raw ≠ level: ++samples < 3"];
  R -> P [label="3rd sample in a row ≠ level\\nPressEvent = 1"];
  P -> P [label="raw = level: samples = 0", fontcolor="{MUTED}"];
  P -> R [label="3 samples released\\nReleaseEvent = 1"];
''')


# ---------------- architecture ----------------
def callgraph():  # docs/architecture.md 1.2 , checked against the #includes of every .c
    body = """
  rankdir=TB; nodesep=0.12; ranksep=0.6; newrank=true; splines=true;
  node [fontsize=8.5, margin="0.05,0.03", height=0.3];
  edge [arrowsize=0.5, penwidth=0.7, color="#7B8794"];
"""
    rows = [
        ('APP · UI', APP, APP_L, ['UIREM', 'UILOC']),
        ('APP · logic', APP, APP_L, ['SEC', 'ALARM', 'LIGHT', 'DOOR', 'CLIMATE', 'HEATER']),
        ('SERVICE', SVC, SVC_L, ['SCHED', 'TERM', 'USERDB', 'ESTORE', 'EVQ', 'MAVG', 'FMT']),
        ('SERVICE util', SVC, SVC_L, ['RINGBUF']),
        ('HAL', HAL, HAL_L, ['LCD_BUF', 'KPAD', 'BUTTON', 'LM35', 'SEVEN_SEG', 'LAMP', 'RELAY', 'LED', 'BUZZER', 'DIMMER', 'SERVO', 'FAN', 'EXT_EEPROM']),
        ('HAL (chip behind chip)', HAL, HAL_L, ['CLCD', 'PCF8574']),
        ('MCAL', MCAL, MCAL_L, ['GIE', 'USART', 'TIMER2', 'DIO', 'ADC', 'TIMER0', 'TIMER1', 'TWI']),
    ]
    prev = None
    for i, (lab, c, f, mods) in enumerate(rows):
        body += f'  L{i} [shape=plaintext, style="", label="{lab}", fontcolor="{c}", fontsize=9];\n'
        for m in mods:
            body += f'  "{m}" [color="{c}", fillcolor="{f}"];\n'
        body += '  {rank=same; L%d; %s}\n' % (i, '; '.join('"%s"' % m for m in mods))
        if prev is not None:
            body += f'  L{prev} -> L{i} [style=invis];\n'
        prev = i
    edges = """UIREM:TERM UIREM:EVQ UIREM:SEC UIREM:USERDB UIREM:ESTORE UIREM:LIGHT UIREM:DOOR UIREM:CLIMATE UIREM:HEATER UIREM:ALARM UIREM:FMT
UILOC:LCD_BUF UILOC:KPAD UILOC:SEC UILOC:LIGHT UILOC:CLIMATE UILOC:HEATER UILOC:ALARM UILOC:FMT
SEC:USERDB SEC:ALARM SEC:EVQ ALARM:HEATER ALARM:CLIMATE ALARM:BUZZER ALARM:EVQ
LIGHT:LAMP LIGHT:DIMMER LIGHT:EVQ DOOR:SERVO CLIMATE:LM35 CLIMATE:MAVG CLIMATE:FAN CLIMATE:EVQ
HEATER:LM35 HEATER:MAVG HEATER:BUTTON HEATER:SEVEN_SEG HEATER:RELAY HEATER:LED HEATER:ESTORE HEATER:EVQ
USERDB:ESTORE ESTORE:EXT_EEPROM ESTORE:EVQ TERM:USART TERM:RINGBUF TERM:GIE EVQ:RINGBUF SCHED:TIMER2 SCHED:GIE
LCD_BUF:CLCD CLCD:PCF8574 LAMP:PCF8574 SEVEN_SEG:PCF8574 PCF8574:TWI EXT_EEPROM:TWI
KPAD:DIO BUTTON:DIO RELAY:DIO LED:DIO BUZZER:DIO LM35:ADC DIMMER:TIMER0 DIMMER:DIO SERVO:TIMER1 SERVO:DIO FAN:TIMER1 FAN:DIO""".split()
    for e in edges:
        a, b = e.split(':')
        st = ''
        if (a, b) in (('SCHED', 'TIMER2'), ('TERM', 'USART'), ('SCHED', 'GIE'), ('TERM', 'GIE')):
            st = f' [style=dashed, color="{SVC}", penwidth=1.0]'
        body += f'  "{a}" -> "{b}"{st};\n'
    render('callgraph', body)


def dataflow():
    body = f'''
  rankdir=LR; nodesep=0.25; ranksep=0.5;
  node [fontsize=9];
  subgraph cluster_in {{ label="inputs"; color="{MUTED}"; style=rounded; fontcolor="{MUTED}";
    kp [label="keypad", shape=box, color="{HAL}", fillcolor="{HAL_L}"]; ua [label="UART RX", color="{MCAL}", fillcolor="{MCAL_L}"];
    btn [label="heater buttons", color="{HAL}", fillcolor="{HAL_L}"]; lmw [label="LM35 water", color="{HAL}", fillcolor="{HAL_L}"]; lma [label="LM35 ambient", color="{HAL}", fillcolor="{HAL_L}"]; }}
  UILOC; UIREM; SEC; ALARM; LIGHT; DOOR; CLIMATE; HEATER;
  EVQ [color="{SVC}", fillcolor="{SVC_L}"]; USERDB [color="{SVC}", fillcolor="{SVC_L}"]; ESTORE [color="{SVC}", fillcolor="{SVC_L}"]; TERM [color="{SVC}", fillcolor="{SVC_L}"];
  subgraph cluster_out {{ label="outputs"; color="{MUTED}"; style=rounded; fontcolor="{MUTED}";
    lcd [label="LCD (I²C 0x27)", color="{HAL}", fillcolor="{HAL_L}"]; lamps [label="lamps (I²C 0x20)", color="{HAL}", fillcolor="{HAL_L}"];
    dim [label="dimmer PWM (T0)", color="{HAL}", fillcolor="{HAL_L}"]; srv [label="servo (T1 A)", color="{HAL}", fillcolor="{HAL_L}"]; fan [label="fan (T1 B)", color="{HAL}", fillcolor="{HAL_L}"];
    rel [label="heating / cooling", color="{HAL}", fillcolor="{HAL_L}"]; seg [label="7-seg (I²C 0x21/0x22)", color="{HAL}", fillcolor="{HAL_L}"];
    buz [label="buzzer", color="{HAL}", fillcolor="{HAL_L}"]; ee [label="24C08 (I²C 0x50)", color="{HAL}", fillcolor="{HAL_L}"]; ut [label="UART TX", color="{MCAL}", fillcolor="{MCAL_L}"]; }}
  kp -> UILOC; ua -> TERM -> UIREM; btn -> HEATER; lmw -> HEATER; lma -> CLIMATE;
  UILOC -> SEC; UIREM -> SEC; SEC -> USERDB -> ESTORE -> ee; SEC -> ALARM;
  UILOC -> LIGHT; UIREM -> LIGHT; UIREM -> DOOR; UILOC -> HEATER; UIREM -> HEATER;
  LIGHT -> lamps; LIGHT -> dim; DOOR -> srv; CLIMATE -> fan; HEATER -> rel; HEATER -> seg; HEATER -> ESTORE; ALARM -> buz;
  ALARM -> HEATER [label="ForceOff", color="{RED}", fontcolor="{RED}"]; ALARM -> CLIMATE [label="ForceOff", color="{RED}", fontcolor="{RED}"];
  UILOC -> lcd;
  {{LIGHT CLIMATE HEATER SEC ALARM ESTORE}} -> EVQ [style=dashed, color="{SVC}"];
  EVQ -> UIREM [label="[INFO] lines", style=dashed, color="{SVC}"]; UIREM -> TERM2 [style=invis];
  TERM2 [label="TERM", color="{SVC}", fillcolor="{SVC_L}"]; UIREM -> TERM2; TERM2 -> ut;
'''
    render('dataflow', body)


def i2c_bus():
    body = f'''
  rankdir=TB; nodesep=0.3; ranksep=0.5; splines=ortho;
  mcu [label="ATmega32 TWI master\\nPC0 = SCL · PC1 = SDA\\nTWBR = 72, TWPS = 0 → 100 kHz", color="{MCAL}", fillcolor="{MCAL_L}", width=3];
  bus [label="I²C bus · 4.7 kΩ pull-ups to +5 V · polled from the main loop only", shape=box, style="filled", fillcolor="#F5F7FA", color="{MUTED}", width=7.5];
  mcu -> bus [arrowhead=none, penwidth=2];
  lcd [label="PCF8574 · 0x27\\nA2 A1 A0 = 111\\nLCD 16×2 (RS RW E BL D4–D7)", color="{HAL}", fillcolor="{HAL_L}"];
  lam [label="PCF8574 · 0x20\\nA2 A1 A0 = 000\\nlamps 1–5 on P0–P4\\nactive low", color="{HAL}", fillcolor="{HAL_L}"];
  ten [label="PCF8574 · 0x21\\nA2 A1 A0 = 001\\n7-seg tens (a–g, dp)", color="{HAL}", fillcolor="{HAL_L}"];
  uni [label="PCF8574 · 0x22\\nA2 A1 A0 = 010\\n7-seg units", color="{HAL}", fillcolor="{HAL_L}"];
  ee [label="24C08 · 0x50 (0x50–0x53)\\nA2 = GND, WP = GND\\nonly block 0 used", color="{SVC}", fillcolor="{SVC_L}"];
  bus -> lcd [arrowhead=none, penwidth=1.6]; bus -> lam [arrowhead=none, penwidth=1.6]; bus -> ten [arrowhead=none, penwidth=1.6];
  bus -> uni [arrowhead=none, penwidth=1.6]; bus -> ee [arrowhead=none, penwidth=1.6];
  {{rank=same; lcd lam ten uni ee}}
'''
    render('i2c_bus', body)


def all_figs():
    for f in (sm_sec, sm_alarm, sm_climate, sm_heater, sm_heater_element, sm_uiloc, sm_uirem, sm_estore,
              sm_light_door, sm_debounce, callgraph, dataflow, i2c_bus):
        f()


if __name__ == '__main__':
    all_figs()
    print('dot figures ->', OUT)
