'''Apply reviewed R2 changes to GitHub R1 source. No character asset changes.''' 
import pathlib,sys
p=pathlib.Path(sys.argv[1]);s=p.read_text()
def replace(a,b):
 global s
 if s.count(a)!=1:raise ValueError('patch anchor missing/ambiguous: '+a[:70])
 s=s.replace(a,b)
replace("a=ease((t-3.2)/1.05);b=ease((t-4.25)/1.05)","b=ease((t-3.2)/1.05);a=ease((t-4.25)/1.05)")
replace("feet['L'][1]-=13*(S(pi*u)+S(pi*a));feet['R'][1]-=13*(S(pi*v)+S(pi*b));x+=40*(u+v-a-b)/2", "feet['L'][1]-=4*(S(pi*u)+S(pi*a));feet['R'][1]-=4*(S(pi*v)+S(pi*b));x+=40*(u+v-a-b)/2")
replace("y+= (3 if rev==1 else 8)*phase; q['Upper_R']=5*phase*S(t*2.1);q['Upper_L']=-5*phase*S(t*2.1)","y+=1.8*(S(pi*u)+S(pi*v)+S(pi*a)+S(pi*b));q['Upper_R']=4*(u-v+a-b);q['Upper_L']=-4*(u-v+a-b)")
replace("q['Upper_R']=36*phase;q['Fore_R']=10*phase;q['Hand_R']=(-5+3*S(t*4))*phase;q['Head']-=2*phase", "q['Upper_R']=24*phase;q['Fore_R']=12*phase;q['Hand_R']=(-4+2*S(t*3))*phase;q['Head']-=1.5*phase;q['Spine']+=.8*phase")
replace("feet={s:np.array([310+sign*26.22,541.68]) for s,sign in [('R',-1),('L',1)]}","feet={s:np.array([310+sign*26.22+(-.46 if s=='L' else 0),541.68]) for s,sign in [('R',-1),('L',1)]}")
replace("l1,l2=117.76,103.96\n   if rev>=2:","l1,l2=117.76,103.96\n   bone_map={b['name']:b for b in m['bones']}\n   if rev>=2:")
replace("dx=feet[s][0]-(x+sign*26.22);dy=math.sqrt(max(0,(l1+l2-.01)**2-dx*dx));y=max(y,feet[s][1]-34.96-dy)","dx=feet[s][0]-(x+sign*26.22);l1=np.linalg.norm(bone_map['Knee_'+s]['pos']);l2=np.linalg.norm(bone_map['Ankle_'+s]['pos']);dy=math.sqrt(max(0,(l1+l2)**2-dx*dx));y=max(y,feet[s][1]-34.96-dy)")
replace("hip=np.array([x+sign*26.22,y+34.96]);f=feet[s];k=ik(hip,f,l1,l2,-sign);q['Hip_'+s]=angle(k-hip);q['Knee_'+s]=angle(f-k)-q['Hip_'+s];q['Ankle_'+s]=-q['Hip_'+s]-q['Knee_'+s']", "hip=np.array([x+sign*26.22,y+34.96]);f=feet[s];b1=bone_map['Knee_'+s]['pos'];b2=bone_map['Ankle_'+s]['pos'];l1=np.linalg.norm(b1);l2=np.linalg.norm(b2);k=ik(hip,f,l1,l2,-sign);q['Hip_'+s]=angle(k-hip)-angle(b1);q['Knee_'+s]=angle(f-k)-angle(b2)-q['Hip_'+s];q['Ankle_'+s]=-q['Hip_'+s]-q['Knee_'+s]")
replace("frames=[];contact=[];maxerr=0;counts={};last=None;maxjump=0;maxgap=0", "frames=[];contact=[];maxerr=0;counts={};last=None;maxjump=0;maxgap=0;min_foot_sep=999;max_knee=0;prev_feet=None;max_planted_slip=0")
replace("last=pts\n   im=", "last=pts\n   min_foot_sep=min(min_foot_sep,float(pts['Ankle_L'][0]-pts['Ankle_R'][0]));max_knee=max(max_knee,abs(q['Knee_L']),abs(q['Knee_R']))\n   if prev_feet is not None:\n    for side in ['L','R']:\n     if np.linalg.norm(feet[side]-prev_feet[0][side])<1e-8:max_planted_slip=max(max_planted_slip,float(np.linalg.norm(pts['Ankle_'+side]-prev_feet[1][side])))\n   prev_feet=({k:v.copy() for k,v in feet.items()},{k:pts['Ankle_'+k].copy() for k in feet})\n   im=")
replace("'variant_frames':counts,'quality'", "'variant_frames':counts,'min_foot_separation_px':round(min_foot_sep,4),'max_knee_angle_deg':round(max_knee,4),'max_planted_ankle_slip_px':round(max_planted_slip,4),'quality'")
pathlib.Path(sys.argv[2]).write_text(s)
