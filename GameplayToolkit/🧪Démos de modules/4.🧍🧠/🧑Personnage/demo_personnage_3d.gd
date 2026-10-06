extends Node3D

## 🧊 Démo des personnages en 3D (construite par code pour rester courte à lire) :
## - Héros (Character3D + PlayerController) : déplacement relatif à la caméra, saut (Espace, double saut),
##   course (Maj), E pour parler.
## - Meunier (NPCController) : même dialogue que la démo 2D (dialogue_meunier.tres).
## - Compagnon (AutomateController.follow) : suit le héros à 2 m.
## - Garde (AutomateController, patrouille) : fait le tour de la place.

const Panneau := preload("../../2.🧭🌍/panneau_demo.gd")
const Dialogue := preload("dialogue_meunier.tres")

var heros: Character3D
var camera: Camera3D
var panneau: Panneau
var systeme: DialogueSystem


func _ready() -> void:
	_environnement()
	systeme = DialogueSystem.new()
	add_child(systeme)
	systeme.variables.set_values({"nom": "Héros", "slimes_restants": 0, "quete_acceptee": false, "quete_finie": false, "recompense_donnee": false, "pieces": 0})

	heros = _personnage("Héros", Vector3(0, 1, 4), Color(0.35, 0.65, 1.0))
	heros.get_node("Deplacement").max_jumps = 2
	heros.add_child(PlayerController.new())

	var meunier := _personnage("Meunier", Vector3(-3, 1, -2), Color(1.0, 0.7, 0.3))
	var pnj := NPCController.new()
	pnj.dialogue = Dialogue
	meunier.add_child(pnj)

	var compagnon := _personnage("Compagnon", Vector3(2, 1, 5), Color(0.5, 0.9, 0.5))
	compagnon.get_node("Deplacement").max_speed = 6.0
	var suiveur := AutomateController.new()
	compagnon.add_child(suiveur)
	suiveur.follow.call_deferred(heros, 2.0)

	var garde := _personnage("Garde", Vector3(6, 1, -6), Color(0.9, 0.35, 0.35))
	garde.get_node("Deplacement").max_speed = 3.0
	var patrouille := AutomateController.new()
	patrouille.patrol_points = [Vector3(6, 0, -6), Vector3(-6, 0, -6), Vector3(-6, 0, 6), Vector3(6, 0, 6)]
	patrouille.start_mode = AutomateController.MODE_PATROL
	patrouille.wait_time = 1.0
	garde.add_child(patrouille)

	camera = Camera3D.new()
	add_child(camera)
	camera.current = true

	var interface := CanvasLayer.new()
	add_child(interface)
	var boite := DialogueBox.new()
	boite.offset_left = 10
	boite.offset_top = 478
	boite.offset_right = 702
	boite.offset_bottom = 638
	interface.add_child(boite)
	boite.set_dialogue_system(systeme)
	panneau = Panneau.new()
	panneau.titre = "🧊 Personnages 3D"
	panneau.aide = "ZQSD / flèches : bouger (relatif à la caméra) · Espace : saut, double saut · Maj : courir · E : parler au meunier (orange). Le compagnon (vert) suit, le garde (rouge) patrouille."
	interface.add_child(panneau)
	heros.get_node("Deplacement").jumped.connect(func(): panneau.journaliser("🦘 saut"))
	heros.get_node("Deplacement").landed.connect(func(): panneau.journaliser("[color=gray]atterrissage[/color]"))
	pnj.talk_started.connect(func(_qui): panneau.journaliser("💬 Le meunier parle"))
	patrouille.patrol_point_reached.connect(func(i: int): panneau.journaliser("[color=gray]🚩 garde : point %d[/color]" % i))


func _process(delta: float) -> void:
	# Caméra qui suit le héros, de trois quarts.
	var cible := heros.global_position + Vector3(0, 7, 9)
	camera.global_position = camera.global_position.lerp(cible, minf(1.0, 5.0 * delta)) if camera.global_position != Vector3.ZERO else cible
	camera.look_at(heros.global_position + Vector3(0, 1, 0))


func _personnage(nom: String, position_depart: Vector3, couleur: Color) -> Character3D:
	var personnage := Character3D.new()
	personnage.name = nom
	personnage.display_name = nom
	personnage.position = position_depart
	var forme := CollisionShape3D.new()
	forme.shape = CapsuleShape3D.new()
	personnage.add_child(forme)
	var deplacement: Node = ClassDB.instantiate(&"GroundMovement3D") # composant du module Mouvement & Physique
	deplacement.name = "Deplacement"
	personnage.add_child(deplacement)
	var corps := MeshInstance3D.new()
	corps.mesh = CapsuleMesh.new()
	var materiau := StandardMaterial3D.new()
	materiau.albedo_color = couleur
	corps.material_override = materiau
	personnage.add_child(corps)
	var nez := MeshInstance3D.new() # repère de l'avant (-Z)
	var boite := BoxMesh.new()
	boite.size = Vector3(0.2, 0.2, 0.4)
	nez.mesh = boite
	nez.position = Vector3(0, 0.5, -0.5)
	personnage.add_child(nez)
	var etiquette := Label3D.new()
	etiquette.text = nom
	etiquette.font_size = 64
	etiquette.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	etiquette.position = Vector3(0, 1.5, 0)
	personnage.add_child(etiquette)
	add_child(personnage)
	return personnage


func _environnement() -> void:
	var lumiere := DirectionalLight3D.new()
	lumiere.rotation_degrees = Vector3(-55, 30, 0)
	lumiere.shadow_enabled = true
	add_child(lumiere)
	var monde := WorldEnvironment.new()
	monde.environment = Environment.new()
	monde.environment.background_mode = Environment.BG_COLOR
	monde.environment.background_color = Color(0.11, 0.12, 0.15)
	monde.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	monde.environment.ambient_light_color = Color(0.5, 0.5, 0.55)
	add_child(monde)
	_bloc(Vector3(0, -0.5, 0), Vector3(30, 1, 30), Color(0.25, 0.3, 0.25))
	_bloc(Vector3(-3, 0.5, -5), Vector3(3, 1, 1), Color(0.5, 0.4, 0.3))
	_bloc(Vector3(3, 0.25, 0), Vector3(2, 0.5, 2), Color(0.45, 0.45, 0.5))
	_bloc(Vector3(5, 0.75, 1), Vector3(1.5, 1.5, 1.5), Color(0.45, 0.45, 0.5))


func _bloc(centre: Vector3, taille: Vector3, couleur: Color) -> void:
	var corps := StaticBody3D.new()
	corps.position = centre
	var forme := CollisionShape3D.new()
	var boite := BoxShape3D.new()
	boite.size = taille
	forme.shape = boite
	corps.add_child(forme)
	var maillage := MeshInstance3D.new()
	var mesh := BoxMesh.new()
	mesh.size = taille
	maillage.mesh = mesh
	var materiau := StandardMaterial3D.new()
	materiau.albedo_color = couleur
	maillage.material_override = materiau
	corps.add_child(maillage)
	add_child(corps)
