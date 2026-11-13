extends RefCounted
# Official outline: 634 m total, Tembo Deck 350 m, Tembo Galleria 450 m.
# Architectural appearance model; section widths estimated visually from official photos.
const SOURCE := "https://www.tokyo-skytree.jp/about/outline/"
const TOTAL := 634.0
var tower: Node3D
var origin: Vector3
var scale_m: float
var steel: StandardMaterial3D
var glazing: StandardMaterial3D
var ribs: Array[Transform3D] = []
var braces: Array[Transform3D] = []

func build(parent: Node3D, base: Vector3, scene_height: float) -> Node3D:
    tower=Node3D.new();tower.name="Skytree rebuilt v9";parent.add_child(tower)
    origin=base;scale_m=scene_height/TOTAL
    steel=_material(Color(0.77,0.82,0.85),0.72,0.0)
    steel.emission_enabled=true;steel.emission=Color(0.58,0.67,0.73);steel.emission_energy_multiplier=0.035
    glazing=_material(Color(0.34,0.45,0.52),0.40,0.12)
    var recessed:=_material(Color(0.54,0.63,0.67),0.85,0.0)
    # Interior silhouette keeps the fine distant lattice from disappearing into a hairline.
    _frustum(0,330,11.0,7.0,recessed)
    _frustum(354,440,6.0,5.2,recessed)
    # Main lattice members follow a curved, tapering envelope.
    var levels: int=34
    var columns: int=12
    for level in range(levels):
        var y0: float=float(level)*330.0/float(levels)
        var y1: float=float(level+1)*330.0/float(levels)
        for column in range(columns):
            var angle: float=TAU*float(column)/float(columns)
            var next_angle: float=TAU*float(column+1)/float(columns)
            var a:=_envelope(y0,angle)
            var b:=_envelope(y1,angle)
            var c:=_envelope(y1,next_angle)
            _bar(ribs,a,b,0.80)
            _bar(braces,a,c,0.43)
            _bar(braces,_envelope(y0,next_angle),b,0.43)
            if level%2==0:
                _bar(braces,a,_envelope(y0,next_angle),0.40)
    # Large lower observation deck: flared underside, glass bands, overhanging roof.
    _frustum(330,339,14.8,19.1,steel)
    _frustum(339,345,19.1,21.5,glazing)
    _frustum(345,350,21.5,23.0,glazing)
    for band in [339.0,343.0,346.5,350.0]:
        var radius: float=lerpf(19.1,23.0,(band-339.0)/11.0)
        _frustum(band,band+0.7,radius+0.4,radius+0.4,steel)
    _frustum(350.7,353.5,24.0,24.0,steel)
    _frustum(353.5,357.0,18.0,15.0,steel)
    # Upper lattice neck, including the service platforms below the Galleria.
    for level in range(11):
        var y0: float=357.0+float(level)*8.0
        var y1: float=y0+8.0
        for side in range(12):
            var a: float=TAU*float(side)/12.0
            var b: float=a+TAU/12.0
            var r0: float=lerpf(12.0,8.3,(y0-357.0)/88.0)
            var r1: float=lerpf(12.0,8.3,(y1-357.0)/88.0)
            _bar(ribs,Vector3(cos(a)*r0,y0,sin(a)*r0),Vector3(cos(a)*r1,y1,sin(a)*r1),0.58)
            _bar(braces,Vector3(cos(a)*r0,y0,sin(a)*r0),Vector3(cos(b)*r1,y1,sin(b)*r1),0.35)
    for platform in [366.0,381.0,396.0,411.0,426.0]:
        _frustum(platform,platform+0.7,13.0,13.0,steel)
    # Smaller Galleria has an inclined spiral walkway, not a second copy of the Deck.
    _frustum(438,442,9.0,14.0,steel)
    _frustum(442,450,14.0,15.0,glazing)
    _frustum(450,453,16.0,15.5,steel)
    for i in range(72):
        var t0: float=float(i)/72.0
        var t1: float=float(i+1)/72.0
        _bar(ribs,Vector3(cos(t0*TAU)*15.2,443.0+t0*7.0,sin(t0*TAU)*15.2),Vector3(cos(t1*TAU)*15.2,443.0+t1*7.0,sin(t1*TAU)*15.2),0.65)
    # Lattice crown up to the antenna base.
    _frustum(453,495,5.2,4.8,recessed)
    for y in range(454,494,8):
        for side in range(10):
            var a: float=TAU*float(side)/10.0
            var b: float=a+TAU/10.0
            _bar(ribs,Vector3(cos(a)*7.0,float(y),sin(a)*7.0),Vector3(cos(a)*6.6,float(y+8),sin(a)*6.6),0.45)
            _bar(braces,Vector3(cos(a)*7.0,float(y),sin(a)*7.0),Vector3(cos(b)*6.6,float(y+8),sin(b)*6.6),0.33)
    _frustum(495,498,8.0,8.0,steel)
    # Gain tower: visible segmented antenna stack instead of one long bare line.
    _frustum(498,530,4.0,3.8,steel)
    _frustum(530,590,3.8,3.1,steel)
    _frustum(590,625,3.1,2.5,steel)
    _frustum(625,634,2.5,1.4,steel)
    for level in range(509,626,9):
        var radius: float=5.1 if level<590 else 4.0
        _frustum(float(level),float(level)+1.1,radius,radius,steel)
    _batch("Main steel ribs",ribs,steel)
    _batch("Fine lattice diagonals",braces,steel)
    print("SKYTREE_V9 total=634 deck=350 galleria=450 ribs=",ribs.size()," braces=",braces.size())
    return tower

