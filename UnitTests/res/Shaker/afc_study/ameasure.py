import sys, json, numpy as np
sys.path.insert(0, sys.argv[1]); from aedge import edges
R=sys.argv[2]
names={'00BD':'AT_A','00BE':'AT_B','00BF':'AT_C','00C0':'AT_D','00C1':'AT_E','00C2':'AT_F','00C4':'AY_A','00C5':'AY_B','00C6':'AY_C','00C7':'AY_D','00C8':'AY_E','00C9':'AY_F','00CA':'AY_G','03AC':'AY_H'}
out={}
print("code crtc | décalage M2 au début (10 %), milieu, fin (90 %) de bande | lignes photo de la bande | largeur papier")
for code,n in names.items():
    for c in range(5):
        t,s=n.split('_')
        try: L,Rr=edges(f"{R}/{t}_CRTC{c}_{s}.webp")
        except Exception as e: print(code,c,'absent'); continue
        H=len(L); ys=np.arange(H)
        res={}
        for name,E in (('R',Rr),('L',L)):
            ok=E>0
            if ok.sum()<80: continue
            # band = rows where the edge leaves a first rough baseline (median of the top quarter rows)
            y_ok=ys[ok]; e_ok=E[ok]
            # iterative : quadratic fit on rows outside a candidate band, band = largest run of residual < -6 px
            mask=np.ones(len(y_ok),bool)
            for it in range(6):
                if mask.sum()<30: break
                co=np.polyfit(y_ok[mask],e_ok[mask],2)
                r=e_ok-np.polyval(co,y_ok)
                sm=np.array([np.median(r[max(0,i-4):i+5]) for i in range(len(r))])
                inb=sm<-5
                if not inb.any(): break
                idx=np.nonzero(inb)[0]
                # longest contiguous (in y) group
                groups=np.split(idx,np.nonzero(np.diff(y_ok[idx])>25)[0]+1)
                gbest=max(groups,key=len)
                y0,y1=y_ok[gbest[0]],y_ok[gbest[-1]]
                mask=(y_ok<y0-12)|(y_ok>y1+25)
            else:
                res[name]=(y0,y1,co,y_ok,r)
        if not res: print(code,c,'pas de bande mesurable'); continue
        name='R' if 'R' in res else 'L'
        y0,y1,co,y_ok,r=res[name]
        both=(L>0)&(Rr>0)&((ys<y0-12)|(ys>y1+25))
        width=np.median((Rr-L)[both]) if both.sum()>10 else np.nan
        k=640.0/width if width==width else np.nan
        f=lambda a,b:float(np.median(r[(y_ok>=y0+(y1-y0)*a)&(y_ok<=y0+(y1-y0)*b)]))*k
        v=(f(0.05,0.15),f(0.4,0.6),f(0.85,0.95))
        out[f"{c}_{code}"]=v
        print(code,c,name,'| %6.1f %6.1f %6.1f'%v,'| %d-%d'%(y0,y1),'| %.0f'%width)
json.dump(out,open(sys.argv[1]+'/a_shifts.json','w'))
