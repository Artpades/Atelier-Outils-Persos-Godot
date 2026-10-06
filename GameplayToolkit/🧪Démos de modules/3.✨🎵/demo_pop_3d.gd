extends Node3D

## 💥 Démo des pops en 3D : PopInstance3D (face à la caméra), screen_space_3d, sol et rebond, scène 3D.
##
## La cible va et vient : les pops restent là où ils sont apparus.

const Panneau := preload("../2.🧭🌍/panneau_demo.gd")

@export var piece: Texture2D
@export var gemme: PackedScene

@onready var panneau: Panneau = $Interface/Panneau
@onready var cible: Node3D = $Cible
@onready var degats: PopSpawner = $Cible/Degats
@onready var degats_ecran: PopSpawner = $Cible/DegatsEcran
@onready var butin: PopSpawner = $Cible/Butin
@onready var gemmes: PopSpawner = $Cible/Gemmes

var _temps := 0.0
var _mouvement := true


func _ready() -> void:
	panneau.ajouter_section("🥊 Dégâts")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Dans le monde 3D", func(): degats.pop_number(-randi_range(5, 40)), rangee)
	panneau.ajouter_bouton("À l'écran", func(): degats_ecran.pop_number(-randi_range(5, 40)), rangee)
	panneau.ajouter_case("Toujours devant (sans test de profondeur)", true, func(actif: bool): degats.style.always_on_top_3d = actif)

	panneau.ajouter_section("🪙 Butin")
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("Pièces (images)", _pieces, rangee)
	panneau.ajouter_bouton("Gemmes (scène 3D)", _gemmes, rangee)
	panneau.ajouter_bouton("Pièce + texte", func(): butin.pop("+25", piece, null, Color(1, 0.85, 0.3)))
	panneau.ajouter_curseur("unit_scale_3d", 0.002, 0.03, 0.001, butin.unit_scale_3d, _echelle)

	panneau.ajouter_section("🎯 Cible")
	panneau.ajouter_case("La cible se déplace", true, func(actif: bool): _mouvement = actif)


func _process(delta: float) -> void:
	if _mouvement:
		_temps += delta
		cible.position.x = -1.8 + sin(_temps) * 2.5


func _pieces() -> void:
	for i in 15:
		butin.pop_texture(piece)


func _gemmes() -> void:
	for i in 6:
		gemmes.pop_scene(gemme)


func _echelle(valeur: float) -> void:
	for spawner in [degats, butin, gemmes]:
		spawner.unit_scale_3d = valeur
