# Left / right paper edges of a T5 photo (green - red contrast) or Sugarbox capture, per CPC line of the paper.
import sys, numpy as np
from PIL import Image

def uniform_filter1d(a, n, axis=-1):
    a=np.moveaxis(np.asarray(a,dtype=float),axis,-1)
    k=np.ones(n)/n
    pad=n//2
    b=np.pad(a,[(0,0)]*(a.ndim-1)+[(pad,n-1-pad)],mode='edge')
    c=np.cumsum(b,axis=-1); c=np.concatenate([np.zeros(c.shape[:-1]+(1,)),c],axis=-1)
    r=(c[...,n:]-c[...,:-n])/n
    return np.moveaxis(r,-1,axis)

def paper_mask(path):
    a=np.asarray(Image.open(path).convert('RGB')).astype(float)
    if path.endswith('.png'):        # Sugarbox capture : paper white, text black, border elsewhere
        return (a[...,0]>200)&(a[...,1]>200)|((a.sum(2)<30)&False), True
    import os
    if os.environ.get('T5_BLUE'):
        bl=a[...,2]-(a[...,0]+a[...,1])/2
        bl=uniform_filter1d(uniform_filter1d(bl,15,axis=1),9,axis=0)
        H,W=bl.shape
        c=bl[H//10:9*H//10, W//10:9*W//10].ravel()
        h,edges_=np.histogram(c,256); mids=(edges_[:-1]+edges_[1:])/2
        best=None
        for t in range(1,255):
            w0=h[:t].sum(); w1=h[t:].sum()
            if w0==0 or w1==0: continue
            m0=(h[:t]*mids[:t]).sum()/w0; m1=(h[t:]*mids[t:]).sum()/w1
            v=w0*w1*(m0-m1)**2
            if best is None or v>best[0]: best=(v,mids[t])
        lum=uniform_filter1d(a.mean(2),15,axis=1)
        return (bl<best[1])&(lum>140), False
    y=a.mean(2)
    # text texture : local standard deviation of the luminance over 15 x 9 photo pixels
    m=uniform_filter1d(uniform_filter1d(y,15,axis=1),9,axis=0)
    m2=uniform_filter1d(uniform_filter1d(y*y,15,axis=1),9,axis=0)
    sd=np.sqrt(np.maximum(m2-m*m,0))
    H,W=sd.shape
    c=sd[H//3:2*H//3, W//3:2*W//3]
    thr=0.5*np.percentile(c,50)
    return sd>thr, False

def edges(mask, min_run):
    H,W=mask.shape
    L=np.full(H,np.nan); R=np.full(H,np.nan)
    for y in range(H):
        m=mask[y]
        # close the text holes : the paper is where the mask is set in most of a 2*min_run window
        f=uniform_filter1d(m.astype(float),min_run)>0.35
        xs=np.nonzero(f)[0]
        if len(xs)<W*0.25: continue
        # longest run
        d=np.diff(np.concatenate(([0],f.astype(int),[0])))
        st=np.nonzero(d==1)[0]; en=np.nonzero(d==-1)[0]
        runs=[(a_,b_) for a_,b_ in zip(st,en) if b_-a_>=W*0.04]
        if not runs: continue
        L[y]=runs[0][0]; R[y]=runs[-1][1]
    return L,R

if __name__=='__main__':
    path=sys.argv[1]
    mask,cap=paper_mask(path)
    L,R=edges(mask, 6 if cap else 40)
    w=R-L
    big=np.nanmax(uniform_filter1d(np.nan_to_num(w),51))
    ys=np.nonzero(np.nan_to_num(uniform_filter1d(np.nan_to_num(w),31))>0.5*big)[0]
    # the longest block of rows
    cut=np.nonzero(np.diff(ys)>20)[0]
    blocks=np.split(ys,cut+1); blk=max(blocks,key=len)
    top,bot=blk.min(),blk.max()
    nl=200
    out=[]
    for i in range(nl):
        y0=int(top+i*(bot-top+1)/nl); y1=int(top+(i+1)*(bot-top+1)/nl)
        l=np.nanmedian(L[y0:max(y1,y0+1)]); r=np.nanmedian(R[y0:max(y1,y0+1)])
        out.append((l,r))
    out=np.array(out)
    np.save(sys.argv[2],np.vstack([out,[[top,bot]]]))
    w=np.nanmedian(out[:,1]-out[:,0])
    print(path, 'top',top,'bot',bot,'width',w)
