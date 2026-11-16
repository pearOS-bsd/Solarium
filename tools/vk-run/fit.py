import struct,subprocess,os,sys,random,json
import numpy as np
from PIL import Image
J=os.environ['CLAUDE_JOB_DIR']+'/tmp'; W=H=512
base=bytearray(open(J+'/bgblock.bin','rb').read())
bg=np.frombuffer(open('bg.rgba','rb').read(),np.uint8).reshape(H,W,4).astype(float)/255
ref=np.array(Image.open('/home/alxb421/pearOS/Solarium/captures/27.0-arm64/regular-340x200-r60.png').convert('RGB')).astype(float)
m=np.zeros((H,W),bool); m[156:356,86:426]=True
# keep the label rows out of the objective ("Glass" text is not part of the glass)
m[236:276,206:306]=False
def setf(b,off,*v): struct.pack_into('<%df'%len(v),b,off,*v)
def getf(b,off,n=1): return struct.unpack_from('<%df'%n,b,off)
def build(p):
    b=bytearray(base)
    blur,bleed,face,o0,o1,o2,o3,rs,bo,ba = p
    setf(b,88,blur,bleed)
    setf(b,252,face)
    setf(b,208,o0,o0-o1,o1-o2,o2-o3)
    ia,ih=getf(base,64,2); oa,oh=getf(base,72,2)
    setf(b,64,ia*rs,ih); setf(b,72,oa*rs,oh)
    setf(b,248,bo)
    a,h=getf(base,96,2); setf(b,96,a*ba,h)
    return b
tmp=J+'/fit.bin'
def score(p,save=None):
    open(tmp,'wb').write(bytes(build(p)))
    subprocess.run([J+'/vkrun',J+'/run/bg.vert.spv',J+'/run/bg.frag.spv','0','4',tmp],capture_output=True)
    fg=np.frombuffer(open('out.rgba','rb').read(),np.uint8).reshape(H,W,4).astype(float)/255
    comp=np.clip(np.nan_to_num(fg[...,:3]+bg[...,:3]*(1-fg[...,3:4])),0,1)*255
    if save: Image.fromarray(comp.astype(np.uint8)).save(save)
    return abs(comp-ref).mean(axis=2)[m].mean()
lo=np.array([1,1,0,0,0,0,0,0,0,0.0]); hi=np.array([80,150,1,1,1,1,1,1.5,1,1.5])
p0=np.array([8,56,1,0.8,0.4,0.5,1,1,0.8,1.0])
best=p0.copy(); bs=score(best); print('start',round(bs,2),flush=True)
random.seed(1); step=(hi-lo)*0.25
for it in range(260):
    q=best.copy()
    for k in random.sample(range(len(q)),random.choice((1,2,3))):
        q[k]=np.clip(q[k]+random.gauss(0,step[k]),lo[k],hi[k])
    s=score(q)
    if s<bs: best,bs=q,s; print(it,round(bs,2),[round(x,2) for x in best],flush=True)
    if it%60==59: step*=0.6
score(best,'fit_best.png'); json.dump(list(best),open('fit_best.json','w')); print('final',round(bs,2))
