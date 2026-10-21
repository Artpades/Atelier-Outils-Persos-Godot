extends "res://🎥Démos de documentation/socle/sequences.gd"

## 🌀 Outils communs aux plans filmés des pages « Mouvement & Physique » : monde physique dans une vue,
## blocs, eau, personnages équipés de composants, et pilote d'intention (move / jump comme un joueur).

const COULEUR_EAU := Color(0.2, 0.45, 0.9, 0.45)
const COULEUR_VENT := Color(0.75, 0.9, 1.0, 0.12)


## Monde 2D (vue encadrée avec sa propre physique). La caméra de la vue est vue.get_meta("camera").
func monde_2d(cadre: Rect2 = Rect2(76, 110, 1000, 440), zoom: float = 1.0) -> SubViewport:
	var vue := vue_2d(cadre)
	var camera: Camera2D = vue.get_meta("camera")
	camera.zoom = Vector2.ONE * zoom
	camera.position.y = 50.0  # sol des arènes (y = 200) en bas de la vue
	return vue


## Bloc statique (StaticBody2D) occupant `rect` (coordonnées du monde).
func bloc(parent: Node, rect: Rect2, couleur: Color = Formes.SOL) -> StaticBody2D:
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
	parent.add_child(corps)
	return corps


## Étendue d'eau (WaterArea2D) occupant `rect`.
func eau(parent: Node, rect: Rect2, reglages: Dictionary = {}) -> WaterArea2D:
	var zone := WaterArea2D.new()
	zone.position = rect.get_center()
	for propriete in reglages:
		zone.set(propriete, reglages[propriete])
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = rect.size
	zone.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(rect.size)
	dessin.color = COULEUR_EAU
	dessin.z_index = 5
	zone.add_child(dessin)
	parent.add_child(zone)
	return zone


## Zone de force (ForceZone2D) occupant `rect`, réglée par `reglages` (direction, strength, mode...).
func zone_force(parent: Node, rect: Rect2, reglages: Dictionary = {}) -> ForceZone2D:
	var zone := ForceZone2D.new()
	zone.position = rect.get_center()
	for propriete in reglages:
		zone.set(propriete, reglages[propriete])
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = rect.size
	zone.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(rect.size)
	dessin.color = COULEUR_VENT
	zone.add_child(dessin)
	parent.add_child(zone)
	return zone


## Personnage 2D (CharacterBody2D, capsule 28 × 48) avec les composants `composants` ({nom: classe} ou
## [classe, ...]) réglés par `reglages` ({nom ou classe: {propriété: valeur}}). Renvoie le corps ; ses
## composants sont accessibles par leur nom (corps.get_node("Sol")...).
func heros_2d(parent: Node, position: Vector2, composants, reglages: Dictionary = {}, couleur: Color = VARIANTES[0]) -> CharacterBody2D:
	var corps := CharacterBody2D.new()
	corps.name = "Heros"
	corps.position = position
	var forme := CollisionShape2D.new()
	forme.shape = CapsuleShape2D.new()
	forme.shape.radius = 14.0
	forme.shape.height = 48.0
	corps.add_child(forme)
	var visuel := Node2D.new()
	visuel.name = "Visuel"
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(28, 48))
	dessin.color = couleur
	visuel.add_child(dessin)
	var oeil := Polygon2D.new()
	oeil.polygon = Formes.rectangle(Vector2(6, 6))
	oeil.position = Vector2(7, -12)
	oeil.color = Color("1b1f2a")
	visuel.add_child(oeil)
	corps.add_child(visuel)
	var liste: Dictionary = composants if composants is Dictionary else {}
	if composants is Array:
		for classe in composants:
			liste[classe] = classe
	for nom in liste:
		var composant: Node = ClassDB.instantiate(liste[nom])
		composant.name = nom
		for cle in [nom, liste[nom]]:
			if reglages.has(cle):
				for propriete in reglages[cle]:
					composant.set(propriete, reglages[cle][propriete])
		corps.add_child(composant)
	parent.add_child(corps)
	return corps


