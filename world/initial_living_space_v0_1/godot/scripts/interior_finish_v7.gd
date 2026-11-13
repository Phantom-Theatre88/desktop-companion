extends RefCounted
# Presentation-only material pass. No changes to Character control or WORLD state.
var finishes: Dictionary = {}
var wood_shader: Shader

func apply(root: Node3D, room: Node3D) -> void:
    wood_shader = Shader.new()
    wood_shader.code = """
shader_type spatial;
render_mode diffuse_burley, specular_schlick_ggx;
uniform vec4 timber_color : source_color = vec4(0.60,0.36,0.18,1.0);
uniform vec3 grain_axis = vec3(0.0,0.0,1.0);
uniform float offset = 0.0;
varying vec3 wp;
float hash(vec3 p){p=fract(p*0.1031);p+=dot(p,p.yzx+33.33);return fract((p.x+p.y)*p.z);}
float noise3(vec3 p){vec3 i=floor(p);vec3 f=fract(p);f=f*f*(3.0-2.0*f);return mix(mix(mix(hash(i),hash(i+vec3(1,0,0)),f.x),mix(hash(i+vec3(0,1,0)),hash(i+vec3(1,1,0)),f.x),f.y),mix(mix(hash(i+vec3(0,0,1)),hash(i+vec3(1,0,1)),f.x),mix(hash(i+vec3(0,1,1)),hash(i+vec3(1)),f.x),f.y),f.z);}
void vertex(){wp=(MODEL_MATRIX*vec4(VERTEX,1.0)).xyz;}
void fragment(){
 vec3 p=wp+vec3(offset);
 vec3 stretch=mix(vec3(75.0),vec3(0.85),grain_axis);
 float long_grain=noise3(p*stretch);
 float fine=noise3(p*stretch*3.2);
 float broad=noise3(p*vec3(2.0));
 float tone=0.96+(long_grain-0.5)*0.12+(fine-0.5)*0.035+(broad-0.5)*0.035;
 ALBEDO=timber_color.rgb*tone;
 ROUGHNESS=0.52+(fine-0.5)*0.10;
 METALLIC=0.0;
 SPECULAR=0.32;
}
"""
    _finish_tree(room)
    # Distant city is supplied by city_v8.
    # Books are supplied by living_details_v10.
    _reading_lamp(root, room)
    print("INTERIOR_V7 finishes=", finishes.size(), " seamless_materials=true")

func _wood(key: String, color: Color, axis: Vector3 = Vector3(0,0,1)) -> ShaderMaterial:
    if not finishes.has(key):
        var m := ShaderMaterial.new()
        m.shader = wood_shader
        m.set_shader_parameter("timber_color", color)
        m.set_shader_parameter("grain_axis", axis)
        m.set_shader_parameter("offset", float(finishes.size()) * 0.17)
        finishes[key] = m
    return finishes[key]

func _plain(key: String, color: Color, rough: float = 0.85, metal: float = 0.0) -> StandardMaterial3D:
    if not finishes.has(key):
        var m := StandardMaterial3D.new()
        m.albedo_color = color
        m.roughness = rough
        m.metallic = metal
        finishes[key] = m
    return finishes[key]

func _cloth(key: String, color: Color, weave_scale: float = 32.0) -> StandardMaterial3D:
    var m := _plain(key, color, 0.94)
    if not m.normal_enabled:
        m.normal_enabled = true
        m.normal_texture = load("res://textures/woven_fine_normal.png")
        m.normal_scale = 0.10
        m.uv1_triplanar = true
        m.uv1_world_triplanar = true
        m.uv1_scale = Vector3.ONE * weave_scale
        m.texture_filter = BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC
        m.cull_mode = BaseMaterial3D.CULL_DISABLED
    return m

