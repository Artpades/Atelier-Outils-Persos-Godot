extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🪢 Aperçu interactif : des lianes (Rope2D) et un héros avec RopeMovement2D ; E pour attraper, haut / bas
## pour grimper, gauche / droite pour se balancer, Espace pour lâcher. Réglages de corde en direct.

var _cordes: Array[Rope2D] = []


func titre() -> String:
	return "Physique de corde"


func aide() -> String:
	return "Sauter vers une liane et appuyer sur E pour l'attraper ; haut / bas : grimper, gauche / droite : se balancer, Espace : lâcher."


func composants() -> Dictionary:
	return {"Sol": "GroundMovement2D", "Corde": "RopeMovement2D"}


func construire_niveau() -> void:
	arene()
	bloc(Rect2(-700, -230, 1400, 30))
	bloc(Rect2(420, 40, 240, 24))
	for x in [-120.0, 180.0]:
		var c := Rope2D.new()
		c.position = Vector2(x, -200)
		c.rope_length = 280.0
		vue.add_child(c)
		_cordes.append(c)
	depart = Vector2(-450, 120)


func reglages() -> void:
	var mouvement: RopeMovement2D = heros.get_node("Corde")
	for reglage in [["swing_force", 200.0, 6000.0, 100.0], ["climb_speed", 20.0, 400.0, 10.0], ["jump_up_velocity", 0.0, 800.0, 10.0]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], mouvement.get(nom), func(v: float): mouvement.set(nom, v))
	panneau.ajouter_section("Cordes")
	for reglage in [["rope_length", 80.0, 420.0, 10.0], ["damping", 0.0, 0.2, 0.005], ["attached_weight", 0.0, 8.0, 0.5]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], _cordes[0].get(nom), func(v: float):
			for c in _cordes:
				c.set(nom, v))
	panneau.ajouter_bouton("Secouer les cordes", _jouer)
	mouvement.grabbed.connect(func(_c): journaliser("grabbed"))
	mouvement.released.connect(func(): journaliser("released"))


func _jouer() -> void:
	for c in _cordes:
		c.apply_impulse_at(c.point_count - 1, Vector2(randf_range(-500, 500), 0))


func _unhandled_key_input(evenement: InputEvent) -> void:
	super(evenement)
	if evenement is InputEventKey and evenement.pressed and evenement.keycode == KEY_E:
		heros.get_node("Corde").try_grab()


func _process(_delta: float) -> void:
	var mouvement: RopeMovement2D = heros.get_node("Corde")
	var info = Rope2D.find_nearest_info(heros.global_position, mouvement.grab_distance)
	afficher_code("$RopeMovement2D.try_grab()   # E\nis_attached() = %s · corde à portée : %s\n$RopeMovement2D.swing_force = %d · climb_speed = %d" % [
		mouvement.is_attached(), info != null, mouvement.swing_force, mouvement.climb_speed])
