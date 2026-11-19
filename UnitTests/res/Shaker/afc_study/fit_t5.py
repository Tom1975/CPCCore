# Score on the T5 photos (RealCpc, user's CTM644, 2026-11-18) : right edge of the paper of the 8 screens, Mode 2
# pixels, per line of the paper, relative to the lines before the band (t5_profiles.json, measured by
# t5_edges.py). The band start line is searched (96..107 for 64 lines, 26..37 for 200 lines) : the paper
# top of the photo is not exactly the line 0 of the display.
import json, os, numpy as np
import afc_model
HERE = os.path.dirname(os.path.abspath(__file__))
PROF = json.load(open(os.path.join(HERE, 't5_profiles.json')))['photo']
SCREENS = {  # photo -> (simulate kwargs, band lines)
 '1': (dict(starts=[0], widths=[0], wmain=[64], nomain=True), 64),
 '2': (dict(starts=[0], widths=[0], wmain=[64], nomain=True), 200),
 '4': (dict(starts=[211], widths=[64]), 200),
 '4b': (dict(starts=[211], widths=[64]), 200),
 '5': (dict(starts=[211], widths=[32], wmain=[64]), 64),
 '6': (dict(starts=[211], widths=[64], wmain=[32]), 64),
 '7': (dict(starts=[883], widths=[64]), 200),
 '8': (dict(starts=[211], widths=[64], more=[([387], [64])]), 64),
}
# Screen 3 left out : the top of its paper is hidden by a reflection, its lines are not placed.
KEEP = {'1': 80, '2': 160, '4': 160, '4b': 160, '5': 80, '6': 40, '7': 160, '8': 80}


def screens(p, show=False):
    res = {}
    for n, (kw, nb) in SCREENS.items():
        out, first = afc_model.simulate(nband=nb, **kw, **p)
        i0 = -first
        sim = out[0, i0 - 6: i0 + 170] - out[0, i0 - 6]
        ph = np.array([np.nan if v is None else v for v in PROF[n]])
        best = None
        for b in (range(96, 108) if nb == 64 else range(26, 38)):
            offs = np.arange(KEEP[n]); lines = b + offs
            ok = (lines < 196) & ~np.isnan(ph[np.clip(lines, 0, 199)])
            d = ph[lines[ok]] - sim[offs[ok] + 6]
            e = float(np.sqrt(np.mean(d * d)))
            if best is None or e < best[0]: best = (e, b)
        res[n] = best
        if show:
            b = best[1]; o = np.arange(0, KEEP[n], 8)
            print(n, 'photo', ' '.join(f"{ph[b + i]:5.0f}" for i in o if b + i < 196))
            print(n, 'sim  ', ' '.join(f"{sim[i + 6]:5.0f}" for i in o if b + i < 196), f"| rms {best[0]:.1f} @ {b}")
    return res


def score(p, show=False):
    r = screens(p, show)
    return float(np.sqrt(np.mean([v[0] ** 2 for v in r.values()])))
