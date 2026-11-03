extends "../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques des pops : PopStyle, PopSpawner (2D, interface, 3D, 3D à l'écran), FXPop.
##
## Avertissement attendu dans la console : pop vide.


func _executer() -> void:
	_tester_style()
	await _tester_pop_2d()
	await _tester_pop_survit_a_la_cible()
	await _tester_nombres()
	await _tester_image_et_texte()
	await _tester_scene_et_pop_vide()
	await _tester_limite()
	await _tester_sol()
	await _tester_rotation()
	await _tester_interface()
	await _tester_pop_3d()
	await _tester_pop_3d_ecran()
	await _tester_fx_pop()


# 🎨 Style

func _tester_style() -> void:
	var style := PopStyle.new()
	_verifier("PopStyle : échelle de départ", is_equal_approx(style.get_scale_at(0.0), style.start_scale))
	_verifier("PopStyle : dépassement pendant l'apparition", is_equal_approx(style.get_scale_at(style.pop_in_duration * 0.6), style.pop_scale))
	_verifier("PopStyle : échelle 1 après l'apparition", is_equal_approx(style.get_scale_at(style.pop_in_duration), 1.0))
	_verifier("PopStyle : réduite à end_scale en fin de vie", is_equal_approx(style.get_scale_at(style.lifetime), style.end_scale))
	_verifier("PopStyle : opaque puis fondu en fin de vie",
		style.get_alpha_at(0.0) == 1.0 and is_equal_approx(style.get_alpha_at(style.lifetime - style.fade_out_duration / 2.0), 0.5) and style.get_alpha_at(style.lifetime) == 0.0)
	var courbe := Curve.new()
	courbe.max_value = 2.0 # bornée à [0, 1] par défaut
	courbe.add_point(Vector2(0, 0.5))
	courbe.add_point(Vector2(1, 2))
	style.scale_curve = courbe
	_verifier("PopStyle : scale_curve remplace l'animation", is_equal_approx(style.get_scale_at(style.lifetime), 2.0))


# 🟦 2D

## Style sans hasard : départ vertical à 300 px/s (les trajectoires deviennent prévisibles).
func _style_vertical() -> PopStyle:
	var style := PopStyle.new()
	style.speed_min = 300.0
	style.speed_max = 300.0
	style.spread_degrees = 0.0
	return style


func _spawner_2d(position := Vector2(200, 300)) -> PopSpawner:
	var ancre: Node2D = _ajouter(Node2D.new())
	ancre.position = position
	var spawner := PopSpawner.new()
	spawner.container = _bac_a_sable
	ancre.add_child(spawner)
	return spawner


func _tester_pop_2d() -> void:
	var spawner := _spawner_2d()
	spawner.style = _style_vertical()
	var signal_pop := _espionner(spawner.popped)
	var pop: Node2D = spawner.pop_text("Salut")
	_verifier("PopSpawner : pop_text crée un PopInstance2D dans le conteneur",
		pop and pop.get_class() == "PopInstance2D" and pop.get_parent() == _bac_a_sable and signal_pop.n == 1)
	var texte: Label = pop.get_node("Text")
	_verifier("PopSpawner : texte et couleur du style", texte.text == "Salut" and texte.label_settings.font_color == Color.WHITE)
	_verifier("PopSpawner : départ au point de la cible, petite échelle", pop.global_position.is_equal_approx(Vector2(200, 300)) and pop.scale.x < 0.5)
	await _secondes(0.25)
	# 300 px/s vers le haut, gravité 600 : environ 56 px plus haut après 0,25 s
	_verifier("PopInstance2D : jaillit vers le haut et a atteint sa taille", pop.global_position.y < 260.0 and pop.scale.x > 0.9, "%s %s" % [pop.global_position, pop.scale])
	var vitesse_avant: Vector2 = pop.get_velocity()
	await _secondes(0.2)
	_verifier("PopInstance2D : la gravité le fait retomber", pop.get_velocity().y > vitesse_avant.y)
	await _secondes(0.8)
	_verifier("PopInstance2D : libéré en fin de vie", not is_instance_valid(pop) and spawner.get_active_count() == 0)
	spawner.get_parent().free()


func _tester_pop_survit_a_la_cible() -> void:
	var spawner := _spawner_2d()
	var pop: Node = spawner.pop_text("Adieu")
	spawner.get_parent().free()
	await _frames(3)
	_verifier("PopInstance2D : survit à la libération de sa cible", is_instance_valid(pop) and pop.is_inside_tree())
	pop.free()


