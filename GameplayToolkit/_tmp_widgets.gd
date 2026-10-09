extends SceneTree

var log_lines := []
func p(s): print("[T] ", s)

func _initialize() -> void:
	call_deferred("run")

func mb(pos: Vector2, pressed: bool, idx := MOUSE_BUTTON_LEFT) -> InputEventMouseButton:
	var e := InputEventMouseButton.new()
	e.position = pos
	e.global_position = pos
	e.button_index = idx
	e.pressed = pressed
	return e

func mm(pos: Vector2) -> InputEventMouseMotion:
	var e := InputEventMouseMotion.new()
	e.position = pos
	e.global_position = pos
	return e

func run() -> void:
	p("viewport size %s" % root.get_visible_rect().size)
	await test_dnd()
	await test_dnd_control()
	#await test_camera()
	#await test_spawner()
	#test_visuals()
	#await test_scene_manager()
	p("FIN")
	quit()

func test_dnd() -> void:
	var world := Node2D.new()
	root.add_child(world)
	var obj := Node2D.new(); obj.name = "Obj"; obj.position = Vector2(100, 100); world.add_child(obj)
	var slot := Node2D.new(); slot.name = "Slot"; slot.position = Vector2(300, 100); world.add_child(slot)
	var sys := DragAndDropSystem.new(); sys.name = "DnD"
	var c := DraggableComponent.new()
	c.target_node_path = NodePath("../Obj")
	c.snap_targets = [NodePath("../Slot")]
	sys.draggables = [c]
	world.add_child(sys)
	var sigs := []
	for s in ["pointed", "depointed", "grabbed", "released", "snapped"]:
		sys.connect(s, func(a = null, b = null): sigs.append(s))
	await process_frame
	root.push_input(mm(Vector2(100, 100)), true)
	root.push_input(mb(Vector2(100, 100), true), true)
	p("dnd occupied after press: %s" % sys.is_occupied())
	root.push_input(mm(Vector2(290, 105)), true)
	await process_frame
	root.push_input(mb(Vector2(290, 105), false), true)
	p("dnd after release pos=%s sigs=%s" % [obj.global_position, sigs])
	# deuxième saisie : relâché dans le vide -> retour à l'initiale (100,100) ou au slot ?
	sigs.clear()
	root.push_input(mm(Vector2(300, 100)), true)
	root.push_input(mb(Vector2(300, 100), true), true)
	root.push_input(mm(Vector2(600, 400)), true)
	await process_frame
	root.push_input(mb(Vector2(600, 400), false), true)
	p("dnd 2nd release (nowhere) pos=%s sigs=%s initial=%s" % [obj.global_position, sigs, c.initial_position_2d])
	# grab by code + toggle mode
	sys.input_mode = DragAndDropSystem.INPUT_MODE_TOGGLE
	sys.grab(c)
	p("grab code occupied=%s" % sys.is_occupied())
	sys.release()
	world.queue_free()
	await process_frame

func test_dnd_control() -> void:
	var ui := Control.new(); ui.size = Vector2(800, 600)
	root.add_child(ui)
	var panel := Panel.new(); panel.name = "Card"; panel.position = Vector2(50, 50); panel.size = Vector2(100, 100)
	ui.add_child(panel)
	var sys := DragAndDropSystem.new()
	var c := DraggableComponent.new(); c.target_node_path = NodePath("../Card")
	sys.draggables = [c]
	ui.add_child(sys)
	await process_frame
	root.push_input(mm(Vector2(80, 80)), true)
	root.push_input(mb(Vector2(80, 80), true), true)
	p("control STOP filter: occupied=%s hovered=%s" % [sys.is_occupied(), sys.get_hovered_component() != null])
	root.push_input(mb(Vector2(80, 80), false), true)
	panel.mouse_filter = Control.MOUSE_FILTER_PASS
	ui.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.push_input(mb(Vector2(80, 80), true), true)
	p("control PASS filter: occupied=%s" % sys.is_occupied())
	root.push_input(mb(Vector2(80, 80), false), true)
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.push_input(mb(Vector2(80, 80), true), true)
	p("control IGNORE filter: occupied=%s" % sys.is_occupied())
	root.push_input(mb(Vector2(80, 80), false), true)
	ui.queue_free()
	await process_frame

func test_camera() -> void:
	var w := Node2D.new(); root.add_child(w)
	var a := Camera2D.new(); a.position = Vector2(0, 0); w.add_child(a)
	var b := Camera2D.new(); b.position = Vector2(500, 0); w.add_child(b)
	var svc := SubViewportContainer.new(); w.add_child(svc)
	var sv := SubViewport.new(); svc.add_child(sv)
	var c := Camera2D.new(); sv.add_child(c)
	await process_frame
	a.make_current()
	p("cam sub enabled before: %s current=%s" % [c.enabled, sv.get_camera_2d() == c])
	var mgr = CameraManager.get_instance()
	p("mgr inside tree right after get_instance: %s" % (mgr.is_inside_tree() if mgr else "null"))
	mgr.switch_2d_camera(b, true, 0.2)
	p("transitioning=%s" % mgr.is_transitioning_2d())
	await create_timer(0.5).timeout
	p("after: current b=%s transitioning=%s sub cam enabled=%s" % [root.get_camera_2d() == b, mgr.is_transitioning_2d(), c.enabled])
	# 3D: interruption & instant
	var w3 := Node3D.new(); root.add_child(w3)
	var c1 := Camera3D.new(); w3.add_child(c1)
	var c2 := Camera3D.new(); c2.position = Vector3(5, 0, 0); w3.add_child(c2)
	await process_frame
	c1.current = true
	var got := []
	mgr.current_camera_3d_switched.connect(func(cam): got.append(cam))
	mgr.switch_3d_camera(c1)
	p("same camera -> signal sync: %d" % got.size())
	mgr.switch_3d_camera(c2, true, 0.1)
	await create_timer(0.3).timeout
	p("3d current c2=%s signals=%d" % [root.get_camera_3d() == c2, got.size()])
	# pause : la transition se fige-t-elle ?
	mgr.switch_3d_camera(c1, true, 0.1)
	paused = true
	await create_timer(0.3, true).timeout
	p("paused: still transitioning=%s" % mgr.is_transitioning_3d())
	paused = false
	await create_timer(0.3).timeout
	p("unpaused: transitioning=%s current c1=%s" % [mgr.is_transitioning_3d(), root.get_camera_3d() == c1])
	w.queue_free(); w3.queue_free()
	await process_frame

