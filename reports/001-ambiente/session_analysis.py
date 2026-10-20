"""Session analysis: slow frames by cause and minute, pipeline sources (KYTY_PIPELINE_SOURCE_LOG), VRAM, LHM
temperatures. Prints Markdown.

  python -I -X utf8 session_analysis.py <emulator .out.log, copied after the emulator exited> <LHM csv folder>
                                        [<session start YYYY-mm-ddTHH:MM:SS> <session end>]
Without start and end, the session is the first to the last SLOW Frame."""
import csv, datetime as dt, glob, os, re, statistics as st, sys

LOG, LHM_DIR = sys.argv[1], sys.argv[2]
T0 = dt.datetime.fromisoformat(sys.argv[3]) if len(sys.argv) > 4 else None
T1 = dt.datetime.fromisoformat(sys.argv[4]) if len(sys.argv) > 4 else None

lines = open(LOG, encoding='utf-8', errors='replace').read().splitlines()

# --- slow frames (unix_ms gives local wall time) ---
frames = []
last_time = None
pipes = []          # (time, kind, ms)
vram = []           # (time or None, used, budget, plain, sysmem)
pipesrc = []        # (time, kind, static, build, unoptimized, ms, blob)
progsrc = []        # (time, label, translated, ms)
prefetch_done = None
for idx, l in enumerate(lines):
    m = re.search(r'SLOW Frame ([\d.]+) ms (.*)', l)
    if m:
        f = {'ms': float(m.group(1))}
        for k, v in re.findall(r'(\w+)=([\d.]+)', m.group(2)):
            f[k] = float(v)
        if 'unix_ms' in f:
            f['t'] = dt.datetime.fromtimestamp(f['unix_ms'] / 1000)
            last_time = f['t']
        frames.append(f)
        continue
    m = re.search(r'SLOW (Async|Create)(Graphics|Compute)Pipeline ([\d.]+) ms', l)
    if m:
        pipes.append((last_time, m.group(1) + m.group(2), float(m.group(3))))
        continue
    m = re.search(r'Video memory \(flip (\d+)\): (\d+) MiB in use of (\d+) MiB .*plain type (\d+) MiB in video memory.*GPU system memory (\d+) MiB', l)
    if m:
        vram.append((last_time, int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))))
        continue
    m = re.search(r'PIPESRC (\S+) key=\w+ static=(\d) build=(\d) unoptimized=(\d) ms=([\d.]+) blob=(\d+)', l)
    if m:
        pipesrc.append((last_time, m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4)), float(m.group(5)),
                        int(m.group(6))))
        continue
    m = re.search(r'PROGSRC (\S+) hash=\w+ translated=(\d) translate_ms=[\d.]+ ms=([\d.]+)', l)
    if m:
        progsrc.append((last_time, m.group(1), int(m.group(2)), float(m.group(3))))
        continue
    if l.startswith('Shader prefetch: all') and prefetch_done is None:
        prefetch_done = (last_time, l)

WAITS = ['compile', 'readback_wait', 'gpu_wait', 'download_wait', 'upload_wait', 'record_wait']


def cause(f):
    parts = {k: f.get(k, 0.0) for k in WAITS + ['idle']}
    k, v = max(parts.items(), key=lambda kv: kv[1])
    if v >= 0.5 * f['ms']:
        return {'idle': 'ociosa (esperando o jogo)'}.get(k, k)
    return 'misto/sem causa >50%'


if T0 is None:
    timed = [f['t'] for f in frames if 't' in f]
    T0, T1 = (min(timed), max(timed)) if timed else (dt.datetime.min, dt.datetime.max)
print(f"frames SLOW: {len(frames)}, com hora: {sum('t' in f for f in frames)}")
print(f"primeiro {min(f['t'] for f in frames if 't' in f)}  ultimo {max(f['t'] for f in frames if 't' in f)}")
ms = sorted(f['ms'] for f in frames)
print(f"ms: min {ms[0]:.1f} mediana {st.median(ms):.1f} p90 {ms[int(len(ms)*0.9)]:.1f} max {ms[-1]:.1f}; soma {sum(ms)/1000:.1f} s")

