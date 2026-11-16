# AFC of the CTM644 (service manual page 26, IC401 = LA7800), physical model, all the screens at once.
# Units : pixel = 62.5 ns ; charge = sink current of the comparator during one pixel ; C413 = 1.
#  - pin 16 : flyback pulse integrated by R406 / C404 (sawtooth +-1) plus the pulse through C403 / C404 (step)
#  - phase comparator : differential pair keyed by the C-HSYNC, current = tanh((saw - delta) / eps), the
#    source side (saw above the reference) is 'rho' times the sink side
#  - pin 1 : the current goes into C413 (10 nF), shared with C414 (1 uF) through R418 (8.2 k) ; R417 (56 k)
#    leaks towards pin 2
#  - oscillator (pin 2) : period = N + hold + g * (mean voltage of pin 1 over the line) ; relaxation oscillator
#    (C415 charged through about 14 k) : a current pulse moves the end of the ramp all the more that it comes
#    late in the ramp, weight exp(-(time left) / isf_tau), ramp starting isf_u0 pixels after the flyback start
#  - hold : H-HOLD setting (VR403 / VR404), free running period minus 1024, in pixels per line
# The band (64 lines with a second C-HSYNC) comes back on every frame of 312 lines : periodic regime.
import numpy as np
N = 1024; F = 192; FRAME = 312; BAND0 = 150; NBAND = 64
PX = 62.5e-9
TA = 8.2e3 * 10e-9 / PX          # R418 * C413 : 1312 px
RATIO = 100.0                    # C414 / C413
TL = 56e3 * 10e-9 / PX           # R417 * C413 : 8960 px


