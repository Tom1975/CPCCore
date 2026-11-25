# Coordinate search from a start point : refine.py '<start json>' '<free names, comma separated>'
import sys, json, multiprocessing as mp, importlib, os
fit = importlib.import_module(os.environ.get('AFC_FIT', 'fit_all'))
start = json.loads(sys.argv[1]); free = sys.argv[2].split(',')
def ev(p):
    try: return fit.score(p)
    except Exception: return 1e9
if __name__ == '__main__':
    p = dict(start); best = ev(p); step = 0.2
    print('start', round(best, 3), flush=True)
    with mp.Pool(16) as pool:
        while step >= 0.01:
            cands = []
            for k in free:
                for sgn in (1, -1):
                    q = dict(p)
                    q[k] = p[k] * (1 + step) ** sgn if p[k] > 0 and k not in ('hold', 'hold_t5', 'delta') else p[k] + sgn * step * 2
                    cands.append(q)
            sc = pool.map(ev, cands)
            i = min(range(len(sc)), key=lambda j: sc[j])
            if sc[i] < best - 1e-4:
                best, p = sc[i], cands[i]
                print(round(best, 3), json.dumps({k: round(v, 5) for k, v in p.items()}), flush=True)
            else:
                step /= 2
                print('step', step, flush=True)
    print('final', round(best, 3), json.dumps({k: round(v, 5) for k, v in p.items()}))
