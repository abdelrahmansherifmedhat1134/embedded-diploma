"""figs_sch.py - schematic fragments per HAL module (schemdraw -> SVG), drawn from docs/pin_map.md and the _cfg.h files."""
import os
import schemdraw
import schemdraw.elements as elm

OUT = os.environ.get('FIG_OUT', os.path.join(os.path.dirname(__file__), 'out', 'images', 'guide'))
os.makedirs(OUT, exist_ok=True)
schemdraw.config(font='Inter', fontsize=9, lw=1.1, color='#1F2933')
MCU = '#6B46C1'; HALC = '#C05621'


def save(d, name):
    d.save(os.path.join(OUT, name + '.svg'))


def ic(name, left, right, color='#1F2933', top=None):
    pins = [elm.IcPin(name=n, side='left') for n in left] + [elm.IcPin(name=n, side='right') for n in right]
    return elm.Ic(pins=pins, edgepadW=0.6, pinspacing=0.6, leadlen=0.4, color=color).label(name, 'top' if top is None else top)


def lcd():
    with schemdraw.Drawing(show=False) as d:
        U = d.add(elm.Ic(pins=[elm.IcPin(name='SCL', side='left'), elm.IcPin(name='SDA', side='left'), elm.IcPin(name='A0', side='left'),
                                elm.IcPin(name='A1', side='left'), elm.IcPin(name='A2', side='left'),
                                elm.IcPin(name='P0', side='right'), elm.IcPin(name='P1', side='right'), elm.IcPin(name='P2', side='right'),
                                elm.IcPin(name='P3', side='right'), elm.IcPin(name='P4', side='right'), elm.IcPin(name='P5', side='right'),
                                elm.IcPin(name='P6', side='right'), elm.IcPin(name='P7', side='right')],
                           pinspacing=0.55, edgepadW=0.6, leadlen=0.4, color=HALC).label('PCF8574 · 0x27', 'top'))
        d.add(elm.Line().left(0.9).at(U.SCL)); d.add(elm.Label().label('PC0 SCL', loc='left'))
        d.add(elm.Line().left(0.9).at(U.SDA)); d.add(elm.Label().label('PC1 SDA', loc='left'))
        for p in ('A0', 'A1', 'A2'):
            d.add(elm.Line().left(0.5).at(getattr(U, p)))
        d.add(elm.Line().at((U.A0[0] - 0.5, U.A0[1])).to((U.A2[0] - 0.5, U.A2[1])))
        d.add(elm.Label().at((U.A1[0] - 0.6, U.A1[1])).label('+5 V\n(A2 A1 A0 = 111)', loc='left', halign='right'))
        for p, t in (('P0', 'RS'), ('P1', 'RW (always 0)'), ('P2', 'E'), ('P3', 'backlight (not connected)'), ('P4', 'D4'), ('P5', 'D5'), ('P6', 'D6'), ('P7', 'D7')):
            d.add(elm.Line().right(1.4).at(getattr(U, p)).label(t, 'right'))
        d.add(elm.Label().at((U.P0[0] + 6.6, (U.P0[1] + U.P7[1]) / 2)).label('LM016L\n16×2 LCD\n(4-bit bus)'))
    save(d, 'sch_lcd')


def lamp():
    with schemdraw.Drawing(show=False) as d:
        d.add(elm.Vdd().label('+5 V'))
        d.add(elm.Resistor().down().label('220 Ω', 'bottom'))
        d.add(elm.LED().down().label('lamp n (LED)', 'bottom'))
        d.add(elm.Line().down(0.6))
        d.add(elm.Dot(open=True).label('PCF8574 0x20, Pn−1\n(LOW = lamp ON)', 'bottom'))
        d.move(4.5, 4)
        d.add(elm.Label().label('active low (D-18, C-2):\nthe PCF8574 can sink 25 mA but\nsources only ≈ 0.1 mA, and it powers\nup with every pin HIGH = all lamps off'))
    save(d, 'sch_lamp')


