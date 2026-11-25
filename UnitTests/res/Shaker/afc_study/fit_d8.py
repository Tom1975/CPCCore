# Score on the 44 D3 screens and on SHAKER D8 "NO HSYNC FOR XX LINES" (#0392, 64 lines without C-HSYNC,
# same module as D3) : shift of the text rows of the photo (CRTC 0), in Mode 2 pixels, at L lines after
# the start of the band. The oscillator runs free : the drift gives the H-HOLD of the session.
import sys, json, numpy as np
import afc_model, fit
D8_LINES = np.array([2, 6, 10, 14, 18, 24, 28, 40, 46, 50])
D8_PHOTO = np.array([-1.5, -2.2, -3.6, -4.4, -5.4, -6.9, -7.3, -9.3, -9.7, -9.8])
D8_WEIGHT = 30.0


def d8(p):
    out, first = afc_model.simulate([0], [0], wmain=[64], nomain=True, **p)
    i0 = -first
    return out[0, i0 + D8_LINES] - out[0, i0 - 3]


def score(params, show=False):
    p = dict(params)
    sim, _ = fit.model(p)
    d = (sim - fit.PROF)[:, 1:64]; d = d[~np.isnan(d)]
    p.pop('kappa', None); p.pop('i0', None)
    a = d8(p) - D8_PHOTO
    if show:
        print('D3 rms', round(float(np.sqrt(np.mean(d * d))), 2), '| D8 rms', round(float(np.sqrt(np.mean(a * a))), 2))
        print('D8 sim', np.round(a + D8_PHOTO, 1).tolist()); print('D8 photo', D8_PHOTO.tolist())
    return float(np.sqrt((np.sum(d * d) + D8_WEIGHT * np.sum(a * a)) / (len(d) + D8_WEIGHT * len(a))))


if __name__ == '__main__':
    print('rms', round(score(json.loads(sys.argv[1]), show=True), 2))
