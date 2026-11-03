extends "res://🎥Démos de documentation/5.🌀🫗/sequences_mouvement.gd"

## 🫨 Plans filmés de la page Physique trémousse (mouvement/physique-tremousse.html) : Jiggle2D / 3D.


func titre_module() -> String:
	return "🫨 Trémousse · Jiggle2D / 3D"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-Jiggle2D": _classe_2d,
		"classe-Jiggle3D": _classe_3d,
		"Jiggle2D.stiffness": _stiffness,
		"Jiggle2D.damping": _damping,
		"Jiggle2D.max_offset": _max_offset,
		"Jiggle2D.lean_amount": _lean,
		"Jiggle2D.squash_amount": _squash,
		"Jiggle2D.gravity": _gravity,
		"Jiggle2D.enabled": _enabled,
		"Jiggle2D.debug_mode": _debug_mode,
		"ex-recettes": _ex_recettes,
		"ex-chaine": _ex_chaine,
		"ex-coup": _ex_coup,
		"ex-teleport": _ex_teleport,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Personnage › Jiggle2D › antenne : l'antenne traîne quand il court, balance quand il s'arrête, s'écrase à l'atterrissage (<code>impulse</code>).",
		"classe-Jiggle2D": "Trois Jiggle2D sur le même support qui bouge : antenne, gelée, queue qui pend.",
		"classe-Jiggle3D": "Un slime 3D (Jiggle3D) qui s'écrase et rebondit après chaque saut.",
		"Jiggle2D.stiffness": "<code>stiffness</code> 40, 140 (défaut) et 400 : mou et traînant, ou vif.",
		"Jiggle2D.damping": "<code>damping</code> 1,5, 7 (défaut) et 20 : rebonds longs ou retour sec.",
		"Jiggle2D.max_offset": "<code>max_offset</code> 8, 24 (défaut) et 60 px : l'écart maximal au repos.",
		"Jiggle2D.lean_amount": "<code>lean_amount</code> 0, 0,02 (défaut) et 0,06 : l'inclinaison selon le décalage.",
		"Jiggle2D.squash_amount": "<code>squash_amount</code> 0, 0,0015 (défaut) et 0,005, borné par <code>max_stretch</code> : l'étirement selon la vitesse.",
		"Jiggle2D.gravity": "<code>gravity</code> (0, 0) (défaut) et (0, 400) : la masse pend.",
		"Jiggle2D.enabled": "<code>enabled = false</code> : retour au repos, plus de simulation ; true : elle reprend.",
		"Jiggle2D.debug_mode": "<code>debug_mode</code> : position de repos, cercle de <code>max_offset</code> et décalage courant (<code>get_offset()</code>).",
		"ex-recettes": "Les quatre recettes : antenne (ressort), gelée (slime), chapeau lourd, queue qui pend.",
		"ex-chaine": "Quatre Jiggle2D enchaînés, de plus en plus mous : une queue souple.",
		"ex-coup": "<code>encaisser()</code> : <code>impulse</code> dans la direction du coup, l'objet 3D tremble.",
		"ex-teleport": "Téléportation : sans <code>reset()</code> l'antenne fouette pour rattraper la distance ; avec, elle repart au repos.",
	}


func reutilisations() -> Dictionary:
	var meme_3d := "Même réglage qu'en 2D, en mètres (voir la vidéo 2D)."
	var r := {
		"Jiggle2D.max_stretch": "Jiggle2D.squash_amount",
		"Jiggle2D.impulse": "mode-emploi",
		"Jiggle2D.reset": "ex-teleport",
		"Jiggle2D.get_offset": "Jiggle2D.debug_mode",
		"Jiggle3D.impulse": "ex-coup",
	}
	for membre in ["damping", "debug_mode", "enabled", "gravity", "lean_amount", "max_offset", "max_stretch", "squash_amount", "stiffness", "get_offset", "reset"]:
		r["Jiggle3D." + membre] = [r.get("Jiggle2D." + membre, "Jiggle2D." + membre), meme_3d]
	return r


