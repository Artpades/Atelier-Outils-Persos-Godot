extends Node3D

## 🖥️ Démo du module HUD spatial interactif : trois InteractiveHUD3D dans une scène 3D.
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_hud.gd), pendant laquelle les
##   HUD de la démo sont suspendus (les tests simulent la souris et ont leur propre caméra).
## - Panneau de contrôle (incliné) : bouton, curseur, interrupteur et champ de texte pilotent le cube.
## - Bulle au-dessus du cube (billboard Y) : nom, vie, boutons Frapper / Soigner.
## - Écran mural (simple face, occulté par le pilier) : liste défilante à la molette.
## Clic droit glissé : tourner la caméra ; molette hors des HUD : zoom. En mode « centre de l'écran », la souris
## est capturée et vise au viseur (Échap pour revenir à la souris).

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_hud.gd")
const COULEURS: Array[Color] = [Color(0.95, 0.45, 0.3), Color(0.35, 0.6, 1.0), Color(0.4, 0.85, 0.45), Color(0.95, 0.8, 0.3), Color(0.75, 0.45, 0.95)]
const CIBLE_CAMERA := Vector3(-0.2, 1.2, -0.4)

@onready var panneau: Panneau = $Interface/Panneau
@onready var viseur: Label = $Interface/Viseur
@onready var camera: Camera3D = $Camera
@onready var cube: MeshInstance3D = $Cube
@onready var huds: Node3D = $HUDs
@onready var hud_panneau: InteractiveHUD3D = $HUDs/PanneauControle
@onready var hud_bulle: InteractiveHUD3D = $HUDs/Bulle
@onready var hud_ecran: InteractiveHUD3D = $HUDs/Ecran

var _vitesse := 1.0
var _rotation_active := true
var _couleur := 0
var _clics := 0
var _lacet := 0.0
var _tangage := -0.3
var _distance := 6.4
var _orbite := false
var _etat: Label
var _source: OptionButton


func _ready() -> void:
	_brancher_contenus()
	for hud: InteractiveHUD3D in [hud_panneau, hud_bulle, hud_ecran]:
		hud.pointer_entered.connect(func(): panneau.journaliser("[color=gray]👆 pointer_entered : %s[/color]" % hud.name))
		hud.pointer_exited.connect(func(): panneau.journaliser("[color=gray]👋 pointer_exited : %s[/color]" % hud.name))
		hud.pointer_pressed.connect(func(position: Vector2, _bouton: int):
			panneau.journaliser("🖱️ pointer_pressed : %s (%d, %d)" % [hud.name, position.x, position.y]))

	panneau.ajouter_section("🖱️ Pointeur")
	_source = panneau.ajouter_options("Source", ["Souris", "Centre de l'écran (viseur)"], 0, _changer_source)
	panneau.ajouter_curseur("Distance max (m)", 0.0, 15.0, 0.5, 0.0, func(valeur: float):
		for hud: InteractiveHUD3D in [hud_panneau, hud_bulle, hud_ecran]:
			hud.max_distance = valeur)
	panneau.ajouter_section("🎨 Affichage")
	panneau.ajouter_options("Billboard de la bulle", ["Désactivé", "Face caméra", "Axe Y"], hud_bulle.billboard,
		func(index: int): hud_bulle.billboard = index as InteractiveHUD3D.BillboardMode)
	panneau.ajouter_case("Bulle toujours au-dessus (always_on_top)", hud_bulle.always_on_top, func(actif: bool): hud_bulle.always_on_top = actif)
	panneau.ajouter_case("Écran double face", hud_ecran.double_sided, func(actif: bool): hud_ecran.double_sided = actif)
	panneau.ajouter_case("Écran occulté par le pilier (check_occlusion)", hud_ecran.check_occlusion, func(actif: bool): hud_ecran.check_occlusion = actif)
	panneau.ajouter_bouton("Relancer les tests", _lancer_tests)
	_etat = panneau.ajouter_etiquette()

	_placer_camera()
	_lancer_tests()


func _process(delta: float) -> void:
	if _rotation_active:
		cube.rotate_y(_vitesse * delta)
	var survoles := PackedStringArray()
	for hud: InteractiveHUD3D in [hud_panneau, hud_bulle, hud_ecran]:
		if hud.is_hovered() or hud.is_captured():
			var position := hud.get_pointer_position()
			survoles.append("%s (%d, %d)%s" % [hud.name, position.x, position.y, " ✊" if hud.is_captured() else ""])
	_etat.text = "Survolé : %s" % (", ".join(survoles) if survoles else "—")


# 🧩 Contenus des HUD

