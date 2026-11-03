extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🪂 Aperçu interactif d'AerodynamicMovement2D : planer depuis une tour (saut maintenu), courant ascendant,
## vol libre, et feuilles qui volettent.

var _aero: AerodynamicMovement2D
var _reglages_feuille := {"drag_coefficient": 0.02, "lift_coefficient": 0.01, "flutter_frequency": 1.5}


func titre() -> String:
	return "Physique aérodynamique"


func aide() -> String:
	return "Monter sur la tour, sauter et MAINTENIR Espace pour planer. Le courant ascendant (zone claire) porte le planeur."


func composants() -> Dictionary:
	return {"Sol": "GroundMovement2D", "Aero": "AerodynamicMovement2D"}


func construire_niveau() -> void:
	arene()
	bloc(Rect2(-560, -60, 120, 260), Formes.DECOR)
	bloc(Rect2(-440, 100, 100, 24))
	zone_force(Rect2(80, -400, 220, 600), {"direction": Vector2.UP, "strength": 160.0})
	depart = Vector2(-380, 60)


func reglages() -> void:
	_aero = heros.get_node("Aero")
	for reglage in [["glide_fall_speed", 10.0, 300.0, 5.0], ["glide_speed", 50.0, 600.0, 10.0], ["glide_gravity_scale", 0.05, 1.0, 0.05], ["wind_influence", 0.0, 6.0, 0.1]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], _aero.get(nom), func(v: float): _aero.set(nom, v))
	panneau.ajouter_case("Vol libre (MODE_FLY, contexte « vol » forcé)", false, func(v: bool):
		_aero.mode = 1 if v else 0
		if v:
			if directeur.get_context(&"vol") == null:
				var vol := MovementContext.new()
				vol.context_name = &"vol"
				vol.condition = MovementContext.CONDITION_MANUAL
				vol.priority = 70
				vol.components = [NodePath("../Aero")]
				var liste: Array = directeur.contexts.duplicate()
				liste.append(vol)
				directeur.contexts = liste
			directeur.set_context(&"vol")
		else:
			directeur.clear_forced_context())
	panneau.ajouter_section("Feuilles (corps rigides)")
	for reglage in [["drag_coefficient", 0.0, 0.1, 0.002], ["lift_coefficient", 0.0, 0.05, 0.001], ["flutter_frequency", 0.0, 6.0, 0.1]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], _reglages_feuille[nom], func(v: float): _reglages_feuille[nom] = v)
	panneau.ajouter_bouton("Lâcher des feuilles", _jouer)


func _jouer() -> void:
	for i in 4:
		var corps := RigidBody2D.new()
		corps.position = Vector2(randf_range(-100, 500), -420)
		corps.mass = 0.05
		var forme := CollisionShape2D.new()
		forme.shape = RectangleShape2D.new()
		forme.shape.size = Vector2(22, 8)
		corps.add_child(forme)
		var dessin := Polygon2D.new()
		dessin.polygon = PackedVector2Array([Vector2(-12, 0), Vector2(-4, -5), Vector2(8, -4), Vector2(12, 0), Vector2(6, 4), Vector2(-6, 4)])
		dessin.color = Color("7bc96f")
		corps.add_child(dessin)
		var aero := AerodynamicMovement2D.new()
		for nom in _reglages_feuille:
			aero.set(nom, _reglages_feuille[nom])
		corps.add_child(aero)
		vue.add_child(corps)


func _process(_delta: float) -> void:
	if _aero:
		afficher_code("$AerodynamicMovement2D.glide_fall_speed = %d · glide_speed = %d\n$AerodynamicMovement2D.wind_influence = %s · mode = %d\nfeuille : drag %s · lift %s · flutter %s Hz" % [
			_aero.glide_fall_speed, _aero.glide_speed, snappedf(_aero.wind_influence, 0.1), _aero.mode,
			_reglages_feuille["drag_coefficient"], _reglages_feuille["lift_coefficient"], _reglages_feuille["flutter_frequency"]])
