extends SceneTree

class Cible extends Node2D:
	@export var vie := 10
	@export var rayon := 20.0
	@export var mort := false

func _initialize():
	_run.call_deferred()

func _run():
	var c := Cible.new()
	root.add_child(c)
	# GizmoHost
	var host := GizmoHost.new()
	c.add_child(host)
	var reg := GizmoRegistry.new()
	var e := GizmoEntry.new()
	e.function = "Gizmos2D.draw_2D_circle"
	var args = []
	var info = ClassDB.class_get_method_list("Gizmos2D").filter(func(m): return m.name == "draw_2D_circle")[0]
	for a in info.args: args.append(a.name + ":" + type_string(a.type))
	print("G0 args=", args, " defaults=", info.default_args)
	print("G0 arguments=", e.arguments.size(), " first expected=", e.arguments[0].expected_info if e.arguments.size() else "")
	e.arguments[1].source = ArgumentBinding.SOURCE_PROPERTY
	e.arguments[1].property_path = NodePath("rayon")
	reg.entries = [e]
	host.registry = reg
	await process_frame
	await process_frame
	print("G1 rid=", host.get_live_rid_count(), " draws=", host.get_draw_count())
	await process_frame
	print("G2 draws(no change)=", host.get_draw_count())
	c.rayon = 30.0
	await process_frame
	print("G3 draws(after change)=", host.get_draw_count())
	e.visibility = GizmoEntry.VISIBILITY_EDITOR
	await process_frame
	print("G4 editor-only in game rid=", host.get_live_rid_count())
	host.queue_free()
	await process_frame
	# Window
	var w = ObjectEditorWindow.open_for(c)
	print("W1 window=", w, " current=", ObjectEditorWindow.get_current(), " parent=", w.get_parent() if w else null)
	print("W2 follow pinned? ", ObjectEditorWindow.follow(root))
	w.set_pinned(true)
	print("W3 follow when pinned=", ObjectEditorWindow.follow(c))
	var b2 = RuntimeUndoBackend.new()
	var w2 = ObjectEditorWindow.open_for(c, b2)
	print("W4 same window=", w2 == w, " backend kept=", w.get_editor().get_undo_backend() == b2)
	ObjectEditorWindow.close_current()
	await process_frame
	# Behaviors
	var pe := ProceduralEditor.new()
	root.add_child(pe)
	var dr := DataRegistry.new()
	var hide := HideIfBehavior.new()
	hide.condition = ArgumentBinding.new()
	hide.condition.source = ArgumentBinding.SOURCE_PROPERTY
	hide.condition.property_path = NodePath("mort")
	hide.targets = PackedStringArray(["vie"])
	var en := EnableIfBehavior.new()
	en.get_condition().property_path = NodePath("mort")
	en.targets = PackedStringArray(["rayon"])
	dr.behaviors = [hide, en]
	pe.registry = dr
	pe.set_target(c)
	await process_frame
	print("B1 vie visible=", pe.get_widget("vie").visible, " rayon locked=", pe.get_widget("rayon").is_locked())
	c.mort = true
	await create_timer(0.3).timeout
	print("B2 vie visible=", pe.get_widget("vie").visible, " rayon locked=", pe.get_widget("rayon").is_locked())
	pe.get_widget("rayon").emit_value(99.0, true)
	print("B3 rayon after emit when locked=", c.rayon)
	# external change sync
	c.vie = 55
	await create_timer(0.3).timeout
	print("S1 last=", pe.get_widget("vie").get_last_value())
	# target freed
	c.queue_free()
	await create_timer(0.3).timeout
	print("S2 widget after free=", pe.get_widget("vie"))
	quit()
