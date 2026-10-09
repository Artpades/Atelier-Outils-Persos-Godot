extends SceneTree

func _initialize() -> void:
	call_deferred("run")

func ok(name: String, cond: bool, extra = "") -> void:
	print(("OK   " if cond else "FAIL ") + name + "  " + str(extra))

func attr(n: String, base: float) -> AttributeDefinition:
	var d := AttributeDefinition.new(); d.attribute_name = n; d.base_value = base; return d

func run() -> void:
	# Exemple AbilitySystem
	var holder := Node.new(); root.add_child(holder)
	var stats := AbilitySystem.new()
	var mana := AttributeDefinition.new(); mana.attribute_name = &"mana"; mana.base_value = 50.0
	stats.attribute_definitions = [mana]
	var boule := Ability.new(); boule.ability_id = &"boule_de_feu"; boule.costs = {&"mana": 20.0}; boule.cooldown = 1.5
	stats.granted_abilities = [boule]
	holder.add_child(stats)
	var raisons = []
	stats.ability_failed.connect(func(id, r): raisons.append(r))
	stats.activate(&"boule_de_feu"); stats.activate(&"boule_de_feu")
	ok("exemple AbilitySystem", stats.get_value("mana") == 30.0 and raisons == ["cooldown"], [stats.get_value("mana"), raisons])

	# Expression exemple
	var a := AbilitySystem.new(); a.attribute_definitions = [attr("force", 10), attr("health", 100)]; holder.add_child(a)
	var e := AbilitySystem.new(); e.attribute_definitions = [attr("armure", 5), attr("health", 100)]
	var ennemi := Node2D.new(); ennemi.add_child(e); holder.add_child(ennemi)
	var coup := AttributeModifier.new(); coup.attribute = &"health"
	coup.magnitude_expression = '-(source.get_value("force") * 2 - target.get_value("armure"))'
	var attaque := GameplayEffect.new(); attaque.modifiers = [coup]
	a.apply_effect_to(attaque, ennemi)
	ok("exemple expression", e.get_value("health") == 85.0, e.get_value("health"))

	# OVERRIDE : le dernier l'emporte
	var o1 := GameplayEffect.new(); o1.duration_policy = GameplayEffect.DURATION_INFINITE
	var m1 := AttributeModifier.new(); m1.attribute = "force"; m1.operation = AttributeModifier.OPERATION_OVERRIDE; m1.magnitude = 1; o1.modifiers = [m1]
	var o2 = o1.duplicate(true); o2.modifiers[0].magnitude = 2
	var add := GameplayEffect.new(); add.duration_policy = GameplayEffect.DURATION_INFINITE
	var m3 := AttributeModifier.new(); m3.attribute = "force"; m3.magnitude = 50; add.modifiers = [m3]
	a.apply_effect(o1); a.apply_effect(o2); a.apply_effect(add)
	ok("override dernier + ignore ajout", a.get_value("force") == 2.0, a.get_value("force"))
	var removed = []
	a.effect_removed.connect(func(ef, h): removed.append(h))
	a.initialize()
	ok("initialize sans signal", removed.is_empty() and a.get_active_effects().is_empty(), removed)

	# BTMoveTo run vs run_distance
	var c := Character2D.new(); var ac := AutomateController.new(); c.add_child(ac)
	var bt := BehaviorTree.new(); bt.process_callback = BehaviorTree.PROCESS_PHYSICS
	var mv := BTMoveTo.new(); mv.target_key = "p"; mv.run = true; bt.add_child(mv)
	c.add_child(bt); root.add_child(c)
	await process_frame
	bt.blackboard.set_value("p", Vector2(500, 0))
	var states = []
	for i in 4:
		await physics_frame
		states.append(c.is_running())
	ok("BTMoveTo.run apres images physiques", true, states)
	quit()
