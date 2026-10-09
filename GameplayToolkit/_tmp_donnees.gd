extends SceneTree
func _mk(nom: String) -> NetworkSession:
	var b := Node.new(); b.name = nom; root.add_child(b)
	set_multiplayer(SceneMultiplayer.new(), b.get_path())
	var s := NetworkSession.new(); s.player_name = nom; b.add_child(s)
	return s
func _initialize() -> void:
	_run.call_deferred()
func _run() -> void:
	var h := _mk("Hote"); var c := _mk("Client")
	print("host err ", h.host(47911))
	var got := []
	c.get_multiplayer().peer_packet.connect(func(id, pkt): got.append(pkt[0]))
	c.message_received.connect(func(de, d, enc): print("client msg ", de, " ", d, " ", enc))
	c.join("127.0.0.1", 47911)
	var t := Time.get_ticks_msec()
	while c.get_state() != NetworkSession.STATE_CONNECTED and Time.get_ticks_msec() - t < 4000:
		await process_frame
	print("client state ", c.get_state(), " players ", c.get_player_count())
	h.send_message({"hp": 3})
	for i in 30: await process_frame
	print("raw peer_packet first bytes seen by game: ", got)
	print("player info keys ", c.get_player_info(1).keys())
	h.close()
	for i in 10: await process_frame
	print("client state after host close ", c.get_state(), " reason ", c.get_last_reason())
	quit()