## Support (carré) avec une antenne (Jiggle2D + tige + pompon) ; renvoie [support, jiggle].
func _antenne(position: Vector2, reglages: Dictionary = {}, couleur: Color = VARIANTES[0]) -> Array:
	var support := carre(position, couleur, 50)
	var jiggle := Jiggle2D.new()
	jiggle.position = Vector2(0, -25)
	for propriete in reglages:
		jiggle.set(propriete, reglages[propriete])
	var tige := Line2D.new()
	tige.points = PackedVector2Array([Vector2.ZERO, Vector2(0, -40)])
	tige.width = 4
	tige.default_color = Color("cfd6e6")
	jiggle.add_child(tige)
	var pompon := Polygon2D.new()
	pompon.polygon = Formes.rectangle(Vector2(16, 16))
	pompon.position = Vector2(0, -46)
	pompon.color = ACCENT
	jiggle.add_child(pompon)
	support.add_child(jiggle)
	return [support, jiggle]


## Va-et-vient brusques des supports (accélérations puis arrêts nets).
func _secouer_supports(supports: Array, fois: int = 4, amplitude: float = 160.0) -> void:
	for i in fois:
		var cible := amplitude if i % 2 == 0 else 0.0
		for s in supports:
			var tween: Tween = s.create_tween()
			tween.tween_property(s, "position:x", s.get_meta("x0") + cible, 0.35).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)
		await secondes(1.3)


func _colonnes_jiggle(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}) -> Array:
	var supports := []
	var jiggles := []
	var n := valeurs.size()
	for i in n:
		var x := 120.0 + i * (900.0 / n)
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		var paire := _antenne(Vector2(x, 360), reglages, VARIANTES[i % 4])
		paire[0].set_meta("x0", x)
		supports.append(paire[0])
		jiggles.append(paire[1])
		etiquette(Vector2(x + 80, 470), textes[i], 17, VARIANTES[i % 4])
	return [supports, jiggles]


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : une antenne qui balance")
	var vue := monde_2d()
	arene(vue)
	var heros := heros_2d(vue, Vector2(-400, 150), {"Sol": "GroundMovement2D"})
	var antenne := Jiggle2D.new()
	antenne.position = Vector2(0, -24)
	var tige := Line2D.new()
	tige.points = PackedVector2Array([Vector2.ZERO, Vector2(0, -30)])
	tige.width = 3
	tige.default_color = Color("cfd6e6")
	antenne.add_child(tige)
	var pompon := Polygon2D.new()
	pompon.polygon = Formes.rectangle(Vector2(12, 12))
	pompon.position = Vector2(0, -34)
	pompon.color = ACCENT
	antenne.add_child(pompon)
	heros.add_child(antenne)
	var sol: GroundMovement2D = heros.get_node("Sol")
	sol.landed.connect(func(): antenne.impulse(Vector2(0, 400)))
	arbre(Vector2(40, 140), PackedStringArray(["Personnage", "  Jiggle2D", "    Antenne (Sprite2D)"]))
	code("func _on_ground_movement_2d_landed() -> void:\n\t$Jiggle2D.impulse(Vector2(0, 400))")
	await piloter(sol, [[1.4, Vector2.RIGHT], [1.0, Vector2.ZERO], [1.2, Vector2.LEFT], [1.0, Vector2.ZERO], [0.6, Vector2.RIGHT, "saut_long"], [1.2, Vector2.ZERO], [0.6, Vector2.ZERO, "saut_long"], [1.2, Vector2.ZERO]])


# 💻 API

func _classe_2d() -> void:
	sujet("Jiggle2D : un nœud qui traîne et rebondit")
	var support := carre(Vector2(300, 360), VARIANTES[0], 70)
	support.set_meta("x0", 300.0)
	var recettes := [{"stiffness": 200.0, "damping": 4.0, "position": Vector2(-20, -35)}, {"stiffness": 60.0, "damping": 3.0, "squash_amount": 0.004, "position": Vector2(0, -35)},
		{"stiffness": 40.0, "damping": 6.0, "gravity": Vector2(0, 400), "position": Vector2(35, 0)}]
	for i in 3:
		var j := Jiggle2D.new()
		for propriete in recettes[i]:
			j.set(propriete, recettes[i][propriete])
		var forme := Polygon2D.new()
		forme.polygon = Formes.rectangle([Vector2(8, 40), Vector2(30, 24), Vector2(40, 10)][i])
		forme.position = [Vector2(0, -20), Vector2(0, -12), Vector2(20, 0)][i]
		forme.color = [ACCENT, BON, VARIANTES[2]][i]
		j.add_child(forme)
		support.add_child(j)
	code("Personnage › Jiggle2D › Antenne\n$Jiggle2D.stiffness = 140.0 · damping = 7.0")
	await _secouer_supports([support], 5, 450.0)


