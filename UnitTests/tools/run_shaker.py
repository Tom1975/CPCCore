#!/usr/bin/env python3
"""Runs the SHAKER 2.7 suite on CPCCore and builds an HTML report.

Each of the 25 CSL scripts (modules A-E x CRTC 0-4) is one gtest case,
Shaker/ShakerSuite.MatchesReference/<module>_<crtc>, run in its own process so
that they run in parallel. Each case writes shaker_result.json and its
screenshots in its working directory; this script gathers them into
<work>/shaker_report/index.html.

SHAKER is a survey, not a list of tests that must pass. A reference is a screenshot validated
by hand against what the SHAKER site (https://shaker.logonsystem.eu) expects on a real CPC :
  OK          same as its validated reference
  KO          differs from its validated reference, or is no longer produced
  to validate no reference yet
The report lets you tick the screenshots that match SHAKER and gives the --accept command
that copies them (from the last run) into res/Shaker/reference.

Python 3.8+, standard library only : Linux, Windows, macOS.

  run_shaker.py --unittests <path to unitTests>                 run the 25 scripts, report
  run_shaker.py --unittests <path> --cases A_0,E_2              some cases only
  run_shaker.py --unittests <path> --report-only                report of the last run
  run_shaker.py --unittests <path> --accept A_0:0001,03FA B_2   validate screenshots of the last run
  run_shaker.py --unittests <path> --revoke A_0:0001            remove references
  run_shaker.py --unittests <path> --fetch-real                 download the real hardware photos

--fetch-real copies the SHAKER site results (api/tests + photos of a real CPC) into
<work>/shaker_real ; the report then shows each screenshot next to the photo of the same SSM code.

--accept / --revoke take <case> (every screenshot of the case) or <case>:<HHLL>,<HHLL>...

Exit code : 0 when no validated screenshot is KO, 1 otherwise.
"""

import argparse
import concurrent.futures
import datetime
import html
import json
import os
import shutil
import subprocess
import sys
import ssl
import time
import urllib.parse
import urllib.request
from pathlib import Path

MODULES = "ABCDE"
CRTCS = range(5)
SUITE = "Shaker/ShakerSuite.MatchesReference"
VERDICTS = ["same", "different", "new", "missing", "unreadable"]
KO = ["different", "missing", "unreadable"]
SHAKER_SITE = "https://shaker.logonsystem.eu"
# Part of a 1024 x 312 screenshot a real monitor shows (CPCCoreEmu/Monitor.cpp) : each line is
# 128 px of back porch (8 us), 832 displayable px (52 us), 64 px of line sync (4 us) ; 288 of the
# 312 lines are displayable, from line 16. The photos show this window only.
VISIBLE = (128, 16, 832, 288)   # x, y, width, height


def site_get(path):
    # The certificate of the site is not checked (proxies that rewrite it) : this host only,
    # public data only (pictures and test list)
    ctx = ssl.create_default_context()
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    req = urllib.request.Request(SHAKER_SITE + path, headers={"User-Agent": "CPCCore run_shaker"})
    with urllib.request.urlopen(req, context=ctx, timeout=60) as r:
        return r.read()


def real_name(test, sub, crtc):
    parts = [test["id"], f"CRTC{crtc}"] + ([sub["subfolder"]] if sub.get("subfolder") else []) + [sub["subTest"]]
    return "_".join(parts) + ".webp"


def fetch_real(real_dir, jobs):
    real_dir.mkdir(parents=True, exist_ok=True)
    tests = json.loads(site_get("/api/tests"))
    (real_dir / "tests.json").write_text(json.dumps(tests, indent=1), encoding="utf-8")
    names = [real_name(t, s, c) for t in tests for s in t.get("subtests", []) for c in s["crtcs"]]
    todo = [n for n in names if not (real_dir / n).exists()]
    print(f"{len(names)} real hardware photos, {len(todo)} to download", flush=True)

    def one(name):
        try:
            (real_dir / name).write_bytes(site_get("/images/cpc/CPC/" + name))
            return None
        except Exception as e:
            return f"{name} : {e}"
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        errors = [e for e in pool.map(one, todo) if e]
    for e in errors:
        print("  " + e)
    print(f"{len(todo) - len(errors)} downloaded in {real_dir}")


TRIAGE = {"ok": ("sûrement OK", "ok"), "doute": ("douteux", "warn"), "ko": ("sûrement KO", "bad")}


def load_triage(source, case):
    """Visual pre-sort of the screenshots against the real hardware photos : res/Shaker/triage/<case>.json,
    {HHLL: {verdict: ok|doute|ko, why}}. A hint for the validation, never a validation by itself."""
    f = source / "res" / "Shaker" / "triage" / f"{case}.json"
    return json.loads(f.read_text(encoding="utf-8")) if f.exists() else {}


def load_real(real_dir):
    """(crtc, HHLL) -> {file, label}"""
    f = real_dir / "tests.json"
    if not f.exists():
        return {}
    real = {}
    for t in json.loads(f.read_text(encoding="utf-8")):
        for s in t.get("subtests", []):
            if not s.get("hex"):
                continue
            sub = (s["subfolder"] + "/" if s.get("subfolder") else "") + s["subTest"]
            for c in s["crtcs"]:
                name = real_name(t, s, c)
                if (real_dir / name).exists():
                    real[(c, s["hex"].upper())] = {"file": name, "label": f"{t['id']} {t['name']} · {sub}"}
    return real


