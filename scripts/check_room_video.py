"""Verify room pixels from BlastEm VDP readback; render captures without desktop access."""
from pathlib import Path
import struct
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'tests/out'
NAMES=['ship','west-lower-door','east-lower-door','west-upper-door','east-upper-door','blocks','west-open-overlap','east-open-overlap','zoomer','west-exit-overlap','east-exit-overlap']


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


def sprites(vram,base,only_palette=None):
    result=np.zeros((224,256),dtype='uint8')
    priority=np.zeros((224,256),dtype=bool)
    index=0;seen=set()
    while index not in seen:
        seen.add(index);entry=base+index*8
        y=(word(vram,entry)&1023)-128
        size=word(vram,entry+2);attr=word(vram,entry+4)
        x=(word(vram,entry+6)&511)-128
        width=((size>>10)&3)+1;height=((size>>8)&3)+1
        for sy in range(height*8) if only_palette is None or ((attr >> 13) & 3) == only_palette else []:
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
    assert len(headers)==len(NAMES)*152 and len(data)==len(NAMES)*65536,'Missing VDP readback'
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
        # Only the two-tile collars around the upper/lower frame edges are high.
        yy,xx=np.indices((224,256));xx+=h[68];yy+=h[69]
        lips=np.zeros((224,256),dtype=bool)
        for dx,dy in ((0,608),(0,1120),(2272,352),(2272,1120)):
            lips |= (xx>=dx)&(xx<dx+32)&(((yy>=dy-8)&(yy<dy+8))|((yy>=dy+56)&(yy<dy+72)))
        assert np.array_equal(ap,lips),name+': terrain priority escaped door collars'
        expected_bg=bg[(np.arange(224)+h[69])%512,:]+16
        expected_bg[(expected_bg&15)==0]=0
        assert np.array_equal(b,expected_bg),name+': sky scroll/palette mismatch'
        # Check every CRAM entry used in the scene against the generated palette.
        source_palette=np.array(fg.getpalette()[:192],dtype='uint8').reshape(64,3)
        for pixel in np.unique(np.concatenate((a.ravel(),b.ravel()))):
            if pixel:assert np.array_equal(colors[pixel],source_palette[pixel]),f'{name}: CRAM {pixel}'
        # Etapa 7.1: the window replaces plane A over the status bar rows; the bar is opaque,
        # high priority (over the sprites) and drawn in the reserved terrain palette (PAL0).
        rows=h[75]&0x7ff
        if rows:
            w,wp=plane(vram,h[75]&0xf800,32,32,0,0)
            hud=slice(0,rows*8)
            assert np.all(w[hud]!=0) and np.all(wp[hud]),name+': status bar must be opaque and over the sprites'
            assert np.all((w[hud]>0)&(w[hud]<16)),name+': status bar must use PAL0'
            a=a.copy();ap=ap.copy();a[hud]=w[hud];ap[hud]=wp[hud]
        s,sp=sprites(vram,h[70])
        composed=np.zeros((224,256),dtype='uint8')
        for pixels,pri,want_high in [(b,bp,False),(a,ap,False),(s,sp,False),
                                     (b,bp,True),(a,ap,True),(s,sp,True)]:
            take=(pixels!=0)&(pri==want_high);composed[take]=pixels[take]
        if 6 <= i < 8 or i >= 9:
            # Independently compare the open door sprite with its transparent source,
            # and prove the SAT order occludes actual Samus pixels at the metal jamb.
            door = np.array(Image.open(ROOT/'res/images/doors/door_cap.png'))[:,128:160]
            west = i in (6,9)
            if not west: door = door[:,::-1]
            left = (0 if west else 2272) - h[68]
            top = 1120 - h[69]
            region = s[top:top+64,left:left+32]
            solid = door != 0
            assert np.array_equal(region[solid], (door+16)[solid]), name+': door must lead the SAT'
            samus,_ = sprites(vram,h[70],only_palette=2)
            # The terrain lip is in front of both Samus and the lowest metal edge;
            # the central frame retains the existing sprite ordering/animation.
            overlap = solid & (samus[top:top+64,left:left+32] != 0) & ~lips[top:top+64,left:left+32]
            assert np.count_nonzero(overlap) > 0, name+': fixture does not overlap Samus'
            assert np.array_equal(composed[top:top+64,left:left+32][overlap], (door+16)[overlap]), name+': metal must occlude Samus'
            assert not np.any((region[~solid] >= 16) & (region[~solid] < 32)), name+': opaque fill outside door shape'
            covered=lips & (samus!=0) & (a!=0)
            assert np.count_nonzero(covered)>0,name+': fixture must overlap terrain collar'
            assert np.array_equal(composed[covered],a[covered]),name+': terrain lip must occlude Samus'
            exposed=(samus!=0)&~lips&(s==samus)
            assert np.count_nonzero(exposed)>0,name+': Samus must remain partly visible'
            print(f'PASS VDP {name}: {np.count_nonzero(covered)} Samus pixels behind terrain, {np.count_nonzero(exposed)} visible')
            print(f'PASS VDP {name}: transparent underlay and {np.count_nonzero(overlap)} Samus pixels occluded by metal')
        Image.fromarray(colors[composed]).resize((768,672),Image.Resampling.NEAREST).save(OUT/f'blastem-{name}.png')
        print(f'PASS VDP {name}: foreground, palette and sky at {h[68]},{h[69]}')


if __name__=='__main__':main()