def simulate(starts, widths, main=771, g=0.1, eps=0.05, delta=0.05, rho=1.0, hold=0.0, step=0.23,
             ta=TA, ratio=RATIO, tl=TL, frames=4, wmain=None, nband=NBAND, more=(), fly=192.0, isf_tau=0.0, isf_u0=0.0, osc_tau=0.0, osc_u0=0.0, lag=0):
    """starts, widths : second pulse of each screen (arrays). Returns shift[screen, i] (pixels, + = right)
    of the picture for the lines of the last frame, and the line of i = 0 relative to the band start."""
    s = np.asarray(starts, float); w = np.asarray(widths, int); C = len(s)
    wm = w if wmain is None else np.asarray(wmain, int)
    F = float(fly)
    off = step * F / N + delta

    def charge(t0, width, t_fb):         # charge of the pulses starting at t0 (absolute), flyback at t_fb
        k = np.arange(64)[None, :]
        u = (t0[:, None] + k + 0.5 - t_fb[:, None]) % N
        v = np.where(u < F, -1 + 2 * u / F + step, 1 - 2 * (u - F) / (N - F)) - off
        t = np.tanh(v / eps); t = np.where(t > 0, rho * t, t)
        if isf_tau:
            # relaxation oscillator (pin 2) : a current pulse moves the end of the ramp all the more that it
            # comes late in the ramp, exp(-(time left) / tau) ; isf_u0 = ramp start, pixels after the flyback start
            r = (u - isf_u0) % N
            t = t * np.exp((r - N) / isf_tau) * (N / isf_tau) / (1 - np.exp(-N / isf_tau))
        return (t * (k < width[:, None])).sum(axis=1)

    a = 1.0 / ta; l = 1.0 / tl if tl else 0.0
    lam, E = np.linalg.eig(np.array([[-a - l, a], [a / ratio, -a / ratio]])); Ei = np.linalg.inv(E)
    lam = lam.real; E = E.real; Ei = Ei.real

    def fstep(x, dt, left=None):         # x (C,2), dt (C,) -> x after dt, integral of V1
        c = x @ Ei.T
        e = np.exp(lam[None, :] * dt[:, None])
        if left is None:
            ie = np.where(np.abs(lam[None, :]) > 1e-12, (e - 1) / lam[None, :], dt[:, None])
        else:
            # pin 2 : the current of R417 is integrated by C415, which forgets with osc_tau ; 'left' is the
            # time between the start of this step and the end of the ramp
            b = lam[None, :] + 1.0 / osc_tau
            ie = np.exp(-left[:, None] / osc_tau) * (np.exp(b * dt[:, None]) - 1) / b
        return (e * c) @ E.T, (ie * c) @ E[0]

    total = frames * FRAME
    isband = np.zeros(total + 4, bool)
    for f in range(frames): isband[f * FRAME + BAND0: f * FRAME + BAND0 + nband] = True
    x = np.zeros((C, 2)); t_fb = np.full(C, float(N - 100)) + osc_u0   # t_fb : start of the oscillator ramp
    rec_t = []
    for k in range(total + 3):
        t_ramp = t_fb; t_fly = t_fb - osc_u0
        L0 = np.floor(t_fb / N).astype(int)
        te = []; q = []
        for dL in (0, 1):
            LL = L0 + dL
            ok_line = (LL >= 0) & (LL < total)
            t0 = LL * N + float(main)
            te.append(np.where(ok_line, t0 + wm / 2.0 - t_fb, 1e9)); q.append(charge(t0, wm, t_fly))
            for s_, w_ in [(s, w)] + [(np.asarray(a_, float), np.asarray(b_, int)) for a_, b_ in more]:
                t0 = LL * N + s_
                te.append(np.where(ok_line & isband[np.clip(LL, 0, total + 3)], t0 + w_ / 2.0 - t_fb, 1e9))
                q.append(charge(t0, w_, t_fly))
        te = np.stack(te, 1); q = np.stack(q, 1)
        te = np.where(te < 0, 1e9, te)
        order = np.argsort(te, axis=1)
        te = np.take_along_axis(te, order, 1); q = np.take_along_axis(q, order, 1)
        period = np.full(C, float(N)) if not (lag and k) else nxt
        for it in range(2):
            y = x; t = np.zeros(C); area = np.zeros(C)
            for j in range(te.shape[1]):
                hit = te[:, j] < period
                tj = np.where(hit, te[:, j], t)
                y, ar = fstep(y, tj - t, period - t if osc_tau else None); area += ar; t = tj
                y = y + np.stack([np.where(hit, q[:, j], 0.0), np.zeros(C)], 1)
            y, ar = fstep(y, period - t, period - t if osc_tau else None); area += ar
            norm = osc_tau * (1 - np.exp(-period / osc_tau)) if osc_tau else period
            nxt = np.clip(N + hold + g * area / norm, N - 64.0, N + 64.0)
            if lag:                      # lag : the period of a line is set by the line before (emulator)
                break
            period = nxt
        rec_t.append(t_fly.copy())
        x = y; t_fb = t_fb + period
    T = np.stack(rec_t, 1)               # flyback times (C, k)
    base = (frames - 1) * FRAME + BAND0
    out = np.zeros((C, FRAME))
    for i in range(FRAME):
        tgt = (base - 100 + i) * N - 100.0     # nominal flyback before the line drawn
        j = np.argmin(np.abs(T - tgt), axis=1)
        out[:, i] = -(T[np.arange(C), j] - tgt)
    return out, -100


def observe(out, first, kappa=1.0, n_photo=65, i0=0.0):
    """What profiles.py measures : shift minus a quadratic fitted on the rows -25..-3 and n+3..n+25."""
    xs = np.arange(out.shape[1]) + first
    idx = np.array(list(range(-25, -3)) + list(range(n_photo + 3, n_photo + 25)), float)
    i = np.arange(n_photo, dtype=float)
    res = np.zeros((out.shape[0], n_photo))
    for c in range(out.shape[0]):
        S = lambda z: np.interp(z * kappa + i0, xs, out[c])
        co = np.polyfit(idx, S(idx), 2)
        res[c] = S(i) - np.polyval(co, i)
    return res