func _brancher_contenus() -> void:
	var controle := hud_panneau.get_content()
	controle.get_node("%Couleur").pressed.connect(func():
		_couleur = (_couleur + 1) % COULEURS.size()
		(cube.material_override as StandardMaterial3D).albedo_color = COULEURS[_couleur]
		_noter("🎨 Nouvelle couleur"))
	controle.get_node("%Vitesse").value_changed.connect(func(valeur: float): _vitesse = valeur)
	controle.get_node("%Vitesse").drag_ended.connect(func(_change: bool): _noter("🌀 Vitesse : %.1f" % _vitesse))
	controle.get_node("%Rotation").toggled.connect(func(actif: bool):
		_rotation_active = actif
		_noter("▶️ Rotation" if actif else "⏸️ Pause"))
	controle.get_node("%Nom").text_changed.connect(func(texte: String):
		hud_bulle.get_content().get_node("%Nom").text = texte if texte else "Cube")
	hud_panneau.pointer_pressed.connect(func(_position: Vector2, _bouton: int):
		_clics += 1
		controle.get_node("%Clics").text = "Clics sur ce panneau : %d" % _clics)

	var bulle := hud_bulle.get_content()
	bulle.get_node("%Frapper").pressed.connect(func(): _modifier_vie(-15))
	bulle.get_node("%Soigner").pressed.connect(func(): _modifier_vie(10))

	hud_ecran.get_content().get_node("%Vider").pressed.connect(func(): hud_ecran.get_content().get_node("%Liste").clear())
	for i in 12:
		_noter("Démarrage n°%d — molette : défiler" % (i + 1))


func _modifier_vie(quantite: float) -> void:
	var vie: ProgressBar = hud_bulle.get_content().get_node("%Vie")
	vie.value += quantite
	_noter("%s Vie : %d" % ["⚔️" if quantite < 0 else "💚", vie.value])


## Ajoute une ligne au journal de l'écran mural et la rend visible.
func _noter(texte: String) -> void:
	var liste: ItemList = hud_ecran.get_content().get_node("%Liste")
	liste.add_item(texte)
	liste.select(liste.item_count - 1)
	liste.ensure_current_is_visible()


# 🎥 Caméra et pointeur

func _changer_source(index: int) -> void:
	var source := InteractiveHUD3D.POINTER_SOURCE_SCREEN_CENTER if index == 1 else InteractiveHUD3D.POINTER_SOURCE_MOUSE
	for hud: InteractiveHUD3D in [hud_panneau, hud_bulle, hud_ecran]:
		hud.pointer_source = source
	viseur.visible = index == 1
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if index == 1 else Input.MOUSE_MODE_VISIBLE
	if _source.selected != index:
		_source.select(index)


func _input(evenement: InputEvent) -> void:
	# Avant les HUD : en mode viseur, la souris tourne la caméra même en visant un HUD.
	var mouvement := evenement as InputEventMouseMotion
	if mouvement and (_orbite or Input.mouse_mode == Input.MOUSE_MODE_CAPTURED):
		_lacet -= mouvement.relative.x * 0.005
		_tangage = clampf(_tangage - mouvement.relative.y * 0.005, -1.4, 0.3)
		_placer_camera()
	var bouton := evenement as InputEventMouseButton
	if bouton and bouton.button_index == MOUSE_BUTTON_RIGHT and not bouton.pressed:
		_orbite = false
	if evenement.is_action_pressed("ui_cancel") and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		_changer_source(0)


func _unhandled_input(evenement: InputEvent) -> void:
	# Après les HUD : un clic droit ou la molette au-dessus d'un HUD lui reviennent.
	var bouton := evenement as InputEventMouseButton
	if bouton == null or not bouton.pressed:
		return
	match bouton.button_index:
		MOUSE_BUTTON_RIGHT:
			_orbite = true
		MOUSE_BUTTON_WHEEL_UP:
			_distance = maxf(_distance - 0.4, 2.5)
			_placer_camera()
		MOUSE_BUTTON_WHEEL_DOWN:
			_distance = minf(_distance + 0.4, 14.0)
			_placer_camera()


func _placer_camera() -> void:
	camera.position = CIBLE_CAMERA + Basis.from_euler(Vector3(_tangage, _lacet, 0)) * Vector3(0, 0, _distance)
	camera.look_at(CIBLE_CAMERA)


func _lancer_tests() -> void:
	_changer_source(0)
	# Les tests simulent la souris : les HUD de la démo ne doivent pas y réagir.
	huds.process_mode = Node.PROCESS_MODE_DISABLED
	await panneau.lancer_tests("Tests du module HUD spatial", SuiteTests, self)
	huds.process_mode = Node.PROCESS_MODE_INHERIT
	camera.make_current() # les tests ont rendu leur propre caméra courante