print('\n### Causa dominante (>= 50% do quadro)')
print('| Causa | Quadros | Tempo total (s) | Pior (ms) |')
print('|---|---:|---:|---:|')
by = {}
for f in frames:
    by.setdefault(cause(f), []).append(f['ms'])
for k, v in sorted(by.items(), key=lambda kv: -sum(kv[1])):
    print(f'| {k} | {len(v)} | {sum(v)/1000:.1f} | {max(v):.1f} |')
tot = {k: sum(f.get(k, 0.0) for f in frames) / 1000 for k in WAITS + ['idle', 'cpu']}
print('\nSoma de cada componente em todos os SLOW Frame (s): ' + ', '.join(f'{k} {v:.1f}' for k, v in tot.items() if v))

print('\n### Compilações de pipeline lentas (SLOW *Pipeline)')
kinds = {}
for _, k, v in pipes:
    kinds.setdefault(k, []).append(v)
for k, v in sorted(kinds.items()):
    print(f'- {k}: {len(v)}, mediana {st.median(v):.0f} ms, max {max(v):.0f} ms')

print('\n### Origem dos pipelines (PIPESRC: KYTY_PIPELINE_SOURCE_LOG=1)')
if not pipesrc:
    print('sem linhas PIPESRC (switch desligado nesta sessão)')
else:
    print('| Tipo | static=1 (pré-compilação) | static=0 | static=0 >= 100 ms | ms somados static=0 | com blob |')
    print('|---|---:|---:|---:|---:|---:|')
    for kind in sorted({p[1] for p in pipesrc}):
        ks = [p for p in pipesrc if p[1] == kind]
        hit = [p for p in ks if p[2] == 1]
        miss = [p for p in ks if p[2] == 0]
        print(f"| {kind} | {len(hit)} | {len(miss)} | {sum(1 for p in miss if p[5] >= 100)} | "
              f"{sum(p[5] for p in miss):.0f} | {sum(1 for p in ks if p[6] > 0)} |")
    builds = {}
    for p in pipesrc:
        builds[(p[3], p[4])] = builds.get((p[3], p[4]), 0) + 1
    print('\nbuild (0 full, 1 fast, 2 optimize), unoptimized: ' + ', '.join(f'{b}/{u}: {n}' for (b, u), n in sorted(builds.items())))
    if progsrc:
        tr = [p for p in progsrc if p[2] == 1]
        print(f'PROGSRC: {len(progsrc)} permutações, {len(tr)} traduzidas na hora ({sum(p[3] for p in tr):.0f} ms), '
              f'{len(progsrc) - len(tr)} adotadas do prefetch')

# --- LHM ---
ids = {'tctl': '/amdcpu/0/temperature/2', 'clk': '/amdcpu/0/clock/1', 'cpuload': '/amdcpu/0/load/0',
       'pkg': '/amdcpu/0/power/0', 'gpu_t': '/gpu-nvidia/0/temperature/0', 'gpu_hot': '/gpu-nvidia/0/temperature/2',
       'gpu_load': '/gpu-nvidia/0/load/0', 'gpu_clk': '/gpu-nvidia/0/clock/0', 'vram_used': '/gpu-nvidia/0/smalldata/1'}
samples = []
for p in glob.glob(os.path.join(LHM_DIR, 'LibreHardwareMonitorLog-*.csv')):
    rows = list(csv.reader(open(p, encoding='utf-8-sig', errors='replace')))
    if len(rows) < 3:
        continue
    head = rows[0]
    col = {k: head.index(v) for k, v in ids.items() if v in head}
    names = rows[1]
    for r in rows[2:]:
        try:
            t = dt.datetime.strptime(r[0], '%m/%d/%Y %H:%M:%S')
        except ValueError:
            continue
        s = {'t': t}
        for k, i in col.items():
            try:
                s[k] = float(r[i])
            except (ValueError, IndexError):
                pass
        samples.append(s)
    if '/gpu-nvidia/0/smalldata/1' in head:
        vram_name = names[head.index('/gpu-nvidia/0/smalldata/1')]