def sevenseg():
    with schemdraw.Drawing(show=False) as d:
        d.add(elm.Vdd().label('+5 V (common anode)'))
        d.add(elm.Line().down(0.5))
        top = d.here
        for i, s in enumerate('abcdefg'):
            d.add(elm.Line().at(top).right(1.0 * i).length(1.0 * i if i else 0.001))
            d.add(elm.LED().down().at((top[0] + 1.0 * i, top[1])).label('', 'bottom'))
            d.add(elm.Label().at((top[0] + 1.0 * i, top[1] + 0.35)).label(s))
            d.add(elm.Resistor().down().length(1.8).label('220 Ω' if i == 0 else '', 'top'))
            d.add(elm.Line().down(0.4))
            d.add(elm.Dot(open=True).label(f'P{i}', 'bottom'))
        d.move(1.2, -0.2)
        d.add(elm.Label().at((top[0] + 3.0, top[1] - 6.6)).label('PCF8574 0x21 (tens) / 0x22 (units) · P7 = dp, kept HIGH'))
    save(d, 'sch_7seg')


def button():
    with schemdraw.Drawing(show=False) as d:
        d.add(elm.Dot(open=True).label('PD6 / PD7 / PB5\n(input, internal pull-up)', 'left'))
        d.add(elm.Button().right().label('ON/OFF · Up · Down'))
        d.add(elm.Ground().right())
        d.move(-3.0, -1.6)
        d.add(elm.Label().label('released = HIGH (pull-up) · pressed = LOW\nBUTTON_PRESSED_LEVEL = DIO_PIN_LOW'))
    save(d, 'sch_button')


def lm35():
    with schemdraw.Drawing(show=False) as d:
        T = d.add(elm.Ic(pins=[elm.IcPin(name='+Vs', side='top'), elm.IcPin(name='VOUT', side='right'), elm.IcPin(name='GND', side='bottom')],
                         size=(1.8, 1.8), color=HALC).label('LM35', 'left'))
        d.add(elm.Vdd().at(T['+Vs']).label('+5 V'))
        d.add(elm.Ground().at(T.GND))
        d.add(elm.Line().right(1.5).at(T.VOUT).label('10 mV/°C', 'top'))
        d.add(elm.Dot(open=True).label('PA0 (ambient) / PA1 (water)', 'right'))
        d.move(-1.5, -3.0)
        d.add(elm.Label().label('ADC reference = internal 2.56 V → AREF pin left open (C-5)'))
    save(d, 'sch_lm35')


def outputs():
    with schemdraw.Drawing(show=False) as d:
        for x, (pin, led, lab) in enumerate((('PB6', 'LED-RED', 'heating element\n(stands for the SSR)'), ('PB7', 'LED-BLUE', 'cooling element'), ('PA3', 'LED', 'heater LED'))):
            d.add(elm.Dot(open=True).at((x * 3.6, 0)).label(pin, 'top'))
            d.add(elm.LED().down().label(led, 'bottom'))
            d.add(elm.Resistor().down().label('220 Ω', 'bottom'))
            d.add(elm.Ground())
            d.add(elm.Label().at((x * 3.6, -7.2)).label(lab))
        d.add(elm.Label().at((5.4, 1.4)).label('active HIGH (RELAY_ON_LEVEL, LED_ON_LEVEL = DIO_PIN_HIGH)'))
    save(d, 'sch_outputs')


def buzzer():
    with schemdraw.Drawing(show=False) as d:
        d.add(elm.Dot(open=True).label('PD3', 'left'))
        d.add(elm.Line().right(1.0))
        d.add(elm.RBox().down().label('buzzer\n3 V / 150 Ω', 'bottom'))
        d.add(elm.Ground())
        d.add(elm.Label().at((6.0, -1.5)).label('Proteus: direct on the pin (≈ 20–33 mA).\nReal board (pin_map C-9): PD3 → 1 kΩ → NPN base,\nbuzzer from +5 V to the collector, flyback diode.'))
    save(d, 'sch_buzzer')


