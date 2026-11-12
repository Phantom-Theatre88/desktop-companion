extends RefCounted
## AKARI Room art pass v1: visual-only. No WORLD state or time changes.
## All shapes are runtime Godot meshes and can later be replaced by authored 3D assets.

static func _fabric(hex_color: String, roughness: float = 0.98, alpha: float = 1.0) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = Color(hex_color, alpha)
	material.roughness = roughness
	material.metallic = 0.0
	material.cull_mode = BaseMaterial3D.CULL_DISABLED
	if alpha < 1.0:
		material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	return material

static func _ellipsoid(parent: Node3D, name: String, position: Vector3, radii: Vector3, material: Material, rotation_radians: Vector3 = Vector3.ZERO) -> MeshInstance3D:
	var mesh_node := MeshInstance3D.new()
	mesh_node.name = name
	var shape := SphereMesh.new()
	shape.radius = 1.0
	shape.height = 2.0
	shape.radial_segments = 24
	shape.rings = 12
	mesh_node.mesh = shape
	mesh_node.position = position
	mesh_node.scale = radii
	mesh_node.rotation = rotation_radians
	mesh_node.material_override = material
	parent.add_child(mesh_node)
	return mesh_node

static func _cloth(parent: Node3D, name: String, width: float, depth: float, x0: float, y0: float, z0: float, mat: Material, drop: float, folds: float, along_window: bool = false) -> void:
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	var nx := 30
	var nz := 28
	for iz in range(nz):
		for ix in range(nx):
			var x1 := float(ix) / float(nx - 1)
			var x2 := float(ix + 1) / float(nx - 1)
			var z1 := float(iz) / float(nz - 1)
			var z2 := float(iz + 1) / float(nz - 1)
			var a := _cloth_vertex(x1, z1, width, depth, x0, y0, z0, drop, folds, along_window)
			var b := _cloth_vertex(x2, z1, width, depth, x0, y0, z0, drop, folds, along_window)
			var c := _cloth_vertex(x2, z2, width, depth, x0, y0, z0, drop, folds, along_window)
			var d := _cloth_vertex(x1, z2, width, depth, x0, y0, z0, drop, folds, along_window)
			for vertex in [a, c, b, a, d, c]:
				surface.add_vertex(vertex)
	surface.generate_normals()
	var mi := MeshInstance3D.new()
	mi.name = name
	mi.mesh = surface.commit()
	mi.material_override = mat
	parent.add_child(mi)

static func _cloth_vertex(u: float, v: float, width: float, depth: float, x0: float, y0: float, z0: float, drop: float, folds: float, along_window: bool) -> Vector3:
	if along_window:
		# Fabric suspended vertically with continuous, non-cylindrical drapes.
		return Vector3(x0 + (u - 0.5) * width, y0 - v * depth, z0 + 0.06 * sin(u * TAU * folds) * (0.5 + v * 0.5))
	var x := (u - 0.5) * width
	var z := (v - 0.5) * depth
	var border := minf(minf(u, 1.0 - u), minf(v, 1.0 - v))
	var edge_fall := clampf((0.13 - border) / 0.13, 0.0, 1.0)
	var wrinkle := 0.021 * sin(u * TAU * folds + v * 4.1) + 0.011 * sin(v * 22.0 + u * 2.0)
	return Vector3(x0 + x, y0 + wrinkle - edge_fall * drop, z0 + z)

