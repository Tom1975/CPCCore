# Shift of each band line (CPC line index from the band start), photos of SHAKER D3, Mode 2 pixels
# profiles.py <this directory> <test work directory>
import sys, json, numpy as np
sys.path.insert(0, sys.argv[1]); from edges import profile
W=sys.argv[2]   # test work directory (holds shaker_real)
tests=json.load(open(W+'/shaker_real/tests.json'))
subs=[s for t in tests if t['id']=='D3' for s in t['subtests']]
PAPER=199; BAND0=82; NB=65            # capture : paper lines 64..262, band lines 146..210
def prof(path):
    L,R=profile(path)
    both=np.nonzero((L>0)&(R>0))[0]
    spans=np.split(both, np.nonzero(np.diff(both)>40)[0]+1)
    top=min(both); bot=max(both); h=bot-top
    yl=lambda i: top+(i+0.5)/PAPER*h
    out={}
    fits={}
    for name,E in (('L',L),('R',R)):
        ref=[y for y in list(range(int(yl(BAND0-25)),int(yl(BAND0-3))))+list(range(int(yl(BAND0+NB+3)),int(yl(BAND0+NB+25)))) if E[y]>0]
        if len(ref)<10: continue
        fits[name]=np.polyfit(ref,[E[y] for y in ref],2)
    if len(fits)<2: return None
    k=640.0/(np.polyval(fits['R'],yl(BAND0+NB/2))-np.polyval(fits['L'],yl(BAND0+NB/2)))
    res=[]
    for i in range(NB):
        y0,y1=int(yl(BAND0+i-0.5)),int(yl(BAND0+i+0.5))
        v=[]
        for name,E in (('L',L),('R',R)):
            ys=[y for y in range(y0,max(y1,y0+1)) if E[y]>0]
            if ys: v.append(np.median([E[y]-np.polyval(fits[name],y) for y in ys])*k)
        res.append(round(float(np.mean(v)),1) if v else None)
    return res
out={}
for crtc in range(5):
    for s in subs:
        try: out[f"{crtc}_{s['hex']}"]=prof(f"{W}/shaker_real/D3_CRTC{crtc}_{s['subTest']}.webp")
        except Exception as e: out[f"{crtc}_{s['hex']}"]=None
json.dump(out,open(sys.argv[1]+'/profiles.json','w'))
for s in subs:
    p=[out[f"{c}_{s['hex']}"] for c in range(3)]
    show=lambda a: ' '.join(f"{a[i]:5.0f}" if a and a[i] is not None else "    ." for i in range(2,65,6))
    print(s['hex'], s['subTest'].ljust(4), '|', show(p[0]), '|', show(p[1]))
