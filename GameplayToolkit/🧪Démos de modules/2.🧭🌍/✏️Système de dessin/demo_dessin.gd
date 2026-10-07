extends Control

## ✏️ Démo du module Système de dessin : un DrawingStudio (atelier complet, inspiré de Clip Studio Paint).
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_dessin.gd) ; l'atelier est masqué
##   pendant les tests (ils simulent la souris, le clavier et le tactile sur leur propre toile).
## - Le panneau ajoute des pinceaux d'exemple : forme texturée (étoile), shaders (arc-en-ciel, néon), et un
##   dessin d'exemple sur plusieurs calques (aplats sous l'encrage, ombre en mode Produit écrêtée).
## - Le journal affiche les signaux de la toile et du document.

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_dessin.gd")
const SHADER_ARC_EN_CIEL := preload("pinceau_arc_en_ciel.gdshader")
const SHADER_NEON := preload("pinceau_neon.gdshader")
const OUTILS := ["pinceau", "gomme", "remplir", "pipette", "rectangle", "ellipse", "lasso", "baguette", "main"]

@onready var panneau: Panneau = $Panneau
@onready var atelier: DrawingStudio = $Atelier

var _toile: DrawingCanvas


func _ready() -> void:
	_toile = atelier.get_canvas()
	_toile.tool_changed.connect(func(outil: int): panneau.journaliser("[color=gray]🛠️ tool_changed : %s[/color]" % OUTILS[outil]))
	_toile.color_picked.connect(func(couleur: Color): panneau.journaliser("💧 color_picked : #%s" % couleur.to_html(false)))
	_toile.stroke_finished.connect(func(): panneau.journaliser("[color=gray]🖌️ stroke_finished (%s)[/color]" % atelier.get_document().get_undo_name()))
	_toile.stroke_rejected.connect(func(): panneau.journaliser("[color=orange]⛔ stroke_rejected : calque masqué ou verrouillé[/color]"))
	atelier.document_saved.connect(func(chemin: String): panneau.journaliser("💾 document_saved : %s" % chemin))
	atelier.document_opened.connect(func(chemin: String): panneau.journaliser("📂 document_opened : %s" % chemin))

	panneau.ajouter_section("🖌️ Pinceaux d'exemple")
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Étoile", _pinceau_etoile, rangee)
	panneau.ajouter_bouton("Arc-en-ciel", _pinceau_shader.bind("Arc-en-ciel (shader)", SHADER_ARC_EN_CIEL), rangee)
	panneau.ajouter_bouton("Néon", _pinceau_shader.bind("Néon (shader)", SHADER_NEON), rangee)
	panneau.ajouter_etiquette("Les pinceaux à shader sont dessinés à travers leur ShaderMaterial pendant le trait, puis rendus par le GPU dans le calque.").modulate.a = 0.7

	panneau.ajouter_section("📄 Document")
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("Dessin d'exemple", _dessin_exemple, rangee)
	panneau.ajouter_bouton("Exporter en PNG", _exporter, rangee)

	panneau.ajouter_section("⌨️ Entrées")
	panneau.ajouter_case("Raccourcis clavier", _toile.shortcuts_enabled, func(actif: bool): _toile.shortcuts_enabled = actif)
	panneau.ajouter_case("Rotation à deux doigts", _toile.touch_rotation, func(actif: bool): _toile.touch_rotation = actif)
	panneau.ajouter_case("Damier de transparence", _toile.checkerboard, func(actif: bool): _toile.checkerboard = actif)
	panneau.ajouter_bouton("Relancer les tests", _lancer_tests)
	_lancer_tests()


func _lancer_tests() -> void:
	atelier.hide()
	await panneau.lancer_tests("Tests du système de dessin", SuiteTests, self)
	atelier.show()


func _ajouter_pinceau(pinceau: DrawingBrush) -> void:
	var pinceaux: Array[DrawingBrush] = []
	pinceaux.assign(atelier.brushes)
	if pinceaux.is_empty():
		for prereglage in [DrawingBrush.PRESET_PEN, DrawingBrush.PRESET_PENCIL, DrawingBrush.PRESET_MARKER, DrawingBrush.PRESET_AIRBRUSH,
				DrawingBrush.PRESET_WATERCOLOR, DrawingBrush.PRESET_INK_VELOCITY]:
			pinceaux.append(DrawingBrush.create_preset(prereglage))
	pinceaux.append(pinceau)
	atelier.brushes = pinceaux
	_toile.brush = pinceau
	_toile.tool = DrawingCanvas.TOOL_BRUSH
	panneau.journaliser("➕ Pinceau « %s » ajouté et sélectionné" % pinceau.resource_name)


