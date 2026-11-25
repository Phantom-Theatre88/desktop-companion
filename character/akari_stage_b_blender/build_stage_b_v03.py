#!/usr/bin/env python3
"""Stage B v03: real new 3D head/eyes/hair sculpt over rejected independent v02 body blockout.

No v0.5 mesh used. Source 2D artworks remain private (never embedded in public code).
Generated 3D model is a QA candidate only; not an approved likeness or rig.
"""
from __future__ import annotations
import math
from pathlib import Path
import numpy as np
import trimesh
from PIL import Image,ImageDraw,ImageFont
import vtk
from vtk.util.numpy_support import numpy_to_vtk

ROOT=Path(__file__).resolve().parent
SOURCE=ROOT/'source'/'AKARI_STAGE_B_3D_BLOCKOUT_v02.glb'
COL={
 'skin':(255,215,193,255),'skin_shadow':(232,157,139,255),'blush':(249,170,159,190),
 'hair':(102,54,46,255),'hair_light':(155,92,68,255),'hair_mid':(124,69,55,255),'hair_dark':(66,35,36,255),
 'white':(255,248,238,255),'iris_edge':(63,36,35,255),'iris':(155,87,54,255),
 'iris_light':(207,135,77,255),'pupil':(45,29,32,255),'shine':(255,255,247,255),
 'lash':(49,28,32,255),'brow':(91,48,43,255),'mouth':(148,57,67,255),'tongue':(245,133,134,255),
 'shirt':(40,40,48,255),'blue':(26,112,217,255),'metal':(131,141,153,255)
}
PARTS=[]

def add(n,m,c):
 m=m.copy();m.visual.vertex_colors=np.tile(np.asarray(COL.get(c,c),np.uint8),(len(m.vertices),1));PARTS.append((n,m));return m

def ell(n,pos,r,c,sub=3):
 m=trimesh.creation.icosphere(subdivisions=sub,radius=1.);m.apply_scale(r);m.apply_translation(pos);return add(n,m,c)

def line(n,points,r,c,nseg=10):
 points=np.asarray(points,float);K=len(points);r=np.broadcast_to(np.asarray(r,dtype=float),(K,));v=[]
 for i in range(K):
  p=points[i];t=points[min(K-1,i+1)]-points[max(0,i-1)];t/=max(np.linalg.norm(t),1e-9)
  anchor=np.array([0,0,1.]) if abs(np.dot(t,[0,0,1.]))<.90 else np.array([1.,0,0]);a=np.cross(t,anchor);a/=max(np.linalg.norm(a),1e-9);b=np.cross(t,a)
  for j in range(nseg):
   th=2*math.pi*j/nseg;v.append(p+r[i]*(a*math.cos(th)+b*math.sin(th)))
 f=[]
 for i in range(K-1):
  for j in range(nseg):
   p=i*nseg+j;q=i*nseg+(j+1)%nseg;u=(i+1)*nseg+j;w=(i+1)*nseg+(j+1)%nseg
   f.extend([[p,q,u],[q,w,u]])
 f += [[0,j,j+1] for j in range(1,nseg-1)];e=(K-1)*nseg;f += [[e,e+j+1,e+j] for j in range(1,nseg-1)]
 return add(n,trimesh.Trimesh(vertices=v,faces=f,process=False),c)

# Preserve only original new body BLOCKOUT, not old v0.5 and not rejected blockout face.
s=trimesh.load(str(SOURCE),force='scene')
assert len(s.geometry)>90
BODY_KEEP=['waist_belt_base','polo_torso','polo_neck','collar_','blue_piping_','shoulder_sleeve_',
 'bare_forearm_','arm_wrist_','hand_','thumb_','cargo_leg_','fabric_fold_','cargo_pouch_',
 'pouch_seam_','safety_boot_','toe_boot_','boot_sole_','boot_blue_','belt', 'metal_buckle',
 'radio_box','radio_antenna','radio_button','tape_roll','tape_hole','carabiner_loop','neck','logo_tile_']
for name,mesh in s.geometry.items():
 if any(name==tag or name.startswith(tag) for tag in BODY_KEEP):
  PARTS.append((name,mesh.copy()))

# --- New head geometry, continuous soft mesh with rounded cheeks, narrow chin ---
y0=1.382;y1=1.683;M=58;N=84
rings=[]
for i in range(M):
 u=i/(M-1);y=y0+(y1-y0)*u
 # elliptical face profile, round cheek at eye level and narrower chin
 rx=.010+.142*(max(0,math.sin(math.pi*u))**.54)
 rz=.035+.100*(max(0,math.sin(math.pi*u))**.52)
 if u<.27:rx*=.75+.25*u/.27
 cz=-.001-(u-.55)*.032
 rings.append((y,rx,rz,cz))
