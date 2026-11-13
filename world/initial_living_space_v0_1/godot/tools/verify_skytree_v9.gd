extends SceneTree

func _initialize() -> void:
    var holder:=Node3D.new()
    root.add_child(holder)
    var model=load("res://scripts/skytree_v9.gd").new()
    var tower: Node3D=model.build(holder,Vector3(4,-4.8,-45),12.4)
    var probe: Array[Transform3D]=[]
    var a:=Vector3(2,3,4)
    var b:=Vector3(8,15,6)
    model._bar(probe,a,b,0.8)
    var transform: Transform3D=probe[0]
    var lower: Vector3=transform*Vector3(0,-0.5,0)
    var upper: Vector3=transform*Vector3(0,0.5,0)
    var scale_m: float=12.4/634.0
    var expected_a:=Vector3(4,-4.8,-45)+a*scale_m
    var expected_b:=Vector3(4,-4.8,-45)+b*scale_m
    var error: float=maxf(lower.distance_to(expected_a),upper.distance_to(expected_b))
    if error>0.00001:
        push_error("Rotated lattice endpoints are incorrect: %s"%error)
        quit(1)
        return
    if model.ribs.size()!=662 or model.braces.size()!=1202:
        push_error("Tower lattice is incomplete.")
        quit(1)
        return
    print("SKYTREE_GEOMETRY_CHECK PASS endpoint_error=",error," tower_sections=",tower.get_child_count()," actual_height_ratio=350/634,450/634")
    holder.queue_free()
    quit(0)