## Forme texturée : une étoile à cinq branches dessinée dans une image (opaque sur fond transparent).
func _pinceau_etoile() -> void:
	var taille := 64
	var image := Image.create_empty(taille, taille, false, Image.FORMAT_RGBA8)
	var centre := Vector2(taille, taille) / 2.0
	var points := PackedVector2Array()
	for i in 10:
		var rayon := 30.0 if i % 2 == 0 else 12.0
		points.append(centre + Vector2.from_angle(-PI / 2 + i * PI / 5) * rayon)
	for y in taille:
		for x in taille:
			if Geometry2D.is_point_in_polygon(Vector2(x + 0.5, y + 0.5), points):
				image.set_pixel(x, y, Color.WHITE)
	var pinceau := DrawingBrush.new()
	pinceau.resource_name = "Étoile"
	pinceau.tip_texture = ImageTexture.create_from_image(image)
	pinceau.size = 40.0
	pinceau.spacing = 1.2
	pinceau.follow_direction = true
	pinceau.size_dynamics = DrawingBrush.DYNAMIC_PRESSURE
	pinceau.size_min = 0.4
	pinceau.stabilization = 0
	_ajouter_pinceau(pinceau)


func _pinceau_shader(nom: String, shader: Shader) -> void:
	var materiau := ShaderMaterial.new()
	materiau.shader = shader
	var pinceau := DrawingBrush.new()
	pinceau.resource_name = nom
	pinceau.material = materiau
	pinceau.size = 28.0
	pinceau.hardness = 0.4
	pinceau.size_dynamics = DrawingBrush.DYNAMIC_PRESSURE | DrawingBrush.DYNAMIC_VELOCITY
	pinceau.size_min = 0.35
	pinceau.stabilization = 10
	_ajouter_pinceau(pinceau)


## Dessin sur trois calques : aplats (remplissage d'après l'encrage), encrage, ombre en mode Produit écrêtée aux aplats.
func _dessin_exemple() -> void:
	atelier.new_document(Vector2i(800, 600))
	var doc := atelier.get_document()
	var encre := DrawingBrush.create_preset(DrawingBrush.PRESET_PEN)
	encre.size = 6.0
	var encrage := doc.get_layer(0)
	encrage.layer_name = "Encrage"
	var centre := Vector2(400, 300)
	doc.begin_stroke(encre, Color(0.12, 0.1, 0.15))
	for i in 121:
		var angle := i * TAU / 120.0
		doc.stroke_to(centre + Vector2(cos(angle) * 220, sin(angle) * 160), 0.6 + 0.4 * sin(angle * 3.0))
	doc.end_stroke()
	doc.draw_line(Vector2(176, 300), Vector2(624, 300), encre, Color(0.12, 0.1, 0.15))
	doc.add_layer("Aplats", 0)
	doc.fill(Vector2i(400, 220), Color(0.95, 0.7, 0.35), 0.1, DrawingDocument.FILL_REFERENCE_ALL_LAYERS, 3, 2)
	doc.fill(Vector2i(400, 380), Color(0.45, 0.65, 0.95), 0.1, DrawingDocument.FILL_REFERENCE_ALL_LAYERS, 3, 2)
	var ombre := doc.add_layer("Ombre", 1)
	ombre.blend_mode = DrawingLayer.LAYER_BLEND_MULTIPLY
	ombre.clipped = true
	ombre.opacity = 0.6
	var aerographe := DrawingBrush.create_preset(DrawingBrush.PRESET_AIRBRUSH)
	aerographe.size = 160.0
	aerographe.flow = 0.2
	doc.draw_line(Vector2(560, 160), Vector2(560, 460), aerographe, Color(0.4, 0.3, 0.6))
	doc.set_current_layer(2)
	doc.clear_history()
	panneau.journaliser("🎨 Dessin d'exemple : aplats remplis d'après l'encrage, ombre écrêtée en mode Produit")


func _exporter() -> void:
	var chemin := "user://demo_dessin.png"
	var erreur := atelier.get_document().export_png(chemin)
	panneau.journaliser("🖼️ export_png(%s) : %s" % [ProjectSettings.globalize_path(chemin), error_string(erreur)])
