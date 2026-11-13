extends RefCounted
# Art-preview city: real 3D depth. Does not modify WORLD time/weather or room geometry.
var city: Node3D
var mats: Dictionary = {}
var rng := RandomNumberGenerator.new()

func build(root: Node3D, room: Node3D) -> void:
    for node in room.get_children():
        var name_text := str(node.name).to_lower()
        if node is GeometryInstance3D and (name_text.begins_with("tokyo") or name_text.begins_with("rooftop") or name_text.begins_with("tower")):
            node.visible=false
    city=Node3D.new();city.name="Tokyo layered city v8";root.add_child(city)
    rng.seed=81311
    # Left foreground and right middle ground frame an open view toward the tower.
    _building(Vector3(-2.15,-4.5,-12.2),Vector3(2.15,7.0,2.2),0)
    _building(Vector3(-1.4,-4.5,-15.4),Vector3(1.35,5.8,1.8),1)
    _building(Vector3(-4.9,-4.5,-17.8),Vector3(1.7,6.4,2.3),2)
    _building(Vector3(2.8,-4.5,-14.6),Vector3(2.0,6.2,2.1),1)
    _building(Vector3(1.7,-4.5,-20.2),Vector3(1.15,4.9,1.9),2)
    _building(Vector3(0.0,-4.5,-22.6),Vector3(1.3,5.1,2.1),0)
    _building(Vector3(5.6,-4.5,-19.0),Vector3(1.9,7.0,1.8),0)
    _building(Vector3(-2.8,-4.5,-24.2),Vector3(1.1,5.0,1.6),2)
    for i in range(19):
        var h: float=rng.randf_range(3.8,7.0)
        _building(Vector3(-16.0+float(i)*1.7,-5.1,rng.randf_range(-39.0,-32.0)),Vector3(rng.randf_range(0.9,1.5),h,1.5),3)
    load("res://scripts/skytree_v9.gd").new().build(city,Vector3(4.0,-4.8,-45.0),12.4)
    _railway()
    # Blossom geometry is supplied by living_details_v10.
    print("CITY_V8 layered_buildings=27 tower=true rail=true")

func _mat(key: String, color: Color, roughness: float=0.86) -> StandardMaterial3D:
    if not mats.has(key):
        var m:=StandardMaterial3D.new();m.albedo_color=color;m.roughness=roughness;mats[key]=m
    return mats[key]

func _mesh(label: String, mesh: Mesh, position: Vector3, material: Material) -> MeshInstance3D:
    var n:=MeshInstance3D.new();n.name=label;n.mesh=mesh;n.position=position;n.material_override=material
    n.gi_mode=GeometryInstance3D.GI_MODE_DISABLED;n.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    city.add_child(n);return n

func _box(label: String, position: Vector3, size: Vector3, material: Material) -> MeshInstance3D:
    var b:=BoxMesh.new();b.size=size;return _mesh(label,b,position,material)

