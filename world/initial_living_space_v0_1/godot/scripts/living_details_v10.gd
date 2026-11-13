extends RefCounted
# Native 3D props only; room, avatar, controller and WORLD state are unchanged.
var stage: Node3D
var rng:=RandomNumberGenerator.new()
var materials: Dictionary={}
var leaves: Array[Array]=[]
var leaf_meshes: Array[ArrayMesh]=[]
var leaf_material: StandardMaterial3D

func build(root: Node3D, room: Node3D) -> void:
    stage=Node3D.new();stage.name="Living details v10";root.add_child(stage)
    rng.seed=10113
    _books(room)
    _laptop(room)
    _plants()
    _blossoms()
    print("LIVING_V10 books=bound_spines laptop=true plants=curved_branch_connected blossoms=petals")

func _mat(key: String, color: Color, rough: float=0.85, metal: float=0.0) -> StandardMaterial3D:
    if not materials.has(key):
        var m:=StandardMaterial3D.new();m.albedo_color=color;m.roughness=rough;m.metallic=metal;materials[key]=m
    return materials[key]

func _mesh(parent: Node3D, name_text: String, mesh: Mesh, position: Vector3, material: Material) -> MeshInstance3D:
    var n:=MeshInstance3D.new();n.name=name_text;n.mesh=mesh;n.position=position;n.material_override=material
    n.gi_mode=GeometryInstance3D.GI_MODE_STATIC;parent.add_child(n);return n

func _box(parent: Node3D, name_text: String, position: Vector3, size: Vector3, material: Material) -> MeshInstance3D:
    var mesh:=BoxMesh.new();mesh.size=size;return _mesh(parent,name_text,mesh,position,material)

func _cylinder(parent: Node3D, name_text: String, position: Vector3, height: float, bottom: float, top: float, material: Material) -> MeshInstance3D:
    var mesh:=CylinderMesh.new();mesh.height=height;mesh.bottom_radius=bottom;mesh.top_radius=top;mesh.radial_segments=32
    return _mesh(parent,name_text,mesh,position,material)

func _rod(parent: Node3D, a: Vector3, b: Vector3, radius: float, material: Material) -> void:
    var n:=_cylinder(parent,"Branch or leaf petiole",(a+b)*0.5,a.distance_to(b),radius,radius*0.65,material)
    n.quaternion=Quaternion(Vector3.UP,(b-a).normalized())

func _surface(parent: Node3D, position: Vector3, width: float, height: float, material: Material) -> MeshInstance3D:
    var mesh:=QuadMesh.new();mesh.size=Vector2(width,height)
    var n:=_mesh(parent,"Printed surface",mesh,position,material);n.rotation.y=PI*0.5;return n

