import sys, json, numpy as np
import afc_model as afc5
P = json.load(open('profiles.json'))
codes = []; S = []; W = []; PROF = []
for line in open('pulses.txt'):
    c, s, w = line.split()
    prof = []
    for i in range(65):
        v = [P[f'{k}_{c}'][i] for k in range(3) if P.get(f'{k}_{c}') and P[f'{k}_{c}'][i] is not None]
        prof.append(np.mean(v) if v else np.nan)
    codes.append(c); S.append(int(s)); W.append(int(w)); PROF.append(prof)
PROF = np.array(PROF)


def model(params):
    p = dict(params); kappa = p.pop('kappa', 1.0); i0 = p.pop('i0', 2.0)
    out, first = afc5.simulate(S, W, **p)
    return afc5.observe(out, first, kappa, i0=i0), out


def score(params, show=False):
    sim, _ = model(params)
    d = (sim - PROF)[:, 1:64]
    if show:
        cols = (1, 2, 3, 4, 6, 8, 12, 24, 36, 48, 60)
        for n, c in enumerate(codes):
            print(c, f"rel {(S[n]-771)%1024:4d} w {W[n]:2d}", 'photo', ' '.join(f"{PROF[n][i]:4.0f}" for i in cols),
                  '| sim', ' '.join(f"{sim[n][i]:4.0f}" for i in cols))
    return float(np.sqrt(np.nanmean(d * d)))


if __name__ == '__main__':
    import time; t = time.time()
    p = json.loads(sys.argv[1]); print('rms', round(score(p, show=len(sys.argv) > 2), 2), round(time.time() - t, 2), 's')