## Active debug_mode sur tous les nœuds qui en ont un sous `racine`.
func debug(racine: Node, actif: bool = true) -> void:
	for noeud in racine.find_children("*", "", true, false):
		if "debug_mode" in noeud:
			noeud.debug_mode = actif


## Pilote d'intention : joue une suite d'étapes [durée, direction (Vector2), action] sur `cible` (composant
## ou MovementDirector). action : "" (rien), "saut" (jump au début), "saut_court" (jump puis release_jump
## après 0,08 s), "saut_long" (jump maintenu toute l'étape), "course" (set_running pendant l'étape).
func piloter(cible: Node, etapes: Array) -> void:
	for etape in etapes:
		var duree: float = etape[0]
		var direction: Vector2 = etape[1]
		var action: String = etape[2] if etape.size() > 2 else ""
		cible.move(direction)
		if action == "course":
			cible.set_running(true)
		if action.begins_with("saut"):
			cible.jump()
		if action == "saut_court":
			await secondes(0.08)
			cible.release_jump()
			await secondes(maxf(duree - 0.08, 0.0))
		else:
			await secondes(duree)
			if action.begins_with("saut"):
				cible.release_jump()
		if action == "course":
			cible.set_running(false)
	cible.move(Vector2.ZERO)


## Sol + murs d'une petite arène 2D (largeur 1600, sol à y = 200), dans la vue.
func arene(vue: Node, largeur: float = 1600.0) -> void:
	bloc(vue, Rect2(-largeur / 2.0, 200, largeur, 400))
	bloc(vue, Rect2(-largeur / 2.0 - 40, -800, 40, 1000))
	bloc(vue, Rect2(largeur / 2.0, -800, 40, 1000))


# 🧊 3D

## Monde 3D (vue encadrée), caméra plus haute et reculée.
func monde_3d(cadre: Rect2 = Rect2(76, 110, 1000, 440)) -> SubViewport:
	var vue := vue_3d(cadre)
	var camera := vue.get_camera_3d()
	camera.position = Vector3(0, 6, 11)
	camera.rotation_degrees.x = -25
	return vue


func bloc_3d(parent: Node, centre: Vector3, taille: Vector3, couleur: Color = Formes.SOL) -> StaticBody3D:
	var corps := StaticBody3D.new()
	corps.position = centre
	var forme := CollisionShape3D.new()
	forme.shape = BoxShape3D.new()
	forme.shape.size = taille
	corps.add_child(forme)
	Formes.cube(corps, Vector3.ZERO, couleur, taille)
	parent.add_child(corps)
	return corps


func heros_3d(parent: Node, position: Vector3, composants, reglages: Dictionary = {}, couleur: Color = VARIANTES[0]) -> CharacterBody3D:
	var corps := CharacterBody3D.new()
	corps.name = "Heros"
	corps.position = position
	var forme := CollisionShape3D.new()
	forme.shape = CapsuleShape3D.new()
	forme.shape.radius = 0.4
	forme.shape.height = 1.6
	corps.add_child(forme)
	var visuel := Node3D.new()
	visuel.name = "Visuel"
	var maillage := MeshInstance3D.new()
	maillage.mesh = CapsuleMesh.new()
	maillage.mesh.radius = 0.4
	maillage.mesh.height = 1.6
	maillage.mesh.material = Formes.matiere(couleur)
	visuel.add_child(maillage)
	Formes.cube(visuel, Vector3(0, 0.45, -0.35), Color("1b1f2a"), Vector3(0.3, 0.12, 0.1))
	corps.add_child(visuel)
	var liste: Dictionary = composants if composants is Dictionary else {}
	if composants is Array:
		for classe in composants:
			liste[classe] = classe
	for nom in liste:
		var composant: Node = ClassDB.instantiate(liste[nom])
		composant.name = nom
		for cle in [nom, liste[nom]]:
			if reglages.has(cle):
				for propriete in reglages[cle]:
					composant.set(propriete, reglages[cle][propriete])
		corps.add_child(composant)
	parent.add_child(corps)
	return corps