func _tester_nombres() -> void:
	var spawner := _spawner_2d()
	spawner.style = PopStyle.new()
	var negatif: Node = spawner.pop_number(-12)
	var positif: Node = spawner.pop_number(3.5, 1)
	_verifier("pop_number : négatif, couleur negative_color",
		negatif.get_node("Text").text == "-12" and negatif.get_node("Text").label_settings.font_color == spawner.style.negative_color)
	_verifier("pop_number : positif signé, décimales, couleur positive_color",
		positif.get_node("Text").text == "+3.5" and positif.get_node("Text").label_settings.font_color == spawner.style.positive_color)
	var rouge: Node = spawner.pop_text("!", Color.RED)
	_verifier("pop_text : couleur imposée", rouge.get_node("Text").label_settings.font_color == Color.RED)
	spawner.clear()
	await _frames()
	_verifier("clear() libère les pops", spawner.get_active_count() == 0 and not is_instance_valid(negatif))
	spawner.get_parent().free()


func _tester_image_et_texte() -> void:
	var spawner := _spawner_2d()
	var image := _texture(Color.YELLOW, 64)
	var pop: Node = spawner.pop("x5", image)
	var icone: Sprite2D = pop.get_node("Icon")
	var texte: Label = pop.get_node("Text")
	var style := PopStyle.new()
	_verifier("pop : image réduite à icon_size", (icone.scale * 64.0).is_equal_approx(style.icon_size))
	var largeur := style.icon_size.x + style.spacing + texte.size.x
	_verifier("pop : image puis texte, rangée centrée",
		is_equal_approx(icone.position.x - style.icon_size.x / 2.0, -largeur / 2.0) and is_equal_approx(texte.position.x + texte.size.x, largeur / 2.0),
		"%s %s %s" % [icone.position, texte.position, texte.size])
	spawner.clear()
	spawner.get_parent().free()
	await _frames()


func _tester_scene_et_pop_vide() -> void:
	var spawner := _spawner_2d()
	var etoile := Polygon2D.new()
	etoile.name = "Etoile"
	var pop: Node = spawner.pop_scene(_empaqueter(etoile))
	_verifier("pop_scene : la scène est instanciée dans le pop", pop.get_node_or_null("Etoile") is Polygon2D)
	_verifier("pop vide : rien n'est créé (avertissement attendu)", spawner.pop() == null and spawner.get_active_count() == 1)
	spawner.clear()
	spawner.get_parent().free()
	await _frames()


func _tester_limite() -> void:
	var spawner := _spawner_2d()
	spawner.max_pops = 3
	var pops := []
	for i in 5:
		pops.append(spawner.pop_text(str(i)))
	await _frames()
	_verifier("max_pops : les plus anciens disparaissent", spawner.get_active_count() == 3 and not is_instance_valid(pops[0]) and is_instance_valid(pops[4]))
	spawner.clear()
	spawner.get_parent().free()


func _tester_sol() -> void:
	var spawner := _spawner_2d()
	var style := PopStyle.new()
	style.use_floor = true
	style.floor_offset = 20.0
	style.gravity = 3000.0
	style.speed_min = 0.0
	style.speed_max = 0.0
	style.lifetime = 5.0
	spawner.style = style
	var pop: Node2D = spawner.pop_text("Pièce")
	await _secondes(0.6)
	_verifier("Sol : le pop retombe et s'arrête floor_offset sous son départ", is_equal_approx(pop.global_position.y, 320.0) and pop.get_velocity().y == 0.0, str(pop.global_position))

	style.bounce = 0.8
	style.speed_min = 400.0
	style.speed_max = 400.0
	style.spread_degrees = 0.0
	var rebond: Node2D = spawner.pop_text("Boing")
	var plus_bas := 0.0
	var remonte := false
	for i in 60:
		await _frames()
		if rebond.global_position.y >= 319.0:
			plus_bas = 1.0
		elif plus_bas > 0.0 and rebond.global_position.y < 310.0:
			remonte = true
	_verifier("Sol : rebond (remonte après avoir touché le sol)", remonte)
	spawner.clear()
	spawner.get_parent().free()


func _tester_rotation() -> void:
	var spawner := _spawner_2d()
	var style := PopStyle.new()
	style.spin_min = 180.0
	style.spin_max = 180.0
	spawner.style = style
	var pop: Node2D = spawner.pop_text("~")
	await _secondes(0.5)
	_verifier("Rotation : spin en degrés par seconde", absf(pop.rotation - PI / 2.0) < 0.15, str(pop.rotation))
	spawner.clear()
	spawner.get_parent().free()


# 🖥️ Interface