v=[]
for i,(y,rx,rz,cz) in enumerate(rings):
 for j in range(N):
  a=2*math.pi*j/N
  x=rx*math.cos(a);z=cz+rz*math.sin(a)
  # cheek bulge in front when y below eyes
  if z>cz and .30< (y-y0)/(y1-y0) <.72:
   z+=.010*math.exp(-((y-1.484)/.044)**2)*(abs(x)/.135)**2
  v.append((x,y,z))
f=[]
for i in range(M-1):
 for j in range(N):
  a=i*N+j;b=i*N+(j+1)%N;c=(i+1)*N+j;d=(i+1)*N+(j+1)%N
  f.extend([[a,b,c],[b,d,c]])
f += [[0,j,j+1] for j in range(1,N-1)]
e=(M-1)*N;f += [[e,e+j+1,e+j] for j in range(1,N-1)]
add('AKARI_V03_Face_Smooth_Sculpt',trimesh.Trimesh(vertices=v,faces=f,process=False),'skin')

# Patch geometry follows front half of sculpted head; no protruding spherical eyeballs.
def face_z(x,y):
 uu=min(.999,max(.001,(y-y0)/(y1-y0)))
 rx=.010+.142*(math.sin(math.pi*uu)**.54);rz=.035+.100*(math.sin(math.pi*uu)**.52)
 if uu<.27:rx*=.75+.25*uu/.27
 cz=-.001-(uu-.55)*.032
 q=(x/max(rx,.001))**2
 out=cz+rz*math.sqrt(max(0.005,1-q))
 if .30<uu<.72:out+=.010*math.exp(-((y-1.484)/.044)**2)*(abs(x)/.135)**2
 return out

def eye_patch(name,cx,cy,rx,ry,color,front=.002,N=64,L=12):
 vv=[(cx,cy,face_z(cx,cy)+front)]
 for r in np.linspace(1/L,1,L):
  for j in range(N):
   t=2*math.pi*j/N
   x=cx+rx*r*math.cos(t);y=cy+ry*r*math.sin(t)
   # upper-eye contour subtly almond-shaped rather than discs
   x+=.005*r*math.sin(t)*math.cos(t)
   vv.append((x,y,face_z(x,y)+front))
 faces=[]
 for j in range(N):faces.append([0,1+(j+1)%N,1+j])
 for k in range(L-1):
  for j in range(N):
   a=1+k*N+j;b=1+k*N+(j+1)%N;c=a+N;d=b+N
   faces.extend([[a,b,c],[b,d,c]])
 return add(name,trimesh.Trimesh(vertices=vv,faces=faces,process=False),color)

def face_line(name, coords, rad, color):
 return line(name,[(x,y,face_z(x,y)+.004) for x,y in coords],rad,color,12)

for sign in [-1,1]:
 x=sign*.063;cy=1.538
 # skin behind eyes, soft lined whites, iris with layered amber tones
 eye_patch(f'eye_white_{sign}',x,cy,.047,.034,'white',.003)
 eye_patch(f'iris_dark_border_{sign}',x+sign*.004,cy-.002,.028,.031,'iris_edge',.006)
 eye_patch(f'iris_amber_{sign}',x+sign*.004,cy-.002,.024,.026,'iris',.008)
 eye_patch(f'iris_lower_gold_{sign}',x+sign*.004,cy-.013,.017,.013,'iris_light',.010)
 eye_patch(f'pupil_{sign}',x+sign*.004,cy+.002,.012,.021,'pupil',.012)
 eye_patch(f'large_eye_sparkle_{sign}',x+sign*.004-.010,cy+.012,.0085,.0095,'shine',.016)
 eye_patch(f'small_eye_sparkle_{sign}',x+sign*.004+.011,cy-.011,.003,.004,'shine',.015)
 eye_curve=[(x-.045,cy+.007),(x-.035,cy+.026),(x-.019,cy+.035),(x,cy+.038),(x+.021,cy+.035),(x+.040,cy+.022),(x+.046,cy+.006)]
 face_line(f'upper_lash_{sign}',eye_curve,[.0035,.0045,.005,.005,.0045,.004,.002],'lash')
 face_line(f'lower_lash_{sign}',[(x-.038,cy-.009),(x-.018,cy-.027),(x+.010,cy-.031),(x+.036,cy-.016)], [.0012,.0018,.0018,.001],'skin_shadow')
 face_line(f'eyebrow_{sign}',[(x-.038,cy+.061),(x-.017,cy+.068),(x+.014,cy+.068),(x+.038,cy+.061)], [.0012,.0027,.0028,.0012],'brow')
 # delicately colored blushed cheeks, not extruded sphere
 eye_patch(f'cheek_tint_{sign}',sign*.094,1.477,.023,.014,'blush',.002,N=48,L=4)
 ell(f'ear_{sign}',(sign*.144,1.494,-.016),(.019,.035,.019),'skin',3)

