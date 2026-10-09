extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func creer_dialogue() -> DialogueResource:
	var bonjour := DialogueLine.new()
	bonjour.node_id = &"bonjour"
	bonjour.speaker = "Marchande"
	bonjour.text = "Bienvenue ! Tu as {pieces} pièces."
	bonjour.next = &"menu"
	var acheter := DialogueOption.new()
	acheter.text = "Acheter une potion (10 pièces)"
	acheter.condition = "pieces >= 10"
	acheter.next = &"achat"
	var partir := DialogueOption.new()
	partir.text = "Au revoir"
	partir.next = &"fin"
	var menu := DialogueChoice.new()
	menu.node_id = &"menu"
	menu.options.assign([acheter, partir])
	var achat := DialogueAction.new()
	achat.node_id = &"achat"
	achat.assignments = ["pieces -= 10", "potions += 1"]
	achat.next = &"menu"
	var fin := DialogueEnd.new()
	fin.node_id = &"fin"
	var dialogue := DialogueResource.new()
	dialogue.start_id = &"bonjour"
	dialogue.nodes.assign([bonjour, menu, achat, fin])
	assert(dialogue.validate().is_empty())
	return dialogue

func _run() -> void:
	var ds := DialogueSystem.new(); root.add_child(ds)
	ds.variables.set_value("pieces", 15)
	var opts := []
	ds.choices_shown.connect(func(q, o, c): opts.append(o.size()))
	ds.start(creer_dialogue())
	ds.advance(); ds.choose(0)
	print("[T] opts sizes ", opts, " pieces ", ds.variables.get_value("pieces"), " potions ", ds.variables.get_value("potions"))
	# local_to_scene transitions
	var proto := Node.new()
	var m := StateMachine.new(); m.name = "M"; proto.add_child(m); m.owner = proto
	var x1 := State.new(); x1.name = "X"; m.add_child(x1); x1.owner = proto
	var t := StateTransition.new(); t.target_state = NodePath("../X"); t.resource_local_to_scene = true
	x1.transitions = [t]
	var ps := PackedScene.new(); ps.pack(proto)
	var i1 := ps.instantiate(); var i2 := ps.instantiate()
	root.add_child(i1); root.add_child(i2)
	print("[T] local_to_scene transitions shared: ", i1.get_node("M/X").transitions[0] == i2.get_node("M/X").transitions[0])
	# turn based
	var agent := Node.new(); root.add_child(agent)
	var bt := BehaviorTree.new(); bt.process_callback = BehaviorTree.PROCESS_MANUAL; agent.add_child(bt)
	var w := BTWait.new(); w.duration = 2.5; bt.add_child(w)
	var statut: int = bt.tick(1.0); var n := 1
	while statut == BTNode.RUNNING:
		statut = bt.tick(1.0); n += 1
	print("[T] turn based ticks ", n)
	quit()