func _tester_interface() -> void:
	var calque: CanvasLayer = _ajouter(CanvasLayer.new())
	var bouton := Button.new()
	bouton.position = Vector2(100, 100)
	bouton.size = Vector2(80, 40)
	calque.add_child(bouton)
	var spawner := PopSpawner.new()
	bouton.add_child(spawner)
	var pop: Node2D = spawner.pop_text("Clic")
	_verifier("Control : pop dans le CanvasLayer de la cible, au centre du Control",
		pop.get_parent() == calque and pop.global_position.is_equal_approx(Vector2(140, 120)), str(pop.global_position))
	calque.free()
	await _frames()


# 🧊 3D

func _scene_3d() -> Node3D:
	var monde: Node3D = _ajouter(Node3D.new())
	var camera := Camera3D.new()
	monde.add_child(camera)
	camera.position = Vector3(0, 1, 5)
	camera.make_current()
	return monde


func _tester_pop_3d() -> void:
	var monde := _scene_3d()
	var ancre := Node3D.new()
	monde.add_child(ancre)
	var spawner := PopSpawner.new()
	spawner.container = monde
	spawner.style = _style_vertical()
	ancre.add_child(spawner)
	var pop: Node3D = spawner.pop("-7", _texture(Color.RED, 32))
	_verifier("3D : PopInstance3D avec Label3D et Sprite3D", pop.get_class() == "PopInstance3D" and pop.get_node("Text") is Label3D and pop.get_node("Icon") is Sprite3D)
	var texte: Label3D = pop.get_node("Text")
	_verifier("3D : unit_scale_3d appliqué (pixel_size), dessiné par-dessus", is_equal_approx(texte.pixel_size, 0.01) and texte.no_depth_test)
	await _secondes(0.3)
	var camera := monde.get_viewport().get_camera_3d()
	# 300 px/s = 3 m/s vers le haut, gravité 6 m/s² : environ 0,6 m plus haut après 0,3 s
	_verifier("3D : jaillit vers le haut", pop.global_position.y > 0.45, str(pop.global_position))
	_verifier("3D : tourné vers la caméra", pop.global_basis.z.normalized().is_equal_approx(camera.global_basis.z.normalized()))
	await _secondes(0.6)
	_verifier("3D : fondu appliqué au texte et au contour", texte.modulate.a < 1.0 and texte.outline_modulate.a < 1.0)
	monde.free()


func _tester_pop_3d_ecran() -> void:
	var monde := _scene_3d()
	var ancre := Node3D.new()
	monde.add_child(ancre)
	var spawner := PopSpawner.new()
	spawner.screen_space_3d = true
	ancre.add_child(spawner)
	var pop: Node2D = spawner.pop_text("Écran")
	var camera := monde.get_viewport().get_camera_3d()
	_verifier("3D à l'écran : PopInstance2D dans un CanvasLayer, à la position projetée",
		pop and pop.get_parent() is CanvasLayer and pop.global_position.is_equal_approx(camera.unproject_position(Vector3.ZERO)))
	var calque := pop.get_parent()
	ancre.position = Vector3(0, 0, 10) # derrière la caméra
	_verifier("3D à l'écran : cible derrière la caméra -> pas de pop", spawner.pop_text("Caché") == null)
	calque.free()
	monde.free()
	await _frames()


# ✨ Effet

func _tester_fx_pop() -> void:
	var conteneur: Node2D = _ajouter(Node2D.new())
	var cible := Node2D.new()
	conteneur.add_child(cible)
	# Sans conteneur : la scène courante (dans une démo), sinon la racine (lanceur headless)
	var arbre := _bac_a_sable.get_tree()
	var parent_des_pops: Node = arbre.current_scene if arbre.current_scene else arbre.root
	var avant := _pops_sous(parent_des_pops).size()
	var effet := FXPop.new()
	effet.text = "+1"
	effet.count = 3
	effet.play_fx(cible)
	var pops := _pops_sous(parent_des_pops)
	_verifier("FXPop : count pops créés au jeu de l'effet", pops.size() - avant == 3)
	for pop in pops.slice(avant):
		pop.free()
	conteneur.free()
	await _frames()


# 🛠️ Utilitaires

func _pops_sous(noeud: Node) -> Array:
	return noeud.get_children().filter(func(n): return n.get_class() == "PopInstance2D")


func _texture(couleur: Color, taille: int) -> ImageTexture:
	var image := Image.create(taille, taille, false, Image.FORMAT_RGBA8)
	image.fill(couleur)
	return ImageTexture.create_from_image(image)