func _books(room: Node3D) -> void:
    var palette: Array[Color]=[Color(0.20,0.27,0.28),Color(0.56,0.39,0.28),Color(0.75,0.71,0.57),Color(0.84,0.82,0.71),Color(0.31,0.39,0.32),Color(0.44,0.30,0.28),Color(0.24,0.29,0.37),Color(0.65,0.57,0.44)]
    var paper:=_mat("paper",Color(0.88,0.86,0.80),0.96)
    paper.albedo_texture=load("res://textures/living/paper_edges.png")
    var shelf_index: int=0
    for shelf in room.get_children():
        if not shelf is MeshInstance3D or not str(shelf.name).begins_with("Bookshelf oak shelf"):continue
        var bounds: AABB=shelf.global_transform*shelf.get_aabb()
        if bounds.end.y>2.0:continue
        var cursor: float=bounds.position.z+0.04
        var count: int=0
        while cursor<bounds.end.z-0.30 and count<23:
            var width: float=rng.randf_range(0.019,0.043)
            var height: float=rng.randf_range(0.19,0.29)
            var depth: float=rng.randf_range(0.18,0.24)
            var style: int=rng.randi_range(0,15)
            var book:=Node3D.new();book.name="Bound book %s-%s"%[shelf_index,count];stage.add_child(book)
            book.position=Vector3(bounds.end.x-0.028-depth*0.5,bounds.end.y+0.001,cursor+width*0.5)
            if count%7==4:book.rotation.x=deg_to_rad(-rng.randf_range(2.0,5.0))
            var cover:=_mat("cover_%s"%(style%8),palette[style%8],0.78)
            _box(book,"Continuous paper block",Vector3(0,height*0.5,0),Vector3(depth-0.008,height-0.008,width-0.005),paper)
            _box(book,"Book cover one",Vector3(0,height*0.5,-width*0.5),Vector3(depth,height,0.0025),cover)
            _box(book,"Book cover two",Vector3(0,height*0.5,width*0.5),Vector3(depth,height,0.0025),cover)
            _box(book,"Bound curved spine support",Vector3(depth*0.5-0.002,height*0.5,0),Vector3(0.006,height,width),cover)
            var spine:=_mat("spine_%02d"%style,Color.WHITE,0.82)
            spine.albedo_texture=load("res://textures/living/spine_%02d.png"%style)
            spine.texture_filter=BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
            _surface(book,Vector3(depth*0.5+0.0025,height*0.5,0),width,height-0.001,spine)
            cursor+=width+0.0025
            if count%6==4:cursor+=rng.randf_range(0.013,0.032)
            count+=1
        if shelf_index%2==0:
            for stack in range(3):
                var h: float=0.022+float(stack)*0.002
                var center:=Vector3(bounds.end.x-0.14,bounds.end.y+0.013+float(stack)*0.027,bounds.end.z-0.17)
                var cover:=_mat("stack_cover_%s"%stack,palette[(stack+shelf_index)%8],0.83)
                _box(stage,"Horizontal book pages",center,Vector3(0.23,h,0.25),paper)
                _box(stage,"Horizontal book upper cover",center+Vector3(0,h*0.5,0),Vector3(0.235,0.0025,0.255),cover)
                _box(stage,"Horizontal bound spine",center+Vector3(0.117,0,0),Vector3(0.004,h+0.003,0.255),cover)
        shelf_index+=1

func _laptop(room: Node3D) -> void:
    for top in room.get_children():
        if not top is MeshInstance3D or not str(top.name).begins_with("Desk solid oak top"):continue
        var bounds: AABB=top.global_transform*top.get_aabb()
        var center:=Vector3(bounds.get_center().x-0.01,bounds.end.y+0.010,bounds.get_center().z)
        var aluminum:=_mat("laptop aluminum",Color(0.42,0.45,0.46),0.40,0.65)
        var black:=_mat("keyboard graphite",Color(0.035,0.04,0.045),0.68)
        _box(stage,"Laptop thin base",center,Vector3(0.255,0.014,0.355),aluminum)
        var keys: int=0
        for row in range(5):
            for col in range(13):
                _box(stage,"Keyboard key",center+Vector3(-0.065+row*0.022,0.0085,-0.143+col*0.023),Vector3(0.017,0.0025,0.018),black)
                keys+=1
        _box(stage,"Trackpad inset",center+Vector3(0.085,0.0075,0),Vector3(0.055,0.001,0.105),_mat("trackpad",Color(0.35,0.38,0.39),0.58,0.4))
        var hinge:=Node3D.new();hinge.name="Laptop opened display hinge";stage.add_child(hinge)
        hinge.position=center+Vector3(-0.123,0.006,0);hinge.rotation.z=deg_to_rad(15)
        _box(hinge,"Laptop display back",Vector3(0,0.112,0),Vector3(0.009,0.224,0.355),aluminum)
        _box(hinge,"Display bezel",Vector3(0.005,0.112,0),Vector3(0.002,0.211,0.343),black)
        var display:=_mat("desktop screen",Color.WHITE,0.34)
        display.albedo_texture=load("res://textures/living/laptop_desktop.png")
        display.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
        display.texture_filter=BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
        _surface(hinge,Vector3(0.008,0.114,0),0.324,0.192,display)
        _box(hinge,"Tiny webcam",Vector3(0.007,0.216,0),Vector3(0.0015,0.003,0.003),black)
        print("LAPTOP_V10 keys=",keys," top_y=",bounds.end.y," center=",center)