func _building(base: Vector3, size: Vector3, style: int) -> void:
    var palettes: Array[Color]=[Color(0.67,0.63,0.55),Color(0.57,0.60,0.59),Color(0.69,0.67,0.61),Color(0.55,0.63,0.68)]
    var concrete:=_mat("concrete_%s"%style,palettes[style])
    var trim:=_mat("edge_%s"%style,palettes[style]*0.83)
    var glass:=_mat("glazing_%s"%style,Color(0.28,0.37,0.42).lerp(palettes[style],0.30 if style==3 else 0.0),0.38)
    var frame:=_mat("window frames",Color(0.60,0.62,0.60),0.52)
    _box("Urban block",base+Vector3(0,size.y*0.5,0),size,concrete)
    var front: float=base.z+size.z*0.5
    var roof: float=base.y+size.y
    _box("Roof cornice",Vector3(base.x,roof+0.035,base.z),Vector3(size.x+0.05,0.07,size.z+0.05),trim)
    _box("Recessed rooftop stairwell",Vector3(base.x-size.x*0.2,roof+0.19,base.z-0.2),Vector3(size.x*0.32,0.38,size.z*0.40),concrete)
    if style!=3:
        _box("Rooftop vent housing",Vector3(base.x+size.x*0.23,roof+0.12,base.z-0.15),Vector3(0.22,0.24,0.25),trim)
        for antenna in range(2):
            _rod(Vector3(base.x-0.18+antenna*0.15,roof+0.35,base.z),Vector3(base.x-0.18+antenna*0.15,roof+0.75,base.z),0.008,frame)
    var floor_height: float=0.68 if style!=3 else 0.65
    var columns: int=maxi(2,int(size.x/0.48))
    var step: float=size.x/float(columns)
    var rows: int=int((size.y-0.40)/floor_height)+1
    for row in range(rows):
        var y: float=base.y+0.42+float(row)*floor_height
        if style==1 and row%2==0:
            _box("Projecting balcony slab",Vector3(base.x,y-0.20,front+0.12),Vector3(size.x-0.10,0.045,0.30),trim)
            _box("Balcony front guard",Vector3(base.x,y-0.03,front+0.25),Vector3(size.x-0.13,0.23,0.024),concrete)
        for col in range(columns):
            var x: float=base.x-size.x*0.5+step*(float(col)+0.5)
            var ww: float=step*0.57
            var center:=Vector3(x,y,front+0.012)
            _box("Inset glass",center,Vector3(ww,0.31,0.018),glass)
            if style!=3:
                _box("Window top reveal",center+Vector3(0,0.168,0.008),Vector3(ww+0.035,0.022,0.045),trim)
                _box("Window sill",center+Vector3(0,-0.168,0.022),Vector3(ww+0.05,0.026,0.07),frame)
                _box("Window vertical mullion",center+Vector3(0,0,0.018),Vector3(0.012,0.30,0.012),frame)
                if (col+row*3)%7==0:
                    var curtain:=_mat("distant indoor curtain",Color(0.69,0.67,0.58))
                    _box("Partly drawn indoor curtain",center+Vector3(ww*0.23,0,0.013),Vector3(ww*0.42,0.27,0.01),curtain)
        # Side facade remains visible from an oblique room camera.
        if style!=3:
            var side_x: float=base.x+size.x*0.5+0.012
            for side_col in range(2):
                _box("Side facade window",Vector3(side_x,y,base.z+float(side_col)*0.56),Vector3(0.018,0.29,0.24),glass)

func _rod(a: Vector3,b: Vector3,radius: float,material: Material) -> void:
    var length: float=a.distance_to(b)
    var mesh:=CylinderMesh.new();mesh.top_radius=radius;mesh.bottom_radius=radius;mesh.height=length;mesh.radial_segments=8
    var n:=_mesh("Structural member",mesh,(a+b)*0.5,material)
    var direction: Vector3=(b-a).normalized()
    n.quaternion=Quaternion(Vector3.UP,direction)

func _taper(position: Vector3,height: float,bottom: float,top: float,material: Material) -> void:
    var mesh:=CylinderMesh.new();mesh.bottom_radius=bottom*1.4;mesh.top_radius=top*1.4;mesh.height=height;mesh.radial_segments=24
    _mesh("Tower tapered body",mesh,position+Vector3(0,height*0.5,0),material)

