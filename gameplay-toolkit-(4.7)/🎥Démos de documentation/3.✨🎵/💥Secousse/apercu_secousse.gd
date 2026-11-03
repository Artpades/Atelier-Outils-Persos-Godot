extends Node2D

## 💥 Aperçu interactif de la page Secousse (export web : demos/web/index.html?demo=effets/secousse).
## Une cible au choix (écran de jeu via sa Camera2D, Node2D, Control, caméra 3D), les réglages du FXShake,
## Jouer / Arrêter, la courbe du décalage et le code GDScript correspondant.
## Pas d'emoji dans les textes : le web n'a pas de police de secours pour les afficher (les accents passent).

const Panneau := preload("res://🎥Démos de documentation/socle/panneau_apercu.gd")
const Traceur := preload("res://🎥Démos de documentation/socle/traceur.gd")

const CENTRE := Vector2(356, 305)
const CIBLES := ["Écran de jeu (Camera2D)", "Objet (Node2D)", "Bouton (Control)", "Caméra 3D (Node3D)"]

var _secousse := FXShake.new()
var _joueur := PlayerFX.new()
var _cibles: Array[Node] = []      # nœud secoué par cible
var _affichages: Array[Node] = []  # nœud à montrer / cacher par cible
var _choix := 0
var _amplitude := 12.0 # en pixels ; divisée par 100 pour la caméra 3D (mètres)
var _traceur: Traceur
var _code: Label


func _ready() -> void:
	RenderingServer.set_default_clear_color(Color("1b1f2a"))
	_joueur.special_effects = [_secousse]
	_secousse.duration = 0.5
	_secousse.frequency = 15.0
	_creer_cibles()

	_traceur = Traceur.new()
	_traceur.largeur = 420.0
	_traceur.hauteur = 90.0
	_traceur.duree_visible = 3.0
	_traceur.position = Vector2(CENTRE.x - 210, 530)
	add_child(_traceur)
	var legende := Label.new()
	legende.text = "Décalage horizontal de la cible au cours du temps"
	legende.position = Vector2(CENTRE.x - 210, 580)
	legende.modulate.a = 0.6
	add_child(legende)

	var interface := CanvasLayer.new()
	add_child(interface)
	_code = Label.new()
	_code.position = Vector2(16, 12)
	_code.add_theme_color_override("font_color", Color("cfe3ff"))
	_code.add_theme_font_size_override("font_size", 14)
	interface.add_child(_code)

	var panneau := Panneau.new()
	panneau.titre = "Secousse (FXShake)"
	panneau.aide = "Choisir une cible, régler la secousse puis Jouer. Rejouer pendant une secousse la relance."
	interface.add_child(panneau)
	panneau.ajouter_options("Cible", PackedStringArray(CIBLES), 0, _choisir)
	panneau.ajouter_section("Réglages")
	panneau.ajouter_curseur("duration (s)", 0.0, 3.0, 0.05, _secousse.duration, func(v: float): _secousse.duration = v; _maj_code())
	panneau.ajouter_curseur("frequency (Hz)", 0.5, 60.0, 0.5, _secousse.frequency, func(v: float): _secousse.frequency = v; _maj_code())
	panneau.ajouter_curseur("max_amplitude", 0.0, 50.0, 0.5, _amplitude, func(v: float): _amplitude = v; _appliquer_amplitude())
	panneau.ajouter_case("decrease_over_time", true, func(v: bool): _secousse.decrease_over_time = v; _maj_code())
	panneau.ajouter_etiquette("duration = 0 : secousse sans fin, jusqu'à Arrêter. En 3D, max_amplitude est divisée par 100 (mètres).").modulate.a = 0.7
	panneau.ajouter_section("Commandes")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Jouer  play()", _jouer, rangee)
	panneau.ajouter_bouton("Arrêter  stop()", _arreter, rangee)
	_secousse.fx_stopped.connect(func(): panneau.journaliser("fx_stopped : position d'origine rendue"))
	_secousse.fx_played.connect(func(): panneau.journaliser("fx_played"))
	_choisir(0)


