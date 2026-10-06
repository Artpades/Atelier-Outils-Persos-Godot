extends Node2D

## ✨ Démo des effets 2D : PlayerFX et les effets FXStretchAndSquash, FXChangeSize (+ FXAudio enchaîné),
## FXShake (+ flash FXFade enchaîné), FXTargeter, FXFade (texture), FXFollower, et TimeManipulatorFX.
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_effets.gd).
## - Partie interactive : la balle rebondit toute seule (étirement en vol, écrasement à l'impact) ; le viseur
##   tourne autour de la tourelle ou suit la souris ; le drone suit le milieu de la balle et du viseur.
##   Le panneau joue / arrête chaque effet, règle le temps (HitStop, ralenti) et la caméra suiveuse.

const Panneau := preload("../2.🧭🌍/panneau_demo.gd")
const SuiteTests := preload("suite_tests_effets.gd")

const GRAVITE := 2000.0
const REBOND := 0.6
const SOL := 250.0 - 20.0 # sol de la station, moins le rayon de la balle
const RAYON_VISEUR := 90.0

@onready var panneau: Panneau = $Interface/Panneau
@onready var balle: Node2D = $Etirement/Balle
@onready var fx_balle: PlayerFX = $Etirement/Balle/PlayerFX
@onready var fx_caisse: PlayerFX = $Taille/Caisse/PlayerFX
@onready var fx_cible: PlayerFX = $Secousse/Cible/PlayerFX
@onready var fx_tourelle: PlayerFX = $Ciblage/Tourelle/PlayerFX
@onready var tourelle: Node2D = $Ciblage/Tourelle
@onready var viseur: Node2D = $Ciblage/Viseur
@onready var fx_panneau: PlayerFX = $Fondu/Panneau/PlayerFX
@onready var fx_drone: PlayerFX = $Suivi/Drone/PlayerFX
@onready var camera: Camera2D = $Camera
@onready var fx_camera: PlayerFX = $Camera/PlayerFX
@onready var temps: TimeManipulatorFX = $Temps

var _vitesse_balle := Vector2(140, 0)
var _viseur_souris := false
var _angle_viseur := 0.0
var _etat_temps: Label


func _ready() -> void:
	panneau.ajouter_section("🪗 Balle")
	panneau.ajouter_case("Orienter selon la vitesse", false, func(actif: bool): _effet(fx_balle).orient_to_velocity = actif)
	panneau.ajouter_case("Écrasement à l'impact", true, func(actif: bool): _effet(fx_balle).enable_impact_squash = actif)
	panneau.ajouter_curseur("Étirement max", 0.0, 2.0, 0.05, _effet(fx_balle).max_stretch, func(v: float): _effet(fx_balle).max_stretch = v)
	panneau.ajouter_bouton("Relancer la balle", _relancer_balle)

	panneau.ajouter_section("🎛️ Effets")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Agrandir", _jouer.bind(fx_caisse, "🦣 Agrandissement + son"), rangee)
	panneau.ajouter_bouton("Rétrécir", _arreter.bind(fx_caisse, "🫘 Retour à la taille d'origine"), rangee)
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("Secouer", _jouer.bind(fx_cible, "💥 Secousse + flash rouge"), rangee)
	panneau.ajouter_bouton("Fondu texture", _jouer.bind(fx_panneau, "🌈 Fondu vers la texture violette"), rangee)
	panneau.ajouter_bouton("Texture d'origine", _arreter.bind(fx_panneau, "🌈 Fondu retour"), rangee)
	panneau.ajouter_case("Viseur suit la souris", false, func(actif: bool): _viseur_souris = actif)
	panneau.ajouter_case("Tourelle : rotation lissée", false, func(actif: bool): _effet(fx_tourelle).add_lerp = actif)

	panneau.ajouter_section("⏯️ Temps")
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("HitStop 0,3 s", _hit_stop, rangee)
	panneau.ajouter_bouton("Ralenti ×0,25", _vitesse.bind(0.25), rangee)
	panneau.ajouter_bouton("Normal", _vitesse.bind(1.0), rangee)
	_etat_temps = panneau.ajouter_etiquette()
	temps.hit_stop_started.connect(func(): panneau.journaliser("⏸️ HitStop"))
	temps.hit_stop_finished.connect(func(): panneau.journaliser("▶️ Fin du HitStop"))

	panneau.ajouter_section("🎥 Caméra")
	panneau.ajouter_case("Caméra suit balle + viseur (zoom auto)", false, _camera_suiveuse)

	panneau.ajouter_section("🧪 Tests")
	panneau.ajouter_bouton("Relancer les tests automatiques", _lancer_tests)

	# Effets permanents : étirement de la balle, ciblage, drone
	for joueur in [fx_balle, fx_tourelle, fx_drone]:
		joueur.play()
	_lancer_tests()


func _process(delta: float) -> void:
	_animer_balle(delta)
	_animer_viseur(delta)
	_etat_temps.text = "Engine.time_scale = %.2f" % Engine.time_scale


func _effet(joueur: PlayerFX) -> Effect:
	return joueur.special_effects[0]


# 🪗 Balle rebondissante

func _animer_balle(delta: float) -> void:
	_vitesse_balle.y += GRAVITE * delta
	balle.position += _vitesse_balle * delta
	if balle.position.y > SOL:
		balle.position.y = SOL
		_vitesse_balle.y = -absf(_vitesse_balle.y) * REBOND
		# Presque à l'arrêt : nouveau lancer
		if absf(_vitesse_balle.y) < 250.0:
			_relancer_balle()
	if absf(balle.position.x) > 70.0:
		balle.position.x = clampf(balle.position.x, -70.0, 70.0)
		_vitesse_balle.x = -_vitesse_balle.x


func _relancer_balle() -> void:
	_vitesse_balle = Vector2(randf_range(-220, 220), -1300)


# 🌻 Viseur

func _animer_viseur(delta: float) -> void:
	if _viseur_souris:
		viseur.global_position = get_global_mouse_position()
		return
	_angle_viseur += delta * 1.5
	viseur.position = tourelle.position + Vector2.from_angle(_angle_viseur) * RAYON_VISEUR


# 🎛️ Commandes

func _jouer(joueur: PlayerFX, message: String) -> void:
	joueur.play()
	panneau.journaliser(message)


func _arreter(joueur: PlayerFX, message: String) -> void:
	joueur.stop()
	panneau.journaliser(message)


func _hit_stop() -> void:
	temps.trigger_hit_stop(0.3)


func _vitesse(echelle: float) -> void:
	temps.set_time_scale(echelle)
	panneau.journaliser("⏱️ Vitesse visée : ×%s" % echelle)


func _camera_suiveuse(actif: bool) -> void:
	camera.enabled = actif
	if actif:
		camera.make_current()
		fx_camera.play()
	else:
		fx_camera.stop()
		camera.position = Vector2(576, 324)
		camera.zoom = Vector2.ONE


func _lancer_tests() -> void:
	# Le TimeManipulatorFX de la démo contrarierait celui des tests (vitesse du jeu)
	temps.process_mode = Node.PROCESS_MODE_DISABLED
	await panneau.lancer_tests("Tests du module d'effets spéciaux", SuiteTests, self)
	temps.process_mode = Node.PROCESS_MODE_INHERIT
	temps.set_time_scale(1.0)
