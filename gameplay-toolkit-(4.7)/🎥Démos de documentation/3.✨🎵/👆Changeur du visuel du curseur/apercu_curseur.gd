extends "res://🎥Démos de documentation/socle/apercu.gd"

## 👆 Aperçu interactif du Changeur de curseur : curseur de base au choix, zone survolable (main, fermée au
## clic), sablier manuel, vitesse d'animation et hotspot. La souris est réelle dans le navigateur.

const Curseurs := preload("res://🎥Démos de documentation/socle/curseurs.gd")

var _base := CursorChanger.new()
var _main := CursorChanger.new()
var _sablier := CursorChanger.new()
var _styles_base: Array[CursorStyle] = []
var _etat: Label


func titre() -> String:
	return "Changeur de curseur (CursorChanger)"


func aide() -> String:
	return "Bouger la souris sur la zone de jeu, survoler le coffre et cliquer dessus."


func _style(images: Array, hotspot: Vector2, fps: float = 10.0) -> CursorStyle:
	var style := CursorStyle.new()
	style.frames = images
	style.hotspot = hotspot
	style.frames_per_second = fps
	return style


func construire() -> void:
	_styles_base = [_style([Curseurs.epee()], Vector2(2, 2)), _style([Curseurs.viseur()], Vector2(16, 16)), CursorStyle.new()]
	_base.style = _styles_base[0]
	add_child(_base)

	var coffre := Area2D.new()
	coffre.position = Vector2(CENTRE.x, 300)
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(220, 140)
	coffre.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(220, 140))
	dessin.color = Color("8a5a3c")
	coffre.add_child(dessin)
	add_child(coffre)
	etiquette(Vector2(CENTRE.x - 120, 380), "Coffre (Area2D) : CursorChanger en mode Hover")
	get_viewport().physics_object_picking = true
	_main.activation_mode = CursorChanger.ACTIVATION_HOVER
	_main.style = _style([Curseurs.main_ouverte()], Vector2(12, 2))
	_main.pressed_style = _style([Curseurs.main_fermee()], Vector2(12, 8))
	coffre.add_child(_main)

	_sablier.activation_mode = CursorChanger.ACTIVATION_MANUAL
	_sablier.style = _style([Curseurs.sablier(0), Curseurs.sablier(1), Curseurs.sablier(2), Curseurs.sablier(3)], Vector2(10, 14), 6.0)
	add_child(_sablier)
	_etat = etiquette(Vector2(30, 600), "", 15, Formes.ACCENT)

	panneau.ajouter_options("Curseur de base (Always)", PackedStringArray(["Épée", "Viseur", "Aucun style (système)"]), 0, func(i: int):
		_base.style = _styles_base[i]
		_base.activate()
		_maj_code())
	panneau.ajouter_case("Coffre : pressed_style (main fermée)", true, func(v: bool): _main.pressed_style = _style([Curseurs.main_fermee()], Vector2(12, 8)) if v else null; _maj_code())
	panneau.ajouter_section("Sablier (Manual)")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("activate()", func(): _sablier.activate(), rangee)
	panneau.ajouter_bouton("deactivate()", func(): _sablier.deactivate(), rangee)
	panneau.ajouter_curseur("frames_per_second", 0.0, 24.0, 1.0, 6.0, func(v: float): _sablier.style.frames_per_second = v; _maj_code())
	_maj_code()


func _process(_delta: float) -> void:
	var lignes := PackedStringArray()
	for paire in [["Base", _base], ["Main (coffre)", _main], ["Sablier", _sablier]]:
		lignes.append("%s : is_active() = %s, is_current() = %s" % [paire[0], paire[1].is_active(), paire[1].is_current()])
	_etat.text = "\n".join(lignes)
	_etat.position.y = 648 - 80


func _maj_code() -> void:
	afficher_code("var style := CursorStyle.new()\nstyle.frames = [preload(\"res://curseurs/epee.png\")]\nstyle.hotspot = Vector2(2, 2)\n$CursorChanger.style = style   # mode Always\n$Coffre/CursorChanger.activation_mode = CursorChanger.ACTIVATION_HOVER\n$Sablier.activate()   # mode Manual, %s images/s" % _sablier.style.frames_per_second)
