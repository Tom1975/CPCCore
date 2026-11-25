# Per-row edges of the paper of a SHAKER photo or capture. An edge counts only when the blue border is right
# next to it (a black HSYNC block or the tube edge next to the paper does not give its position) : -1 otherwise.
import numpy as np
from PIL import Image
def profile(path):
    a=np.asarray(Image.open(path).convert('RGB')).astype(float)
    r,g,b=a[...,0],a[...,1],a[...,2]
    blue=(b-r>110)&(b>170)
    k=np.ones(17)/17
    H,W=blue.shape
    L=np.full(H,-1); R=np.full(H,-1)
    for y in range(H):
        rs=np.convolve((r[y]+g[y])/2,k,mode="same")
        paper=(rs>60)&~blue[y]
        d=np.diff(np.concatenate(([0],paper.astype(int),[0])))
        st=np.nonzero(d==1)[0]; en=np.nonzero(d==-1)[0]
        runs=[(s,e) for s,e in zip(st,en) if e-s>=24]
        if not runs: continue
        x0=runs[0][0]; x1=runs[-1][1]
        if x1-x0<W*0.3: continue
        lb=blue[y,max(0,x0-14):max(0,x0-3)]
        rb=blue[y,min(W,x1+3):min(W,x1+14)]
        if len(lb) and lb.mean()>0.6: L[y]=x0
        if len(rb) and rb.mean()>0.6: R[y]=x1
    return L,R
