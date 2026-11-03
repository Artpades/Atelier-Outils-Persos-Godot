extends "res://🎥Démos de documentation/5.🌀🫗/apercu_mouvement.gd"

## 🌬️ Aperçu interactif de Knockback2D : un héros au clavier touché par un ennemi à pointes, des caisses ;
## clic = explosion qui repousse tout autour. Réglages du recul du héros.

var _recul: Knockback2D
var _ennemi: Node2D
var _caisses: Array[RigidBody2D] = []


func titre() -> String:
	return "Recul (Knockback2D)"


func aide() -> String:
	return "Clavier : flèches, Espace. Toucher l'ennemi rouge ; cliquer dans la zone de jeu pour une explosion."


func composants() -> Dictionary:
	return {"Sol": "GroundMovement2D", "Recul": "Knockback2D"}


func construire_niveau() -> void:
	arene()
	_ennemi = Formes.carre(vue, Vector2(120, 177), Formes.MAUVAIS, 46)
	for i in 3:
		var corps := RigidBody2D.new()
		corps.position = Vector2(360 + i * 60, 170)
		var forme := CollisionShape2D.new()
		forme.shape = RectangleShape2D.new()
		forme.shape.size = Vector2(44, 44)
		corps.add_child(forme)
		var dessin := Polygon2D.new()
		dessin.polygon = Formes.rectangle(Vector2(44, 44))
		dessin.color = Color("a0714f")
		corps.add_child(dessin)
		var recul := Knockback2D.new()
		recul.name = "Recul"
		corps.add_child(recul)
		vue.add_child(corps)
		_caisses.append(corps)


func reglages() -> void:
	_recul = heros.get_node("Recul")
	for reglage in [["base_force", 50.0, 1500.0, 25.0], ["duration", 0.0, 1.5, 0.05], ["friction", 100.0, 5000.0, 100.0], ["upward_bias", 0.0, 1.5, 0.05]]:
		var nom: String = reglage[0]
		panneau.ajouter_curseur(nom, reglage[1], reglage[2], reglage[3], _recul.get(nom), func(v: float): _recul.set(nom, v))
	panneau.ajouter_case("enabled", true, func(v: bool): _recul.enabled = v)
	_recul.knocked_back.connect(func(v: Vector2): journaliser("knocked_back(%s)" % v.round()))
	_recul.recovered.connect(func(): journaliser("recovered"))


func _physics_process(delta: float) -> void:
	super(delta)
	if _recul and absf(heros.position.x - _ennemi.position.x) < 40 and absf(heros.position.y - _ennemi.position.y) < 60 and not _recul.is_knocked():
		_recul.apply_knockback_from(_ennemi.global_position)


func _unhandled_input(evenement: InputEvent) -> void:
	if evenement is InputEventMouseButton and evenement.pressed and ZONE.has_point(evenement.position):
		var centre: Vector2 = vue.get_canvas_transform().affine_inverse() * (get_global_mouse_position() - Vector2(16, 150))
		for corps in [heros] + _caisses:
			var d: float = corps.global_position.distance_to(centre)
			if d < 300.0:
				corps.get_node("Recul").apply_knockback_from(centre, 2.0 * (1.0 - d / 300.0))


func _process(_delta: float) -> void:
	if _recul:
		afficher_code("$Knockback2D.apply_knockback_from(position_ennemi)\nis_knocked() = %s · get_lock_remaining() = %.2f\nbase_force = %d · duration = %s · friction = %d" % [
			_recul.is_knocked(), _recul.get_lock_remaining(), _recul.base_force, snappedf(_recul.duration, 0.01), _recul.friction])