func _finish_tree(node: Node) -> void:
    if node is MeshInstance3D:
        var mi := node as MeshInstance3D
        var n := str(mi.name).to_lower()
        var m: Material = null
        if n.begins_with("oak floorboard"):
            var index: int = int(n.split(" ")[2].split(".")[0])
            var variation: float = 1.0 + sin(float(index)*4.3)*0.035
            m = _wood("floor_%s" % (index%5), Color(0.58,0.35,0.19)*variation)
        elif n.begins_with("bookshelf") and not n.contains("pin"):
            var upright: bool = n.contains("upright") or n.contains("back")
            m = _wood("shelf_vertical" if upright else "shelf_horizontal",Color(0.57,0.36,0.20),Vector3(0,1,0) if upright else Vector3(0,0,1))
        elif n.contains("oak") or n.begins_with("single bed") and not (n.contains("duvet") or n.contains("hem")) or n.begins_with("right bedside") or n.contains("timber trim") or n.begins_with("low table"):
            var vertical: bool = n.contains("leg") or n.contains("post") or n.contains("foot")
            m = _wood("furniture_upright" if vertical else "furniture_long",Color(0.65,0.43,0.25),Vector3(0,1,0) if vertical else Vector3(0,0,1))
        elif n.contains("ivory filled duvet") or n.contains("ivory plump pillow") or n.contains("mattress"):
            m = _cloth("ivory_linen",Color(0.86,0.82,0.73),42.0)
        elif n.begins_with("rose blanket"):
            m = _cloth("rose_throw",Color(0.64,0.39,0.39),22.0)
        elif n.begins_with("rose outer curtain") or n.begins_with("curtain tie band"):
            var curtain := _cloth("curtain",Color(0.60,0.39,0.37),36.0)
            curtain.backlight_enabled = true
            curtain.backlight = Color(0.10,0.06,0.045)
            m = curtain
        elif n.begins_with("rose chair") or n.contains("chair back perimeter"):
            m = _cloth("chair_rose",Color(0.57,0.36,0.35))
        elif n.begins_with("rose floor") or n.begins_with("small rose") or n.begins_with("pouf panel"):
            m = _cloth("cushions",Color(0.64,0.42,0.40),24.0)
        elif n.begins_with("ivory rug"):
            m = _cloth("rug",Color(0.77,0.73,0.64),18.0)
        elif n.begins_with("drawer") and not (n.contains("pull") or n.contains("foot")):
            m = _plain("painted_cabinet",Color(0.79,0.77,0.70),0.72)
        elif n.begins_with("linen pendant"):
            var shade := _cloth("pendant",Color(0.85,0.74,0.55),24.0)
            shade.emission_enabled = true
            shade.emission = Color(0.46,0.30,0.10)
            shade.emission_energy_multiplier = 0.20
            m = shade
        elif n.begins_with("window") and not (n.contains("clear pane") or n.contains("reveal") or n.contains("lintel") or n.contains("jamb")) or n.contains("handrail") or n.contains("guard upright") or n.contains("curtain pole"):
            m = _plain("bronze_frame",Color(0.15,0.14,0.11),0.42,0.45)
        elif n.begins_with("chair") or n.contains("powdercoat") or n.contains("stretcher") or n.contains("drawer recessed") or n.contains("shelf pin"):
            m = _plain("charcoal_metal",Color(0.065,0.065,0.06),0.66,0.20)
        elif n.begins_with("tokyo facade"):
            m = _plain("city_glass",Color(0.27,0.36,0.40),0.36,0.15)
        elif n.begins_with("tokyo building"):
            var seed_value: int = int(n.split(" ")[2].split(".")[0])
            var colors: Array[Color] = [Color(0.57,0.56,0.52),Color(0.62,0.61,0.57),Color(0.53,0.56,0.56),Color(0.66,0.62,0.55)]
            m = _plain("building_%s" % (seed_value%4),colors[seed_value%4])
        elif n.begins_with("tower"):
            m = _plain("tower_haze",Color(0.55,0.60,0.62),0.64,0.12)
        elif n.contains("parapet") or n.contains("walking surface") or n.contains("structural slab"):
            m = _plain("concrete",Color(0.58,0.58,0.54),0.94)
        elif n.contains("wall") or n.contains("ceiling") or n.contains("lintel") or n.contains("jamb"):
            m = _plain("plaster",Color(0.81,0.78,0.71),0.97)
        if m != null:
            mi.material_override = m
            for i in range(mi.get_surface_override_material_count()):
                mi.set_surface_override_material(i, null)
    for child in node.get_children():
        _finish_tree(child)

func _box(root: Node3D, name_text: String, position: Vector3, dimensions: Vector3, material: Material) -> void:
    var mesh := BoxMesh.new()
    mesh.size = dimensions
    var instance := MeshInstance3D.new()
    instance.name = name_text
    instance.mesh = mesh
    instance.material_override = material
    instance.position = position
    instance.gi_mode = GeometryInstance3D.GI_MODE_STATIC
    root.add_child(instance)

func _distant_city(root: Node3D) -> void:
    var rng := RandomNumberGenerator.new()
    rng.seed = 7106
    var distant := _plain("distant_haze",Color(0.53,0.61,0.66))
    var glass := _plain("distant_windows",Color(0.43,0.52,0.59),0.7)
    for index in range(22):
        var height: float = rng.randf_range(2.0,6.0)
        var x: float = -13.0+float(index)*1.25
        var z: float = rng.randf_range(-26.0,-21.0)
        _box(root,"Distant Tokyo block",Vector3(x,height*0.5-2.4,z),Vector3(rng.randf_range(0.7,1.1),height,0.8),distant)
        for row in range(int(height/0.4)):
            _box(root,"Distant narrow window band",Vector3(x,float(row)*0.4-2.1,z+0.406),Vector3(0.57,0.09,0.012),glass)

