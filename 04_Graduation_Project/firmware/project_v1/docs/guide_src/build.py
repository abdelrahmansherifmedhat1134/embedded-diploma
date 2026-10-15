"""
build.py - generates docs/Technical_Guide.pdf

    python build.py              (figures + PDF)
    python build.py --no-figs    (PDF only)

Needs: Python 3.10+, matplotlib, graphviz (dot), playwright + chromium, pypdf.
Environment: FW_ROOT = firmware/project_v1 (default: two levels up from this file).

Macros in chapters/*.html
  {{ expr }}                         python expression (N = numbers from gnum.py, f() = number format)
  <x-code src=".." fn="name"/>       a whole function from the source file (must be <= 15 lines)
  <x-code src=".." lines="a-b"/>     a line range ; grep="text" n="k" = k lines from the first match
  <x-fig src="a.svg" w="90" label="computed|simavr|proteus|reused|code">caption</x-fig>
  <x-ph>what is missing</x-ph>       a clearly marked placeholder for a Proteus capture
"""
import os, re, sys, html, json, shutil, subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
FW = os.path.abspath(os.environ.get('FW_ROOT', os.path.join(HERE, '..', '..')))
os.environ['FW_ROOT'] = FW
sys.path.insert(0, HERE)
import gnum
from gnum import N

OUT = os.environ.get('GUIDE_OUT', os.path.join(HERE, 'out'))
IMG = os.path.join(OUT, 'images', 'guide')
os.environ['FIG_OUT'] = IMG
REPO_IMG = os.path.abspath(os.path.join(FW, '..', '..', 'presentation', 'images'))
ARCH_SVG = os.path.join(FW, 'docs', 'images', 'architecture.svg')


def f(v, nd=2):
    return gnum.fmt(v, nd)


# ------------------------------------------------------------------ macros
def read_src(rel):
    p = os.path.join(FW, rel)
    return open(p, encoding='latin-1').read().replace('\r\n', '\n').replace('\r', '\n').split('\n')


def code_block(attrs):
    rel = attrs['src']
    lines = read_src(rel)
    if 'fn' in attrs:
        name = attrs['fn']
        start = None
        for i, l in enumerate(lines):
            if re.match(r'^[A-Za-z_].*\b' + re.escape(name) + r'\s*\(', l) and not l.rstrip().endswith(';'):
                start = i
                break
        if start is None:
            raise ValueError(f'{rel}: function {name} not found')
        depth, end = 0, None
        for i in range(start, len(lines)):
            depth += lines[i].count('{') - lines[i].count('}')
            if '{' in ''.join(lines[start:i + 1]) and depth == 0:
                end = i
                break
        a, b = start + 1, end + 1
    elif 'grep' in attrs:
        pat = attrs['grep']
        a = next(i for i, l in enumerate(lines) if pat in l) + 1
        b = a + int(attrs.get('n', 10)) - 1
    else:
        a, b = map(int, attrs['lines'].split('-'))
    body = lines[a - 1:b]
    # drop blank lines inside short excerpts
    keep = [(a + k, l) for k, l in enumerate(body) if l.strip() != '']
    if len(keep) > 15:
        ERRS.append(f'excerpt {rel}:{a}-{b} has {len(keep)} lines (> 15)')
    out = []
    for no, l in keep:
        l = l.replace('\t', '    ')
        out.append(f'<span class="ln">{no:>3}</span>{html.escape(l)}')
    title = attrs.get('title', '')
    return (f'<figure class="code"><figcaption><span class="path">{html.escape(rel)}</span> · lines {a}–{b}'
            f'{" · " + html.escape(title) if title else ""}</figcaption><pre>' + '\n'.join(out) + '</pre></figure>')


FIGNO = {'ch': 0, 'n': 0}
ERRS = []
LABELS = {'computed': 'computed from the code and F_CPU', 'simavr': 'measured in simavr (cycle-accurate simulation of app.elf)',
          'proteus': 'measured in Proteus (repository capture)', 'reused': 'reused from the repository', 'code': 'drawn from the source code'}


def fig_block(attrs, caption):
    FIGNO['n'] += 1
    no = f'{FIGNO["ch"]}.{FIGNO["n"]}'
    src = attrs['src']
    w = attrs.get('w', '100')
    lab = attrs.get('label', 'code')
    path = src if src.startswith(('http', '/')) else f'images/guide/{src}'
    tag = f'<span class="flab flab-{lab}">{LABELS[lab]}</span>'
    return (f'<figure class="fig" style="width:{w}%"><img src="{path}"/>'
            f'<figcaption><b>Figure {no}</b> {caption} {tag}</figcaption></figure>')


def ph_block(text):
    return f'<div class="ph"><div class="ph-t">PLACEHOLDER — Proteus capture not supplied</div><div>{text}</div></div>'


def expand(txt):
    def ev(m):
        expr = m.group(1).strip()
        try:
            return str(eval(expr, {'N': N, 'f': f, 'R': N['SIM'], 'round': round, 'max': max, 'min': min, 'len': len, 'sum': sum}))
        except Exception as e:
            raise ValueError(f'bad expression {{{{ {expr} }}}}: {e}')
    txt = re.sub(r'\{\{(.+?)\}\}', ev, txt, flags=re.S)
    txt = re.sub(r'<x-code\s+([^>]*?)/>', lambda m: code_block(dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))), txt)

    def figrep(m):
        return fig_block(dict(re.findall(r'(\w+)="([^"]*)"', m.group(1))), m.group(2))
    out = []
    # figure numbering restarts per <h1>
    parts = re.split(r'(<h1[^>]*>)', txt)
    for p in parts:
        if p.startswith('<h1'):
            m = re.search(r'data-ch="(\w+)"', p)
            FIGNO['ch'] = m.group(1) if m else FIGNO['ch']
            FIGNO['n'] = 0
            out.append(p)
        else:
            p = re.sub(r'<x-fig\s+([^>]*)>(.*?)</x-fig>', figrep, p, flags=re.S)
            p = re.sub(r'<x-ph>(.*?)</x-ph>', lambda m: ph_block(m.group(1)), p, flags=re.S)
            out.append(p)
    return ''.join(out)