func _leaf_mesh(variation: int) -> ArrayMesh:
    var st:=SurfaceTool.new();st.begin(Mesh.PRIMITIVE_TRIANGLES)
    var rows: int=18
    var points: Array[Vector3]=[]
    var uvs: Array[Vector2]=[]
    for row in range(rows+1):
        var t: float=float(row)/float(rows)
        var width: float=pow(sin(PI*t),0.72)*(0.045+float(variation)*0.003)
        for side in range(3):
            var across: float=float(side-1)
            var ridge: float=0.009*sin(PI*t)*(1.0-absf(across))
            var droop: float=-0.022*t*t
            points.append(Vector3(across*width*(1.0+0.07*sin(t*8.0+variation)),0.17*t,ridge+droop))
            uvs.append(Vector2(float(side)*0.5,1.0-t))
    for row in range(rows):
        for side in range(2):
            var a: int=row*3+side;var b: int=a+1;var c: int=a+3;var d: int=c+1
            for idx in [a,b,c,b,d,c]:st.set_uv(uvs[idx]);st.add_vertex(points[idx])
    st.generate_normals();return st.commit()

func _plants() -> void:
    leaf_material=_mat("leaf",Color.WHITE,0.72)
    leaf_material.albedo_texture=load("res://textures/living/leaf_color.png")
    leaf_material.cull_mode=BaseMaterial3D.CULL_DISABLED
    leaf_material.backlight_enabled=true;leaf_material.backlight=Color(0.12,0.20,0.07)
    for i in range(4):leaf_meshes.append(_leaf_mesh(i));leaves.append([])
    _potted(Vector3(-1.00,-0.035,-5.05),true)
    _potted(Vector3(1.04,-0.035,-5.14),false)
    for variant in range(4):
        _batch("Curved leaves variant %s"%variant,leaf_meshes[variant],leaves[variant],leaf_material)

func _potted(base: Vector3, tall_pot: bool) -> void:
    var pot_h: float=0.40 if tall_pot else 0.29
    var radius: float=0.125 if tall_pot else 0.16
    var ceramic:=_mat("ivory pot" if tall_pot else "earth pot",Color(0.77,0.76,0.68) if tall_pot else Color(0.54,0.39,0.28),0.86)
    var dirt:=_mat("soil",Color(0.10,0.075,0.045),0.98)
    var bark:=_mat("young branches",Color(0.27,0.29,0.16),0.92)
    _cylinder(stage,"Open tapered plant pot",base+Vector3(0,pot_h*0.5,0),pot_h,radius*0.76,radius,ceramic)
    _cylinder(stage,"Pot raised lip",base+Vector3(0,pot_h-0.009,0),0.024,radius*1.025,radius*1.025,ceramic)
    _cylinder(stage,"Recessed soil",base+Vector3(0,pot_h+0.004,0),0.008,radius*0.91,radius*0.91,dirt)
    var stems: int=3 if tall_pot else 4
    for main in range(stems):
        var angle: float=float(main)*2.39996+0.4
        var height: float=rng.randf_range(0.64,0.97)
        var start:=base+Vector3(cos(angle)*0.02,pot_h,sin(angle)*0.02)
        var end:=start+Vector3(cos(angle)*0.10,height,sin(angle)*0.10)
        _rod(stage,start,end,0.007 if not tall_pot else 0.0045,bark)
        for branch in range(6):
            var t: float=0.24+float(branch)*0.125
            var attach: Vector3=start.lerp(end,t)
            var phi: float=angle+float(branch)*2.39996
            var branch_end:=attach+Vector3(cos(phi)*rng.randf_range(0.12,0.23),rng.randf_range(0.025,0.09),sin(phi)*rng.randf_range(0.12,0.23))
            _rod(stage,attach,branch_end,0.0028,bark)
            for leaf in range(4):
                var point: Vector3=attach.lerp(branch_end,0.3+float(leaf)*0.23)
                var leaf_angle: float=phi+(0.75 if leaf%2==0 else -0.75)
                var direction:=Vector3(cos(leaf_angle),rng.randf_range(0.06,0.36),sin(leaf_angle)).normalized()
                var petiole: Vector3=point+direction*0.035
                _rod(stage,point,petiole,0.0011,bark)
                var x_axis: Vector3=direction.cross(Vector3.UP).normalized()
                var z_axis: Vector3=x_axis.cross(direction).normalized()
                var size: float=rng.randf_range(0.85,1.35)*(0.75 if tall_pot else 1.0)
                var basis:=Basis(x_axis,direction,z_axis)*Basis(Vector3.UP,rng.randf_range(-1.15,1.15))
                basis=basis.scaled(Vector3.ONE*size)
                leaves[rng.randi_range(0,3)].append(Transform3D(basis,petiole))

