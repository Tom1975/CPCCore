import sys, json, random, multiprocessing as mp
import fit as fit4
SPACE = json.loads(sys.argv[1])       # name -> [lo, hi, 'log'|'lin'] or a fixed value
NR = int(sys.argv[2]); SEED = int(sys.argv[3]) if len(sys.argv) > 3 else 1
def ev(p):
    try: return (fit4.score(p), p)
    except Exception as e: return (1e9, p)
def draw(rnd):
    p = {}
    for k, v in SPACE.items():
        if not isinstance(v, list): p[k] = v
        elif v[2] == 'log': p[k] = 10 ** rnd.uniform(v[0], v[1])
        else: p[k] = rnd.uniform(v[0], v[1])
    return p
def refine(start):
    # coordinate search with shrinking steps on the free parameters
    best, p = start; step = 0.25
    free = [k for k, v in SPACE.items() if isinstance(v, list)]
    for it in range(25):
        moved = False
        for k in free:
            for sgn in (1, -1):
                q = dict(p)
                if SPACE[k][2] == 'log': q[k] = p[k] * (1 + step) ** sgn
                else: q[k] = p[k] + sgn * step * (SPACE[k][1] - SPACE[k][0]) * 0.2
                s = ev(q)[0]
                if s < best - 1e-4: best, p, moved = s, q, True; break
        if not moved:
            step /= 2
            if step < 0.01: break
    return best, p
if __name__ == '__main__':
    rnd = random.Random(SEED)
    with mp.Pool(16) as pool:
        res = pool.map(ev, [draw(rnd) for _ in range(NR)], chunksize=4)
        res.sort(key=lambda r: r[0])
        print('random best', round(res[0][0], 2))
        out = pool.map(refine, res[:16], chunksize=1)
    out.sort(key=lambda r: r[0])
    for s, p in out[:6]: print(round(s, 2), json.dumps({k: round(v, 5) for k, v in p.items()}))