def all_cases():
    return [f"{m}_{c}" for m in MODULES for c in CRTCS]


def case_dir(work, case):
    # Same name as TestWorkspace gives the working directory of a test
    return work / f"Shaker_ShakerSuite.MatchesReference_{case}"


def run_case(binary, case, env, timeout):
    start = time.time()
    try:
        proc = subprocess.run(
            [str(binary), f"--gtest_filter={SUITE}/{case}"],
            cwd=str(binary.parent), env=env, timeout=timeout,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output = proc.stdout.decode("utf-8", "replace")
        code = proc.returncode
    except subprocess.TimeoutExpired as e:
        output = (e.stdout or b"").decode("utf-8", "replace") + f"\nTIMEOUT after {timeout} s"
        code = -1
    return case, code, time.time() - start, output


def load_results(work, cases):
    results = {}
    for case in cases:
        f = case_dir(work, case) / "shaker_result.json"
        if f.exists():
            try:
                results[case] = json.loads(f.read_text(encoding="utf-8"))
            except ValueError as e:
                results[case] = {"case": case, "script_ok": False, "error": f"unreadable result : {e}", "screenshots": []}
    return results


def counts(result):
    c = {v: 0 for v in VERDICTS}
    for s in result.get("screenshots", []):
        c[s["verdict"]] = c.get(s["verdict"], 0) + 1
    return c


def ko(c):
    return sum(c[v] for v in KO)


def reference_dir(source, case):
    return source / "res" / "Shaker" / "reference" / case[0] / case[2:]


def select_screens(work, specs):
    """<case>[:<HHLL>,...] -> [(case, png of the last run)]"""
    chosen = []
    for spec in specs:
        case, _, codes = spec.partition(":")
        r = load_results(work, [case]).get(case)
        if r is None:
            sys.exit(f"{case} : no result in {case_dir(work, case)}")
        screens = Path(r.get("screens_dir", ""))
        names = [s["name"] for s in r.get("screenshots", []) if s["verdict"] != "missing"]
        if codes:
            wanted = {f"SUGARBOX_{case[2:]}_{code.upper()}" for code in codes.split(",")}
            unknown = wanted - set(names)
            if unknown:
                sys.exit(f"{case} : not in the last run : {', '.join(sorted(unknown))}")
            names = [n for n in names if n in wanted]
        chosen += [(case, screens / (n + ".png")) for n in names]
    return chosen


def set_verdicts(work, case, names, verdict):
    """The reference now equals the screenshot of the last run (accept), or is gone (revoke) : the
    verdicts of that run follow, so that the report is right without running the script again."""
    f = case_dir(work, case) / "shaker_result.json"
    if not f.exists():
        return
    r = json.loads(f.read_text(encoding="utf-8"))
    shots = r.get("screenshots", [])
    for sh in shots:
        if sh["name"] in names:
            sh["verdict"], sh["pixels"] = verdict, (0 if verdict == "same" else -1)
    # A revoked reference the run did not produce : nothing left to show
    r["screenshots"] = [sh for sh in shots if not (sh["name"] in names and sh["verdict"] == "new"
                                                   and not (Path(r.get("screens_dir", "")) / (sh["name"] + ".png")).exists())]
    f.write_text(json.dumps(r, indent=2), encoding="utf-8")


def accept(work, source, specs):
    done = {}
    for case, png in select_screens(work, specs):
        d = reference_dir(source, case)
        d.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(png, d / png.name)
        done.setdefault(case, set()).add(png.stem)
        print(f"  validated {case} {png.stem}")
    for case, names in done.items():
        set_verdicts(work, case, names, "same")


def revoke(work, source, specs):
    for spec in specs:
        case, _, codes = spec.partition(":")
        d = reference_dir(source, case)
        files = sorted(d.glob("*.png")) if not codes else \
            [d / f"SUGARBOX_{case[2:]}_{code.upper()}.png" for code in codes.split(",")]
        for f in files:
            if f.exists():
                f.unlink()
                set_verdicts(work, case, {f.stem}, "new")
                print(f"  revoked {case} {f.stem}")
        for empty in (d, d.parent):   # <crtc>, then <module>
            if empty.is_dir() and not any(empty.iterdir()):
                empty.rmdir()


CSS = """
:root { --bg:#fff; --fg:#1d1d1f; --muted:#6e6e73; --line:#d2d2d7; --ok:#1a7f37; --bad:#c62828; --warn:#b26a00; --card:#f5f5f7; }
@media (prefers-color-scheme: dark) { :root { --bg:#161618; --fg:#f2f2f2; --muted:#a1a1a6; --line:#3a3a3c; --ok:#4cc26a; --bad:#ff6b6b; --warn:#f0b04a; --card:#202022; } }
body { background:var(--bg); color:var(--fg); font:14px/1.45 system-ui, sans-serif; margin:0 auto; max-width:1400px; padding:16px 16px 96px; }
h1 { font-size:22px; } h2 { font-size:18px; margin-top:32px; border-bottom:1px solid var(--line); padding-bottom:4px; }
table { border-collapse:collapse; } td, th { border:1px solid var(--line); padding:6px 10px; text-align:center; }
.ok { color:var(--ok); } .bad { color:var(--bad); } .warn { color:var(--warn); } .muted { color:var(--muted); }
.big { font-size:18px; }
.grid { display:grid; grid-template-columns:repeat(auto-fill, minmax(560px, 1fr)); gap:12px; }
.shot { background:var(--card); border-radius:8px; padding:8px; }
.shot label { cursor:pointer; }
.shot img { width:100%; image-rendering:pixelated; display:block; margin-top:4px; }
.crt { overflow:hidden; position:relative; margin-top:4px; }
.crt img.sb { position:absolute; margin:0; max-width:none; }
body.full .crt { overflow:visible; position:static; aspect-ratio:auto !important; }
body.full .crt img.sb { position:static; width:100% !important; }
.zwrap { position:relative; display:block; }
.zone { position:absolute; border:2px solid #ff2bd6; box-shadow:0 0 0 1px #000; pointer-events:none; }
.zone span { position:absolute; left:0; top:100%; background:#ff2bd6; color:#000; font:600 11px system-ui; padding:0 3px; white-space:nowrap; }
body.nozones .zone, body.full .crt .zone { display:none; }
.shot .pair { display:grid; grid-template-columns:1fr 1fr; gap:4px; }
.shot .pair img.real { aspect-ratio:auto; image-rendering:auto; }
.tag { font-size:12px; font-weight:600; text-transform:uppercase; }
pre { background:var(--card); padding:8px; overflow-x:auto; white-space:pre-wrap; }
a { color:inherit; }
#bar { position:fixed; left:0; right:0; bottom:0; background:var(--card); border-top:1px solid var(--line); padding:8px 16px; display:none; }
#bar code { display:block; overflow-x:auto; white-space:nowrap; margin-bottom:4px; }
"""

SCRIPT = """
// Ticking writes nothing : it builds the command shown at the bottom, which copies (--accept) or
// removes (--revoke) the references
const boxes = k => [...document.querySelectorAll(`input.${k}:checked`)];
function specs(k) {
  const byCase = {};
  for (const b of boxes(k)) (byCase[b.dataset.case] ||= []).push(b.dataset.code);
  return Object.entries(byCase).map(([c, codes]) => c + ':' + codes.join(','));
}
function refresh() {
  const a = specs('accept'), r = specs('revoke');
  document.getElementById('bar').style.display = a.length || r.length ? 'block' : 'none';
  document.getElementById('cmd').textContent = CMD + (a.length ? ' --accept ' + a.join(' ') : '') + (r.length ? ' --revoke ' + r.join(' ') : '');
  document.getElementById('count').textContent = `${boxes('accept').length} à valider, ${boxes('revoke').length} à retirer`;
}
document.addEventListener('change', e => { if (e.target.matches('input.accept, input.revoke')) refresh(); });
function copyCmd() { navigator.clipboard.writeText(document.getElementById('cmd').textContent); }
function tickTriage(c) { document.querySelectorAll(`input.accept[data-case='${c}']`).forEach(b => b.checked = b.dataset.triage === 'ok'); refresh(); }
// Only the window a real monitor shows (VISIBLE) : the screenshot is shifted and enlarged inside a
// box of the window's proportions, its lines doubled (the monitor draws each line twice)
function crt(img) {
  const [x, y, w, h] = VISIBLE, W = img.naturalWidth, H = img.naturalHeight;
  if (!W) return;
  img.closest('.crt').style.aspectRatio = w + ' / ' + (h * 2);
  img.style.width = (W / w * 100) + '%';
  img.style.left = (-x / w * 100) + '%';
  img.style.top = (-y / h * 100) + '%';
  img.style.aspectRatio = W + ' / ' + (H * 2);
}
document.querySelectorAll('.crt img.sb').forEach(i => i.complete ? crt(i) : i.addEventListener('load', () => crt(i)));
// Sort and filter of the screenshots, inside each case
const RANK_V = { different: 0, missing: 0, unreadable: 0, new: 2, same: 4 };
const RANK_T = { ko: 0, doute: 1, ok: 2, '': 1.5 };
function rank(d) { return RANK_V[d.dataset.verdict] < 1 ? 0 : d.dataset.verdict === 'same' ? 9 : 1 + RANK_T[d.dataset.triage]; }
function keep(d, f) {
  const realKo = RANK_V[d.dataset.verdict] < 1, t = d.dataset.triage;
  if (!f) return true;
  if (f === 'ko') return realKo || t === 'ko';
  if (f === 'doute') return realKo || t === 'ko' || t === 'doute';
  if (f === 'todo') return d.dataset.verdict === 'new';
  if (f === 'ok') return d.dataset.verdict === 'same';
}
function arrange() {
  const s = document.getElementById('sort').value, f = document.getElementById('filter').value;
  let n = 0, t = 0;
  document.querySelectorAll('.grid').forEach(g => {
    const shots = [...g.children];
    shots.sort((a, b) => s === 'code' ? a.dataset.code.localeCompare(b.dataset.code)
      : (rank(a) - rank(b)) || a.dataset.code.localeCompare(b.dataset.code));
    shots.forEach(d => { g.appendChild(d); const k = keep(d, f); d.style.display = k ? '' : 'none'; n += k; t++; });
  });
  document.getElementById('shown').textContent = `${n} / ${t} captures affichées`;
  try { localStorage.setItem('shaker_view', JSON.stringify([s, f])); } catch (e) {}
}
try { const [s, f] = JSON.parse(localStorage.getItem('shaker_view') || '[]'); if (s) sort.value = s; if (f !== undefined) filter.value = f; } catch (e) {}
document.getElementById('sort').onchange = document.getElementById('filter').onchange = arrange;
arrange();
function tickCase(c, on) { document.querySelectorAll(`input.accept[data-case='${c}']`).forEach(b => b.checked = on); refresh(); }
"""

# Opened by a click on a screenshot : the same window as the report (VISIBLE), lines doubled, as
# large as the browser window ; "zone complète" shows the whole screenshot
VIEWER = """<!doctype html><html lang='fr'><head><meta charset='utf-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'><title>Comparaison SHAKER</title>
<style>
html, body { height:100%; margin:0; }
body { background:#111; color:#ddd; font:13px system-ui, sans-serif; display:flex; flex-direction:column; }
#bar { padding:6px 12px; display:flex; gap:14px; flex-wrap:wrap; align-items:center; }
#panes { flex:1; display:flex; gap:6px; padding:0 6px 6px; min-height:0; }
@media (max-width:800px) { #panes { flex-direction:column; } }
.pane { flex:1; position:relative; background:#000; min-width:0; min-height:0; cursor:grab; }
.pane.drag { cursor:grabbing; }
.pane canvas { position:absolute; inset:0; width:100%; height:100%; }
.pane .t { position:absolute; left:6px; top:4px; background:#000a; padding:1px 6px; pointer-events:none; }
a { color:inherit; } button { font:inherit; }
</style></head><body>
<div id='bar'>
  <b id='title'></b>
  <label><input type='checkbox' id='link' checked> vues liées</label>
  <label><input type='checkbox' id='full'> zone complète Sugarbox</label>
  <label><input type='checkbox' id='zon' checked> zones du pré-tri (z)</label>
  <button id='reset'>Ajuster (0)</button>
  <button id='z1'>Pixel 1:1 (1)</button>
  <span style='opacity:.7'>molette : zoom · glisser : déplacer · délier pour caler une vue sur l'autre</span>
</div>
<div id='panes'>
  <div class='pane' id='p0'><canvas></canvas><span class='t'></span></div>
  <div class='pane' id='p1'><canvas></canvas><span class='t'></span></div>
</div>
<script>
const VISIBLE = __VISIBLE__;
const q = new URLSearchParams(location.search);
const clean = u => (u || '').split('?')[0];
document.getElementById('title').textContent = q.get('title') || '';
// Each view : an image, its source rectangle, the vertical scale of its pixels (2 : the monitor draws
// each line twice) ; zoom z and centre (cx, cy) in fractions of the source rectangle
let ZONES = []; try { ZONES = JSON.parse(q.get('zones') || '[]'); } catch (e) {}
const views = [
  { el: p0, src: clean(q.get('img')), sy: 2, smooth: false, label: 'Sugarbox', on: 'sb' },
  { el: p1, src: clean(q.get('real')), sy: 1, smooth: true, label: 'Machine réelle', on: 'real' },
].filter(v => v.src);
if (views.length < 2) p1.remove();
views.forEach(v => {
  v.cv = v.el.querySelector('canvas'); v.z = 1; v.cx = 0.5; v.cy = 0.5;
  v.el.querySelector('.t').innerHTML = `${v.label} · <a href='${v.src}' target='_blank' style='pointer-events:auto'>fichier ↗</a>`;
  v.img = new Image(); v.img.onload = () => { fit(v); draw(v); }; v.img.src = v.src + '?' + Date.now();
});
function rect(v) {
  const W = v.img.naturalWidth, H = v.img.naturalHeight;
  if (v.sy === 2 && !full.checked) return VISIBLE;
  return [0, 0, W, H];
}
// Base scale : the source rectangle fits the pane
function base(v) {
  const [, , w, h] = rect(v), r = v.el.getBoundingClientRect();
  return Math.min(r.width / w, r.height / (h * v.sy));
}
function fit(v) { v.z = 1; v.cx = 0.5; v.cy = 0.5; }
function draw(v) {
  if (!v.img.naturalWidth) return;
  const r = v.el.getBoundingClientRect(), dpr = devicePixelRatio || 1;
  v.cv.width = Math.round(r.width * dpr); v.cv.height = Math.round(r.height * dpr);
  const c = v.cv.getContext('2d'); c.setTransform(dpr, 0, 0, dpr, 0, 0);
  c.imageSmoothingEnabled = v.smooth && v.z * base(v) < 2;
  c.fillStyle = '#000'; c.fillRect(0, 0, r.width, r.height);
  const [x, y, w, h] = rect(v), k = base(v) * v.z;
  const dw = w * k, dh = h * v.sy * k;
  const ox = r.width / 2 - v.cx * dw, oy = r.height / 2 - v.cy * dh;
  c.drawImage(v.img, x, y, w, h, ox, oy, dw, dh);
  // Zones of the pre-sort : fractions of the visible window (Sugarbox) or of the photo
  if (zon.checked && !(v.on === 'sb' && full.checked)) {
    c.font = '600 12px system-ui'; c.lineWidth = 2;
    for (const z of ZONES.filter(z => z.on === v.on)) {
      const zx = ox + z.x * dw, zy = oy + z.y * dh, zw = z.w * dw, zh = z.h * dh;
      c.strokeStyle = '#000'; c.lineWidth = 4; c.strokeRect(zx, zy, zw, zh);
      c.strokeStyle = '#ff2bd6'; c.lineWidth = 2; c.strokeRect(zx, zy, zw, zh);
      if (z.note) { const tw = c.measureText(z.note).width + 6;
        c.fillStyle = '#ff2bd6'; c.fillRect(zx, zy + zh, tw, 16); c.fillStyle = '#000'; c.fillText(z.note, zx + 3, zy + zh + 12); }
    }
  }
}
const all = () => views.forEach(draw);
const peers = v => link.checked ? views : [v];
function zoomAt(v, f, px, py) {
  // the point under the cursor stays put, in every linked view (same fractions)
  const r = v.el.getBoundingClientRect(), [, , w, h] = rect(v), k = base(v) * v.z;
  const fx = v.cx + (px - r.width / 2) / (w * k), fy = v.cy + (py - r.height / 2) / (h * v.sy * k);
  for (const u of peers(v)) {
    const nz = Math.min(200, Math.max(0.2, u.z * f));
    u.cx = fx - (fx - u.cx) * u.z / nz; u.cy = fy - (fy - u.cy) * u.z / nz; u.z = nz;
  }
  all();
}
views.forEach(v => {
  v.el.addEventListener('wheel', e => { e.preventDefault(); const r = v.el.getBoundingClientRect();
    zoomAt(v, Math.exp(-e.deltaY * 0.0015), e.clientX - r.left, e.clientY - r.top); }, { passive: false });
  v.el.addEventListener('pointerdown', e => {
    if (e.target.tagName === 'A') return;
    v.el.setPointerCapture(e.pointerId); v.el.classList.add('drag');
    let lx = e.clientX, ly = e.clientY;
    const move = m => {
      const [, , w, h] = rect(v), k = base(v) * v.z;
      const dx = (m.clientX - lx) / (w * k), dy = (m.clientY - ly) / (h * v.sy * k);
      for (const u of peers(v)) { u.cx -= dx; u.cy -= dy; }
      lx = m.clientX; ly = m.clientY; all();
    };
    const up = () => { v.el.removeEventListener('pointermove', move); v.el.classList.remove('drag'); };
    v.el.addEventListener('pointermove', move); v.el.addEventListener('pointerup', up, { once: true });
  });
});
reset.onclick = () => { views.forEach(fit); all(); };
// 1:1 : one Sugarbox pixel per screen pixel (lines doubled), the photo follows at the same fraction
z1.onclick = () => { const v = views[0]; const f = 1 / (base(v) * v.z); peers(v).forEach(u => u.z *= f); all(); };
full.onchange = () => { views.forEach(fit); all(); };
zon.onchange = all;
if (!ZONES.length) zon.parentElement.style.display = 'none';
addEventListener('resize', all);
addEventListener('keydown', e => { if (e.key === 'z') { zon.checked = !zon.checked; all(); } if (e.key === '0') reset.onclick(); if (e.key === '1') z1.onclick();
  if (e.key === '+') zoomAt(views[0], 1.25, views[0].el.clientWidth / 2, views[0].el.clientHeight / 2);
  if (e.key === '-') zoomAt(views[0], 0.8, views[0].el.clientWidth / 2, views[0].el.clientHeight / 2); });
</script></body></html>
"""

LABELS = {"same": "OK", "different": "KO · différente", "missing": "KO · non produite",
          "unreadable": "KO · illisible", "new": "à valider"}


def build_report(work, cases, results, command, wall_seconds, real_dir, source):
    out = work / "shaker_report"
    if out.exists():
        shutil.rmtree(out)
    (out / "img").mkdir(parents=True)
    stamp = int(time.time())

    def copy(src, name):
        src = Path(src)
        if not src.exists():
            return None
        dst = out / "img" / name
        shutil.copyfile(src, dst)
        # Same name from one run to the next : the stamp keeps the browser from showing a cached one
        return f"img/{name}?{stamp}"

    real = load_real(real_dir)
    real_src = os.path.relpath(real_dir, out).replace(os.sep, "/")
    with_photo = sum(1 for case, r in results.items() for s in r.get("screenshots", [])
                     if (int(case[2:]), s["name"].rsplit("_", 1)[-1]) in real)

    total = {v: 0 for v in VERDICTS}
    for r in results.values():
        for k, v in counts(r).items():
            total[k] += v
    shots_total = sum(total.values())
    failed_scripts = [c for c in cases if c in results and not results[c].get("script_ok")]
    not_run = [c for c in cases if c not in results]

    h = []
    h.append("<!doctype html><html lang='fr'><head><meta charset='utf-8'>"
             "<meta name='viewport' content='width=device-width, initial-scale=1'>"
             f"<title>Rapport SHAKER</title><style>{CSS}</style></head><body>")
    h.append("<h1>SHAKER 2.7 — état des lieux CPCCore</h1>")
    h.append(f"<p class='muted'>{datetime.datetime.now():%Y-%m-%d %H:%M} · "
             f"{len(results)}/{len(cases)} scripts · {wall_seconds:.0f} s</p>")
    h.append(f"<p class='big'><span class='ok'><b>{total['same']}</b> OK</span> · "
             f"<span class='{'bad' if ko(total) else 'muted'}'><b>{ko(total)}</b> KO</span> · "
             f"<span class='warn'><b>{total['new']}</b> à valider</span> "
             f"<span class='muted'>sur {shots_total} captures</span></p>")
    h.append(f"<p><span class='{'warn' if failed_scripts else 'muted'}'><b>{len(failed_scripts)}</b> scripts interrompus"
             f"{' : ' + ', '.join(failed_scripts) if failed_scripts else ''}</span>")
    if not_run:
        h.append(f" · <span class='bad'><b>{len(not_run)}</b> sans résultat</span>")
    h.append("</p><p class='muted'>" + (f"{with_photo} captures ont leur photo sur machine réelle "
             f"(<a href='{SHAKER_SITE}/tests'>site SHAKER</a>, module E absent du site)."
             if real else "Pas de photos sur machine réelle : lancer avec --fetch-real.") + "</p>")
    h.append("<p class='muted'>OK : identique à la capture validée d'après le site SHAKER · "
             "KO : différente d'une capture validée, ou plus produite · à valider : pas encore de référence. "
             "Cocher les captures conformes à SHAKER, puis lancer la commande affichée en bas.</p>")
    h.append("<p><label><input type='checkbox' onchange='document.body.classList.toggle(\"full\", this.checked)'> "
             "captures Sugarbox en zone complète (synchros comprises) ; par défaut, seule la fenêtre visible "
             f"sur un moniteur ({VISIBLE[2]} × {VISIBLE[3]} lignes) est affichée, lignes doublées</label> · "
             "<label><input type='checkbox' id='zones' checked onchange='document.body.classList.toggle(\"nozones\", !this.checked)'> "
             "zones du pré-tri</label></p>")

    h.append("<p>Trier : <select id='sort'><option value='ko'>KO d'abord (réels, puis pré-tri), puis douteux</option>"
             "<option value='code'>code SSM</option></select> · Afficher : <select id='filter'>"
             "<option value=''>tout</option><option value='ko'>KO (réels ou pré-tri)</option>"
             "<option value='doute'>KO + douteux</option><option value='todo'>à valider</option>"
             "<option value='ok'>OK</option></select> <span id='shown' class='muted'></span></p>")

    # Matrix module x CRTC
    h.append("<table><tr><th>Module</th>" + "".join(f"<th>CRTC {c}</th>" for c in CRTCS) + "</tr>")
    for m in MODULES:
        h.append(f"<tr><th>{m}</th>")
        for c in CRTCS:
            case = f"{m}_{c}"
            if case not in cases:
                h.append("<td class='muted'>—</td>")
                continue
            r = results.get(case)
            if r is None:
                h.append(f"<td class='bad'><a href='#{case}'>pas de résultat</a></td>")
                continue
            n = counts(r)
            parts = [f"<span class='ok'>{n['same']} OK</span>"]
            if ko(n):
                parts.append(f"<span class='bad'>{ko(n)} KO</span>")
            if n["new"]:
                parts.append(f"<span class='warn'>{n['new']} ?</span>")
            txt = " · ".join(parts) + f" <span class='muted'>/ {sum(n.values())}</span>"
            if not r.get("script_ok"):
                txt += " ⚠"
            h.append(f"<td><a href='#{case}'>{txt}</a></td>")
        h.append("</tr>")
    h.append("</table><p class='muted'>? à valider · ⚠ script interrompu</p>")

    for case in cases:
        r = results.get(case)
        h.append(f"<h2 id='{case}'>Module {case[0]} · CRTC {case[2]}</h2>")
        if r is None:
            h.append("<p class='bad'>Aucun résultat (le test n'a pas tourné ou a planté).</p>")
            log = case_dir(work, case) / "gtest.log"
            if log.exists():
                h.append(f"<pre>{html.escape(log.read_text(encoding='utf-8', errors='replace')[-4000:])}</pre>")
            continue
        n = counts(r)
        triage = load_triage(source, case)
        h.append(f"<p>{html.escape(Path(r.get('script', '')).name)} · {r.get('emulated_seconds', 0):.0f} s émulées en "
                 f"{r.get('real_seconds', 0):.0f} s · {r.get('ssm_codes', 0)} codes SSM · "
                 f"{n['same']} OK · {ko(n)} KO · {n['new']} à valider")
        if n["new"]:
            h.append(f" · <a href='javascript:tickCase(\"{case}\", true)'>tout cocher</a> · "
                     f"<a href='javascript:tickCase(\"{case}\", false)'>tout décocher</a>")
            if triage:
                h.append(f" · <a href='javascript:tickTriage(\"{case}\")'>cocher les « sûrement OK »</a>")
        h.append("</p>")
        if triage:
            from collections import Counter
            t = Counter(x["verdict"] for x in triage.values())
            h.append("<p>Pré-tri visuel (comparaison à la photo, à confirmer) : "
                     + " · ".join(f"<span class='{TRIAGE[k][1]}'>{t[k]} {TRIAGE[k][0]}</span>" for k in TRIAGE if t[k])
                     + "</p>")
        if not r.get("script_ok"):
            h.append(f"<p class='warn'>Script interrompu : {html.escape(r.get('error', ''))}</p>")
            log = case_dir(work, case) / "csl.log"
            if log.exists():
                tail = log.read_text(encoding="utf-8", errors="replace").splitlines()[-12:]
                h.append("<pre>" + html.escape("\n".join(tail)) + "</pre>")
        if r.get("script_changes"):
            h.append("<p class='muted'>Script corrigé par le test : " + html.escape("; ".join(r["script_changes"])) + "</p>")
        screens = Path(r.get("screens_dir", ""))
        refs = Path(r.get("reference_dir", ""))
        order = {"different": 0, "missing": 1, "unreadable": 2, "new": 3, "same": 4}
        shots = sorted(r.get("screenshots", []), key=lambda s: (order.get(s["verdict"], 9), s["name"]))
        h.append("<div class='grid'>")
        for s in shots:
            name, v = s["name"], s["verdict"]
            code = name.rsplit("_", 1)[-1]
            photo = real.get((int(case[2:]), code))
            mine = copy(screens / (name + ".png"), f"{case}_{name}.png")
            ref = copy(refs / (name + ".png"), f"{case}_{name}.ref.png") if v in ("different", "missing") else None
            diff = copy(screens / (name + ".diff.png"), f"{case}_{name}.diff.png") if v == "different" else None
            tri = triage.get(code)
            zones = (tri or {}).get("zones", [])

            def zone_html(on):
                return "".join(f"<div class='zone' style='left:{z['x']*100:.1f}%;top:{z['y']*100:.1f}%;width:{z['w']*100:.1f}%;"
                               f"height:{z['h']*100:.1f}%'><span>{html.escape(z.get('note', ''))}</span></div>"
                               for z in zones if z.get("on") == on)
            photo_url = f"{real_src}/{photo['file']}" if photo else ""

            def view(img, what):
                title = f"{case} #{code} · {what}" + (f" · {photo['label']}" if photo else "")
                z = f"&zones={urllib.parse.quote(json.dumps(zones, ensure_ascii=False))}" if zones and img == mine else ""
                return (f"view.html?img={img}&real={photo_url}&title={urllib.parse.quote(title)}{z}").replace("'", "%27")
            if photo:
                test_id = photo["file"].split("_", 1)[0]
                page = f"{SHAKER_SITE}/tests?crtc={case[2:]}&test={test_id}&cpc_CPC=1"
                photo_html = (f"<div><span class='muted'>machine réelle · {html.escape(photo['label'])} · "
                              f"<a href='{real_src}/{photo['file']}' target='_blank'>ouvrir la photo ↗</a> · "
                              f"<a href='{html.escape(page)}' target='_blank'>site SHAKER ↗</a></span>"
                              f"<a href='{view(mine, 'Sugarbox')}' target='_blank'><span class='zwrap'><img class='real' loading='lazy' "
                              f"src='{real_src}/{photo['file']}'>{zone_html('real')}</span></a></div>")
            else:
                photo_html = "<div><span class='muted'>pas de photo sur machine réelle</span></div>"
            tri_html = (f"<div class='{TRIAGE[tri['verdict']][1]}'>pré-tri : <b>{TRIAGE[tri['verdict']][0]}</b> — "
                        f"{html.escape(tri.get('why', ''))}</div>") if tri and tri.get("verdict") in TRIAGE else ""
            cls = "ok" if v == "same" else ("warn" if v == "new" else "bad")
            label = LABELS.get(v, v) + (f" · {s['pixels']} px" if s.get("pixels", -1) > 0 else "")
            tv = tri["verdict"] if tri else ""
            h.append(f"<div class='shot' data-code='{html.escape(code)}' data-verdict='{v}' data-triage='{tv}'><div><b>#{html.escape(code)}</b> <span class='tag {cls}'>{html.escape(label)}</span>")
            if v in ("new", "different") and mine:
                what = "valider" if v == "new" else "remplacer la référence"
                h.append(f" <label><input type='checkbox' class='accept' data-case='{case}' data-code='{html.escape(code)}'"
                         f" data-triage='{tv}'> {what}</label>")
            elif v == "same":
                h.append(f" <label><input type='checkbox' class='revoke' data-case='{case}' data-code='{html.escape(code)}'>"
                         f" retirer la validation</label>")
            h.append("</div>" + tri_html)
            if v == "different":
                h.append(f"<div class='pair'><div><span class='muted'>obtenu</span><a href='{view(mine, "Sugarbox")}' target='_blank'><div class='crt'><img class='sb' loading='lazy' src='{mine}'></div></a></div>"
                         f"{photo_html}</div>")
                h.append(f"<div class='pair'><div><span class='muted'>référence validée</span><a href='{view(ref, "référence validée")}' target='_blank'><div class='crt'><img class='sb' loading='lazy' src='{ref}'></div></a></div>")
                h.append(f"<div><span class='muted'>différences</span><a href='{view(diff, "différences")}' target='_blank'><div class='crt'><img class='sb' loading='lazy' src='{diff}'></div></a></div></div>" if diff else "</div>")
            elif v == "missing":
                h.append(f"<div class='pair'><div><span class='muted'>référence (non produite)</span>"
                         f"<a href='{view(ref, "référence validée")}' target='_blank'><div class='crt'><img class='sb' loading='lazy' src='{ref}'></div></a></div>{photo_html}</div>")
            elif mine:
                h.append(f"<div class='pair'><div><span class='muted'>Sugarbox</span><a href='{view(mine, "Sugarbox")}' target='_blank'><div class='crt'><img class='sb' loading='lazy' src='{mine}'>{zone_html('sb')}</div></a></div>"
                         f"{photo_html}</div>")
            h.append("</div>")
        h.append("</div>")
    h.append("<div id='bar'><b id='count'></b> — rien n'est enregistré tant que cette commande n'est pas lancée "
             "(puis relancer les cas pour voir les nouveaux verdicts) :"
             "<code id='cmd'></code><button onclick='copyCmd()'>Copier</button></div>")
    h.append(f"<script>const CMD = {json.dumps(command)}; const VISIBLE = {json.dumps(VISIBLE)};{SCRIPT}</script>")
    h.append("</body></html>")
    (out / "index.html").write_text("\n".join(h), encoding="utf-8")
    (out / "view.html").write_text(VIEWER.replace("__VISIBLE__", json.dumps(VISIBLE)), encoding="utf-8")

    summary = {
        "cases": len(cases), "with_result": len(results), "totals": total,
        "ok": total["same"], "ko": ko(total), "to_validate": total["new"],
        "failed_scripts": failed_scripts, "not_run": not_run,
    }
    (out / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    return out / "index.html", summary


def main():
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unittests", required=True, type=Path, help="unitTests binary")
    ap.add_argument("--fixtures", type=Path, help="fixtures directory (default : <binary dir>/fixtures)")
    ap.add_argument("--source", type=Path, default=here.parent, help="UnitTests source directory (res/)")
    ap.add_argument("--work", type=Path, help="working directory of the tests (default : <binary dir>/test-work)")
    ap.add_argument("--cases", help="comma separated cases, e.g. A_0,E_2 (default : all)")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--timeout", type=int, default=3600, help="seconds per script")
    ap.add_argument("--report-only", action="store_true")
    ap.add_argument("--accept", nargs="+", metavar="CASE[:HHLL,...]",
                    help="validate screenshots of the last run (copied into the references), then rebuild the report")
    ap.add_argument("--revoke", nargs="+", metavar="CASE[:HHLL,...]", help="remove references")
    ap.add_argument("--fetch-real", action="store_true", help="download the real hardware photos of the SHAKER site")
    ap.add_argument("--real", type=Path, help="real hardware photos (default : <work>/shaker_real)")
    args = ap.parse_args()

    binary = args.unittests.resolve()
    fixtures = (args.fixtures or binary.parent / "fixtures").resolve()
    work = (args.work or binary.parent / "test-work").resolve()
    source = args.source.resolve()
    cases = args.cases.split(",") if args.cases else all_cases()
    real_dir = (args.real or work / "shaker_real").resolve()
    command = f"python3 {Path(__file__).resolve()} --unittests {binary}"

    if args.fetch_real:
        fetch_real(real_dir, 16)
        if not args.report_only:
            return 0

    if args.accept or args.revoke:
        if args.accept:
            accept(work, source, args.accept)
        if args.revoke:
            revoke(work, source, args.revoke)
        args.report_only = True   # the verdicts of the last run follow : rebuild the report

    start = time.time()
    if not args.report_only:
        if not binary.exists():
            sys.exit(f"{binary} not found")
        env = dict(os.environ)
        env["CPCCORE_SHAKER"] = "1"
        env["CPCCORE_SHAKER_REFERENCE"] = str(source / "res" / "Shaker" / "reference")
        env["CPCCORE_TEST_PATHS"] = f"FIXTURES={fixtures}|FIXTURES_EXTRA={source}|WORK={work}"
        # Longest first : better use of the workers
        order = sorted(cases, key=lambda c: "BDCAE".index(c[0]))
        print(f"{len(cases)} scripts, {args.jobs} in parallel", flush=True)
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futures = [pool.submit(run_case, binary, c, env, args.timeout) for c in order]
            for f in concurrent.futures.as_completed(futures):
                case, code, seconds, output = f.result()
                d = case_dir(work, case)
                d.mkdir(parents=True, exist_ok=True)
                (d / "gtest.log").write_text(output, encoding="utf-8")
                print(f"  {case}: {'ok' if code == 0 else 'KO'} ({seconds:.0f} s)", flush=True)

    results = load_results(work, cases)
    report, summary = build_report(work, cases, results, command, time.time() - start, real_dir, source)
    print(f"\n{summary['ok']} OK, {summary['ko']} KO, {summary['to_validate']} to validate; "
          f"{len(summary['failed_scripts'])} scripts stopped: {', '.join(summary['failed_scripts']) or '-'}")
    print(f"report : {report}")
    return 1 if summary["ko"] or summary["not_run"] else 0


if __name__ == "__main__":
    sys.exit(main())