func make_scene(script_src: String) -> PackedScene:
	var n := Node2D.new()
	if script_src != "":
		var s := GDScript.new(); s.source_code = script_src; s.reload(); n.set_script(s)
	var ps := PackedScene.new(); ps.pack(n)
	n.free()
	return ps

func test_spawner() -> void:
	var holder := Node2D.new(); root.add_child(holder)
	var sp := Spawner.new()
	var pos := Marker2D.new(); pos.position = Vector2(42, 7); sp.add_child(pos)
	var e := SpawnEntry.new(); e.scenes = [make_scene("")]
	sp.entries = [e]
	holder.add_child(sp)
	var before := root.get_child_count()
	sp.spawn()
	p("spawner default parent = root: %s ; new root children=%d" % [root.get_child_count() - before == 1, root.get_child_count() - before])
	var inst = root.get_child(root.get_child_count() - 1)
	p("spawned at %s" % inst.global_position)
	inst.queue_free()
	# Pool spawned same frame as ready
	var src := "extends Node2D\nvar ready_done := false\nfunc _ready(): ready_done = true\nfunc on_spawn_from_pool(): print('[T] on_spawn_from_pool in_tree=', is_inside_tree(), ' ready=', ready_done, ' pm=', process_mode)\n"
	var scene := make_scene(src)
	var pool := PoolSpawner.new()
	var pe := SpawnPoolEntry.new(); pe.scenes = [scene]; pe.initial_pool_size_per_scene = 2
	pool.entries = [pe]
	var m := Marker2D.new(); m.position = Vector2(10, 10); pool.add_child(m)
	var cont := Node2D.new(); cont.name = "Cont"; holder.add_child(cont)
	pool.parent_node = cont
	holder.add_child(pool)
	p("pool avail after ready=%d cont children=%d" % [pool.get_available_count(scene), cont.get_child_count()])
	pool.spawn()
	await process_frame
	p("pool avail after spawn=%d cont children=%d" % [pool.get_available_count(scene), cont.get_child_count()])
	var spawned = cont.get_child(0)
	for ch in cont.get_children():
		if ch.visible: spawned = ch
	p("pool spawned pos=%s visible=%s" % [spawned.global_position, spawned.visible])
	var desp := []
	pool.despawned.connect(func(i): desp.append(i))
	pool.despawn(spawned)
	p("despawn: visible=%s pm=%s avail=%d sig=%d" % [spawned.visible, spawned.process_mode, pool.get_available_count(scene), desp.size()])
	# exhausted
	pool.spawn(); pool.spawn(); pool.spawn()
	p("after 3 spawns (pool 2): cont children=%d avail=%d" % [cont.get_child_count(), pool.get_available_count(scene)])
	# ALL scenes + RANDOM positions
	holder.queue_free()
	await process_frame

func test_visuals() -> void:
	var host := Node2D.new(); root.add_child(host)
	var sw := VisualsSwapper.new(); host.add_child(sw)
	var anim := AnimatedSprite2D.new(); anim.name = "Anim"; host.add_child(anim)
	var old := Node.new(); anim.add_child(old)
	var reg := VisualsRegister.new(); reg.node_visual = NodePath("../Anim")
	reg.texture = PlaceholderTexture2D.new()
	var asset := VisualsAsset.new(); asset.suit_type_name = "a"; asset.visuals_registers = [reg]
	sw.visuals_assets = [asset]
	sw.swap_by_name("a")
	p("visuals AnimatedSprite2D w/ texture only: children=%d" % anim.get_child_count())
	reg.modeling_import = make_scene("")
	sw.swap_by_name("a")
	p("visuals AnimatedSprite2D w/ scene: children=%d (old queued=%s)" % [anim.get_child_count(), old.is_queued_for_deletion()])
	sw.swap_by_name("inconnu")
	host.queue_free()

func test_scene_manager() -> void:
	var sm = SceneManager.get_instance()
	await process_frame
	var ev := []
	sm.loading_started.connect(func(): ev.append("started"))
	sm.loading_progress.connect(func(x): ev.append("p%d" % x))
	sm.load_finished.connect(func(): ev.append("finished"))
	sm.load_scene_async("res://n_existe_pas.tscn")
	await create_timer(0.3).timeout
	p("scene bad path events=%s loading=%s" % [ev, sm.is_loading()])
	ev.clear()
	var ps := make_scene("")
	ResourceSaver.save(ps, "res://_tmp_widgets_scene.tscn")
	sm.load_scene_async("res://_tmp_widgets_scene.tscn")
	sm.load_scene_async("res://_tmp_widgets_scene.tscn")
	await create_timer(0.5).timeout
	p("scene ok events=%s current=%s" % [ev, current_scene])
	DirAccess.remove_absolute(ProjectSettings.globalize_path("res://_tmp_widgets_scene.tscn"))
