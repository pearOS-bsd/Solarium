import struct,subprocess,os,sys,random,json
import numpy as np
from PIL import Image
J=os.environ['CLAUDE_JOB_DIR']+'/tmp'; W=H=512
# usage: fit.py SCENE GLASS_W GLASS_H RADIUS   (SCENE = name of the capture in captures/27.0-arm64, without extension)
SCENE,GW,GH,RAD=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]),float(sys.argv[4])
CAP='/home/alxb421/pearOS/Solarium/captures/27.0-arm64/'+SCENE
subprocess.run(['python3','/home/alxb421/pearOS/Solarium/tools/vk-run/gen_bg_block.py',CAP+'.txt',J+'/blk_'+SCENE+'.bin','512','512',str(GW/2),str(GH/2),str(RAD)],check=True,capture_output=True)
base=bytearray(open(J+'/blk_'+SCENE+'.bin','rb').read())
bg=np.frombuffer(open('bg.rgba','rb').read(),np.uint8).reshape(H,W,4).astype(float)/255
ref=np.array(Image.open(CAP+'.png').convert('RGB')).astype(float)
m=np.zeros((H,W),bool); m[256-GH//2:256+GH//2,256-GW//2:256+GW//2]=True
# keep the label rows out of the objective ("Glass" text is not part of the glass)
m[236:276,:]=m[236:276,:]&False if False else m[236:276,:]; m[238:274,256-GW//2+20:256+GW//2-20]=False
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
tmp=J+'/fit_'+SCENE+'.bin'
def score(p,save=None):
    open(tmp,'wb').write(bytes(build(p)))
    subprocess.run([J+'/vkrun',J+'/run/bg.vert.spv',J+'/run/bg.frag.spv','0','4',tmp],capture_output=True)
    fg=np.frombuffer(open('out.rgba','rb').read(),np.uint8).reshape(H,W,4).astype(float)/255
    comp=np.clip(np.nan_to_num(fg[...,:3]+bg[...,:3]*(1-fg[...,3:4])),0,1)*255
    if save: Image.fromarray(comp.astype(np.uint8)).save(save)
    return abs(comp-ref).mean(axis=2)[m].mean()
lo=np.array([1,1,0,0,0,0,0,0,0,0.0]); hi=np.array([80,150,1,1,1,1,1,1.5,1,1.5])
p0=np.array([getf(base,88,1)[0],getf(base,92,1)[0],getf(base,252,1)[0],*getf(base,208,1),getf(base,208,1)[0]-getf(base,212,1)[0],getf(base,208,1)[0]-getf(base,212,1)[0]-getf(base,216,1)[0],getf(base,208,1)[0]-getf(base,212,1)[0]-getf(base,216,1)[0]-getf(base,220,1)[0],1,getf(base,248,1)[0],1.0])
best=p0.copy(); bs=score(best); print('start',round(bs,2),flush=True)
random.seed(1); step=(hi-lo)*0.25
for it in range(260):
    q=best.copy()
    for k in random.sample(range(len(q)),random.choice((1,2,3))):
        q[k]=np.clip(q[k]+random.gauss(0,step[k]),lo[k],hi[k])
    s=score(q)
    if s<bs: best,bs=q,s; print(it,round(bs,2),[round(x,2) for x in best],flush=True)
    if it%60==59: step*=0.6
score(best,'fit_'+SCENE+'.png'); json.dump(list(best),open('fit_'+SCENE+'.json','w')); print('final',round(bs,2))
