"""Build editable native Godot meshes; no external assets or generated bitmap art.
Coordinates in metres. Re-run from any cwd to reproduce the scene and props.
"""
from pathlib import Path
import math
ROOT = Path(__file__).resolve().parents[1]

COLORS = dict(wood='0.39,0.23,0.12', oak='0.62,0.41,0.23', edge='0.26,0.15,0.09',
    wall='0.76,0.70,0.59', trim='0.42,0.31,0.22', cream='0.88,0.82,0.68',
    pink='0.58,0.32,0.32', linen='0.77,0.62,0.51', green='0.18,0.34,0.16',
    leaf='0.29,0.45,0.19', pot='0.59,0.36,0.23', metal='0.13,0.15,0.16',
    screen='0.17,0.31,0.49', gold='0.88,0.57,0.22', rug='0.68,0.60,0.48',
    city='0.32,0.37,0.43', city2='0.43,0.44,0.45', glass='0.45,0.62,0.69',
    light='1.0,0.70,0.29', book1='0.24,0.36,0.37', book2='0.61,0.32,0.22',
    book3='0.73,0.65,0.45', book4='0.33,0.34,0.45')

def v(values): return 'Vector3(' + ', '.join(f'{x:.5f}' for x in values) + ')'

class Scene:
    def __init__(self, name, script=False):
        self.name, self.script = name, script
        self.resources, self.nodes, self.ext = [], [], []
        self.materials = {}
        self.i = 0
        self.node(name, 'Node3D', None, extra=('script = ExtResource("room")' if script else ''))
    def node(self, name, kind, parent='.', pos=None, rot=None, extra=''):
        attrs = f'[node name="{name}" type="{kind}"'
        if parent is not None: attrs += f' parent="{parent}"'
        attrs += ']'
        body = [attrs]
        if pos is not None: body += ['position = ' + v(pos)]
        if rot is not None: body += ['rotation = ' + v(rot)]
        if extra: body += [extra]
        self.nodes.append('\n'.join(body))
    def mat(self, color):
        if color not in self.materials:
            self.materials[color] = 'mat_' + color
            props = f'albedo_color = Color({COLORS[color]},1)\nroughness = 0.85'
            if color in ('screen','light'):
                props += f'\nemission_enabled = true\nemission = Color({COLORS[color]},1)\nemission_energy_multiplier = 0.65'
            self.resources += [f'[sub_resource type="StandardMaterial3D" id="mat_{color}"]\n{props}']
        return self.materials[color]
    def box(self, name, size, pos, color, parent='.', collision=False, rot=None):
        self.i += 1
        ident = f'box{self.i}'
        mat = self.mat(color)
        self.resources += [f'[sub_resource type="BoxMesh" id="{ident}"]\nsize = {v(size)}\nmaterial = SubResource("{mat}")']
        self.node(name,'MeshInstance3D',parent,pos,rot,f'mesh = SubResource("{ident}")')
        if collision:
            shape = 'shape'+str(self.i)
            self.resources += [f'[sub_resource type="BoxShape3D" id="{shape}"]\nsize = {v(size)}']
            path = (parent+'/' if parent!='.' else '')+name
            self.node('Body','StaticBody3D',path)
            self.node('Shape','CollisionShape3D',path+'/Body',extra=f'shape = SubResource("{shape}")')
    def cylinder(self,name,radius,height,pos,color,parent='.',rot=None,top=None):
        self.i += 1
        ident='cyl'+str(self.i)
        mat=self.mat(color)
        self.resources += [f'[sub_resource type="CylinderMesh" id="{ident}"]\ntop_radius = {radius if top is None else top}\nbottom_radius = {radius}\nheight = {height}\nradial_segments = 24\nmaterial = SubResource("{mat}")']
        self.node(name,'MeshInstance3D',parent,pos,rot,f'mesh = SubResource("{ident}")')
    def sphere(self,name,size,pos,color,parent='.'):
        self.i += 1
        ident='sphere'+str(self.i)
        mat=self.mat(color)
        self.resources += [f'[sub_resource type="SphereMesh" id="{ident}"]\nradius = 0.5\nheight = 1.0\nmaterial = SubResource("{mat}")']
        self.node(name,'MeshInstance3D',parent,pos,extra=f'scale = {v(size)}\nmesh = SubResource("{ident}")')
    def instance(self,name,path,parent,pos):
        self.ext += [f'[ext_resource type="PackedScene" path="res://scenes/props/{path}.tscn" id="{path}"]']
        self.nodes += [f'[node name="{name}" parent="{parent}" instance=ExtResource("{path}")]\nposition = {v(pos)}']
    def save(self,path):
        ext = ([f'[ext_resource type="Script" path="res://scripts/room.gd" id="room"]'] if self.script else []) + self.ext
        text = '\n\n'.join(['[gd_scene load_steps=%d format=3]' % (len(self.resources)+len(ext)+1)] + ext + self.resources + self.nodes)+'\n'
        (ROOT/path).write_text(text)

