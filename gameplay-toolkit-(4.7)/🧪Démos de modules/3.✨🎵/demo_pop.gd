extends Node2D

## 💥 Démo des pops : PopSpawner, PopStyle et FXPop.
##
## - Clic sur le mannequin : chiffre de dégâts qui retombe (pop_number) + secousse et « PAF ! » (FXPop enchaîné).
## - Clic sur le coffre : gerbe de pièces (images) qui rebondissent sur le sol + total « x12 » (image et texte).
## - Clic ailleurs : une étoile (scène instanciée) jaillit sous la souris (paramètre « at »).
## - Bouton « Acheter » : pop dans l'interface (CanvasLayer).

const Panneau := preload("../2.🧭🌍/panneau_demo.gd")
const SuiteTests := preload("suite_tests_pop.gd")

@export var piece: Texture2D
@export var etoile: PackedScene

@onready var panneau: Panneau = $Interface/Panneau
@onready var mannequin: Node2D = $Mannequin
@onready var degats: PopSpawner = $Mannequin/Degats
@onready var coup: PlayerFX = $Mannequin/PlayerFX
@onready var coffre: Node2D = $Coffre
@onready var pieces: PopSpawner = $Coffre/Pieces
@onready var total: PopSpawner = $Coffre/Total
@onready var clic: PopSpawner = $Clic
@onready var bourse_label: Label = $Hud/Bourse
@onready var acheter: Button = $Hud/Acheter
@onready var pop_achat: PopSpawner = $Hud/Acheter/Pop

var _bourse := 0
var _nombre_pieces := 12
var _actifs: Label


func _ready() -> void:
	acheter.pressed.connect(_acheter)

	panneau.ajouter_section("🥊 Dégâts")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Frapper", _frapper.bind(false), rangee)
	panneau.ajouter_bouton("Coup critique", _frapper.bind(true), rangee)
	panneau.ajouter_curseur("Gravité", -400.0, 2000.0, 50.0, degats.style.gravity, func(v: float): degats.style.gravity = v)
	panneau.ajouter_curseur("Dispersion (°)", 0.0, 180.0, 5.0, degats.style.spread_degrees, func(v: float): degats.style.spread_degrees = v)

	panneau.ajouter_section("🪙 Coffre")
	panneau.ajouter_bouton("Ouvrir le coffre", _ouvrir)
	panneau.ajouter_curseur("Nombre de pièces", 1.0, 60.0, 1.0, _nombre_pieces, func(v: float): _nombre_pieces = int(v))
	panneau.ajouter_curseur("Rebond", 0.0, 0.95, 0.05, pieces.style.bounce, func(v: float): pieces.style.bounce = v)
	panneau.ajouter_case("Sol", true, func(actif: bool): pieces.style.use_floor = actif)

	panneau.ajouter_section("⭐ Objets")
	panneau.ajouter_bouton("Pluie d'étoiles", _pluie_etoiles)
	_actifs = panneau.ajouter_etiquette()

	panneau.ajouter_section("🧪 Tests")
	panneau.ajouter_bouton("Relancer les tests automatiques", _lancer_tests)
	_lancer_tests()


func _process(_delta: float) -> void:
	var nombre := 0
	for spawner in [degats, pieces, total, clic, pop_achat]:
		nombre += spawner.get_active_count()
	_actifs.text = "Pops affichés : %d" % nombre


func _unhandled_input(event: InputEvent) -> void:
	var bouton := event as InputEventMouseButton
	if not bouton or not bouton.pressed or bouton.button_index != MOUSE_BUTTON_LEFT:
		return
	var position_souris := get_global_mouse_position()
	if _rectangle(mannequin).has_point(position_souris):
		_frapper(false)
	elif _rectangle(coffre).has_point(position_souris):
		_ouvrir()
	else:
		clic.pop("", null, etoile, Color(0, 0, 0, 0), position_souris)


func _rectangle(noeud: Node2D) -> Rect2:
	var sprite: Sprite2D = noeud.get_node("Corps")
	return Rect2(noeud.global_position - sprite.get_rect().size * sprite.scale / 2.0, sprite.get_rect().size * sprite.scale)


# 🥊 Dégâts

func _frapper(critique: bool) -> void:
	var valeur := randi_range(5, 25)
	if critique:
		degats.pop_text("-%d !" % (valeur * 3), Color(1, 0.85, 0.2))
	else:
		degats.pop_number(-valeur)
	coup.play()


# 🪙 Pièces

func _ouvrir() -> void:
	for i in _nombre_pieces:
		pieces.pop_texture(piece)
	total.pop("x%d" % _nombre_pieces, piece)
	_bourse += _nombre_pieces
	_maj_bourse()


func _acheter() -> void:
	if _bourse < 10:
		pop_achat.pop_text("Pas assez !", Color(1, 0.35, 0.3))
		return
	_bourse -= 10
	pop_achat.pop("-10", piece, null, Color(1, 0.6, 0.4))
	_maj_bourse()


func _maj_bourse() -> void:
	bourse_label.text = "Pièces : %d" % _bourse


# ⭐ Objets

func _pluie_etoiles() -> void:
	for i in 10:
		clic.pop("", null, etoile, Color(0, 0, 0, 0), Vector2(randf_range(60, 660), randf_range(140, 420)))


func _lancer_tests() -> void:
	panneau.lancer_tests("Tests des pops", SuiteTests, self)