static func apply(room: Node3D) -> void:
	var bed := room.get_node_or_null("Furniture/bed") as Node3D
	if bed != null:
		for node_name in ["Duvet", "FoldedThrow"]:
			var old := bed.get_node_or_null(node_name) as Node3D
			if old != null:
				old.visible = false
		for node in bed.get_children():
			if node.name.begins_with("QuiltSeam"):
				(node as Node3D).visible = false
		var blush := _fabric("d9a0aa")
		var ivory := _fabric("fff1df")
		var soft_pink := _fabric("b97784")
		_cloth(bed, "SoftDuvet", 1.68, 1.86, 0.0, 0.91, 0.28, blush, 0.25, 4.0)
		_cloth(bed, "FoldedKnitThrow", 1.72, 0.49, 0.0, 0.99, 0.76, soft_pink, 0.11, 8.0)
		_ellipsoid(bed, "PillowSoftLeft", Vector3(-0.43, 0.84, -0.77), Vector3(0.41, 0.13, 0.25), ivory, Vector3(0.07, -0.12, 0.04))
		_ellipsoid(bed, "PillowSoftRight", Vector3(0.41, 0.84, -0.77), Vector3(0.41, 0.13, 0.25), ivory, Vector3(-0.06, 0.13, -0.04))
	var furniture := room.get_node_or_null("Furniture") as Node3D
	if furniture != null:
		var old_pouf := furniture.get_node_or_null("KnittedPouf") as Node3D
		if old_pouf != null:
			old_pouf.visible = false
		for node in furniture.get_children():
			if node.name.begins_with("PoufRib"):
				(node as Node3D).visible = false
		_ellipsoid(furniture, "RoundedKnittedPouf", Vector3(-0.66, 0.23, 1.87), Vector3(0.45, 0.24, 0.45), _fabric("c88b96"))
		var old_rug := furniture.get_node_or_null("Rug") as Node3D
		if old_rug != null:
			old_rug.visible = false
		for node in furniture.get_children():
			if node.name.begins_with("RugWeave"):
				(node as Node3D).visible = false
		_ellipsoid(furniture, "SoftOvalRug", Vector3(-0.20, 0.022, 1.03), Vector3(1.82, 0.028, 1.29), _fabric("e9ddd1"))
		var cushion := furniture.get_node_or_null("FloorCushion") as MeshInstance3D
		if cushion != null:
			cushion.material_override = _fabric("eee0d7")
		var ceiling_light := furniture.get_node_or_null("PendantLight") as OmniLight3D
		if ceiling_light != null:
			ceiling_light.light_energy = 0.58
		var bedside_light := furniture.get_node_or_null("BedsideLight") as OmniLight3D
		if bedside_light != null:
			bedside_light.light_energy = 0.35
	var desk_light := room.get_node_or_null("Furniture/desk/TaskLight") as OmniLight3D
	if desk_light != null:
		desk_light.light_energy = 0.32
	# Replace the thick cylindrical placeholder curtains with translucent, pleated fabric.
	var architecture := room.get_node_or_null("Architecture") as Node3D
	if architecture != null:
		for child in architecture.get_children():
			if child.name.begins_with("Curtain"):
				(child as Node3D).visible = false
		var curtain_mat := _fabric("e7c7b5", 0.99, 0.76)
		_cloth(architecture, "LeftCurtainSheer", 0.78, 2.82, -2.20, 2.97, -2.47, curtain_mat, 0.0, 5.0, true)
		_cloth(architecture, "RightCurtainSheer", 0.78, 2.82, 2.46, 2.97, -2.47, curtain_mat, 0.0, 5.0, true)
	# Foliage with differently oriented leaves; seed-like deterministic layout.
	var leaf_a := _fabric("647f4d")
	var leaf_b := _fabric("80955c")
	for item in [["WindowPlant", 1.6], ["DeskPlant", 0.95], ["ShelfPlant", 0.7], ["BedsidePlant", 0.8]]:
		var plant := room.get_node_or_null(str(item[0])) as Node3D
		if plant == null:
			continue
		for node in plant.get_children():
			if node.name.begins_with("Leaf"):
				(node as Node3D).visible = false
		for i in range(19):
			var angle := float(i) * 2.39996
			var radius := 0.14 + float(i % 5) * 0.026
			var px := cos(angle) * radius
			var pz := sin(angle) * radius
			var py := 0.51 + float(i % 7) * 0.083
			_ellipsoid(plant, "SoftLeaf%d" % i, Vector3(px, py, pz), Vector3(0.17, 0.045, 0.105), leaf_a if i % 3 == 0 else leaf_b, Vector3(0.25 * sin(angle), angle, 0.32 * cos(angle)))