func _creer_cibles() -> void:
	# Écran de jeu : un petit niveau vu par une Camera2D, secouée
	var cadre := SubViewportContainer.new()
	cadre.stretch = true
	cadre.size = Vector2(560, 280)
	cadre.position = CENTRE - cadre.size / 2.0
	add_child(cadre)
	var vue := SubViewport.new()
	cadre.add_child(vue)
	var fond := ColorRect.new()
	fond.color = Color("232a3a")
	fond.size = Vector2(1400, 1000)
	fond.position = -fond.size / 2.0
	vue.add_child(fond)
	for bloc in [Rect2(-400, 100, 800, 300), Rect2(-200, 20, 120, 20), Rect2(60, -30, 160, 20), Rect2(-30, 60, 60, 40)]:
		var forme := ColorRect.new()
		forme.color = Color("3a4256") if bloc.size.x > 700 else Color("56607a")
		forme.position = bloc.position
		forme.size = bloc.size
		vue.add_child(forme)
	var heros := ColorRect.new()
	heros.color = Color("4ea1ff")
	heros.position = Vector2(-150, 60)
	heros.size = Vector2(26, 40)
	vue.add_child(heros)
	var camera_2d := Camera2D.new()
	vue.add_child(camera_2d)
	_ajouter_cible(camera_2d, cadre)

	# Objet 2D
	var objet := Node2D.new()
	objet.position = CENTRE
	var carre := Polygon2D.new()
	carre.polygon = PackedVector2Array([Vector2(-60, -60), Vector2(60, -60), Vector2(60, 60), Vector2(-60, 60)])
	carre.color = Color("f2a33a")
	objet.add_child(carre)
	add_child(objet)
	_ajouter_cible(objet, objet)

	# Bouton
	var bouton := Button.new()
	bouton.text = "Valider"
	bouton.add_theme_font_size_override("font_size", 26)
	bouton.size = Vector2(180, 64)
	bouton.position = CENTRE - bouton.size / 2.0
	bouton.pressed.connect(_jouer)
	add_child(bouton)
	_ajouter_cible(bouton, bouton)

	# Caméra 3D
	var cadre_3d := SubViewportContainer.new()
	cadre_3d.stretch = true
	cadre_3d.size = Vector2(560, 280)
	cadre_3d.position = CENTRE - cadre_3d.size / 2.0
	add_child(cadre_3d)
	var vue_3d := SubViewport.new()
	vue_3d.own_world_3d = true
	cadre_3d.add_child(vue_3d)
	var environnement := WorldEnvironment.new()
	environnement.environment = Environment.new()
	environnement.environment.background_mode = Environment.BG_COLOR
	environnement.environment.background_color = Color("232a3a")
	environnement.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environnement.environment.ambient_light_color = Color(0.55, 0.6, 0.7)
	vue_3d.add_child(environnement)
	var soleil := DirectionalLight3D.new()
	soleil.rotation_degrees = Vector3(-50, 30, 0)
	vue_3d.add_child(soleil)
	for pave in [[Vector3(0, -0.1, 0), Vector3(12, 0.2, 10), Color("3a4256")], [Vector3(0, 0.7, 0), Vector3(1.4, 1.4, 1.4), Color("c678dd")],
			[Vector3(-2.5, 0.4, -1.5), Vector3(0.8, 0.8, 0.8), Color("56607a")], [Vector3(2.6, 0.4, -1.2), Vector3(0.8, 0.8, 0.8), Color("56607a")]]:
		var maillage := BoxMesh.new()
		maillage.size = pave[1]
		var matiere := StandardMaterial3D.new()
		matiere.albedo_color = pave[2]
		maillage.material = matiere
		var instance := MeshInstance3D.new()
		instance.mesh = maillage
		instance.position = pave[0]
		vue_3d.add_child(instance)
	var camera_3d := Camera3D.new()
	camera_3d.position = Vector3(0, 2.2, 5.5)
	camera_3d.rotation_degrees.x = -18
	vue_3d.add_child(camera_3d)
	_ajouter_cible(camera_3d, cadre_3d)


func _ajouter_cible(cible: Node, affichage: Node) -> void:
	_cibles.append(cible)
	_affichages.append(affichage)


func _choisir(index: int) -> void:
	if _secousse.is_activated():
		_joueur.stop()
	_choix = index
	for i in _affichages.size():
		_affichages[i].visible = i == index
	if _joueur.get_parent():
		_joueur.get_parent().remove_child(_joueur)
	_cibles[index].add_child(_joueur)

	var cible := _cibles[index]
	var origine = cible.position
	var est_3d := cible is Node3D
	_traceur.echelle = 120.0 if est_3d else 1.5
	_traceur.mesure = func() -> float: return (cible.position.x - origine.x)
	_traceur.vider()
	_appliquer_amplitude()


func _appliquer_amplitude() -> void:
	_secousse.max_amplitude = _amplitude / 100.0 if _cibles[_choix] is Node3D else _amplitude
	_maj_code()


func _jouer() -> void:
	_joueur.play()


func _arreter() -> void:
	_joueur.stop()


func _maj_code() -> void:
	var chemin: String = ["$Camera2D", "$Objet", "$Bouton", "$Camera3D"][_choix]
	_code.text = "var shake: FXShake = %s/PlayerFX.special_effects[0]\nshake.duration = %s\nshake.frequency = %s\nshake.max_amplitude = %s\nshake.decrease_over_time = %s\n%s/PlayerFX.play()" % [
		chemin, snappedf(_secousse.duration, 0.01), _secousse.frequency, snappedf(_secousse.max_amplitude, 0.001),
		_secousse.decrease_over_time, chemin]
