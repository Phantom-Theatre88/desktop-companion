'''Offline v07 front-facing sprite experiment; not production NERVA. Original v07 zip required, no assets redistributed.''' 
import argparse,json,math,pathlib,zipfile,tempfile,hashlib
from PIL import Image,ImageDraw
import numpy as np
P=pathlib.Path
S=math.sin; C=math.cos; pi=math.pi
def ease(t):
 t=max(0,min(1,t));return t*t*(3-2*t)
def rot(a):
 a=math.radians(a);return np.array([[C(a),-S(a)],[S(a),C(a)]])
def ik(h,f,l1,l2,side):
 d=f-h;r=np.linalg.norm(d);u=d/r;a=(l1*l1-l2*l2+r*r)/(2*r);b=math.sqrt(max(0,l1*l1-a*a));return h+a*u+side*b*np.array([-u[1],u[0]])
def angle(v):return math.degrees(math.atan2(-v[0],v[1]))
def run(root,out,rev):
 assets=root/'assets';m=json.loads((assets/'target_rig_manifest.json').read_text());variants=json.loads((assets/'arm_variants.json').read_text());out.mkdir(parents=True,exist_ok=True)
 cache={}; W,H=620,650; fps=30;n=180;report={}
 def tex(s):
  f=s['file'];
  if f not in cache:cache[f]=Image.open(assets/f).convert('RGBA')
  return cache[f]
 for mode in ['gesture','step','seat']:
  dest=out/mode;dest.mkdir(exist_ok=True);frames=[];contact=[];maxerr=0;counts={};last=None;maxjump=0;maxgap=0
  for i in range(n):
   t=i/fps;phase=ease((t-.7)/1.1)*ease((5.3-t)/1.1);breath=S(t*2*pi/4.3);q={};x=310.;y=285.
   q.update(Chest=.5*breath,Head=1.6*S(t*.8),Hair=1.3*S(t*1.4-.4),Spine=.4*S(t*.9))
   feet={s:np.array([310+sign*26.22,541.68]) for s,sign in [('R',-1),('L',1)]}
   if mode=='step':
    u=ease((t-.7)/1.05);v=ease((t-1.75)/1.05);a=ease((t-3.2)/1.05);b=ease((t-4.25)/1.05)
    feet['L'][0]+=40*(u-a);feet['R'][0]+=40*(v-b)
    feet['L'][1]-=13*(S(pi*u)+S(pi*a));feet['R'][1]-=13*(S(pi*v)+S(pi*b));x+=40*(u+v-a-b)/2
    y+= (3 if rev==1 else 8)*phase; q['Upper_R']=5*phase*S(t*2.1);q['Upper_L']=-5*phase*S(t*2.1)
   elif mode=='seat':
    # A frontal crouch geometry probe, NOT a seated angle drawing.
    y+=36*phase;q['Spine']=-3*phase;q['Head']=2*phase;q['Upper_R']=10*phase;q['Upper_L']=-10*phase
   else:
    q['Upper_R']=36*phase;q['Fore_R']=10*phase;q['Hand_R']=(-5+3*S(t*4))*phase;q['Head']-=2*phase
   l1,l2=117.76,103.96
   if rev>=2:
    for s,sign in [('R',-1),('L',1)]:
     dx=feet[s][0]-(x+sign*26.22);dy=math.sqrt(max(0,(l1+l2-.01)**2-dx*dx));y=max(y,feet[s][1]-34.96-dy)
   for s,sign in [('R',-1),('L',1)]:
    hip=np.array([x+sign*26.22,y+34.96]);f=feet[s];k=ik(hip,f,l1,l2,-sign);q['Hip_'+s]=angle(k-hip);q['Knee_'+s]=angle(f-k)-q['Hip_'+s];q['Ankle_'+s]=-q['Hip_'+s]-q['Knee_'+s]
   mats={};pts={}
   for b in m['bones']:
    name=b['name'];par=b['parent'];p=np.array(b['pos']);a=q.get(name,0)
    if par is None:pts[name]=np.array([x,y])+p;mats[name]=rot(a)
    else:pts[name]=pts[par]+mats[par]@p;mats[name]=mats[par]@rot(a)
   for s in ['L','R']:maxerr=max(maxerr,float(np.linalg.norm(pts['Ankle_'+s]-feet[s])))
   if last is not None:maxjump=max(maxjump,max(float(np.linalg.norm(pts[k]-last[k])) for k in pts))
   last=pts
   im=Image.new('RGBA',(W,H),(228,232,235,255));d=ImageDraw.Draw(im);d.ellipse((x-78,591,x+78,608),fill=(204,210,216,255))
   if mode=='seat':d.rounded_rectangle((235,365,385,390),5,fill=(90,102,113));d.line((245,385,245,600),fill=(90,102,113),width=5);d.line((375,385,375,600),fill=(90,102,113),width=5)
   for s in sorted(m['sprites'],key=lambda v:v['z']):
    name=s['name'];bone=s['bone'];a=abs(q.get('Upper_'+name[-1],0));spec=s
    if name in variants and a>=30:spec=variants[name]['high' if a>=65 else 'mid'];counts[name]=counts.get(name,0)+1
    img=tex(spec);sx,sy=spec['size'];ox,oy=spec['offset'];A=mats[bone]@np.diag([sx/img.width,sy/img.height]);origin=pts[bone]+mats[bone]@np.array([ox,oy]);inv=np.linalg.inv(A)
    corners=np.array([origin,origin+A@[img.width,0],origin+A@[0,img.height],origin+A@[img.width,img.height]])
    lo=np.maximum([0,0],np.floor(corners.min(axis=0)-2)).astype(int);hi=np.minimum([W,H],np.ceil(corners.max(axis=0)+2)).astype(int)
    if np.any(hi<=lo):continue
    off=inv@(lo-origin);layer=img.transform(tuple(hi-lo),Image.Transform.AFFINE,(*inv[0],off[0],*inv[1],off[1]),Image.Resampling.BICUBIC);im.alpha_composite(layer,tuple(lo))
   frames.append(im.convert('RGB'))
   if i in [0,30,60,90,120,179]:contact.append(im.convert('RGB').resize((310,325)))
  sheet=Image.new('RGB',(930,650))
  for j,f in enumerate(contact):sheet.paste(f,(310*(j%3),325*(j//3)))
  sheet.save(dest/'sheet.png');frames[0].save(dest/'neutral.png');frames[90].save(dest/'middle.png');frames[0].save(dest/'motion.gif',save_all=True,append_images=frames[1:],duration=33,loop=0)
  report[mode]={'frames':n,'ankle_target_error_px':round(maxerr,4),'max_joint_frame_displacement_px':round(maxjump,4),'variant_frames':counts,'quality':'not yet visually approved','scope':'front-facing planar proxy, not depth-correct walking or seating'}
 (out/'metrics.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
if __name__=='__main__':
 a=argparse.ArgumentParser();a.add_argument('source');a.add_argument('out');a.add_argument('--rev',type=int,default=2);p=a.parse_args();src=P(p.source)
 if src.is_file():
  with tempfile.TemporaryDirectory() as tmp:
   with zipfile.ZipFile(src) as z:z.extractall(tmp)
   run(P(tmp)/'AKARI_Godot_2D_v07',P(p.out),p.rev)
 else:run(src,P(p.out),p.rev)
