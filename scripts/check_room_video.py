"""Verify room pixels from BlastEm VDP readback; render captures without desktop access."""
from pathlib import Path
import struct
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'tests/out'
NAMES=['ship','west-lower-door','east-lower-door','west-upper-door','east-upper-door','blocks']


def word(data,address):
    return int.from_bytes(data[address:address+2],'big')


def tile_pixel(vram,attr,x,y):
    if attr&0x800:x=7-x
    if attr&0x1000:y=7-y
    value=vram[(attr&0x7ff)*32+y*4+x//2]
    value=(value&15) if x&1 else value>>4
    return ((attr>>9)&0x30)+value if value else 0


def plane(vram,base,width,height,hscroll,vscroll):
    result=np.zeros((224,256),dtype='uint8')
    priority=np.zeros((224,256),dtype=bool)
    for y in range(224):
        for x in range(256):
            px=(x-hscroll)%(width*8);py=(y+vscroll)%(height*8)
            attr=word(vram,base+(py//8*width+px//8)*2)
            result[y,x]=tile_pixel(vram,attr,px%8,py%8)
            priority[y,x]=bool(attr&0x8000)
    return result,priority


def sprites(vram,base):
    result=np.zeros((224,256),dtype='uint8')
    priority=np.zeros((224,256),dtype=bool)
    index=0;seen=set()
    while index not in seen:
        seen.add(index);entry=base+index*8
        y=(word(vram,entry)&1023)-128
        size=word(vram,entry+2);attr=word(vram,entry+4)
        x=(word(vram,entry+6)&511)-128
        width=((size>>10)&3)+1;height=((size>>8)&3)+1
        for sy in range(height*8):
            for sx in range(width*8):
                xx=x+sx;yy=y+sy
                if not(0<=xx<256 and 0<=yy<224) or result[yy,xx]:continue
                tx=width*8-1-sx if attr&0x800 else sx
                ty=height*8-1-sy if attr&0x1000 else sy
                tile_attr=(attr&0xE000)|((attr&0x7ff)+(tx//8)*height+ty//8)
                pixel=tile_pixel(vram,tile_attr,tx%8,ty%8)
                if pixel:
                    result[yy,xx]=pixel;priority[yy,xx]=bool(attr&0x8000)
        index=size&127
        if not index:break
    return result,priority


def main():
    headers=(OUT/'scene-headers.bin').read_bytes()
    data=(OUT/'scene-vram.bin').read_bytes()
    assert len(headers)==6*152 and len(data)==6*65536,'Missing VDP readback'
    fg=Image.open(ROOT/'res/images/maps/landing_site/foreground.png')
    bg=np.array(Image.open(ROOT/'res/images/maps/landing_site/background.png'))
    for i,name in enumerate(NAMES):
        h=struct.unpack('>76H',headers[i*152:(i+1)*152]);vram=data[i*65536:(i+1)*65536]
        colors=np.array([tuple(((c>>s)&7)*32 for s in (1,5,9)) for c in h[:64]],dtype='uint8')
        a,ap=plane(vram,h[64],h[66],h[67],word(vram,h[72]),h[73])
        b,bp=plane(vram,h[65],h[66],h[67],word(vram,h[72]+2),h[74])
        expected=np.array(fg.crop((h[68],h[69],h[68]+256,h[69]+224)))
        # Transparent pixels have no palette identity in the VDP.
        expected[(expected&15)==0]=0
        mismatch=int(np.count_nonzero(a!=expected))
        if mismatch:
            Image.fromarray(colors[a]).save(OUT/f'blastem-{name}-plane.png')
        assert not mismatch,f'{name}: {mismatch} foreground pixels differ from source map (camera {h[68:70]})'
        expected_bg=bg[(np.arange(224)+h[69])%512,:]+16
        expected_bg[(expected_bg&15)==0]=0
        assert np.array_equal(b,expected_bg),name+': sky scroll/palette mismatch'
        # Check every CRAM entry used in the scene against the generated palette.
        source_palette=np.array(fg.getpalette()[:192],dtype='uint8').reshape(64,3)
        for pixel in np.unique(np.concatenate((a.ravel(),b.ravel()))):
            if pixel:assert np.array_equal(colors[pixel],source_palette[pixel]),f'{name}: CRAM {pixel}'
        s,sp=sprites(vram,h[70])
        composed=np.zeros((224,256),dtype='uint8')
        for pixels,pri,want_high in [(b,bp,False),(a,ap,False),(s,sp,False),
                                     (b,bp,True),(a,ap,True),(s,sp,True)]:
            take=(pixels!=0)&(pri==want_high);composed[take]=pixels[take]
        Image.fromarray(colors[composed]).resize((768,672),Image.Resampling.NEAREST).save(OUT/f'blastem-{name}.png')
        print(f'PASS VDP {name}: foreground, palette and sky at {h[68]},{h[69]}')


if __name__=='__main__':main()
