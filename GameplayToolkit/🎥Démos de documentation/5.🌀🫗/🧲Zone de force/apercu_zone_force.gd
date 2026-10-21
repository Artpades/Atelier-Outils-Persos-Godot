extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🧲 Aperçu interactif de ForceZone2D : une zone réglable (mode, direction, intensité, atténuation,
## turbulence), un héros au clavier et des caisses à lâcher.

var _zone: ForceZone2D


func titre() -> String:
	return "Zone de force (ForceZone2D)"


func aide() -> String:
	return "Flèches / Espace pour traverser la zone. Lâcher des caisses et changer le mode : vent, aimant, tourbillon, freinage."


func construire_niveau() -> void:
	arene()
	_zone = zone_force(Rect2(-150, -250, 380, 450), {"direction": Vector2.UP, "strength": 1200.0, "debug_mode": true})


func reglages() -> void:
	panneau.ajouter_options("mode", PackedStringArray(["MODE_DIRECTIONAL", "MODE_ATTRACT", "MODE_VORTEX", "MODE_DRAG"]), 0, func(i: int):
		_zone.mode = i
		_zone.strength = [1200.0, 2500.0, 1500.0, 4.0][i])
	panneau.ajouter_curseur("Angle de direction (°)", -180.0, 180.0, 5.0, -90.0, func(v: float): _zone.direction = Vector2.from_angle(deg_to_rad(v)))
	panneau.ajouter_curseur("strength", 0.0, 3000.0, 20.0, 1200.0, func(v: float): _zone.strength = v)
	panneau.ajouter_options("falloff", PackedStringArray(["FALLOFF_NONE", "FALLOFF_LINEAR", "FALLOFF_INVERSE_SQUARE"]), 0, func(i: int): _zone.falloff = i)
	panneau.ajouter_curseur("turbulence", 0.0, 1.0, 0.05, 0.0, func(v: float): _zone.turbulence = v)
	panneau.ajouter_case("affect_characters", true, func(v: bool): _zone.affect_characters = v)
	panneau.ajouter_case("enabled", true, func(v: bool): _zone.enabled = v)
	panneau.ajouter_bouton("Lâcher des caisses", _jouer)


func _jouer() -> void:
	for i in 4:
		var corps := RigidBody2D.new()
		corps.position = Vector2(randf_range(-120, 200), -300)
		var forme := CollisionShape2D.new()
		forme.shape = RectangleShape2D.new()
		forme.shape.size = Vector2(26, 26)
		corps.add_child(forme)
		var dessin := Polygon2D.new()
		dessin.polygon = Formes.rectangle(Vector2(26, 26))
		dessin.color = Color("a0714f")
		corps.add_child(dessin)
		vue.add_child(corps)


func _process(_delta: float) -> void:
	if _zone and heros:
		afficher_code("$ForceZone2D.mode = %d · strength = %d\n$ForceZone2D.direction = %s\nForceZone2D.get_acceleration_on($Heros) = %s" % [
			_zone.mode, _zone.strength, _zone.direction.snapped(Vector2(0.01, 0.01)), ForceZone2D.get_acceleration_on(heros).round()])
