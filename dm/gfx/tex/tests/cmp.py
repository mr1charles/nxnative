import os, subprocess, struct, io, sys
import numpy as np
from PIL import Image
from ref import FMT
def refdec(fmt, data, w, h):
    dxgi = FMT[fmt][0]
    hdr = b'DDS ' + struct.pack('<7I', 124, 0x1007|0x80000, h, w, 0, 0, 1) + b'\0'*44
    hdr += struct.pack('<2I4s5I', 32, 4, b'DX10', 0,0,0,0,0) + struct.pack('<5I', 0x1000,0,0,0,0)
    hdr += struct.pack('<5I', dxgi, 3, 0, 1, 0)
    im = Image.open(io.BytesIO(hdr+data)); im.load()
    return im
if __name__!="__main__": raise SystemExit
bb = {'bc1':8,'bc2':16,'bc3':16,'bc4':8,'bc5':16,'bc7':16}
for fmt in bb:
    worst=0; bad=0; tot=0
    for (w,h) in [(64,64),(30,18),(4,4),(7,5)]:
        n=((w+3)//4)*((h+3)//4)
        for seed in range(20 if (w,h)==(64,64) else 5):
            rng=np.random.default_rng(seed*7+w)
            d=bytearray(rng.integers(0,256,n*bb[fmt],dtype=np.uint8).tobytes())
            if fmt=='bc7':
                for i in range(n):
                    if d[i*16]==0: d[i*16]=1<<(i%8)
            out=subprocess.run(['./bcdump',fmt,str(w),str(h)],input=bytes(d),capture_output=True).stdout
            im=refdec({'bc4':'bc4','bc5':'bc5'}.get(fmt,fmt),bytes(d),w,h)
            ref=np.array(im)
            ch={'bc4':1,'bc5':2}.get(fmt,4)
            mine=np.frombuffer(out,dtype=np.uint8).reshape(h,w,ch)
            if ref.ndim==2: ref=ref[:,:,None]
            if fmt=='bc5': ref=ref[:,:,:2]
            if ref.shape[2]==3 and ch==4: ref=np.concatenate([ref,np.full((h,w,1),255,np.uint8)],2)
            df=np.abs(mine.astype(int)-ref.astype(int)); worst=max(worst,df.max()); bad+=(df>1).any(axis=2).sum(); tot+=w*h
    print(fmt,'max diff',worst,'texels off by >1:',bad,'of',tot)
