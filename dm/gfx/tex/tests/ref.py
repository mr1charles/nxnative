import struct, io
from PIL import Image
FMT = {'bc1':(71,None),'bc2':(74,None),'bc3':(77,None),'bc4':(80,None),'bc5':(83,None),'bc7':(98,None),'bc6h':(95,None)}
def decode(fmt, data, w, h):
    dxgi = FMT[fmt][0]
    hdr = b'DDS ' + struct.pack('<7I', 124, 0x1007|0x80000, h, w, 0, 0, 1) + b'\0'*44
    hdr += struct.pack('<2I4s5I', 32, 4, b'DX10', 0,0,0,0,0) + struct.pack('<5I', 0x1000,0,0,0,0)
    hdr += struct.pack('<5I', dxgi, 3, 0, 1, 0)
    im = Image.open(io.BytesIO(hdr+data)); im.load()
    return im.convert('RGBA')
if __name__ == '__main__':
    import os
    d = os.urandom(16*4)
    im = decode('bc7', d, 8, 8); print(im.size, im.getpixel((0,0)))