def plant(s,name,pos,scale=1):
    s.node(name,'Node3D','.',pos,extra=f'scale = {v((scale,)*3)}')
    s.cylinder('Pot',.18,.32,(0,.16,0),'pot',name,top=.23)
    for i in range(7):
        angle=i*2.4
        x,z=math.cos(angle)*.23,math.sin(angle)*.23
        s.cylinder(f'Stem{i}',.014,.65,(x*.5,.56,z*.5),'green',name)
        s.sphere(f'Leaf{i}',(.27,.13,.18),(x,.66+(i%3)*.14,z),'leaf' if i%2 else 'green',name)

s=Scene('Desk')
s.box('Top',(1.8,.09,.72),(0,.78,0),'oak',collision=True)
for x in (-.8,.8):
    for z in (-.27,.27): s.box(f'Leg{x}{z}',(.055,.75,.055),(x,.375,z),'metal')
s.box('Drawers',(.47,.65,.62),(.59,.38,0),'cream',collision=True)
for y in (.3,.58):
    s.box(f'Drawer{y}',(.43,.23,.025),(.59,y,.325),'linen')
    s.box(f'Handle{y}',(.12,.018,.03),(.59,y,.35),'metal')
s.box('LaptopBase',(.57,.028,.36),(-.1,.845,.02),'metal')
s.box('LaptopLid',(.57,.37,.025),(-.1,1.04,-.13),'metal',rot=(-.17,0,0))
s.box('LaptopScreen',(.52,.31,.008),(-.1,1.04,-.109),'screen',rot=(-.17,0,0))
for i in range(6): s.box(f'Keyboard{i}',(.45,.003,.015),(-.1,.862,-.08+i*.036),'city')
s.cylinder('Mug',.062,.13,(-.58,.895,.15),'cream')
s.cylinder('TaskLampBase',.12,.025,(.62,.85,-.18),'metal')
s.cylinder('TaskLampStem',.015,.43,(.62,1.06,-.18),'metal',rot=(0,0,-.2))
s.cylinder('TaskLampShade',.13,.13,(.58,1.29,-.18),'gold',top=.065)
s.node('TaskLight','OmniLight3D','.',(.58,1.2,-.13),extra='light_color = Color(1,0.72,0.39,1)\nlight_energy = 0.55\nomni_range = 2.2')
s.box('ChairSeat',(.55,.10,.53),(-.08,.51,.75),'pink',collision=True)
s.box('ChairBack',(.55,.62,.09),(-.08,.88,.98),'pink',rot=(-.12,0,0))
s.cylinder('ChairPost',.04,.45,(-.08,.23,.75),'metal')
for a in range(5):
    angle=a*math.tau/5
    s.box(f'ChairFoot{a}',(.06,.05,.48),(-.08+math.sin(angle)*.18,.08,.75+math.cos(angle)*.18),'metal',rot=(0,angle,0))
s.box('DrapedThrow',(.20,.67,.04),(.08,.87,1.04),'linen')
s.save('scenes/props/desk.tscn')

s=Scene('Bookshelf')
s.box('Back',(.88,2.9,.055),(0,1.45,-.19),'wood',collision=True)
for x in (-.45,.45): s.box(f'Side{x}',(.075,2.9,.44),(x,1.45,0),'oak',collision=True)
for k in range(7):
    y=.06+k*.46
    s.box(f'Shelf{k}',(.88,.055,.44),(0,y,0),'oak',collision=True)
    if k==6: continue
    for j in range(8):
        height=.27+((j*7+k*3)%5)*.024
        s.box(f'Book{k}_{j}',(.065,height,.23),(-.36+j*.095,y+.04+height/2,.035),f'book{1+(j+k)%4}')
        s.box(f'SpineBand{k}_{j}',(.055,.012,.006),(-.36+j*.095,y+.10,.154),'cream')
plant(s,'TopPlant',(0,2.94,0),.7)
s.save('scenes/props/bookshelf.tscn')