# Tiny nose and curved gentle smile; eyes dominate the character.
ell('nose_tip',(0,1.477,face_z(0,1.477)+.007),(.006,.006,.005),'skin_shadow',2)
face_line('subtle_smile',[(-.027,1.435),(-.013,1.428),(0,1.426),(.015,1.429),(.028,1.438)], [.001,.002,.002,.002,.001],'mouth')

# --- Sculpted hairstyles: scalp silhouette + MANY flowing layered tapered strands ---
ell('hair_scap_back',(0,1.592,-.083),(.141,.120,.099),'hair',4)
ell('hair_crown',(0,1.659,-.042),(.145,.065,.106),'hair',4)
# detailed flowing fringe: several overlapping tapered clumps with varied bends
bangs=[(-.137,-.107,1.600),(-.120,-.068,1.574),(-.094,-.032,1.575),(-.058,.006,1.584),(-.014,.054,1.571),(.042,.093,1.584),(.090,.127,1.603)]
for i,(start,end,tip_y) in enumerate(bangs):
 pts=[];r=[]
 for t in np.linspace(0,1,9):
  xx=start+(end-start)*t+(.005*math.sin(t*math.pi))
  yy=1.676-(1.676-tip_y)*(t**.84)
  zz=.057+.045*math.sin(.82*math.pi*t)+.016*t
  pts.append((xx,yy,zz));r.append(.020*(1-t)**.60+.0008)
 line(f'fringe_sculpt_{i:02}',pts,r,'hair' if i%3 else 'hair_mid',14)
 # swept highlight following strand upper left
 if i%2==0:
  line(f'fringe_gloss_{i:02}',[(p[0]-.006,p[1]+.002,p[2]+.016) for p in pts[:6]],np.linspace(.001,.003,6),'hair_light',8)
# side locks flow around jaw, revealing eyes
for sign in [-1,1]:
 for strand in range(3):
  xx=sign*(.128+.015*strand)
  pts=[(xx,1.641,.026),(xx+sign*.005,1.585,.069),(xx+sign*.005,1.511,.071),(xx+sign*.003,1.456-.025*strand,.048)]
  line(f'face_frame_{sign}_{strand}',pts,[.024,.023,.015,.001],'hair' if strand<2 else 'hair_mid',14)
  line(f'side_highlight_{sign}_{strand}',[(p[0]+sign*.006,p[1],p[2]+.017) for p in pts[:3]],[.001,.003,.001],'hair_light',8)
# ponytail attaches high at left rear, flows backward then down as layered large silky strands
base=np.array([-.060,1.684,-.121]);tie=np.array([-.084,1.671,-.169])
ell('pony_scrunchie',tie,(.035,.026,.031),'hair_dark',3)
for i in range(25):
 phi=2*math.pi*i/25
 sx=math.cos(phi)*.045;sz=math.sin(phi)*.034
 pts=[];rad=[]
 for k,t in enumerate(np.linspace(0,1,13)):
  x=-.092-.058*t+sx*(.45+.9*t)+.021*math.sin(t*math.pi*1.5+i*.37)*t
  y=1.68-.258*(t**1.1)+.025*math.sin(math.pi*t+i*.15)
  z=-.180-.066*t+sz+(.022*math.cos(i*.79+t*math.pi))*t
  pts.append((x,y,z));rad.append(.0145*(math.sin(math.pi*(.07+.88*t)))**.6 +.001)
 line(f'pony_strand_{i:02}',pts,rad,'hair_mid' if i%5==0 else ('hair_light' if i%7==0 else 'hair'),10)
# loose flyaways
for i in range(8):
 phi=2.40*i
 line(f'flyaway_{i:02}',[(base[0],1.684,-.14),(-.09+.07*math.cos(phi),1.746+.008*math.sin(phi),-.16-.024*math.sin(phi)),(-.173+.06*math.cos(phi),1.713+.015*math.sin(phi),-.19)], [.0035,.0020,.0004],'hair_light' if i%4==0 else 'hair',7)

# Additional polo placket detail, buttons/blue trim in stand-in torso.
for idx,y in enumerate([1.243,1.214,1.186]):
 ell(f'polo_button_{idx}',(.003,y,.090),(.005,.005,.003),'metal',2)

