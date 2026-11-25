# Left and right edges of the paper on a SHAKER module A photo (yellow border), per photo row
import sys, numpy as np
from PIL import Image
def edges(path):
    a=np.asarray(Image.open(path).convert('RGB')).astype(float)
    r,g,b=a[...,0],a[...,1],a[...,2]
    yellow=(g-b>60)&(g>120)                  # border
    paper=(b>150)&(g>150)&(r>130)&~yellow
    H,W=paper.shape; L=np.full(H,-1.0); Rr=np.full(H,-1.0)
    k=np.ones(9)/9
    for y in range(H):
        p=np.convolve(paper[y].astype(float),k,mode='same')>0.5
        yb=np.convolve(yellow[y].astype(float),k,mode='same')>0.5
        d=np.diff(np.concatenate(([0],p.astype(int),[0])))
        st=np.nonzero(d==1)[0]; en=np.nonzero(d==-1)[0]
        runs=[(s,e) for s,e in zip(st,en) if e-s>=12]
        if not runs: continue
        x0=runs[0][0]; x1=runs[-1][1]
        if x1-x0<W*0.4: continue
        if yb[max(0,x0-16):max(1,x0-4)].mean()>0.6: L[y]=x0
        if yb[min(W-1,x1+4):min(W,x1+16)].mean()>0.6: Rr[y]=x1
    return L,Rr
if __name__=='__main__':
    L,Rr=edges(sys.argv[1])
    for y in range(0,len(L),12): print(y, L[y], Rr[y])
