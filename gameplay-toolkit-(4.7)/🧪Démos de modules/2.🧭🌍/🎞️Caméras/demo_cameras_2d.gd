extends Node2D

## 🎞️ Démo du module caméras (2D) : CameraManager et CameraPanner.
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_cameras.gd).
## - Partie interactive : basculer entre deux caméras fixes (position, zoom, rotation et offset différents)
##   et une caméra libre (CameraPanner) : bouton du milieu maintenu, molette, flèches, gestes du trackpad.

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_cameras.gd")

@onready var panneau: Panneau = $Interface/Panneau
@onready var camera_libre: CameraPanner = $CamLibre
@onready var cameras: Array[Camera2D] = [$CamA, $CamB, $CamLibre]

var _gestionnaire: CameraManager
var _fluide := true
var _duree := 1.0
var _etat: Label


func _ready() -> void:
	_gestionnaire = CameraManager.get_instance()
	_gestionnaire.current_camera_2d_switched.connect(_on_camera_basculee)

	panneau.ajouter_section("🎥 Caméras")
	for camera in cameras:
		panneau.ajouter_bouton(camera.name, _basculer.bind(camera))
	_etat = panneau.ajouter_etiquette()

	panneau.ajouter_section("〰️ Transition")
	panneau.ajouter_case("Transition fluide", _fluide, func(actif: bool): _fluide = actif)
	panneau.ajouter_curseur("Durée (s)", 0.1, 3.0, 0.1, _duree, func(valeur: float): _duree = valeur)

	panneau.ajouter_section("🖐️ Caméra libre (CameraPanner)")
	panneau.ajouter_curseur("Facteur de zoom", 1.05, 2.0, 0.05, camera_libre.zoom_factor, func(valeur: float): camera_libre.zoom_factor = valeur)
	panneau.ajouter_curseur("Lissage", 1.0, 60.0, 1.0, camera_libre.lerp_speed, func(valeur: float): camera_libre.lerp_speed = valeur)
	panneau.ajouter_options("Bouton de pan", PackedStringArray(["Gauche", "Droit", "Milieu"]), camera_libre.pan_mouse_button - 1,
		func(index: int): camera_libre.pan_mouse_button = (index + 1) as MouseButton)
	panneau.ajouter_bouton("Recentrer", func(): camera_libre.teleport(Vector2(400, 250), 1.0))

	panneau.ajouter_section("🧪 Tests")
	panneau.ajouter_bouton("Relancer les tests automatiques", _lancer_tests)
	_lancer_tests()


func _process(_delta: float) -> void:
	var courante := get_viewport().get_camera_2d()
	_etat.text = "Caméra courante : %s%s\nCaméra libre : position (%.0f, %.0f), zoom %.2f" % [
		courante.name if courante else "aucune",
		"  (transition en cours)" if _gestionnaire.is_transitioning_2d() else "",
		camera_libre.position.x, camera_libre.position.y, camera_libre.zoom.x]


func _basculer(camera: Camera2D) -> void:
	_gestionnaire.switch_2d_camera(camera, _fluide, _duree)


func _on_camera_basculee(camera: Camera2D) -> void:
	panneau.journaliser("📡 current_camera_2d_switched → [b]%s[/b]" % camera.name)


func _lancer_tests() -> void:
	panneau.lancer_tests("Tests du module caméras", SuiteTests, self)
