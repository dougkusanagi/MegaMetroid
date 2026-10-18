"""Rebuild Landing Site art from cached original references, without an emulator.

The map uses palette attributes per 8x8 tile. PAL0 terrain, PAL1 sky/doors,
PAL2 unchanged Power Suit, PAL3 unchanged hardware beam colors plus ship shades.
No dithering, rescaling or hand-painted replacement tiles.
"""
from pathlib import Path
import gzip
import json
import numpy as np
from PIL import Image, ImageOps

ROOT = Path(__file__).resolve().parents[1]
REF = ROOT / 'tests/reference/super_metroid/landing_site'
OUT = ROOT / 'res/images/maps/landing_site'


def hardware(color):
    return tuple(int(c) >> 5 for c in color)


def palette(image):
    p = image.getpalette()
    return [tuple(p[i:i+3]) for i in range(0, 48, 3)]


def save_indexed(data, colors, path):
    im = Image.fromarray(data.astype('uint8'))
    im = im.convert('P')
    im.putpalette([v for c in colors for v in c] + [0] * (768 - len(colors)*3))
    im.save(path, transparency=0)


def decode_background():
    v = gzip.decompress((REF / 'snesVideoRam.gz').read_bytes())
    c = gzip.decompress((REF / 'snesCgRam.gz').read_bytes())
    state = dict(line.split('=', 1) for line in gzip.decompress((REF/'state.txt.gz').read_bytes()).decode().splitlines() if '=' in line)
    base = int(state['state.ppu.layers[1].tilemapAddress']) * 2
    chr_base = int(state['state.ppu.layers[1].chrAddress']) * 2
    assert state['state.ppu.bgMode'] == '1'
    assert state['state.ppu.layers[1].doubleHeight'] == 'true'
    colors = []
    for i in range(256):
        n = int.from_bytes(c[i*2:i*2+2], 'little')
        colors.append(tuple(((n >> s) & 31) * 255 // 31 for s in (0, 5, 10)))
    result = np.zeros((512,256,3), dtype=np.uint8)
    for y in range(512):
        for x in range(256):
            off = base + ((y//256)*1024 + (y//8%32)*32 + x//8)*2
            t = int.from_bytes(v[off:off+2], 'little')
            ix, iy = x%8, y%8
            if t & 0x4000: ix = 7-ix
            if t & 0x8000: iy = 7-iy
            tile = chr_base + (t & 1023)*32
            n = sum(((v[tile+iy*2+(p//2)*16+p%2] >> (7-ix))&1)<<p for p in range(4))
            result[y,x] = colors[((t >> 10)&7)*16+n]
    return result


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    (ROOT / 'tests/out').mkdir(parents=True, exist_ok=True)
    art = Image.open(REF/'foreground.png').convert('RGB')
    sky = np.all(np.array(art)==[53,133,186], axis=2)
    # Closed door art: a left wall door faces right; the opposite wall is mirrored.
    door = Image.open(ROOT/'res/images/doors-original-sgdk/doors_door_r00.png').crop((0,0,32,64)).convert('RGBA')
    placements = [(0,608,False),(0,1120,False),(2272,352,True),(2272,1120,True)]
    for x,y,flip in placements:
        image = ImageOps.mirror(door) if flip else door
        art.paste(image,(x,y),image)
    ship = Image.open(ROOT/'res/images/starship-original-sgdk/starship_hatch_00.png').convert('RGBA')
    art.paste(ship,(1057,1101),ship)
    # Any opaque object pixel must supersede the original sky placeholder.
    sky &= np.all(np.array(art)==[53,133,186], axis=2)
    rgb = np.array(art)
    bg = decode_background()
    # Neutral clouds must stay neutral: truncating 98/90/90 independently would
    # turn the original grey sky red (96/64/64) at the VDP's 3-bit precision.
    neutral = bg.max(axis=2).astype(int)-bg.min(axis=2).astype(int) < 20
    grey = np.clip(np.rint(bg.mean(axis=2)/32)*32, 0, 224).astype('uint8')
    bg[neutral] = np.repeat(grey[:,:,None],3,axis=2)[neutral]
    # Terrain colors sampled from the cached original room, not the edited map.
    terrain_colors = [(0,0,0),(0,0,8),(0,8,8),(8,8,16),(24,24,32),
                      (8,32,24),(32,32,40),(24,40,24),(40,48,80),(48,64,24),
                      (112,72,80),(72,64,88),(112,64,88),(72,96,56),(96,128,96)]
    terrain = [(0,0,0)] + [hardware(c) for c in terrain_colors]
    bg_colors = sorted({hardware(c) for c in bg.reshape(-1,3)})
    # Merge neutral sky shades; use the spare entries for closed door shades.
    door_extra = [(0,68,152),(24,116,232),(120,172,232),(112,72,80),(144,104,112),(208,168,176)]
    bank1 = [(0,0,0)] + bg_colors + [hardware(c) for c in door_extra]
    bank1 += [(0,0,0)] * (16-len(bank1))
    assert len(bank1)==16
    samus = [hardware(c) for c in palette(Image.open(ROOT/'res/images/samus-original-sgdk/palettes/pal_samus_power_suit.png'))]
    beam = Image.open(ROOT/'res/images/weapons-original-sgdk/weapons_power_beam.png')
    beam_colors = sorted({hardware(c) for c in np.array(beam.convert('RGB')).reshape(-1,3) if tuple(c)!=(0,136,255)})
    bank3 = [(0,0,0)] + beam_colors + [hardware(c) for c in [(0,0,0),(72,48,0),(128,96,0),(168,160,0),(0,160,72),(0,248,112)]]
    assert len(bank3)==16
    banks = np.array([terrain,bank1,samus,bank3],dtype=np.int16)
    h,w = rgb.shape[:2]
    tiles = (rgb//32).reshape(h//8,8,w//8,8,3).transpose(0,2,1,3,4).reshape(-1,64,3)
    mask = sky.reshape(h//8,8,w//8,8).transpose(0,2,1,3).reshape(-1,64)
    # Cache repeated source tiles; include transparency in the cache key.
    keys = np.concatenate((tiles.reshape(-1,192),mask),axis=1)
    unique,inverse=np.unique(keys,axis=0,return_inverse=True)
    pixels=unique[:,:192].reshape(-1,64,3).astype(np.int16)
    transparent=unique[:,192:].astype(bool)
    results=[]; errors=[]
    for bank in banks:
        diff=pixels[:,:,None,:]-bank[None,None,1:,:]
        distance=(diff.astype(np.int32)**2 * [3,4,2]).sum(axis=3)
        indices=distance.argmin(axis=2)+1
        error=distance.min(axis=2);error[transparent]=0
        results.append(indices);errors.append(error.sum(axis=1))
    chosen=np.array(errors).argmin(axis=0)
    indexed=np.zeros((len(unique),64),dtype=np.uint8)
    for i in range(4):
        selected=chosen==i
        indexed[selected]=results[i][selected]+16*i
    indexed[transparent]=0
    fg=indexed[inverse].reshape(h//8,w//8,8,8).transpose(0,2,1,3).reshape(h,w)
    colors=[tuple(int(v)*32 for v in c) for c in banks.reshape(-1,3)]
    save_indexed(fg,colors,OUT/'foreground.png')
    bg_indices=np.array([bank1.index(hardware(c)) for c in bg.reshape(-1,3)]).reshape(512,256)
    # Opaque black must not use the transparent index.
    bg_indices[bg_indices==0]=1
    save_indexed(bg_indices,[tuple(v*32 for v in c) for c in bank1],OUT/'background.png')
    from build_beam_sheet import build as build_beam
    build_beam([tuple(v*32 for v in c) for c in bank3])
    report={'source':'tests/reference/super_metroid/landing_site/manifest.json',
        'ship':{'source':'starship_hatch_00.png','x':1057,'y':1101,'width':192,'height':80},
        'doors':[{'x':x,'y':y,'flip':flip} for x,y,flip in placements],
        'palette_tile_counts':np.bincount(chosen[inverse],minlength=4).tolist(),
        'limits':['Static closed doors and ship; no transitions or destruction.',
                  'Per-tile palette choice approximates object colors to four CRAM banks.',
                  'Original beam colors preserved at 3-bit VDP precision; neutral sky shades kept grey.',
                  'Existing collision grid retained; ship is decorative.']}
    (OUT/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    # A view composed from the actual indexed resources, useful for source-art QA.
    view=Image.open(OUT/'foreground.png').convert('RGBA')
    background=Image.open(OUT/'background.png').convert('RGB')
    for label,x,y in [('ship',1024,976),('west-door',0,1024),('east-door',2048,1024)]:
        preview=Image.new('RGB',(256,224))
        for row in range(224):preview.paste(background.crop((0,(y+row)%512,256,(y+row)%512+1)),(0,row))
        part=view.crop((x,y,x+256,y+224));preview.paste(part,(0,0),part)
        preview.resize((768,672),Image.Resampling.NEAREST).save(ROOT/'tests/out'/f'landing-{label}.png')
    print('Landing Site regenerated: terrain, sky, four closed doors, block tower and static ship.')


if __name__=='__main__': main()