func _material(color: Color, roughness: float, metallic: float) -> StandardMaterial3D:
    var m:=StandardMaterial3D.new();m.albedo_color=color;m.roughness=roughness;m.metallic=metallic;return m

func _envelope(height: float, angle: float) -> Vector3:
    var t: float=height/330.0
    var radius: float=lerpf(36.0,14.8,pow(t,0.66))
    # Threefold base gradually becomes circular; appearance approximation, not structural CAD.
    radius*=1.0+0.10*cos(angle*3.0)*pow(1.0-t,2.0)
    return Vector3(cos(angle)*radius,height,sin(angle)*radius)

func _bar(transforms: Array[Transform3D], a: Vector3, b: Vector3, radius: float) -> void:
    var direction: Vector3=(b-a).normalized()
    var basis:=Basis(Quaternion(Vector3.UP,direction))
    basis.x *= radius*scale_m
    basis.y *= a.distance_to(b)*scale_m
    basis.z *= radius*scale_m
    transforms.append(Transform3D(basis,origin+(a+b)*0.5*scale_m))

func _batch(label: String, transforms: Array[Transform3D], material: Material) -> void:
    var unit:=CylinderMesh.new();unit.top_radius=1.0;unit.bottom_radius=1.0;unit.height=1.0;unit.radial_segments=6
    var batch:=MultiMesh.new();batch.transform_format=MultiMesh.TRANSFORM_3D;batch.mesh=unit;batch.instance_count=transforms.size()
    for i in range(transforms.size()):batch.set_instance_transform(i,transforms[i])
    var node:=MultiMeshInstance3D.new();node.name=label;node.multimesh=batch;node.material_override=material;node.layers=3
    node.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF;node.gi_mode=GeometryInstance3D.GI_MODE_DISABLED;tower.add_child(node)

func _frustum(low: float, high: float, bottom_radius: float, top_radius: float, material: Material) -> void:
    var mesh:=CylinderMesh.new();mesh.bottom_radius=bottom_radius*scale_m;mesh.top_radius=top_radius*scale_m;mesh.height=(high-low)*scale_m;mesh.radial_segments=48
    var node:=MeshInstance3D.new();node.name="Tower section %.1f-%.1f"%[low,high];node.mesh=mesh;node.material_override=material
    node.position=origin+Vector3(0,(low+high)*0.5*scale_m,0);node.layers=3;node.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF;node.gi_mode=GeometryInstance3D.GI_MODE_DISABLED;tower.add_child(node)
