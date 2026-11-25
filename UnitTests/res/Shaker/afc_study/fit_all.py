# Joint score : SHAKER D3 + D8 (hold, the monitor of the SHAKER photos) and T5 (hold_t5, the user's monitor).
import sys, json, numpy as np
import fit_d8, fit_t5
W_T5 = 1.0


def score(params, show=False):
    p = dict(params); hold_t5 = p.pop('hold_t5')
    a = fit_d8.score(p, show)
    q = dict(p); q['hold'] = hold_t5
    b = fit_t5.score(q, show)
    if show: print('D3+D8', round(a, 2), '| T5', round(b, 2))
    return float(np.sqrt((a * a + W_T5 * b * b) / (1 + W_T5)))


if __name__ == '__main__':
    print('rms', round(score(json.loads(sys.argv[1]), show=True), 2))