# Keep newly sculpted candidate at existing scale for source coordinate continuity;
# high hair strand exceeds 1.7m => explicitly normalized to 1.63m character height.
# Mesh data (not pose) is scaled uniformly, preserving body anatomy ratios.
height=max(float(m.bounds[1][1]) for _,m in PARTS)
scale=1.63/height
for _,m in PARTS:m.apply_scale(scale)
scene=trimesh.Scene()
for name,mesh in PARTS:scene.add_geometry(mesh,node_name=name,geom_name=name)
out=ROOT/'AKARI_StageB_V03_NEW_FACE_HAIR_UNAPPROVED.glb'
out.write_bytes(scene.export(file_type='glb'))
print('3D exported',out,'parts',len(PARTS),'old-height',height,'scaled-height',scene.bounds[1][1], 'bytes',out.stat().st_size)

# True orthographic 3D rasterization using VTK offscreen, not generated 2D lookalikes.
views=[('front',(0,.80,3.85)),('side',(3.85,.80,0)),('back',(0,.80,-3.85)),('three_quarter',(2.75,.85,2.75))]
for label,camera_pos in views:
 renderer=vtk.vtkRenderer();renderer.SetBackground(.99,.987,.99)
 win=vtk.vtkRenderWindow();win.SetSize(750,950);win.SetOffScreenRendering(1);win.AddRenderer(renderer);win.SetMultiSamples(4)
 for name,m in PARTS:
  poly=vtk.vtkPolyData();points=vtk.vtkPoints();points.SetData(numpy_to_vtk(np.ascontiguousarray(m.vertices),deep=True));poly.SetPoints(points)
  cells=vtk.vtkCellArray()
  for ff in m.faces:
   cells.InsertNextCell(3)
   for idx in ff:cells.InsertCellPoint(int(idx))
  poly.SetPolys(cells)
  normals=vtk.vtkPolyDataNormals();normals.SetInputData(poly);normals.SetFeatureAngle(105);normals.SplittingOff();normals.ConsistencyOn();normals.ComputePointNormalsOn();normals.Update()
  mapper=vtk.vtkPolyDataMapper();mapper.SetInputConnection(normals.GetOutputPort())
  actor=vtk.vtkActor();actor.SetMapper(mapper)
  c=np.asarray(m.visual.vertex_colors)[0]/255.;actor.GetProperty().SetColor(*c[:3]);actor.GetProperty().SetOpacity(float(c[3]));actor.GetProperty().SetAmbient(.45);actor.GetProperty().SetDiffuse(.53);actor.GetProperty().SetSpecular(.11);actor.GetProperty().SetSpecularPower(18)
  renderer.AddActor(actor)
 cam=renderer.GetActiveCamera();cam.SetPosition(*camera_pos);cam.SetFocalPoint(0,.79,0);cam.SetViewUp(0,1,0);cam.ParallelProjectionOn();cam.SetParallelScale(1.02)
 key=vtk.vtkLight();key.SetPosition(2.5,3.5,3.0);key.SetIntensity(.80);renderer.AddLight(key)
 fill=vtk.vtkLight();fill.SetPosition(-2.0,2.0,-2.0);fill.SetIntensity(.42);renderer.AddLight(fill)
 win.Render();capture=vtk.vtkWindowToImageFilter();capture.SetInput(win);capture.Update();writer=vtk.vtkPNGWriter();writer.SetInputConnection(capture.GetOutputPort());writer.SetFileName(str(ROOT/f'AKARI_StageB_v03_{label}.png'));writer.Write();win.Finalize()
 print('rendered',label)
contact=Image.new('RGB',(1500,1970),(250,250,252));d=ImageDraw.Draw(contact)
try:font=ImageFont.truetype('/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc',25)
except:font=ImageFont.load_default()
labels={'front':'正面 / Front','side':'右側面 / Side','back':'背面 / Back','three_quarter':'斜め45° / Three-quarter'}
for i,(label,_) in enumerate(views):
 x=(i%2)*750;y=(i//2)*985+35
 im=Image.open(ROOT/f'AKARI_StageB_v03_{label}.png').convert('RGB');contact.paste(im,(x,y));d.text((x+20,y-31),labels[label],font=font,fill=(35,35,40))
contact.save(ROOT/'AKARI_StageB_v03_four_views.png')
# Reference comparison small orthographic crops, private (not for GitHub)
refs=Image.open(ROOT/'reference'/'design_sheet.jpeg').convert('RGB')
front=Image.open(ROOT/'AKARI_StageB_v03_front.png').convert('RGB')
comparison=Image.new('RGB',(1550,950),'#f7f7fa');comparison.paste(refs.resize((800,533)),(0,150));comparison.paste(front.resize((750,950)),(800,0));comparison.save(ROOT/'AKARI_StageB_v03_reference_comparison_PRIVATE.png')