s=Scene('Bed')
s.box('Frame',(1.66,.35,2.45),(0,.31,0),'oak',collision=True)
s.box('Mattress',(1.59,.25,2.34),(0,.61,0),'cream',collision=True)
s.box('Headboard',(1.72,.92,.09),(0,.72,-1.2),'oak',collision=True)
s.box('Duvet',(1.63,.17,1.75),(0,.81,.3),'pink')
for i in range(19): s.box(f'QuiltSeam{i}',(.018,.012,1.72),(-.77+i*.085,.902,.3),'linen')
for x in (-.4,.4): s.sphere(f'Pillow{x}',(.72,.22,.45),(x,.82,-.81),'linen')
s.box('FoldedThrow',(1.65,.055,.45),(0,.92,.7),'cream')
s.save('scenes/props/bed.tscn')

s=Scene('LowTable')
s.box('Top',(1.3,.09,.7),(0,.46,0),'oak',collision=True)
for x in (-.5,.5):
    for z in (-.23,.23): s.box(f'Leg{x}{z}',(.07,.42,.07),(x,.21,z),'wood')
s.box('Book',(.34,.045,.23),(-.3,.53,.02),'book1')
s.box('Pages',(.32,.025,.225),(-.3,.56,.02),'cream')
s.cylinder('Vase',.065,.21,(.21,.61,-.06),'glass',top=.04)
for i in range(5):
    x=.21+math.cos(i*2)*.08
    s.cylinder(f'Stem{i}',.006,.26,(x,.82,-.06),'green')
    s.sphere(f'Flower{i}',(.1,.07,.09),(x,.95,-.06),'cream')
s.cylinder('Candle',.065,.15,(.43,.57,.07),'light')
s.node('CandleLight','OmniLight3D','.',(.43,.77,.07),extra='light_color = Color(1,0.63,0.25,1)\nlight_energy = 0.25\nomni_range = 1.4')
s.save('scenes/props/low_table.tscn')

s=Scene('AKARIRoom',True)
s.node('Architecture','Node3D')
s.box('Floor',(6.6,.16,5.4),(0,-.08,0),'oak','Architecture',True)
for i in range(33):
    s.box(f'FloorSeam{i}',(.012,.002,5.35),(-3.2+i*.2,.002,0),'wood','Architecture')
s.box('LeftWall',(.12,3.35,5.4),(-3.3,1.675,0),'wall','Architecture',True)
s.box('RightWall',(.12,3.35,5.4),(3.3,1.675,0),'wall','Architecture',True)
# Rear wall surrounds an actual opening, not a wall-sized background image.
for name,size,pos in [('BackLeft',(1.15,3.35,.12),(-2.72,1.675,-2.7)),('BackRight',(.85,3.35,.12),(2.87,1.675,-2.7)),('BackSill',(4.6,.25,.12),(.15,.125,-2.7)),('BackLintel',(4.6,.4,.12),(.15,3.15,-2.7))]:
    s.box(name,size,pos,'wall','Architecture',True)
for x in (-3.21,3.21):
    s.box(f'Skirting{x}',(.055,.13,5.3),(x,.08,0),'trim','Architecture')
    s.box(f'Cornice{x}',(.10,.10,5.3),(x,3.19,0),'trim','Architecture')
s.node('Window','Node3D','Architecture')
for x in (-2.15,.15,2.45): s.box(f'Mullion{x}',(.055,2.74,.10),(x,1.62,-2.62),'metal','Architecture/Window')
for y in (.27,1.18,2.98): s.box(f'Rail{y}',(4.68,.055,.12),(.15,y,-2.62),'metal','Architecture/Window')
s.box('BalconyFloor',(4.7,.10,.8),(.15,.06,-3.12),'city','Architecture')
for i in range(17): s.box(f'BalconyBar{i}',(.022,.91,.022),(-2.08+i*.28,.75,-3.52),'metal','Architecture')
s.box('BalconyRail',(4.8,.04,.04),(.15,1.22,-3.52),'metal','Architecture')
for side,x in enumerate((-2.20,2.46)):
    for i in range(9):
        s.cylinder(f'Curtain{side}_{i}',.07,2.82,(x+(i-4)*.075,1.56,-2.47),'linen','Architecture')
s.node('Furniture','Node3D')
for name,pos in [('desk',(-1.8,0,-1.15)),('bookshelf',(-2.78,0,.25)),('bed',(2.05,0,-.15)),('low_table',(0,0,.9))]: s.instance(name,name,'Furniture',pos)
s.box('Rug',(3.35,.025,2.35),(-.2,.02,1.03),'rug','Furniture')
for i in range(30): s.box(f'RugWeave{i}',(3.3,.004,.012),(-.2,.036,-.1+i*.078),'linen','Furniture')
s.sphere('FloorCushion',(.95,.18,.69),(.87,.13,1.45),'linen','Furniture')
s.cylinder('KnittedPouf',.4,.28,(-.66,.16,1.87),'pink','Furniture')
for i in range(16):
    a=i*math.tau/16
    s.cylinder(f'PoufRib{i}',.016,.22,(-.66+math.sin(a)*.395,.16,1.87+math.cos(a)*.395),'linen','Furniture')