def dimmer():
    with schemdraw.Drawing(show=False) as d:
        d.add(elm.Dot(open=True).label('PB3 / OC0\n7.8 kHz PWM', 'left'))
        d.add(elm.Resistor().right().label('R6 10 kΩ'))
        n = d.add(elm.Dot())
        d.add(elm.Capacitor().down().at(n.center).label('C1 10 µF', 'bottom'))
        d.add(elm.Ground())
        d.add(elm.Line().right(1.2).at(n.center))
        Q = d.add(elm.BjtNpn(circle=True).right().anchor('base').label('Q4 2N2222', 'right'))
        d.add(elm.Ground().at(Q.emitter))
        d.add(elm.Lamp().up().at(Q.collector).label('12 V lamp', 'bottom'))
        d.add(elm.Vdd().label('+12 V'))
        d.add(elm.Label().at((1.0, -4.2)).label('RC filter τ = R6·C1 = 100 ms → PWM becomes a 0–5 V level'))
    save(d, 'sch_dimmer')


def fan():
    with schemdraw.Drawing(show=False) as d:
        d.add(elm.Dot(open=True).label('PD4 / OC1B\n50 Hz PWM', 'left'))
        d.add(elm.Resistor().right().label('R3 1 kΩ'))
        Q = d.add(elm.BjtNpn(circle=True).right().anchor('base').label('Q1 2N2222', 'right'))
        d.add(elm.Ground().at(Q.emitter))
        d.add(elm.Line().up(0.4).at(Q.collector))
        top = d.here
        d.add(elm.Motor().up().label('DC fan', 'top'))
        d.add(elm.Vdd().label('motor supply'))
        d.add(elm.Diode().up().at((top[0] + 1.6, top[1])).reverse().label('1N4007', 'bottom'))
        d.add(elm.Line().left(1.6).at((top[0] + 1.6, top[1])))
        d.add(elm.Line().left(1.6).at((top[0] + 1.6, top[1] + 3)))
    save(d, 'sch_fan')


def servo():
    with schemdraw.Drawing(show=False) as d:
        S = d.add(elm.Ic(pins=[elm.IcPin(name='SIG', side='left'), elm.IcPin(name='+5V', side='top'), elm.IcPin(name='GND', side='bottom')],
                         size=(2.2, 1.8), color=HALC).label('door servo\n(MOTOR-PWMSERVO)', 'right'))
        d.add(elm.Line().left(1.6).at(S.SIG))
        d.add(elm.Dot(open=True).label('PD5 / OC1A\n1–2 ms every 20 ms', 'left'))
        d.add(elm.Vdd().at(S['+5V']).label('+5 V'))
        d.add(elm.Ground().at(S.GND))
    save(d, 'sch_servo')


def eeprom():
    with schemdraw.Drawing(show=False) as d:
        U = d.add(elm.Ic(pins=[elm.IcPin(name='A0', side='left'), elm.IcPin(name='A1', side='left'), elm.IcPin(name='A2', side='left'),
                                elm.IcPin(name='WP', side='right'), elm.IcPin(name='SCL', side='right'), elm.IcPin(name='SDA', side='right')],
                           pinspacing=0.7, edgepadW=0.8, color='#2F855A').label('24C08 (NM24C08)', 'top'))
        d.add(elm.Line().left(0.6).at(U.A2)); d.add(elm.Ground())
        d.add(elm.Label().at((U.A0[0] - 0.4, U.A0[1] + 0.2)).label('A0, A1 not used (block select)', 'left'))
        d.add(elm.Line().right(0.6).at(U.WP)); d.add(elm.Ground())
        d.add(elm.Line().right(2.2).at(U.SCL).label('SCL', 'right'))
        d.add(elm.Line().right(2.2).at(U.SDA).label('SDA', 'right'))
        d.add(elm.Resistor().up().at((U.SDA[0] + 1.6, U.SDA[1])).label('4.7 kΩ', 'bottom'))
        d.add(elm.Vdd().label('+5 V'))
        d.add(elm.Dot().at((U.SDA[0] + 1.6, U.SDA[1])))
        d.add(elm.Label().at((U.SCL[0] + 1.0, U.WP[1] - 1.4)).label('WP = GND: writes allowed (C-7) · A2 = GND → 0x50'))
    save(d, 'sch_eeprom')


def all_figs():
    for f in (lcd, lamp, sevenseg, button, lm35, outputs, buzzer, dimmer, fan, servo, eeprom):
        try:
            f()
        except Exception as e:
            print('schematic', f.__name__, 'failed:', e)
            raise


if __name__ == '__main__':
    all_figs()
    print('schematics ->', OUT)
