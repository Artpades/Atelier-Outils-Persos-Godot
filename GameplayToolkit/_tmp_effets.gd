extends SceneTree

func _initialize() -> void:
	call_deferred("_run")

func _run() -> void:
	var host := Node2D.new()
	root.add_child(host)
	var pfx := PlayerFX.new()
	host.add_child(pfx)
	var main := FXChangeSize.new()
	var after := FXShake.new()
	after.duration = 0.2
	main.effects_on_stop_fx = [after]
	pfx.special_effects = [main]
	pfx.play()
	await process_frame
	pfx.stop()
	var moved := false
	for i in 10:
		await process_frame
		if host.position != Vector2.ZERO:
			moved = true
	await create_timer(0.4).timeout
	print("stop-chained shake moved: ", moved, " still activated: ", after.is_activated())
	# fade signal / FXAudio on Control positional
	var c := Control.new()
	root.add_child(c)
	var au := FXAudio.new()
	au.positional = true
	au.stream = AudioStreamWAV.new()
	au.play_fx(c)
	print("positional on Control: ", au.get_audio_player().get_class())
	# FXFollower without PlayerFX
	var f := FXFollower.new()
	f.targets = [NodePath("..")]
	f.play_fx(host)
	f.process_fx(0.1, host)
	print("follower w/o player: ok (no crash), player=", f.get_player_fx())
	quit()
