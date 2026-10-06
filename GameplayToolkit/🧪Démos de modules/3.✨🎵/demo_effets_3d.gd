extends Node3D

## ✨ Démo des effets 3D : FXStretchAndSquash, FXChangeSize (+ FXAudio 3D), FXShake (+ couleur FXFade),
## FXFade (texture d'un Sprite3D), FXFollower, FXTargeter (caméra qui recule pour tout cadrer), TimeManipulatorFX.
##
## - Le cube rebondit tout seul ; les lunes tournent et le drone suit leur milieu.
## - Le panneau joue / arrête les effets, règle le temps et bascule sur la caméra de ciblage.

const Panneau := preload("../2.🧭🌍/panneau_demo.gd")

const GRAVITE := 30.0
const REBOND := 0.65
const SOL := 0.5

@onready var panneau: Panneau = $Interface/Panneau
@onready var cube: Node3D = $Etirement/Cube
@onready var fx_cube: PlayerFX = $Etirement/Cube/PlayerFX
@onready var fx_caisse: PlayerFX = $Taille/Caisse/PlayerFX
@onready var fx_sphere: PlayerFX = $Secousse/Sphere/PlayerFX
@onready var fx_panneau: PlayerFX = $Fondu/Panneau/PlayerFX
@onready var lune_a: Node3D = $Lunes/LuneA
@onready var lune_b: Node3D = $Lunes/LuneB
@onready var fx_drone: PlayerFX = $Lunes/Drone/PlayerFX
@onready var camera_fixe: Camera3D = $CameraFixe
@onready var camera_ciblage: Camera3D = $CameraCiblage
@onready var fx_camera: PlayerFX = $CameraCiblage/PlayerFX
@onready var temps: TimeManipulatorFX = $Temps

var _vitesse_cube := 0.0
var _angle_lunes := 0.0
var _ecart_lunes := 1.5
var _etat_temps: Label


func _ready() -> void:
	panneau.ajouter_section("🪗 Cube")
	panneau.ajouter_case("Orienter selon la vitesse", false, func(actif: bool): fx_cube.special_effects[0].orient_to_velocity = actif)
	panneau.ajouter_bouton("Relancer le cube", _relancer_cube)

	panneau.ajouter_section("🎛️ Effets")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Agrandir", _jouer.bind(fx_caisse, "🦣 Agrandissement + son 3D"), rangee)
	panneau.ajouter_bouton("Rétrécir", _arreter.bind(fx_caisse, "🫘 Taille d'origine"), rangee)
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("Secouer", _jouer.bind(fx_sphere, "💥 Secousse + rougeur"), rangee)
	panneau.ajouter_bouton("Fondu texture", _jouer.bind(fx_panneau, "🌈 Fondu vers la texture violette"), rangee)
	panneau.ajouter_bouton("Texture d'origine", _arreter.bind(fx_panneau, "🌈 Fondu retour"), rangee)
	panneau.ajouter_curseur("Écart des lunes", 0.5, 8.0, 0.1, _ecart_lunes, func(v: float): _ecart_lunes = v)

	panneau.ajouter_section("🎥 Caméra de ciblage")
	panneau.ajouter_case("Utiliser la caméra de ciblage", false, _camera_ciblage)
	panneau.ajouter_case("Réglage de la distance", true, func(actif: bool): fx_camera.special_effects[0].enable_camera_3d_depth_control = actif)

	panneau.ajouter_section("⏯️ Temps")
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("HitStop 0,3 s", func(): temps.trigger_hit_stop(0.3), rangee)
	panneau.ajouter_bouton("Ralenti ×0,25", func(): temps.set_time_scale(0.25), rangee)
	panneau.ajouter_bouton("Normal", func(): temps.set_time_scale(1.0), rangee)
	_etat_temps = panneau.ajouter_etiquette()

	for joueur in [fx_cube, fx_drone, fx_camera]:
		joueur.play()


func _process(delta: float) -> void:
	# Cube rebondissant
	_vitesse_cube -= GRAVITE * delta
	cube.position.y += _vitesse_cube * delta
	if cube.position.y < SOL:
		cube.position.y = SOL
		_vitesse_cube = absf(_vitesse_cube) * REBOND
		if _vitesse_cube < 3.0:
			_relancer_cube()

	# Lunes en orbite
	_angle_lunes += delta
	var decalage := Vector3(cos(_angle_lunes), 0, sin(_angle_lunes)) * _ecart_lunes
	lune_a.position = Vector3(0, 1.2, 0) + decalage
	lune_b.position = Vector3(0, 1.2 + sin(_angle_lunes * 2.0) * 0.6, 0) - decalage

	_etat_temps.text = "Engine.time_scale = %.2f" % Engine.time_scale


func _relancer_cube() -> void:
	_vitesse_cube = 16.0


func _jouer(joueur: PlayerFX, message: String) -> void:
	joueur.play()
	panneau.journaliser(message)


func _arreter(joueur: PlayerFX, message: String) -> void:
	joueur.stop()
	panneau.journaliser(message)


func _camera_ciblage(actif: bool) -> void:
	(camera_ciblage if actif else camera_fixe).make_current()
