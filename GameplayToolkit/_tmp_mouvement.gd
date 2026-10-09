extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _frames(n: int) -> void:
	for i in n:
		await physics_frame

func _run() -> void:
	var monde := Node2D.new()
	get_root().add_child(monde)
	var corde := Rope2D.new()
	corde.position = Vector2(0, 0)
	monde.add_child(corde)
	var c := CharacterBody2D.new()
	c.position = Vector2(10, 100)
	monde.add_child(c)
	var d := MovementDirector.new()
	c.add_child(d)
	var g := GroundMovement2D.new()
	g.name = "Sol"
	c.add_child(g)
	var rm := RopeMovement2D.new()
	rm.name = "Corde"
	c.add_child(rm)
	await _frames(3)
	print("C contextes: ", d.contexts.map(func(x): return x.context_name))
	print("C try_grab: ", rm.try_grab(), " pos=", corde.get_attached_position(c))
	await _frames(2)
	print("C contexte: ", d.get_current_context(), " sol actif=", g.active)
	var p0 := corde.get_attached_position(c)
	d.move(Vector2(0, -1))
	await _frames(30)
	print("C apres move.y=-1 : ", p0, " -> ", corde.get_attached_position(c))
	d.move(Vector2(0, -1))
	await _frames(200)
	print("C min: ", corde.get_attached_position(c))
	d.move(Vector2.ZERO)
	d.jump()
	await _frames(2)
	print("C apres saut: attache=", rm.is_attached(), " vy=", c.velocity.y, " regrab=", rm.try_grab())
	# Jiggle reset
	quit()