plant(s,'WindowPlant',(.95,.13,-2.98),1.6)
plant(s,'DeskPlant',(-2.33,0,-.22),.95)
plant(s,'ShelfPlant',(-2.83,2.83,.58),.7)
plant(s,'BedsidePlant',(2.85,.77,-1.75),.8)
s.box('BedsideCabinet',(.62,.76,.5),(2.88,.38,-1.77),'wood','Furniture',True)
s.sphere('BedsideGlow',(.22,.22,.22),(2.66,.91,-1.65),'light','Furniture')
s.node('BedsideLight','OmniLight3D','Furniture',(2.66,1.1,-1.65),extra='light_color = Color(1,0.72,0.43,1)\nlight_energy = 0.6\nomni_range = 3.0')
s.cylinder('Pendant',.45,.34,(0,2.98,-.1),'cream','Furniture')
s.cylinder('PendantBottom',.43,.015,(0,2.805,-.1),'light','Furniture')
s.cylinder('PendantWire',.012,.25,(0,3.2,-.1),'metal','Furniture')
s.node('PendantLight','OmniLight3D','Furniture',(0,2.67,-.1),extra='light_color = Color(1,0.79,0.56,1)\nlight_energy = 1.25\nomni_range = 6.5\nshadow_enabled = true')
# Wall prints, and a compact air conditioner echo the reference without new characters.
s.box('ArtFrame',(.03,.80,.55),(-3.21,2.15,-1.3),'oak','Furniture')
s.box('ArtPrint',(.035,.69,.44),(-3.185,2.15,-1.3),'screen','Furniture')
s.box('AirConditioner',(.28,.40,.98),(3.11,2.91,-1.92),'cream','Furniture')
for i in range(4): s.box(f'ACSlat{i}',(.015,.014,.83),(2.96,2.78+i*.028,-1.92),'linen','Furniture')
s.node('TokyoInspiredSkyline','Node3D')
# Stylized backdrop only, not simulated city geometry or an exact Tokyo survey.
for row in range(2):
    for i in range(13):
        x=-7+i*1.13
        h=1.3+((i*7+row*3)%8)*.34
        z=-6.4-row*3
        s.box(f'Building{row}_{i}',(.88,h,.85),(x,h/2-.1,z),'city' if row else 'city2','TokyoInspiredSkyline')
        for floor in range(int(h/.35)):
            for col in range(3):
                s.box(f'CityWindow{row}_{i}_{floor}_{col}',(.12,.15,.014),(x-.27+col*.27,.22+floor*.35,z+.432),'light' if (i+floor+col)%4==0 else 'glass','TokyoInspiredSkyline')
s.cylinder('TowerShaft',.065,3.8,(1.45,2.8,-10.5),'city','TokyoInspiredSkyline',top=.025)
s.cylinder('TowerDeck',.15,.12,(1.45,3.55,-10.5),'city2','TokyoInspiredSkyline')
s.cylinder('TowerAntenna',.014,1.1,(1.45,5.1,-10.5),'metal','TokyoInspiredSkyline',top=.004)
s.resources += ['[sub_resource type="ProceduralSkyMaterial" id="sky_material"]\nsky_top_color = Color(0.35,0.48,0.64,1)\nsky_horizon_color = Color(0.9,0.65,0.49,1)\nground_bottom_color = Color(0.16,0.19,0.24,1)', '[sub_resource type="Sky" id="sky"]\nsky_material = SubResource("sky_material")', '[sub_resource type="Environment" id="environment"]\nbackground_mode = 2\nsky = SubResource("sky")\nambient_light_source = 3\nambient_light_color = Color(0.75,0.81,1,1)\nambient_light_energy = 0.5\ntonemap_mode = 2']
s.node('WorldEnvironment','WorldEnvironment',extra='environment = SubResource("environment")')
s.node('Sun','DirectionalLight3D',rot=(-.6,-.6,0),extra='light_color = Color(1,0.79,0.6,1)\nlight_energy = 0.6\nshadow_enabled = true\ndirectional_shadow_max_distance = 35.0')
s.node('Camera3D','Camera3D',pos=(0,2.35,5.7),extra='current = true\nfov = 39.0\nnear = 0.05\nfar = 80.0')
s.save('scenes/akari_room.tscn')
print('Built AKARI Room and four editable prop scenes.')
