extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🌀 Socle des aperçus web des pages « Mouvement & Physique » : un niveau 2D dans une vue, un héros piloté
## au clavier (flèches : déplacement, Espace : saut, Maj : course, Haut / Bas : commande verticale) via un
## MovementDirector, et une case pour les gizmos de débogage. Un aperçu dérivé surcharge construire_niveau()
## (blocs, eau, zones) et composants() (les composants du héros), puis ajoute ses réglages.

const COULEUR_EAU := Color(0.2, 0.45, 0.9, 0.45)
const COULEUR_VENT := Color(0.75, 0.9, 1.0, 0.12)

var vue: SubViewport
var heros: CharacterBody2D
var directeur: MovementDirector
var depart := Vector2(-300, 120)
var _etat: Label


## Composants du héros : { nom : classe } (le MovementDirector est ajouté en plus).
func composants() -> Dictionary:
	return {"Sol": "GroundMovement2D"}


## Niveau de la page (appelé avant la création du héros).
func construire_niveau() -> void:
	arene()


func construire() -> void:
	vue = vue_2d(Rect2(16, 150, 680, 470))
	vue.get_meta("camera").position.y = 40.0
	construire_niveau()
	heros = CharacterBody2D.new()
	heros.name = "Heros"
	heros.position = depart
	var forme := CollisionShape2D.new()
	forme.shape = CapsuleShape2D.new()
	forme.shape.radius = 14.0
	forme.shape.height = 48.0
	heros.add_child(forme)
	var visuel := Polygon2D.new()
	visuel.name = "Visuel"
	visuel.polygon = Formes.rectangle(Vector2(28, 48))
	visuel.color = Formes.VARIANTES[0]
	heros.add_child(visuel)
	var liste := composants()
	for nom in liste:
		var composant: Node = ClassDB.instantiate(liste[nom])
		composant.name = nom
		heros.add_child(composant)
	directeur = MovementDirector.new()
	directeur.name = "Directeur"
	heros.add_child(directeur)
	vue.add_child(heros)
	_etat = etiquette(Vector2(20, 120), "", 15, Formes.ACCENT)
	directeur.context_changed.connect(func(_avant, apres): journaliser("contexte : %s" % apres))
	panneau.ajouter_case("Gizmos de débogage (debug_mode)", true, _debug)
	panneau.ajouter_bouton("Replacer le héros", func(): heros.position = depart; heros.velocity = Vector2.ZERO)
	_debug.call_deferred(true)
	reglages()


## Réglages propres à la page (panneau.ajouter_...).
func reglages() -> void:
	pass


func _debug(actif: bool) -> void:
	for noeud in vue.find_children("*", "", true, false):
		if "debug_mode" in noeud:
			noeud.debug_mode = actif


func _physics_process(_delta: float) -> void:
	if heros == null:
		return
	var direction := Vector2(float(Input.is_key_pressed(KEY_RIGHT)) - float(Input.is_key_pressed(KEY_LEFT)),
		float(Input.is_key_pressed(KEY_DOWN)) - float(Input.is_key_pressed(KEY_UP)))
	directeur.move(direction)
	directeur.set_vertical_input(-direction.y)
	directeur.set_running(Input.is_key_pressed(KEY_SHIFT))
	if heros.position.y > 1500:
		heros.position = depart
		heros.velocity = Vector2.ZERO
	_etat.text = "Contexte : %s · vitesse : %d px/s" % [directeur.get_current_context(), heros.velocity.length()]


func _unhandled_key_input(evenement: InputEvent) -> void:
	if evenement is InputEventKey and evenement.keycode == KEY_SPACE and not evenement.echo:
		if evenement.pressed:
			directeur.jump()
		else:
			directeur.release_jump()


func _jouer() -> void:
	directeur.jump()


# 🏗️ Niveau

func bloc(rect: Rect2, couleur: Color = Formes.SOL) -> StaticBody2D:
	var corps := StaticBody2D.new()
	corps.position = rect.get_center()
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = rect.size
	corps.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(rect.size)
	dessin.color = couleur
	corps.add_child(dessin)
	vue.add_child(corps)
	return corps


func eau(rect: Rect2) -> WaterArea2D:
	var zone := WaterArea2D.new()
	zone.position = rect.get_center()
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = rect.size
	zone.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(rect.size)
	dessin.color = COULEUR_EAU
	dessin.z_index = 5
	zone.add_child(dessin)
	vue.add_child(zone)
	return zone


func zone_force(rect: Rect2, reglages_zone: Dictionary = {}) -> ForceZone2D:
	var zone := ForceZone2D.new()
	zone.position = rect.get_center()
	for propriete in reglages_zone:
		zone.set(propriete, reglages_zone[propriete])
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = rect.size
	zone.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(rect.size)
	dessin.color = COULEUR_VENT
	zone.add_child(dessin)
	vue.add_child(zone)
	return zone


func arene(largeur: float = 1400.0) -> void:
	bloc(Rect2(-largeur / 2.0, 200, largeur, 400))
	bloc(Rect2(-largeur / 2.0 - 40, -900, 40, 1100))
	bloc(Rect2(largeur / 2.0, -900, 40, 1100))