func _batch(label: String, mesh: Mesh, transforms: Array, material: Material) -> void:
    var batch:=MultiMesh.new();batch.transform_format=MultiMesh.TRANSFORM_3D;batch.mesh=mesh;batch.instance_count=transforms.size()
    for i in range(transforms.size()):batch.set_instance_transform(i,transforms[i])
    var n:=MultiMeshInstance3D.new();n.name=label;n.multimesh=batch;n.material_override=material;n.gi_mode=GeometryInstance3D.GI_MODE_STATIC
    n.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF;stage.add_child(n)

func _blossoms() -> void:
    var base:=Vector3(-2.0,-0.7,-9.0)
    var bark:=_mat("cherry branches",Color(0.30,0.25,0.22))
    _rod(stage,base,base+Vector3(0,1.2,0),0.033,bark)
    var flower:=SurfaceTool.new();flower.begin(Mesh.PRIMITIVE_TRIANGLES)
    for petal in range(5):
        var angle: float=float(petal)*TAU/5.0
        var center:=Vector3(cos(angle)*0.012,sin(angle)*0.012,0)
        for corner in range(8):
            flower.add_vertex(center+Vector3(0,0,0.003))
            for a in [float(corner)*TAU/8.0,float(corner+1)*TAU/8.0]:
                var pos:=Vector3(cos(a)*0.008,sin(a)*0.012,0)
                flower.add_vertex(center+pos)
    flower.generate_normals()
    var mesh:=flower.commit()
    var bloom:=_mat("pale cherry petals",Color(0.91,0.76,0.77),0.96);bloom.cull_mode=BaseMaterial3D.CULL_DISABLED
    var transforms: Array[Transform3D]=[]
    for branch in range(12):
        var angle: float=float(branch)*2.39996
        var start:=base+Vector3(0,0.6+float(branch%4)*0.18,0)
        var end:=base+Vector3(cos(angle)*rng.randf_range(0.45,0.85),rng.randf_range(1.2,1.7),sin(angle)*0.36)
        _rod(stage,start,end,0.010,bark)
        for twig in range(5):
            var a: Vector3=start.lerp(end,0.28+float(twig)*0.14)
            var b: Vector3=a+Vector3(rng.randf_range(-0.18,0.18),rng.randf_range(0.08,0.26),rng.randf_range(-0.16,0.16))
            _rod(stage,a,b,0.0025,bark)
            for i in range(40):
                var location: Vector3=a.lerp(b,rng.randf())+Vector3(rng.randf_range(-0.04,0.04),rng.randf_range(-0.035,0.035),rng.randf_range(-0.04,0.04))
                var basis:=Basis.from_euler(Vector3(rng.randf_range(-0.7,0.7),rng.randf_range(-0.9,0.9),rng.randf_range(-PI,PI)))
                transforms.append(Transform3D(basis.scaled(Vector3.ONE*rng.randf_range(0.7,1.3)),location))
    _batch("Five petal cherry blossoms",mesh,transforms,bloom)