func _books(root: Node3D) -> void:
    # Books occupy existing shelves, with the same floor plan and clear circulation.
    var palette: Array[Color] = [Color(0.22,0.30,0.32),Color(0.56,0.32,0.25),Color(0.64,0.59,0.43),Color(0.79,0.75,0.64),Color(0.27,0.36,0.24)]
    # Shelf coordinates are populated from the imported shelf bounds in a separate pass.
    var room: Node = root.get_node("ImportedRoom")
    var rng := RandomNumberGenerator.new()
    rng.seed = 7107
    for shelf in room.get_children():
        if not shelf is MeshInstance3D or not str(shelf.name).begins_with("Bookshelf oak shelf"):
            continue
        var bounds: AABB = shelf.global_transform * shelf.get_aabb()
        if bounds.end.y > 2.00:
            continue
        var cursor: float = bounds.position.z + 0.05
        var count: int = 0
        while cursor < bounds.end.z-0.08 and count < 16:
            var width: float = rng.randf_range(0.025,0.05)
            var height: float = rng.randf_range(0.18,0.26)
            var color_index: int = rng.randi_range(0,4)
            var cover := _plain("book_cover_%s" % color_index,palette[color_index],0.83)
            var paper := _plain("paper",Color(0.82,0.79,0.69),0.94)
            var x: float = bounds.get_center().x
            var y: float = bounds.end.y+height*0.5+0.002
            var z: float = cursor+width*0.5
            _box(root,"Book paper block",Vector3(x,y,z),Vector3(0.165,height-0.008,width-0.006),paper)
            _box(root,"Book bound spine",Vector3(bounds.end.x-0.018,y,z),Vector3(0.012,height,width),cover)
            _box(root,"Book front cover",Vector3(x,y,z-width*0.5),Vector3(0.18,height,0.003),cover)
            _box(root,"Book back cover",Vector3(x,y,z+width*0.5),Vector3(0.18,height,0.003),cover)
            cursor += width+0.004
            if count % 5 == 3:
                cursor += rng.randf_range(0.015,0.05)
            count += 1

func _reading_lamp(root: Node3D, room: Node3D) -> void:
    for top in room.get_children():
        if not top is MeshInstance3D or not str(top.name).begins_with("Desk solid oak top"):
            continue
        var bounds: AABB = top.global_transform * top.get_aabb()
        var origin := Vector3(bounds.get_center().x-0.10,bounds.end.y,bounds.position.z+0.24)
        var dark := _plain("lamp_enamel",Color(0.10,0.115,0.10),0.43,0.3)
        var base := CylinderMesh.new()
        base.top_radius=0.085; base.bottom_radius=0.09; base.height=0.018
        _mesh(root,"Reading lamp foot",base,origin+Vector3(0,0.009,0),dark)
        var stem := CylinderMesh.new()
        stem.top_radius=0.006;stem.bottom_radius=0.006;stem.height=0.33
        _mesh(root,"Reading lamp stem",stem,origin+Vector3(0,0.175,0),dark)
        _box(root,"Reading lamp short arm",origin+Vector3(0.06,0.34,0),Vector3(0.13,0.012,0.012),dark)
        var head := origin+Vector3(0.12,0.305,0)
        var shade := CylinderMesh.new()
        shade.top_radius=0.035;shade.bottom_radius=0.075;shade.height=0.075
        shade.cap_top=false;shade.cap_bottom=false
        var shade_material := dark.duplicate() as StandardMaterial3D
        shade_material.cull_mode=BaseMaterial3D.CULL_DISABLED
        _mesh(root,"Open enamel reading shade",shade,head,shade_material)
        var bulb := SphereMesh.new()
        bulb.radius=0.023;bulb.height=0.033
        var glow := _plain("lamp_bulb",Color(0.95,0.75,0.40),0.6)
        glow.emission_enabled=true;glow.emission=Color(1.0,0.64,0.26);glow.emission_energy_multiplier=1.3
        _mesh(root,"Reading lamp warm bulb",bulb,head-Vector3(0,0.02,0),glow)
        var light := SpotLight3D.new()
        light.name="Desk warm pool"
        root.add_child(light)
        light.position=head-Vector3(0,0.055,0)
        light.light_color=Color(1.0,0.72,0.46)
        light.light_energy=0.55
        light.spot_range=1.5
        light.spot_angle=53.0
        light.shadow_enabled=false
        light.light_specular=0.25
        light.look_at(origin+Vector3(0.18,0,0.20),Vector3.FORWARD)

func _mesh(root: Node3D, label: String, mesh: Mesh, position: Vector3, material: Material) -> void:
    var instance := MeshInstance3D.new()
    instance.name=label
    instance.mesh=mesh
    instance.material_override=material
    instance.position=position
    instance.gi_mode=GeometryInstance3D.GI_MODE_STATIC
    root.add_child(instance)