func _classe_3d() -> void:
	sujet("Jiggle3D : un slime qui rebondit")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(20, 1, 10))
	var corps := Node3D.new()
	corps.position = Vector3(0, 0, 0)
	vue.add_child(corps)
	var slime := Jiggle3D.new()
	slime.stiffness = 60.0
	slime.damping = 3.0
	slime.squash_amount = 0.08
	slime.position = Vector3(0, 0.6, 0)
	Formes.sphere(slime, Vector3.ZERO, BON, 0.6)
	corps.add_child(slime)
	code("$Jiggle3D.stiffness = 60.0\n$Jiggle3D.squash_amount = 0.08")
	for i in 4:
		var saut := corps.create_tween()
		saut.tween_property(corps, "position:y", 2.0, 0.35).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		saut.tween_property(corps, "position:y", 0.0, 0.35).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		await saut.finished
		slime.impulse(Vector3(0, -4, 0))
		await secondes(1.0)


func _stiffness() -> void:
	sujet("stiffness : raideur du ressort")
	code("$Jiggle2D.stiffness = 140.0")
	var c := _colonnes_jiggle("stiffness", [40.0, 140.0, 400.0], ["40", "140 (défaut)", "400"])
	await _secouer_supports(c[0])


func _damping() -> void:
	sujet("damping : amortissement")
	code("$Jiggle2D.damping = 7.0")
	var c := _colonnes_jiggle("damping", [1.5, 7.0, 20.0], ["1,5", "7 (défaut)", "20"])
	await _secouer_supports(c[0])


func _max_offset() -> void:
	sujet("max_offset : décalage maximal")
	code("$Jiggle2D.max_offset = 24.0")
	var c := _colonnes_jiggle("max_offset", [8.0, 24.0, 60.0], ["8", "24 (défaut)", "60"], {"stiffness": 60.0, "debug_mode": true})
	await _secouer_supports(c[0])


func _lean() -> void:
	sujet("lean_amount : inclinaison")
	code("$Jiggle2D.lean_amount = 0.02")
	var c := _colonnes_jiggle("lean_amount", [0.0, 0.02, 0.06], ["0", "0,02 (défaut)", "0,06"])
	await _secouer_supports(c[0])


func _squash() -> void:
	sujet("squash_amount et max_stretch : étirement")
	code("$Jiggle2D.squash_amount = 0.0015\n$Jiggle2D.max_stretch = 0.35")
	var c := _colonnes_jiggle("squash_amount", [0.0, 0.0015, 0.005], ["0", "0,0015 (défaut)", "0,005"], {"stiffness": 80.0, "damping": 3.0})
	await _secouer_supports(c[0])


func _gravity() -> void:
	sujet("gravity : faire pendre la masse")
	code("$Jiggle2D.gravity = Vector2(0, 400)")
	var c := _colonnes_jiggle("gravity", [Vector2.ZERO, Vector2(0, 400)], ["(0, 0) (défaut)", "(0, 400)"], {"stiffness": 40.0, "max_offset": 50.0})
	await _secouer_supports(c[0])


func _enabled() -> void:
	sujet("enabled : activer ou figer la simulation")
	var c := _colonnes_jiggle("enabled", [true], ["enabled"])
	var j: Jiggle2D = c[1][0]
	for etat in [true, false, true]:
		j.enabled = etat
		code("$Jiggle2D.enabled = %s" % etat)
		legende("enabled = %s" % etat)
		await _secouer_supports(c[0], 2)


func _debug_mode() -> void:
	sujet("debug_mode et get_offset()")
	var c := _colonnes_jiggle("debug_mode", [true], ["debug_mode = true"], {"stiffness": 60.0})
	var j: Jiggle2D = c[1][0]
	var decalage := etiquette(Vector2(576, 150), "", 18, ACCENT)
	get_tree().process_frame.connect(func():
		if is_instance_valid(decalage):
			decalage.text = "get_offset() = %s" % j.get_offset().snapped(Vector2(0.1, 0.1)))
	code("$Jiggle2D.debug_mode = true")
	await _secouer_supports(c[0], 5)