samples.sort(key=lambda s: s['t'])
sess = [s for s in samples if T0 <= s['t'] <= T1]


def agg(xs, k):
    v = [s[k] for s in xs if k in s and s[k] == s[k]]
    return (st.mean(v), max(v)) if v else (float('nan'), float('nan'))


if sess:
    print(f'\n### LHM na sessao: {len(sess)} amostras de {sess[0]["t"]:%H:%M:%S} a {sess[-1]["t"]:%H:%M:%S}')
else:
    print('\n### LHM na sessao: nenhuma amostra (LibreHardwareMonitor sem Log Sensors): temperaturas nao medidas')
for k in ids if sess else []:
    a, b = agg(sess, k)
    print(f'- {k}: media {a:.1f}, max {b:.1f}')
try:
    print('  (smalldata/1 =', vram_name, ')')
except NameError:
    pass
# throttling check: tctl >= 90 or clock drop
hot = [s for s in sess if s.get('tctl', 0) >= 85]
print(f'- amostras com Tctl >= 85 C: {len(hot)}; Tctl >= 90 C: {sum(1 for s in sess if s.get("tctl", 0) >= 90)}')

# --- per minute ---
print('\n### Por minuto')
print('| Minuto | SLOW Frame | Travado (s) | Causa principal | Pipelines lentos | Tctl média/máx °C | Clock CPU médio MHz | Carga CPU % | GPU °C | Carga GPU % |')
print('|---|---:|---:|---|---:|---|---:|---:|---:|---:|')
t = T0.replace(second=0)
while t <= T1:
    t2 = t + dt.timedelta(minutes=1)
    fr = [f for f in frames if 't' in f and t <= f['t'] < t2]
    pp = [p for p in pipes if p[0] and t <= p[0] < t2]
    ls = [s for s in samples if t <= s['t'] < t2]
    cs = {}
    for f in fr:
        cs[cause(f)] = cs.get(cause(f), 0) + f['ms']
    main = max(cs.items(), key=lambda kv: kv[1])[0] if cs else '-'
    ta, tm = agg(ls, 'tctl')
    print(f"| {t:%H:%M} | {len(fr)} | {sum(f['ms'] for f in fr)/1000:.1f} | {main} | {len(pp)} | {ta:.0f}/{tm:.0f} | "
          f"{agg(ls,'clk')[0]:.0f} | {agg(ls,'cpuload')[0]:.0f} | {agg(ls,'gpu_t')[0]:.0f} | {agg(ls,'gpu_load')[0]:.0f} |")
    t = t2

print('\n### Memoria de video (amostras do emulador)')
print('| Hora (aprox.) | flip | Em uso MiB | Budget MiB | Tipo "plain" na VRAM MiB | Memoria de sistema da GPU MiB |')
print('|---|---:|---:|---:|---:|---:|')
for tt, fl, u, b, pl, sm in vram:
    print(f"| {tt:%H:%M:%S} | {fl} | {u} | {b} | {pl} | {sm} |" if tt else f"| antes do 1o SLOW | {fl} | {u} | {b} | {pl} | {sm} |")
over = sum(1 for v in vram if v[2] > v[3])
print(f'\nacima do budget: {over} de {len(vram)}')

print('\n### Prefetch')
print(prefetch_done[0], prefetch_done[1][:160] if prefetch_done else None)

print('\n### 10 piores quadros')
for f in sorted(frames, key=lambda f: -f['ms'])[:10]:
    parts = ' '.join(f'{k}={f[k]:.0f}' for k in ['idle', 'cpu'] + WAITS if k in f)
    print(f"- {f.get('t', '?'):%H:%M:%S} {f['ms']:.0f} ms: {parts}")