func _tower(base: Vector3,height: float) -> void:
    var steel:=_mat("tower silver atmospheric",Color(0.61,0.67,0.70),0.62)
    var shade:=_mat("tower glazing",Color(0.38,0.48,0.55),0.45)
    _taper(base,height*0.55,0.18,0.055,steel)
    _taper(base+Vector3(0,height*0.55,0),height*0.23,0.055,0.033,steel)
    _taper(base+Vector3(0,height*0.78,0),height*0.22,0.026,0.007,steel)
    for deck in [[0.55,0.19,0.18],[0.73,0.12,0.12]]:
        var y: float=base.y+height*float(deck[0])
        _taper(Vector3(base.x,y-0.13,base.z),0.13,float(deck[1])*0.55,float(deck[1]),steel)
        _taper(Vector3(base.x,y,base.z),float(deck[2]),float(deck[1]),float(deck[1])*0.85,shade)
        _taper(Vector3(base.x,y+float(deck[2]),base.z),0.035,float(deck[1])*1.10,float(deck[1])*1.10,steel)
    # Sparse structural diagonals; no oversized open wire cage in the distant silhouette.
    for side in range(3):
        var angle: float=float(side)*TAU/3.0
        for level in range(12):
            var t: float=float(level)/12.0
            var t2: float=float(level+1)/12.0
            var r: float=lerpf(0.20,0.058,t)
            var r2: float=lerpf(0.20,0.058,t2)
            var a:=base+Vector3(cos(angle)*r,height*0.54*t,sin(angle)*r)
            var b:=base+Vector3(cos(angle+0.65)*r2,height*0.54*t2,sin(angle+0.65)*r2)
            _rod(a,b,0.006,steel)

func _railway() -> void:
    var first_child: int=city.get_child_count()
    var concrete:=_mat("viaduct",Color(0.45,0.47,0.46))
    var steel:=_mat("rail steel",Color(0.27,0.31,0.32),0.6)
    _box("Rail viaduct deck",Vector3(0,-0.38,-18),Vector3(16,0.16,0.70),concrete)
    _box("Rail viaduct fascia",Vector3(0,-0.29,-17.62),Vector3(16,0.16,0.045),steel)
    for x in [-6.0,-2.5,1.0,4.5]:
        _box("Viaduct support pier",Vector3(x,-2.4,-18),Vector3(0.22,4.0,0.35),concrete)
    var body:=_mat("commuter body",Color(0.76,0.78,0.74),0.48)
    var stripe:=_mat("commuter green stripe",Color(0.30,0.43,0.27),0.6)
    var glass:=_mat("commuter windows",Color(0.24,0.34,0.39),0.35)
    for car in range(4):
        var x: float=-4.5+float(car)*1.8
        _box("Commuter carriage",Vector3(x,-0.03,-18),Vector3(1.72,0.45,0.47),body)
        _box("Commuter narrow color band",Vector3(x,-0.09,-17.758),Vector3(1.69,0.045,0.008),stripe)
        for win in range(7):
            _box("Commuter carriage window",Vector3(x-0.68+float(win)*0.225,0.07,-17.756),Vector3(0.16,0.17,0.01),glass)
        for axle in [-0.54,0.54]:
            _box("Commuter bogie",Vector3(x+axle,-0.27,-18),Vector3(0.28,0.10,0.30),steel)

    for child_index in range(first_child,city.get_child_count()):
        var segment:=city.get_child(child_index) as Node3D
        segment.position.y+=0.9

func _cherry_tree(base: Vector3) -> void:
    var bark:=_mat("cherry bark",Color(0.30,0.24,0.21))
    _rod(base,base+Vector3(0,1.65,0),0.035,bark)
    var blossoms:=SphereMesh.new();blossoms.radius=1.0;blossoms.height=2.0;blossoms.radial_segments=8;blossoms.rings=4
    var bloom:=_mat("cherry pale blossoms",Color(0.79,0.56,0.60),0.94)
    var batch:=MultiMesh.new();batch.transform_format=MultiMesh.TRANSFORM_3D;batch.mesh=blossoms;batch.instance_count=640
    for i in range(640):
        var offset:=Vector3(rng.randf_range(-0.85,0.85),rng.randf_range(-0.28,0.40),rng.randf_range(-0.40,0.40))
        var size: float=rng.randf_range(0.025,0.065)
        batch.set_instance_transform(i,Transform3D(Basis.IDENTITY.scaled(Vector3(size,size*0.65,size)),base+Vector3(0,1.65,0)+offset))
    var n:=MultiMeshInstance3D.new();n.name="Cherry blossom canopy";n.multimesh=batch;n.material_override=bloom;n.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF;city.add_child(n)