# 📌 Exemples

func _ex_recettes() -> void:
	sujet("Exemple : quels réglages pour quel effet ?")
	code("antenne : stiffness 200 · damping 4 · lean 0,03\ngelée : 60 · 3 · squash 0,004\nchapeau : 140 · 10 · max_offset 8\nqueue : 40 · 6 · gravity (0, 400)")
	var recettes := [[{"stiffness": 200.0, "damping": 4.0, "lean_amount": 0.03}, "Antenne"], [{"stiffness": 60.0, "damping": 3.0, "squash_amount": 0.004}, "Gelée"],
		[{"stiffness": 140.0, "damping": 10.0, "max_offset": 8.0}, "Chapeau"], [{"stiffness": 40.0, "damping": 6.0, "gravity": Vector2(0, 400)}, "Queue"]]
	var supports := []
	for i in 4:
		var x := 110.0 + i * 240.0
		var paire := _antenne(Vector2(x, 360), recettes[i][0], VARIANTES[i])
		paire[0].set_meta("x0", x)
		supports.append(paire[0])
		etiquette(Vector2(x + 60, 470), recettes[i][1], 17, VARIANTES[i])
	await _secouer_supports(supports, 4, 120.0)


func _ex_chaine() -> void:
	sujet("Exemple : une queue souple (Jiggle2D enchaînés)")
	var corps := carre(Vector2(450, 330), VARIANTES[0], 60)
	corps.set_meta("x0", 450.0)
	var parent: Node2D = corps
	for i in 4:
		var segment := Jiggle2D.new()
		segment.position = Vector2(-26 if i == 0 else -22, 10 if i == 0 else 0)
		segment.stiffness = 120.0 - i * 20.0
		var boule := Formes.disque(segment, Vector2.ZERO, VARIANTES[1].darkened(i * 0.1), 10 - i)
		boule.name = "Boule"
		parent.add_child(segment)
		parent = segment
	code("for i in 4:\n\tvar segment := Jiggle2D.new()\n\tsegment.position = Vector2(-12, 0)\n\tsegment.stiffness = 120.0 - i * 20.0\n\tparent.add_child(segment); parent = segment")
	for i in 3:
		var tween := corps.create_tween()
		tween.tween_property(corps, "position", Vector2(750, 250), 0.6).set_trans(Tween.TRANS_SINE)
		tween.tween_property(corps, "position", Vector2(450, 400), 0.6).set_trans(Tween.TRANS_SINE)
		tween.tween_property(corps, "position", Vector2(450, 330), 0.4)
		await tween.finished
		await secondes(0.6)


func _ex_coup() -> void:
	sujet("Exemple : un objet 3D touché qui tremble")
	var vue := monde_3d()
	bloc_3d(vue, Vector3(0, -0.5, 0), Vector3(20, 1, 10))
	var mannequin := Jiggle3D.new()
	mannequin.position = Vector3(0, 1, 0)
	mannequin.stiffness = 90.0
	mannequin.damping = 4.0
	Formes.cube(mannequin, Vector3.ZERO, Color("c9a46b"), Vector3(1, 2, 1))
	vue.add_child(mannequin)
	code("func encaisser(direction: Vector3) -> void:\n\t$Jiggle3D.impulse(direction.normalized() * 4.0)")
	for direction in [Vector3(1, 0, 0), Vector3(-1, 0, 0.5), Vector3(0, 0, -1), Vector3(1, 0.5, 1)]:
		await secondes(0.8)
		legende("encaisser(%s)" % direction)
		mannequin.impulse(direction.normalized() * 4.0)
		await secondes(0.6)


func _ex_teleport() -> void:
	sujet("Exemple : éviter le « fouet » après une téléportation")
	for avec_reset in [false, true]:
		var paire := _antenne(Vector2(250, 360), {"stiffness": 60.0, "damping": 3.0, "max_offset": 200.0})
		code("global_position = vers\n%s" % ("$Jiggle2D.reset()" if avec_reset else "# sans reset()"))
		legende("Sans reset() : l'antenne rattrape toute la distance" if not avec_reset else "Avec reset() : elle repart au repos")
		await secondes(0.8)
		paire[0].position.x = 850
		if avec_reset:
			paire[1].reset()
		await secondes(2.0)
		paire[0].queue_free()
