# Joint score : the 44 D3 screens (hold) and the module A references (hold_a, another photo session).
# Module A, #00BD-#00C1, #00C4-#00C7, #00CA, #03AC : 2nd C-HSYNC of 64 px at 211, band of 88 lines, photo
# shift at 10 %, 50 %, 90 % of the band (a_shifts.json, median of the CRTC 0-2 photos).
import sys, json, numpy as np
import afc_model, fit
A_PHOTO = np.array([-12.9, -13.2, -12.4]); A_ROWS = (9, 44, 79); A_WEIGHT = 40.0


def module_a(p, hold_a, widths=(64,), wmain=64):
    q = dict(p); q['hold'] = hold_a
    out, first = afc_model.simulate([211] * len(widths), list(widths), wmain=[wmain] * len(widths), nband=88, **q)
    # ameasure.py takes its baseline away from the band (the rows where the picture comes back are left
    # out) : here, the line before the band
    i0 = -first
    return out[:, [i0 + r for r in A_ROWS]] - out[:, [i0 - 3]]


def score(params, show=False):
    p = dict(params); hold_a = p.pop('hold_a', p['hold'])
    sim, _ = fit.model(p)
    d = (sim - fit.PROF)[:, 1:64]; d = d[~np.isnan(d)]
    a = module_a(p, hold_a)[0] - A_PHOTO
    if show:
        print('D3 rms', round(float(np.sqrt(np.mean(d * d))), 2), '| module A sim', np.round(a + A_PHOTO, 1), 'photo', A_PHOTO)
        print('#00C8 (38 px) / #00C9 (50 px) sim', np.round(module_a(p, hold_a, (38, 50)), 1).tolist(),
              'photo [-3.4, -5.4, -7.3] / [-4.3, -10.0, -16.3]')
    return float(np.sqrt((np.sum(d * d) + A_WEIGHT * np.sum(a * a)) / (len(d) + A_WEIGHT * len(a))))


if __name__ == '__main__':
    print('rms', round(score(json.loads(sys.argv[1]), show=True), 2))