# ------------------------------------------------------------------ TOC
def slug(s):
    return re.sub(r'[^a-z0-9]+', '-', re.sub(r'<[^>]+>', '', s).lower()).strip('-')


def add_ids_and_toc(body, pages=None):
    entries = []

    def hrep(m):
        lvl, attrs, inner = m.group(1), m.group(2), m.group(3)
        if 'id=' in attrs:
            hid = re.search(r'id="([^"]+)"', attrs).group(1)
        else:
            hid = slug(inner)
            attrs += f' id="{hid}"'
        if 'notoc' not in attrs:
            entries.append((int(lvl), hid, re.sub(r'<[^>]+>', '', inner)))
        return f'<h{lvl}{attrs}>{inner}</h{lvl}>'
    body = re.sub(r'<h([123])([^>]*)>(.*?)</h\1>', hrep, body, flags=re.S)
    rows = []
    for lvl, hid, title in entries:
        if lvl > 2:
            continue
        pg = pages.get(title, '') if pages else '000'
        rows.append(f'<div class="toc{lvl}"><a href="#{hid}"><span class="tt">{html.escape(title)}</span>'
                    f'<span class="dots"></span><span class="pg">{pg}</span></a></div>')
    return body.replace('<!--TOC-->', '\n'.join(rows)), entries


# ------------------------------------------------------------------ PDF
HEADER = '''<div style="font-family:Inter,sans-serif;font-size:7px;color:#7B8794;width:100%;padding:0 16mm;display:flex;justify-content:space-between">
<span>ATmega32 Smart Home + Water Heater — Complete Technical Guide</span><span>v1.0</span></div>'''
FOOTER = '''<div style="font-family:Inter,sans-serif;font-size:7px;color:#7B8794;width:100%;padding:0 16mm;display:flex;justify-content:space-between">
<span>Abdelrahman Sherif Medhat · AMIT Embedded Systems Diploma</span><span><span class="pageNumber"></span> / <span class="totalPages"></span></span></div>'''


def render(html_path, pdf_path):
    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        b = p.chromium.launch()
        pg = b.new_page()
        pg.goto('file://' + html_path)
        pg.wait_for_load_state('networkidle')
        pg.pdf(path=pdf_path, format='A4', print_background=True, outline=True, tagged=True, display_header_footer=True,
               header_template=HEADER, footer_template=FOOTER, margin=dict(top='17mm', bottom='16mm', left='16mm', right='16mm'),
               prefer_css_page_size=True)
        b.close()


def outline_pages(pdf_path):
    import pypdf
    r = pypdf.PdfReader(pdf_path)
    res = {}

    def walk(o):
        for it in o:
            if isinstance(it, list):
                walk(it)
            else:
                t = it.title.strip()
                if t not in res:
                    res[t] = r.get_destination_page_number(it) + 1
    walk(r.outline)
    return res, len(r.pages)


def main():
    if '--no-figs' not in sys.argv:
        import figs_dot, figs_mpl, figs_sch
        figs_sch.all_figs()
        figs_dot.all_figs()
        figs_mpl.all_figs()
    os.makedirs(IMG, exist_ok=True)
    shutil.copy(ARCH_SVG, os.path.join(IMG, 'architecture.svg'))
    for n in os.listdir(REPO_IMG):
        shutil.copy(os.path.join(REPO_IMG, n), os.path.join(IMG, 'repo_' + n))
    chapters = sorted(x for x in os.listdir(os.path.join(HERE, 'chapters')) if x.endswith('.html'))
    body = '\n'.join(open(os.path.join(HERE, 'chapters', c), encoding='utf-8').read() for c in chapters)
    body = expand(body)
    if ERRS:
        raise SystemExit('\n'.join(ERRS))
    # chapter label outside the <h1>, so TOC entries and PDF bookmarks carry the title only
    body = re.sub(r'<h1([^>]*)><span class="num">(.*?)</span>', r'<div class="chap"><span class="num">\2</span></div><h1 class="ah"\1>', body)
    css = open(os.path.join(HERE, 'style.css')).read()

    def page(b):
        return f'<!doctype html><html lang="en"><head><meta charset="utf-8"><title>ATmega32 Smart Home + Water Heater — Complete Technical Guide</title><style>{css}</style></head><body>{b}</body></html>'
    html_path = os.path.join(OUT, 'guide.html')
    pdf_path = os.path.join(OUT, 'Technical_Guide.pdf')
    b1, entries = add_ids_and_toc(body)
    open(html_path, 'w').write(page(b1))
    render(html_path, pdf_path)
    pages, total = outline_pages(pdf_path)
    b2, _ = add_ids_and_toc(body, pages)
    open(html_path, 'w').write(page(b2))
    render(html_path, pdf_path)
    pages2, total = outline_pages(pdf_path)
    if pages2 != pages:
        print('warning: page numbers moved in pass 2 , one more pass')
        b3, _ = add_ids_and_toc(body, pages2)
        open(html_path, 'w').write(page(b3))
        render(html_path, pdf_path)
    print(f'{pdf_path}: {total} pages, {os.path.getsize(pdf_path) / 1e6:.2f} MB')


if __name__ == '__main__':
    main()
