import struct,subprocess,os,sys
import numpy as np
from PIL import Image
J=os.environ['CLAUDE_JOB_DIR']+'/tmp'; W=H=512
blk=bytearray(open(J+'/bgblock.bin','rb').read())
bg=np.frombuffer(open('bg.rgba','rb').read(),np.uint8).reshape(H,W,4).astype(float)/255
ref=np.array(Image.open('/home/alxb421/pearOS/Solarium/captures/27.0-arm64/regular-340x200-r60.png').convert('RGB')).astype(int)
m=np.zeros((H,W),bool); m[156:356,86:426]=True
def run(b,name):
    open(J+'/v.bin','wb').write(bytes(b))
    subprocess.run([J+'/vkrun',J+'/run/bg.vert.spv',J+'/run/bg.frag.spv','0','4',J+'/v.bin'],capture_output=True)
    fg=np.frombuffer(open('out.rgba','rb').read(),np.uint8).reshape(H,W,4).astype(float)/255
    comp=np.clip(np.nan_to_num(fg[...,:3]+bg[...,:3]*(1-fg[...,3:4])),0,1)
    img=(comp*255).astype(np.uint8); d=abs(img.astype(int)-ref).mean(axis=2)[m].mean()
    Image.fromarray(img).save(f'abl_{name}.png'); print(f'{name:28s} diff {d:6.1f}  centre {tuple(img[256,256])}')
    return d
def setf(b,off,*v): struct.pack_into('<%df'%len(v),b,off,*v)
run(blk,'base')
def var(name,fn):
    b=bytearray(blk); fn(b); run(b,name)
var('no-bleed(amount0)',lambda b:setf(b,96,0,0))
var('no-shadow',lambda b:setf(b,104,0,0))
var('no-refraction',lambda b:(setf(b,64,0,0),setf(b,72,0,0)))
var('blur-opacity-all1',lambda b:setf(b,208,1,0,0,0))
var('refr-opacity0',lambda b:setf(b,272,0))
var('refr-opacity1',lambda b:setf(b,272,1))
var('face-opacity0',lambda b:setf(b,252,0))
var('bleed-opacity0',lambda b:setf(b,248,0))
var('halves-zero',lambda b:b.__setitem__(slice(128,200),bytes(72)))
var('blur x4',lambda b:setf(b,88,5*1.6*4,35*1.6))
