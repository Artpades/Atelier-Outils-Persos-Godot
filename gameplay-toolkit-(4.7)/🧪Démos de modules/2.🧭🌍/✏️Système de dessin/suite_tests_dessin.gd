extends "../suite_tests_base.gd"

## Suite de tests automatiques du module Système de dessin (DrawingBrush, DrawingLayer, DrawingDocument,
## DrawingCanvas, DrawingStudio).
##
## Le moteur (DrawingDocument) est testé directement ; la toile reçoit des évènements simulés (push_input en
## coordonnées du Viewport, toile placée en haut à gauche : le panneau des démos intercepte les clics à droite).
## Les touches de modification (Espace, Alt, Maj) passent par Input.parse_input_event, que la toile consulte.
## Pinceau à shader : en headless (pas de rendu), appliqué sans son shader ; avec rendu (démo), rendu par le GPU.
## Erreurs attendues dans la console : suppression du dernier calque, commit_stroke_image sans trait en cours,
## ouverture d'un fichier absent par l'atelier.

const TAILLE_TOILE := Vector2(600, 500)
const NOIR := Color(0, 0, 0, 1)
const ROUGE := Color(1, 0, 0, 1)
const BLEU := Color(0, 0, 1, 1)
const BLANC := Color(1, 1, 1, 1)


func _executer() -> void:
	_tester_pinceau()
	_tester_calque()
	_tester_document_traits()
	_tester_dynamique()
	_tester_modes_pinceau()
	_tester_remplissage()
	_tester_selection()
	_tester_calques()
	_tester_composition()
	_tester_historique()
	_tester_trait_differe()
	await _tester_sauvegarde()
	await _tester_toile_dessin()
	await _tester_toile_outils()
	await _tester_toile_navigation()
	await _tester_toile_clavier()
	await _tester_toile_tactile()
	await _tester_atelier()


# 🛠️ Utilitaires

## Document blanc (fond) de p_taille avec un calque vide.
func _document(taille := Vector2i(100, 100)) -> DrawingDocument:
	return DrawingDocument.create(taille)


## Pinceau rond net, sans dynamique ni stabilisation.
func _pinceau(taille := 6.0, opacite := 1.0) -> DrawingBrush:
	var pinceau := DrawingBrush.new()
	pinceau.size = taille
	pinceau.opacity = opacite
	pinceau.hardness = 1.0
	pinceau.size_dynamics = 0
	pinceau.stabilization = 0
	return pinceau


func _alpha(calque: DrawingLayer, x: int, y: int) -> float:
	return calque.get_pixel(x, y).a


## Nombre de pixels d'une colonne dont l'alpha dépasse 0,5 (épaisseur d'un trait horizontal).
func _epaisseur(calque: DrawingLayer, x: int) -> int:
	var n := 0
	for y in calque.get_size().y:
		if calque.get_pixel(x, y).a > 0.5:
			n += 1
	return n


func _proche(a: Color, b: Color, marge := 0.02) -> bool:
	return absf(a.r - b.r) <= marge and absf(a.g - b.g) <= marge and absf(a.b - b.b) <= marge and absf(a.a - b.a) <= marge


## Carré creux de traits noirs (x0, y0) - (x1, y1), avec une ouverture de p_trou pixels au milieu du côté gauche.
func _carre(doc: DrawingDocument, x0: int, y0: int, x1: int, y1: int, trou := 0) -> void:
	var pinceau := _pinceau(3.0)
	doc.draw_line(Vector2(x0, y0), Vector2(x1, y0), pinceau, NOIR)
	doc.draw_line(Vector2(x1, y0), Vector2(x1, y1), pinceau, NOIR)
	doc.draw_line(Vector2(x1, y1), Vector2(x0, y1), pinceau, NOIR)
	if trou <= 0:
		doc.draw_line(Vector2(x0, y1), Vector2(x0, y0), pinceau, NOIR)
	else:
		var milieu := (y0 + y1) / 2.0
		doc.draw_line(Vector2(x0, y1), Vector2(x0, milieu + trou / 2.0 + 1.5), pinceau, NOIR)
		doc.draw_line(Vector2(x0, milieu - trou / 2.0 - 1.5), Vector2(x0, y0), pinceau, NOIR)


func _toile() -> DrawingCanvas:
	var toile := DrawingCanvas.new()
	toile.position = Vector2.ZERO
	toile.size = TAILLE_TOILE
	toile.document = DrawingDocument.create(Vector2i(300, 250), Color(1, 1, 1, 0))
	toile.brush = _pinceau(8.0)
	toile.eraser = DrawingBrush.create_preset(DrawingBrush.PRESET_HARD_ERASER)
	_ajouter(toile)
	toile.fit_to_view() # 2 px d'écran par pixel du dessin
	return toile


func _bouton(toile: DrawingCanvas, point: Vector2, appuye: bool, bouton := MOUSE_BUTTON_LEFT) -> void:
	var evenement := InputEventMouseButton.new()
	evenement.button_index = bouton
	evenement.pressed = appuye
	evenement.position = toile.canvas_to_screen(point)
	evenement.global_position = evenement.position
	if appuye and bouton == MOUSE_BUTTON_LEFT:
		evenement.button_mask = MOUSE_BUTTON_MASK_LEFT
	toile.get_viewport().push_input(evenement, true)


func _mouvement(toile: DrawingCanvas, point: Vector2, pression := 1.0, vitesse := Vector2.ZERO, gomme := false) -> void:
	var evenement := InputEventMouseMotion.new()
	evenement.position = toile.canvas_to_screen(point)
	evenement.global_position = evenement.position
	evenement.pressure = pression
	evenement.velocity = vitesse
	evenement.pen_inverted = gomme
	evenement.button_mask = MOUSE_BUTTON_MASK_LEFT
	toile.get_viewport().push_input(evenement, true)


## Glisser de a à b (points du dessin) en p_pas mouvements.
func _glisser(toile: DrawingCanvas, a: Vector2, b: Vector2, pas := 10, pression := 1.0) -> void:
	_bouton(toile, a, true)
	for i in range(1, pas + 1):
		_mouvement(toile, a.lerp(b, float(i) / pas), pression)
	_bouton(toile, b, false)


func _molette(toile: DrawingCanvas, ecran: Vector2, haut: bool) -> void:
	var evenement := InputEventMouseButton.new()
	evenement.button_index = MOUSE_BUTTON_WHEEL_UP if haut else MOUSE_BUTTON_WHEEL_DOWN
	evenement.pressed = true
	evenement.factor = 1.0
	evenement.position = ecran
	evenement.global_position = ecran
	toile.get_viewport().push_input(evenement, true)


## Touche maintenue ou relâchée dans l'état d'Input (lu par la toile au moment du clic).
func _modificateur(touche: Key, appuye: bool) -> void:
	var evenement := InputEventKey.new()
	evenement.keycode = touche
	evenement.physical_keycode = touche
	evenement.pressed = appuye
	Input.parse_input_event(evenement)
	Input.flush_buffered_events()


## Touche envoyée au Control qui a le focus.
func _touche(toile: DrawingCanvas, touche: Key, ctrl := false, maj := false) -> void:
	for appuye in [true, false]:
		var evenement := InputEventKey.new()
		evenement.keycode = touche
		evenement.physical_keycode = touche
		evenement.pressed = appuye
		evenement.ctrl_pressed = ctrl
		evenement.shift_pressed = maj
		toile.get_viewport().push_input(evenement, true)


func _toucher(toile: DrawingCanvas, index: int, point: Vector2, appuye: bool) -> void:
	var evenement := InputEventScreenTouch.new()
	evenement.index = index
	evenement.position = toile.canvas_to_screen(point)
	evenement.pressed = appuye
	toile.get_viewport().push_input(evenement, true)


func _glisser_doigt(toile: DrawingCanvas, index: int, ecran: Vector2) -> void:
	var evenement := InputEventScreenDrag.new()
	evenement.index = index
	evenement.position = ecran
	evenement.pressure = 1.0
	toile.get_viewport().push_input(evenement, true)


# 🖌️ Pinceau

func _tester_pinceau() -> void:
	var pinceau := DrawingBrush.new()
	_verifier("Pinceau par défaut : 12 px, opaque, taille à la pression (minimum 20 %), stabilisation 6",
		is_equal_approx(pinceau.size, 12.0) and pinceau.opacity == 1.0 and pinceau.size_dynamics == DrawingBrush.DYNAMIC_PRESSURE
		and is_equal_approx(pinceau.size_min, 0.2) and pinceau.opacity_dynamics == 0 and pinceau.stabilization == 6)
	pinceau.size = 0.0
	pinceau.flow = 0.0
	pinceau.stabilization = 500
	_verifier("Bornes : taille ≥ 0,5 px, densité ≥ 0,01, stabilisation ≤ 100",
		pinceau.size == 0.5 and is_equal_approx(pinceau.flow, 0.01) and pinceau.stabilization == 100)

	pinceau = _pinceau(10.0)
	pinceau.size_dynamics = DrawingBrush.DYNAMIC_PRESSURE
	pinceau.size_min = 0.2
	_verifier("Taille à la pression : 20 % sans pression, 60 % à mi-pression, 100 % à pleine pression",
		is_equal_approx(pinceau.get_size_factor(0.0, 0.0), 0.2) and is_equal_approx(pinceau.get_size_factor(0.5, 0.0), 0.6)
		and is_equal_approx(pinceau.get_dab_size(1.0, 0.0), 10.0))
	pinceau.size_dynamics = DrawingBrush.DYNAMIC_VELOCITY
	pinceau.velocity_max = 1000.0
	_verifier("Taille à la vitesse : pleine à l'arrêt, minimale à velocity_max (le trait s'affine)",
		is_equal_approx(pinceau.get_size_factor(0.3, 0.0), 1.0) and is_equal_approx(pinceau.get_size_factor(1.0, 1000.0), 0.2)
		and is_equal_approx(pinceau.get_size_factor(1.0, 5000.0), 0.2))
	pinceau.size_dynamics = DrawingBrush.DYNAMIC_PRESSURE | DrawingBrush.DYNAMIC_VELOCITY
	_verifier("Pression et vitesse combinées (produit des entrées)", is_equal_approx(pinceau.get_size_factor(0.5, 500.0), lerpf(0.2, 1.0, 0.25)))
	pinceau.opacity_dynamics = DrawingBrush.DYNAMIC_PRESSURE
	pinceau.opacity_min = 0.1
	pinceau.opacity = 0.8
	_verifier("Opacité à la pression : opacity × facteur (minimum opacity_min)",
		is_equal_approx(pinceau.get_dab_opacity(0.0, 0.0), 0.08) and is_equal_approx(pinceau.get_dab_opacity(1.0, 0.0), 0.8))
	var courbe := Curve.new()
	courbe.add_point(Vector2(0, 0.5))
	courbe.add_point(Vector2(1, 0.5))
	pinceau.pressure_curve = courbe
	_verifier("pressure_curve : remplace la réponse linéaire", is_equal_approx(pinceau.get_opacity_factor(0.0, 0.0), lerpf(0.1, 1.0, 0.5)))
	pinceau.size_dynamics = 0
	_verifier("Sans dynamique : facteur 1 quelles que soient pression et vitesse", pinceau.get_size_factor(0.0, 99999.0) == 1.0)

	var prereglages := true
	for p in [DrawingBrush.PRESET_PEN, DrawingBrush.PRESET_PENCIL, DrawingBrush.PRESET_MARKER, DrawingBrush.PRESET_AIRBRUSH,
			DrawingBrush.PRESET_WATERCOLOR, DrawingBrush.PRESET_INK_VELOCITY, DrawingBrush.PRESET_HARD_ERASER, DrawingBrush.PRESET_SOFT_ERASER]:
		var b := DrawingBrush.create_preset(p)
		prereglages = prereglages and b != null and not b.resource_name.is_empty()
	_verifier("Préréglages : tous créés et nommés", prereglages)
	_verifier("Préréglages : gommes en mode effacer, crayon avec grain",
		DrawingBrush.create_preset(DrawingBrush.PRESET_HARD_ERASER).blend_mode == DrawingBrush.BRUSH_BLEND_ERASE
		and DrawingBrush.create_preset(DrawingBrush.PRESET_PENCIL).texture != null)
	var grain := DrawingBrush.create_grain_texture(32, 3, 1.5)
	_verifier("create_grain_texture : texture carrée de la taille demandée", grain != null and grain.get_size() == Vector2(32, 32))

	# Forme texturée : une croix (opaque sur fond transparent) ; les coins du tampon restent vides.
	var croix := Image.create_empty(16, 16, false, Image.FORMAT_RGBA8)
	for i in 16:
		for j in range(6, 10):
			croix.set_pixel(i, j, Color.WHITE)
			croix.set_pixel(j, i, Color.WHITE)
	var forme := _pinceau(32.0)
	forme.tip_texture = ImageTexture.create_from_image(croix)
	var doc := _document(Vector2i(64, 64))
	doc.begin_stroke(forme, NOIR)
	doc.stroke_to(Vector2(32, 32))
	doc.end_stroke()
	var calque := doc.get_layer(0)
	_verifier("tip_texture : la forme du tampon suit la texture (centre et branches peints, coins vides)",
		_alpha(calque, 32, 32) > 0.9 and _alpha(calque, 32, 19) > 0.9 and _alpha(calque, 19, 32) > 0.9 and _alpha(calque, 20, 20) < 0.05,
		"centre %.2f branche %.2f coin %.2f" % [_alpha(calque, 32, 32), _alpha(calque, 32, 19), _alpha(calque, 20, 20)])


# 🗂️ Calque

func _tester_calque() -> void:
	var calque := DrawingLayer.create(Vector2i(20, 10), "Fond")
	_verifier("DrawingLayer.create : taille, nom, vide, réglages par défaut",
		calque.get_size() == Vector2i(20, 10) and calque.layer_name == "Fond" and calque.is_empty() and calque.visible
		and calque.opacity == 1.0 and calque.blend_mode == DrawingLayer.LAYER_BLEND_NORMAL and not calque.alpha_locked and not calque.clipped and not calque.locked)
	var image := Image.create_empty(4, 3, false, Image.FORMAT_RGB8)
	image.fill(Color(0.2, 0.4, 0.6))
	calque.set_image(image)
	_verifier("set_image : converti en RGBA8, la taille suit l'image", calque.get_size() == Vector2i(4, 3) and calque.get_image().get_format() == Image.FORMAT_RGBA8
		and _proche(calque.get_pixel(1, 1), Color(0.2, 0.4, 0.6)) and not calque.is_empty())
	var png := calque.get_png_data()
	var copie := DrawingLayer.new()
	copie.set_png_data(png)
	_verifier("png_data : aller-retour des pixels (forme sauvegardée)", copie.get_size() == Vector2i(4, 3) and _proche(copie.get_pixel(3, 2), Color(0.2, 0.4, 0.6)))
	_verifier("png_data : réutilisé tant que les pixels ne changent pas", calque.get_png_data() == png)


# 📄 Document : traits

func _tester_document_traits() -> void:
	var doc := _document()
	_verifier("DrawingDocument.create : un calque vide, fond blanc, historique vide",
		doc.get_layer_count() == 1 and doc.get_layer(0).is_empty() and doc.background_color == BLANC and not doc.can_undo() and doc.get_current_layer() == 0)
	var image := Image.create_empty(8, 6, false, Image.FORMAT_RGBA8)
	image.fill(ROUGE)
	var depuis_image := DrawingDocument.create_from_image(image)
	_verifier("create_from_image : taille de l'image, un calque qui la contient, fond transparent",
		depuis_image.get_size() == Vector2i(8, 6) and depuis_image.get_layer_count() == 1 and depuis_image.get_layer(0).get_pixel(7, 5) == ROUGE
		and depuis_image.background_color.a == 0.0)

	var zones := _espionner(doc.region_changed)
	var calque := doc.get_layer(0)
	doc.draw_line(Vector2(10, 50), Vector2(90, 50), _pinceau(6.0), BLEU)
	_verifier("draw_line : le trait est peint, rien autour", _proche(calque.get_pixel(50, 50), BLEU) and _alpha(calque, 50, 40) == 0.0 and _alpha(calque, 5, 50) == 0.0)
	_verifier("Épaisseur du trait ≈ taille du pinceau (6 px)", absi(_epaisseur(calque, 50) - 6) <= 1, str(_epaisseur(calque, 50)))
	_verifier("region_changed émis pour la zone du trait", zones.n >= 1 and (zones.args[0] as Rect2i).has_point(Vector2i(50, 50)))
	_verifier("Historique : une étape nommée comme le pinceau (« Trait » sans nom)", doc.get_undo_count() == 1 and doc.get_undo_name() == "Trait")
	doc.undo()
	_verifier("undo : le calque redevient vide", calque.is_empty() and doc.can_redo())
	doc.redo()
	_verifier("redo : le trait revient", _proche(calque.get_pixel(50, 50), BLEU))

	# Plafond d'opacité : dans un même trait, les tampons ne dépassent pas l'opacité ; un second trait s'y ajoute.
	doc = _document()
	calque = doc.get_layer(0)
	var demi := _pinceau(8.0, 0.5)
	doc.begin_stroke(demi, NOIR)
	for x in [10, 90, 10, 90]:
		doc.stroke_to(Vector2(x, 50))
	doc.end_stroke()
	_verifier("Un trait repassé sur lui-même garde son opacité (0,5)", absf(_alpha(calque, 50, 50) - 0.5) < 0.02, str(_alpha(calque, 50, 50)))
	doc.draw_line(Vector2(10, 50), Vector2(90, 50), demi, NOIR)
	_verifier("Un second trait se superpose (0,75)", absf(_alpha(calque, 50, 50) - 0.75) < 0.02, str(_alpha(calque, 50, 50)))

	# Densité faible : accumulation progressive (aérographe).
	doc = _document()
	calque = doc.get_layer(0)
	var aero := _pinceau(10.0)
	aero.flow = 0.05
	aero.spacing = 0.1
	doc.draw_line(Vector2(10, 30), Vector2(12, 30), aero, NOIR)
	var court := _alpha(calque, 11, 30)
	doc.draw_line(Vector2(10, 70), Vector2(40, 70), aero, NOIR)
	var long := _alpha(calque, 25, 70)
	_verifier("Densité (flow) faible : la couleur s'accumule au fil des tampons, sans dépasser l'opacité",
		court < 0.6 and long > court and long <= 1.0, "court %.2f long %.2f" % [court, long])

	# Pression au fil du trait (interpolée entre deux points).
	doc = _document()
	calque = doc.get_layer(0)
	var plume := _pinceau(20.0)
	plume.size_dynamics = DrawingBrush.DYNAMIC_PRESSURE
	plume.size_min = 0.1
	doc.begin_stroke(plume, NOIR)
	doc.stroke_to(Vector2(10, 50), 0.1)
	doc.stroke_to(Vector2(90, 50), 1.0)
	doc.end_stroke()
	_verifier("Pression interpolée le long du segment : le trait s'épaissit", _epaisseur(calque, 20) < _epaisseur(calque, 85),
		"%d puis %d" % [_epaisseur(calque, 20), _epaisseur(calque, 85)])

	# Trait annulé : rien dans le calque ni dans l'historique.
	doc = _document()
	doc.begin_stroke(_pinceau(), NOIR)
	doc.stroke_to(Vector2(10, 10))
	doc.stroke_to(Vector2(80, 80))
	var pendant := _alpha(doc.get_layer(0), 45, 45)
	doc.cancel_stroke()
	_verifier("cancel_stroke : visible pendant le trait, puis effacé, sans étape d'historique",
		pendant > 0.9 and doc.get_layer(0).is_empty() and not doc.can_undo() and not doc.is_stroking())


func _tester_dynamique() -> void:
	var doc := _document()
	var calque := doc.get_layer(0)
	var encre := _pinceau(16.0)
	encre.size_dynamics = DrawingBrush.DYNAMIC_VELOCITY
	encre.size_min = 0.1
	encre.velocity_max = 1000.0
	doc.begin_stroke(encre, NOIR)
	doc.stroke_to(Vector2(10, 30), 1.0, 0.0)
	doc.stroke_to(Vector2(90, 30), 1.0, 0.0)
	doc.end_stroke()
	doc.begin_stroke(encre, NOIR)
	doc.stroke_to(Vector2(10, 70), 1.0, 1000.0)
	doc.stroke_to(Vector2(90, 70), 1.0, 1000.0)
	doc.end_stroke()
	var lent := 0
	var rapide := 0
	for y in range(15, 46):
		lent += 1 if _alpha(calque, 50, y) > 0.5 else 0
	for y in range(55, 86):
		rapide += 1 if _alpha(calque, 50, y) > 0.5 else 0
	_verifier("Taille à la vitesse : trait lent épais, trait rapide fin", lent >= 14 and rapide <= 3, "lent %d rapide %d" % [lent, rapide])

	var lavis := _pinceau(12.0)
	lavis.opacity_dynamics = DrawingBrush.DYNAMIC_PRESSURE
	doc = _document()
	doc.begin_stroke(lavis, NOIR)
	doc.stroke_to(Vector2(20, 50), 0.3)
	doc.stroke_to(Vector2(80, 50), 0.3)
	doc.end_stroke()
	_verifier("Opacité à la pression : pression 0,3 → alpha 0,3", absf(_alpha(doc.get_layer(0), 50, 50) - 0.3) < 0.03, str(_alpha(doc.get_layer(0), 50, 50)))


func _tester_modes_pinceau() -> void:
	var doc := _document()
	var calque := doc.get_layer(0)
	doc.fill(Vector2i(0, 0), ROUGE)
	var gomme := _pinceau(10.0)
	gomme.blend_mode = DrawingBrush.BRUSH_BLEND_ERASE
	doc.draw_line(Vector2(10, 50), Vector2(90, 50), gomme, NOIR)
	_verifier("Mode gomme : efface (alpha 0) sous le trait seulement", _alpha(calque, 50, 50) == 0.0 and calque.get_pixel(50, 20) == ROUGE)

	var derriere := _pinceau(10.0)
	derriere.blend_mode = DrawingBrush.BRUSH_BLEND_BEHIND
	doc.draw_line(Vector2(50, 10), Vector2(50, 90), derriere, BLEU)
	_verifier("Mode derrière : ne change pas les pixels opaques, remplit les trous", calque.get_pixel(50, 20) == ROUGE and _proche(calque.get_pixel(50, 50), BLEU))

	doc = _document()
	calque = doc.get_layer(0)
	doc.draw_line(Vector2(10, 50), Vector2(90, 50), _pinceau(6.0), ROUGE)
	doc.set_layer_property(0, "alpha_locked", true)
	doc.draw_line(Vector2(50, 10), Vector2(50, 90), _pinceau(10.0), BLEU)
	_verifier("Verrou de transparence : recolore les pixels peints, sans peindre ailleurs",
		_proche(calque.get_pixel(50, 50), BLEU) and _alpha(calque, 50, 20) == 0.0 and calque.get_pixel(20, 50) == ROUGE)

	doc.set_layer_property(0, "locked", true)
	_verifier("Calque verrouillé : begin_stroke refusé", not doc.begin_stroke(_pinceau(), NOIR))
	doc.set_layer_property(0, "locked", false)
	doc.set_layer_property(0, "visible", false)
	_verifier("Calque masqué : begin_stroke refusé, remplissage refusé", not doc.begin_stroke(_pinceau(), NOIR) and not doc.fill(Vector2i(5, 5), NOIR))


# 🪣 Remplissage

func _tester_remplissage() -> void:
	var doc := _document()
	var calque := doc.get_layer(0)
	_carre(doc, 20, 20, 80, 80)
	_verifier("fill : remplit l'intérieur d'un carré fermé", doc.fill(Vector2i(50, 50), BLEU, 0.1, DrawingDocument.FILL_REFERENCE_LAYER, 0, 0)
		and calque.get_pixel(50, 50) == BLEU and _alpha(calque, 5, 5) == 0.0)
	_verifier("fill : étape « Remplissage » dans l'historique", doc.get_undo_name() == "Remplissage")
	doc.undo()
	doc.fill(Vector2i(50, 50), BLEU, 0.1, DrawingDocument.FILL_REFERENCE_LAYER, 0, 2)
	_verifier("fill + expand : la zone s'étend sous le trait (pixel du trait recouvert)", calque.get_pixel(21, 50) == BLEU or calque.get_pixel(22, 50) == BLEU)
	_verifier("fill hors du dessin : refusé", not doc.fill(Vector2i(-1, 5), BLEU))

	doc = _document()
	calque = doc.get_layer(0)
	_carre(doc, 20, 20, 80, 80, 6)
	doc.fill(Vector2i(50, 50), BLEU, 0.1, DrawingDocument.FILL_REFERENCE_LAYER, 0, 0)
	_verifier("Trou de 6 px sans fermeture : la couleur fuit à l'extérieur", calque.get_pixel(5, 5) == BLEU)
	doc.undo()
	doc.fill(Vector2i(50, 50), BLEU, 0.1, DrawingDocument.FILL_REFERENCE_LAYER, 5, 0)
	_verifier("Fermeture des trous (5 px) : pas de fuite, intérieur rempli jusqu'aux traits",
		_alpha(calque, 5, 5) == 0.0 and calque.get_pixel(50, 50) == BLEU and calque.get_pixel(24, 30) == BLEU and calque.get_pixel(76, 76) == BLEU)

	# Référence « tous les calques » : traits sur un calque, couleur sur un autre (comme en encrage / aplats).
	doc = _document()
	_carre(doc, 20, 20, 80, 80)
	var aplats := doc.add_layer("Aplats")
	doc.fill(Vector2i(50, 50), ROUGE, 0.1, DrawingDocument.FILL_REFERENCE_ALL_LAYERS)
	_verifier("Référence tous les calques : remplit le calque courant selon les traits d'un autre",
		aplats.get_pixel(50, 50) == ROUGE and aplats.get_pixel(5, 5).a == 0.0 and doc.get_layer(0).get_pixel(50, 50).a == 0.0)
	doc.undo()
	doc.fill(Vector2i(50, 50), ROUGE, 0.1, DrawingDocument.FILL_REFERENCE_LAYER)
	_verifier("Référence calque courant (vide) : tout le calque est rempli", aplats.get_pixel(5, 5) == ROUGE)

	doc = _document()
	calque = doc.get_layer(0)
	doc.draw_line(Vector2(10, 20), Vector2(30, 20), _pinceau(4.0), ROUGE)
	doc.draw_line(Vector2(60, 70), Vector2(90, 70), _pinceau(4.0), ROUGE)
	doc.fill(Vector2i(20, 20), BLEU, 0.1, DrawingDocument.FILL_REFERENCE_LAYER, 0, 0, false)
	_verifier("Non contigu : toutes les zones de même couleur sont remplies", calque.get_pixel(75, 70) == BLEU and _alpha(calque, 50, 50) == 0.0)
	var doc_tolerance := _document()
	var degrade := doc_tolerance.get_layer(0)
	doc_tolerance.fill(Vector2i(0, 0), Color(0.5, 0.5, 0.5))
	doc_tolerance.select_rect(Rect2i(50, 0, 50, 100))
	doc_tolerance.fill_selection(Color(0.56, 0.56, 0.56))
	doc_tolerance.deselect()
	doc_tolerance.fill(Vector2i(10, 10), ROUGE, 0.01)
	_verifier("Tolérance faible : s'arrête au changement de gris", degrade.get_pixel(10, 10) == ROUGE and degrade.get_pixel(80, 10) != ROUGE)
	doc_tolerance.undo()
	doc_tolerance.fill(Vector2i(10, 10), ROUGE, 0.1)
	_verifier("Tolérance 10 % : franchit l'écart de 0,06", degrade.get_pixel(80, 10) == ROUGE)


# ⬚ Sélection

func _tester_selection() -> void:
	var doc := _document()
	var signaux := _espionner(doc.selection_changed)
	_verifier("Sans sélection : tout est sélectionnable (valeur 1), pas de masque", not doc.has_selection() and doc.get_selection_value(3, 3) == 1.0 and doc.get_selection_image() == null)
	doc.select_rect(Rect2i(10, 10, 30, 20))
	_verifier("select_rect : bornes exactes, masque L8", doc.has_selection() and doc.get_selection_bounds() == Rect2i(10, 10, 30, 20)
		and doc.get_selection_image().get_format() == Image.FORMAT_L8 and signaux.n == 1)
	doc.select_rect(Rect2i(30, 10, 30, 20), DrawingDocument.SELECTION_ADD)
	_verifier("Mode ajouter : union", doc.get_selection_bounds() == Rect2i(10, 10, 50, 20))
	doc.select_rect(Rect2i(0, 0, 25, 100), DrawingDocument.SELECTION_SUBTRACT)
	_verifier("Mode soustraire", doc.get_selection_bounds() == Rect2i(25, 10, 35, 20))
	doc.select_rect(Rect2i(40, 0, 100, 100), DrawingDocument.SELECTION_INTERSECT)
	_verifier("Mode intersection", doc.get_selection_bounds() == Rect2i(40, 10, 20, 20))
	doc.select_ellipse(Rect2i(20, 20, 60, 40))
	_verifier("select_ellipse : centre sélectionné, coin du rectangle exclu",
		doc.get_selection_value(50, 40) == 1.0 and doc.get_selection_value(21, 21) == 0.0 and doc.get_selection_bounds().size.x >= 58)
	doc.select_polygon(PackedVector2Array([Vector2(10, 10), Vector2(90, 10), Vector2(10, 90)]))
	_verifier("select_polygon (lasso) : triangle", doc.get_selection_value(20, 20) == 1.0 and doc.get_selection_value(80, 80) == 0.0)
	doc.invert_selection()
	_verifier("invert_selection", doc.get_selection_value(20, 20) == 0.0 and doc.get_selection_value(80, 80) == 1.0)
	doc.deselect()
	_verifier("deselect", not doc.has_selection())
	doc.undo()
	_verifier("La sélection est annulable", doc.has_selection() and doc.get_selection_value(80, 80) == 1.0)
	doc.select_all()
	_verifier("select_all", doc.get_selection_bounds() == Rect2i(0, 0, 100, 100))

	# La sélection limite les outils.
	doc = _document()
	var calque := doc.get_layer(0)
	doc.select_rect(Rect2i(0, 0, 50, 100))
	doc.draw_line(Vector2(10, 50), Vector2(90, 50), _pinceau(6.0), NOIR)
	_verifier("Pinceau limité à la sélection", _alpha(calque, 30, 50) > 0.9 and _alpha(calque, 70, 50) == 0.0)
	doc.fill(Vector2i(30, 20), ROUGE)
	_verifier("Remplissage limité à la sélection", calque.get_pixel(30, 20) == ROUGE and _alpha(calque, 70, 20) == 0.0)
	doc.select_rect(Rect2i(0, 0, 20, 20))
	doc.clear_selection_content()
	_verifier("clear_selection_content : efface dans la sélection seulement", _alpha(calque, 10, 10) == 0.0 and calque.get_pixel(30, 30) == ROUGE)
	doc.fill_selection(BLEU)
	_verifier("fill_selection", calque.get_pixel(10, 10) == BLEU)

	doc = _document()
	_carre(doc, 20, 20, 80, 80)
	doc.select_by_color(Vector2i(50, 50), 0.1)
	_verifier("select_by_color (baguette) : l'intérieur du carré", doc.get_selection_value(50, 50) == 1.0 and doc.get_selection_value(5, 5) == 0.0
		and doc.get_selection_value(20, 50) == 0.0)


# 🗂️ Calques

func _tester_calques() -> void:
	var doc := _document()
	var signaux := _espionner(doc.layers_changed)
	var dessus := doc.add_layer()
	_verifier("add_layer : nommé automatiquement, au-dessus du calque courant, devient courant",
		doc.get_layer_count() == 2 and doc.get_layer(1) == dessus and doc.get_current_layer() == 1 and dessus.layer_name == "Calque 2" and signaux.n >= 1)
	var milieu := doc.add_layer("Milieu", 1)
	_verifier("add_layer à un index", doc.get_layer(1) == milieu and doc.get_layer(2) == dessus and doc.find_layer(milieu) == 1)
	doc.move_layer(1, 2)
	_verifier("move_layer : ordre et calque courant suivis", doc.get_layer(2) == milieu and doc.get_current() == milieu)
	var copie := doc.duplicate_layer(2)
	_verifier("duplicate_layer : copie au-dessus, nom « (copie) »", doc.get_layer(3) == copie and copie.layer_name == "Milieu (copie)" and doc.get_undo_name() == "Dupliquer le calque")
	doc.remove_layer(3)
	_verifier("remove_layer", doc.get_layer_count() == 3 and doc.find_layer(copie) == -1)
	doc.undo()
	_verifier("Suppression annulable (le même calque revient à sa place)", doc.get_layer(3) == copie)

	var seul := _document()
	seul.remove_layer(0) # erreur attendue
	_verifier("Le dernier calque ne peut pas être supprimé", seul.get_layer_count() == 1)

	doc = _document()
	var bas := doc.get_layer(0)
	doc.fill(Vector2i(0, 0), BLANC)
	var haut := doc.add_layer()
	doc.fill(Vector2i(0, 0), ROUGE)
	doc.set_layer_property(1, "blend_mode", DrawingLayer.LAYER_BLEND_MULTIPLY)
	_verifier("merge_down : fusion selon le mode du calque (produit rouge × blanc = rouge)", doc.merge_down(1) and doc.get_layer_count() == 1
		and bas.get_pixel(5, 5) == ROUGE and doc.get_current() == bas)
	doc.undo()
	_verifier("Fusion annulable : deux calques, pixels d'origine", doc.get_layer_count() == 2 and doc.get_layer(1) == haut and bas.get_pixel(5, 5) == BLANC)
	_verifier("merge_down du calque le plus bas : refusé", not doc.merge_down(0))
	doc.clear_layer(0)
	_verifier("clear_layer", bas.is_empty() and doc.get_undo_name() == "Effacer le calque")

	doc = _document()
	var historique := _espionner(doc.history_changed)
	for opacite in [0.9, 0.8, 0.7]:
		doc.set_layer_property(0, "opacity", opacite)
	_verifier("set_layer_property : changements successifs du même réglage = une seule étape",
		doc.get_undo_count() == 1 and doc.get_undo_name() == "Opacité du calque" and historique.n >= 3)
	doc.undo()
	_verifier("…annulée d'un coup (opacité 1)", doc.get_layer(0).opacity == 1.0)


# 🖼️ Composition

func _tester_composition() -> void:
	var doc := _document(Vector2i(10, 10))
	doc.background_color = Color(1, 1, 1, 0)
	doc.fill(Vector2i(0, 0), Color(0.5, 0.5, 0.5))
	var dessus := doc.add_layer()
	doc.fill(Vector2i(0, 0), Color(0.5, 0.5, 0.5))
	var attendus := {
		DrawingLayer.LAYER_BLEND_NORMAL: 0.5,
		DrawingLayer.LAYER_BLEND_MULTIPLY: 0.25,
		DrawingLayer.LAYER_BLEND_SCREEN: 0.75,
		DrawingLayer.LAYER_BLEND_ADD: 1.0,
		DrawingLayer.LAYER_BLEND_SUBTRACT: 0.0,
		DrawingLayer.LAYER_BLEND_DIFFERENCE: 0.0,
		DrawingLayer.LAYER_BLEND_DARKEN: 0.5,
	}
	var justes := true
	var detail := ""
	for mode in attendus:
		dessus.blend_mode = mode
		var obtenu := doc.get_composite_image().get_pixel(5, 5).r
		if absf(obtenu - attendus[mode]) > 0.01:
			justes = false
			detail += "mode %d : %.3f ; " % [mode, obtenu]
	_verifier("Modes de fusion (gris 0,5 sur gris 0,5) : normal, produit, écran, ajout, soustraction, différence, obscurcir", justes, detail)

	dessus.blend_mode = DrawingLayer.LAYER_BLEND_NORMAL
	dessus.opacity = 0.5
	doc.get_layer(0).set_image(_image_unie(10, 10, BLANC))
	dessus.set_image(_image_unie(10, 10, NOIR))
	_verifier("Opacité du calque : noir à 50 % sur blanc = gris 0,5", absf(doc.get_composite_image().get_pixel(5, 5).r - 0.5) < 0.01)
	dessus.visible = false
	_verifier("Calque masqué : non composé", doc.get_composite_image().get_pixel(5, 5) == BLANC)

	# Écrêtage : le calque écrêté n'apparaît que là où son calque de base est peint.
	doc = _document(Vector2i(20, 10))
	doc.background_color = Color(1, 1, 1, 0)
	doc.select_rect(Rect2i(0, 0, 10, 10))
	doc.fill_selection(BLANC)
	doc.deselect()
	var ecrete := doc.add_layer()
	doc.fill(Vector2i(0, 0), ROUGE)
	doc.set_layer_property(1, "clipped", true)
	var composee := doc.get_composite_image()
	_verifier("Écrêtage : visible sur la base peinte, invisible ailleurs", composee.get_pixel(5, 5) == ROUGE and composee.get_pixel(15, 5).a == 0.0)
	doc.set_layer_property(0, "visible", false)
	_verifier("Écrêtage : base masquée → calque écrêté masqué aussi", doc.get_composite_image().get_pixel(5, 5).a == 0.0)
	ecrete.clipped = false

	doc = _document(Vector2i(10, 10))
	doc.background_color = Color(0, 0, 1, 1)
	_verifier("pick_color tous calques : couleur affichée, fond compris", doc.pick_color(Vector2i(5, 5)) == BLEU)
	_verifier("pick_color calque courant : transparent", doc.pick_color(Vector2i(5, 5), false).a == 0.0)
	_verifier("get_composite_image(false) : sans le fond", doc.get_composite_image(false).get_pixel(5, 5).a == 0.0)


func _image_unie(largeur: int, hauteur: int, couleur: Color) -> Image:
	var image := Image.create_empty(largeur, hauteur, false, Image.FORMAT_RGBA8)
	image.fill(couleur)
	return image


# ↩️ Historique et taille

func _tester_historique() -> void:
	var doc := _document()
	doc.history_limit = 3
	for i in 5:
		doc.draw_line(Vector2(10, 10 + i * 10), Vector2(90, 10 + i * 10), _pinceau(), NOIR)
	_verifier("history_limit : seules les 3 dernières étapes sont gardées", doc.get_undo_count() == 3)
	var n := 0
	while doc.undo():
		n += 1
	_verifier("Annuler tout : 3 annulations, les 2 premiers traits restent", n == 3 and _alpha(doc.get_layer(0), 50, 10) > 0.9 and _alpha(doc.get_layer(0), 50, 30) == 0.0)
	doc.draw_line(Vector2(10, 90), Vector2(90, 90), _pinceau(), NOIR)
	_verifier("Une nouvelle action vide le rétablissement", not doc.can_redo())

	doc.size = Vector2i(60, 40)
	_verifier("size : calques recadrés, historique et sélection vidés", doc.get_size() == Vector2i(60, 40) and doc.get_layer(0).get_size() == Vector2i(60, 40)
		and not doc.can_undo() and _alpha(doc.get_layer(0), 50, 10) > 0.9)


# ✨ Trait différé (pinceau à shader)

func _tester_trait_differe() -> void:
	var doc := _document()
	var calque := doc.get_layer(0)
	var apercus := _espionner(doc.stroke_changed)
	doc.begin_stroke(_pinceau(10.0), ROUGE, true)
	doc.stroke_to(Vector2(10, 50))
	doc.stroke_to(Vector2(90, 50))
	var apercu := doc.get_stroke_preview(Rect2i(0, 0, 100, 100))
	_verifier("Trait différé : calque intact pendant le trait, aperçu disponible (stroke_changed)",
		calque.is_empty() and doc.is_stroke_deferred() and apercus.n >= 1 and apercu.get_pixel(50, 50).a > 0.9 and _proche(apercu.get_pixel(50, 50), ROUGE))
	var rendu := _image_unie(100, 100, Color(0, 1, 0, 0))
	for x in range(10, 90):
		rendu.set_pixel(x, 50, Color(0, 1, 0, 1)) # « shader » : trait vert d'un pixel
	doc.commit_stroke_image(rendu, Vector2i.ZERO)
	_verifier("commit_stroke_image : le rendu remplace le trait, une étape d'historique",
		calque.get_pixel(50, 50) == Color(0, 1, 0, 1) and _alpha(calque, 50, 53) == 0.0 and not doc.is_stroking() and doc.get_undo_count() == 1)
	doc.commit_stroke_image(rendu, Vector2i.ZERO) # erreur attendue : aucun trait en cours
	doc.begin_stroke(_pinceau(10.0), ROUGE, true)
	doc.stroke_to(Vector2(50, 10))
	doc.stroke_to(Vector2(50, 90))
	doc.end_stroke()
	_verifier("end_stroke d'un trait différé : appliqué tel quel (sans shader)", _proche(calque.get_pixel(50, 20), ROUGE))


# 💾 Sauvegarde

func _tester_sauvegarde() -> void:
	var doc := _document(Vector2i(40, 30))
	doc.background_color = Color(0.9, 0.9, 0.8)
	doc.draw_line(Vector2(5, 15), Vector2(35, 15), _pinceau(4.0), BLEU)
	var encre := doc.add_layer("Encre")
	doc.draw_line(Vector2(20, 2), Vector2(20, 28), _pinceau(4.0), ROUGE)
	doc.set_layer_property(1, "blend_mode", DrawingLayer.LAYER_BLEND_MULTIPLY)
	doc.set_layer_property(1, "opacity", 0.7)
	for chemin in ["user://test_dessin.tres", "user://test_dessin.res"]:
		var erreur := ResourceSaver.save(doc, chemin)
		var relu: DrawingDocument = ResourceLoader.load(chemin, "", ResourceLoader.CACHE_MODE_IGNORE)
		_verifier("Sauvegarde %s : taille, fond, calques, réglages et pixels conservés" % chemin.get_extension(),
			erreur == OK and relu != null and relu.get_size() == Vector2i(40, 30) and relu.background_color.is_equal_approx(doc.background_color)
			and relu.get_layer_count() == 2 and relu.get_layer(1).layer_name == "Encre" and relu.get_layer(1).blend_mode == DrawingLayer.LAYER_BLEND_MULTIPLY
			and is_equal_approx(relu.get_layer(1).opacity, 0.7) and relu.get_layer(0).get_pixel(10, 15) == BLEU and relu.get_layer(1).get_pixel(20, 5) == encre.get_pixel(20, 5)
			and not relu.can_undo())
	var png := "user://test_dessin.png"
	_verifier("export_png : image composée avec le fond", doc.export_png(png) == OK and _proche(Image.load_from_file(png).get_pixel(2, 2), Color(0.9, 0.9, 0.8)))
	for chemin in ["user://test_dessin.tres", "user://test_dessin.res", png]:
		DirAccess.remove_absolute(chemin)
	await _frames(0)


# 🖼️ Toile

func _tester_toile_dessin() -> void:
	var defaut := DrawingCanvas.new()
	_verifier("Toile par défaut : pinceau, noir sur blanc, raccourcis et gestes actifs, pas de document hors de l'arbre",
		defaut.tool == DrawingCanvas.TOOL_BRUSH and defaut.primary_color == NOIR and defaut.secondary_color == BLANC and defaut.shortcuts_enabled
		and defaut.touch_gestures and defaut.document == null and defaut.brush == null and defaut.get_active_brush() != null)
	defaut.default_document_size = Vector2i(320, 200)
	defaut.size = TAILLE_TOILE
	_ajouter(defaut)
	_verifier("Dans le jeu : document créé automatiquement (default_document_size, un calque)",
		defaut.document != null and defaut.document.get_size() == Vector2i(320, 200) and defaut.document.get_layer_count() == 1)
	defaut.queue_free()

	var toile := _toile()
	await _frames(1)
	_verifier("fit_to_view : dessin entier visible et centré", toile.canvas_to_screen(Vector2(150, 125)).is_equal_approx(TAILLE_TOILE / 2)
		and toile.zoom > 1.5 and toile.zoom < 2.0)
	toile.rotate_at(30, Vector2(100, 100))
	toile.flip_h = true
	toile.zoom_at(1.7, Vector2(250, 80))
	_verifier("Aller-retour écran ↔ dessin exact après rotation, miroir et zoom",
		toile.screen_to_canvas(toile.canvas_to_screen(Vector2(123, 45))).is_equal_approx(Vector2(123, 45)))
	toile.reset_rotation()
	toile.fit_to_view()

	var debuts := _espionner(toile.stroke_started)
	var fins := _espionner(toile.stroke_finished)
	var calque: DrawingLayer = toile.document.get_layer(0)
	_glisser(toile, Vector2(20, 40), Vector2(200, 40))
	_verifier("Glisser à la souris : trait peint de bout en bout (stroke_started / stroke_finished)",
		_alpha(calque, 20, 40) > 0.9 and _alpha(calque, 110, 40) > 0.9 and _alpha(calque, 199, 40) > 0.9 and _alpha(calque, 110, 60) == 0.0
		and debuts.n == 1 and fins.n == 1 and not toile.is_stroking())

	toile.brush.stabilization = 100
	_glisser(toile, Vector2(20, 80), Vector2(200, 80), 4)
	_verifier("Stabilisation forte : le trait rattrape le point de relâchement", _alpha(calque, 198, 80) > 0.9)
	toile.brush.stabilization = 0

	_bouton(toile, Vector2(250, 200), true)
	_bouton(toile, Vector2(250, 200), false)
	_verifier("Simple clic : un tampon", _alpha(calque, 250, 200) > 0.9 and _alpha(calque, 250, 215) == 0.0)

	toile.brush.size_dynamics = DrawingBrush.DYNAMIC_PRESSURE
	toile.brush.size_min = 0.1
	toile.brush.size = 16.0
	_glisser(toile, Vector2(20, 130), Vector2(140, 130), 10, 0.1)
	_glisser(toile, Vector2(20, 170), Vector2(140, 170), 10, 1.0)
	_verifier("Pression du stylet transmise : trait léger fin, appuyé épais",
		_epaisseur_zone(calque, 80, 115, 145) < _epaisseur_zone(calque, 80, 150, 190))
	toile.brush.size_dynamics = 0
	toile.brush.size = 8.0

	_mouvement(toile, Vector2(30, 40), 0.0, Vector2.ZERO, true) # survol du stylet retourné : bout gomme
	_bouton(toile, Vector2(30, 40), true)
	_mouvement(toile, Vector2(60, 40), 1.0, Vector2.ZERO, true)
	_bouton(toile, Vector2(60, 40), false)
	_mouvement(toile, Vector2(0, 0), 0.0)
	_verifier("Bout gomme du stylet : efface, même avec l'outil pinceau", _alpha(calque, 45, 40) == 0.0 and _alpha(calque, 100, 40) > 0.9)

	var rejets := _espionner(toile.stroke_rejected)
	toile.document.set_layer_property(0, "locked", true)
	_glisser(toile, Vector2(20, 230), Vector2(100, 230))
	_verifier("Calque verrouillé : stroke_rejected, rien n'est peint", rejets.n == 1 and _alpha(calque, 60, 230) == 0.0)
	toile.document.set_layer_property(0, "locked", false)

	# Pinceau à shader en headless : pas de rendu possible, le trait est appliqué sans le shader.
	var shader := Shader.new()
	shader.code = "shader_type canvas_item;\nvoid fragment() { COLOR = vec4(0.0, 1.0, 0.0, texture(TEXTURE, UV).a); }"
	var materiau := ShaderMaterial.new()
	materiau.shader = shader
	toile.brush.material = materiau
	toile.primary_color = ROUGE
	_glisser(toile, Vector2(20, 110), Vector2(120, 110))
	for i in 12: # rendu GPU puis relecture (quelques images), ou repli immédiat sans rendu
		if not toile.document.is_stroking():
			break
		await _frames(1)
	if DisplayServer.get_name() == "headless":
		_verifier("Pinceau à shader sans rendu (headless) : trait appliqué dans sa couleur, sans le shader",
			_proche(calque.get_pixel(70, 110), ROUGE) and not toile.document.is_stroking())
	else:
		_verifier("Pinceau à shader : le trait est rendu par le shader (vert) puis appliqué au calque",
			_proche(calque.get_pixel(70, 110), Color(0, 1, 0, 1), 0.03) and not toile.document.is_stroking(), str(calque.get_pixel(70, 110)))
	toile.brush.material = null
	toile.primary_color = NOIR
	toile.queue_free()
	await _frames(1)


func _epaisseur_zone(calque: DrawingLayer, x: int, y0: int, y1: int) -> int:
	var n := 0
	for y in range(y0, y1):
		if calque.get_pixel(x, y).a > 0.5:
			n += 1
	return n


func _tester_toile_outils() -> void:
	var toile := _toile()
	await _frames(1)
	var doc := toile.document
	var calque := doc.get_layer(0)
	_carre(doc, 20, 20, 120, 120)

	toile.tool = DrawingCanvas.TOOL_FILL
	toile.primary_color = BLEU
	_bouton(toile, Vector2(70, 70), true)
	_bouton(toile, Vector2(70, 70), false)
	_verifier("Outil remplir : clic dans le carré", calque.get_pixel(70, 70) == BLEU and calque.get_pixel(200, 200).a == 0.0)

	toile.tool = DrawingCanvas.TOOL_EYEDROPPER
	var prelevements := _espionner(toile.color_picked)
	_bouton(toile, Vector2(20, 20), true)
	_bouton(toile, Vector2(20, 20), false)
	_verifier("Pipette : prélève la couleur affichée (color_picked)", toile.primary_color == NOIR and prelevements.n == 1)
	_bouton(toile, Vector2(200, 200), true)
	_bouton(toile, Vector2(200, 200), false)
	_verifier("Pipette sur du transparent (fond transparent) : couleur inchangée", toile.primary_color == NOIR)

	toile.tool = DrawingCanvas.TOOL_BRUSH
	_modificateur(KEY_ALT, true)
	_bouton(toile, Vector2(70, 70), true)
	_bouton(toile, Vector2(70, 70), false)
	_modificateur(KEY_ALT, false)
	_verifier("Alt + clic au pinceau : pipette temporaire, rien n'est peint", toile.primary_color == BLEU and calque.get_pixel(70, 70) == BLEU
		and toile.tool == DrawingCanvas.TOOL_BRUSH)

	toile.tool = DrawingCanvas.TOOL_SELECT_RECT
	_glisser(toile, Vector2(150, 150), Vector2(200, 180), 3)
	_verifier("Sélection rectangle au glisser", doc.has_selection() and doc.get_selection_bounds() == Rect2i(150, 150, 50, 30))
	_modificateur(KEY_SHIFT, true)
	_glisser(toile, Vector2(150, 200), Vector2(200, 230), 3)
	_modificateur(KEY_SHIFT, false)
	_verifier("Maj : ajoute à la sélection", doc.get_selection_bounds() == Rect2i(150, 150, 50, 80))
	_bouton(toile, Vector2(10, 10), true)
	_bouton(toile, Vector2(10, 10), false)
	_verifier("Simple clic : désélectionne", not doc.has_selection())

	toile.tool = DrawingCanvas.TOOL_SELECT_ELLIPSE
	_glisser(toile, Vector2(150, 150), Vector2(250, 230), 3)
	_verifier("Sélection ellipse", doc.has_selection() and doc.get_selection_value(200, 190) == 1.0 and doc.get_selection_value(152, 152) == 0.0)

	toile.tool = DrawingCanvas.TOOL_SELECT_LASSO
	_bouton(toile, Vector2(150, 20), true)
	for point in [Vector2(250, 20), Vector2(250, 120), Vector2(150, 120)]:
		_mouvement(toile, point)
	_bouton(toile, Vector2(150, 120), false)
	_verifier("Lasso : polygone suivi par le pointeur", doc.get_selection_value(200, 70) == 1.0 and doc.get_selection_value(200, 200) == 0.0)

	toile.tool = DrawingCanvas.TOOL_SELECT_WAND
	_bouton(toile, Vector2(70, 70), true)
	_bouton(toile, Vector2(70, 70), false)
	_verifier("Baguette : zone de même couleur", doc.get_selection_value(70, 70) == 1.0 and doc.get_selection_value(200, 70) == 0.0)

	toile.tool = DrawingCanvas.TOOL_ERASER
	doc.deselect()
	_glisser(toile, Vector2(30, 70), Vector2(110, 70))
	_verifier("Outil gomme : utilise le pinceau eraser", calque.get_pixel(70, 70).a == 0.0 and toile.get_active_brush() == toile.eraser)

	var avant := calque.get_png_data()
	toile.tool = DrawingCanvas.TOOL_PAN
	var decalage := toile.view_offset
	_glisser(toile, Vector2(100, 100), Vector2(140, 100), 2)
	_verifier("Outil main : déplace la vue sans dessiner", toile.view_offset.x > decalage.x + 50 and calque.get_png_data() == avant)
	toile.queue_free()
	await _frames(1)


func _tester_toile_navigation() -> void:
	var toile := _toile()
	await _frames(1)
	var calque: DrawingLayer = toile.document.get_layer(0)
	var vues := _espionner(toile.view_changed)

	var ecran := Vector2(180, 140)
	var sous_curseur := toile.screen_to_canvas(ecran)
	var zoom := toile.zoom
	_molette(toile, ecran, true)
	_verifier("Molette : zoom avant de 15 %, le point sous le curseur reste fixe",
		is_equal_approx(toile.zoom, zoom * 1.15) and toile.canvas_to_screen(sous_curseur).is_equal_approx(ecran) and vues.n >= 1)
	_molette(toile, ecran, false)
	_verifier("Molette arrière : zoom rétabli", is_equal_approx(toile.zoom, zoom))

	var decalage := toile.view_offset
	_modificateur(KEY_SPACE, true)
	_glisser(toile, Vector2(100, 100), Vector2(130, 120), 3)
	_modificateur(KEY_SPACE, false)
	_verifier("Espace + glisser : déplace la vue, ne dessine pas", not toile.view_offset.is_equal_approx(decalage) and calque.is_empty())

	_modificateur(KEY_SPACE, true)
	_modificateur(KEY_SHIFT, true)
	_glisser(toile, Vector2(220, 60), Vector2(220, 200), 4)
	_modificateur(KEY_SHIFT, false)
	_modificateur(KEY_SPACE, false)
	_verifier("Maj + Espace + glisser : rotation de la vue", absf(toile.view_rotation) > 5.0 and calque.is_empty(), str(toile.view_rotation))

	var evenement := InputEventMouseButton.new()
	evenement.button_index = MOUSE_BUTTON_MIDDLE
	evenement.pressed = true
	evenement.position = Vector2(200, 200)
	toile.get_viewport().push_input(evenement, true)
	var deplacement := InputEventMouseMotion.new()
	deplacement.position = Vector2(260, 200)
	deplacement.button_mask = MOUSE_BUTTON_MASK_MIDDLE
	decalage = toile.view_offset
	toile.get_viewport().push_input(deplacement, true)
	evenement = evenement.duplicate()
	evenement.pressed = false
	evenement.position = Vector2(260, 200)
	toile.get_viewport().push_input(evenement, true)
	_verifier("Clic du milieu + glisser : déplace la vue", toile.view_offset.is_equal_approx(decalage + Vector2(60, 0)))

	toile.fit_to_view()
	var zoom_ajuste := toile.zoom
	toile.size = TAILLE_TOILE * 0.5
	await _frames(1)
	_verifier("Après fit_to_view, le dessin reste ajusté quand la toile change de taille", toile.zoom < zoom_ajuste * 0.6)
	toile.zoom_at(2.0, Vector2(50, 50))
	var zoom_manuel := toile.zoom
	toile.size = TAILLE_TOILE
	await _frames(1)
	_verifier("…jusqu'à un zoom manuel", is_equal_approx(toile.zoom, zoom_manuel))
	toile.queue_free()
	await _frames(1)


func _tester_toile_clavier() -> void:
	var toile := _toile()
	await _frames(1)
	toile.grab_focus()
	var doc := toile.document
	var outils := {KEY_E: DrawingCanvas.TOOL_ERASER, KEY_G: DrawingCanvas.TOOL_FILL, KEY_I: DrawingCanvas.TOOL_EYEDROPPER, KEY_L: DrawingCanvas.TOOL_SELECT_LASSO,
		KEY_W: DrawingCanvas.TOOL_SELECT_WAND, KEY_H: DrawingCanvas.TOOL_PAN, KEY_B: DrawingCanvas.TOOL_BRUSH}
	var justes := true
	var changements := _espionner(toile.tool_changed)
	for touche in outils:
		_touche(toile, touche)
		justes = justes and toile.tool == outils[touche]
	_verifier("Raccourcis d'outils B E G I L W H (tool_changed)", justes and changements.n == outils.size())
	_touche(toile, KEY_M)
	var rectangle := toile.tool == DrawingCanvas.TOOL_SELECT_RECT
	_touche(toile, KEY_M)
	_verifier("M : sélection rectangle, puis ellipse", rectangle and toile.tool == DrawingCanvas.TOOL_SELECT_ELLIPSE)
	toile.tool = DrawingCanvas.TOOL_BRUSH

	var taille := toile.brush.size
	_touche(toile, KEY_BRACKETRIGHT)
	var plus := toile.brush.size
	_touche(toile, KEY_BRACKETLEFT)
	_touche(toile, KEY_BRACKETLEFT)
	_verifier("] et [ : taille du pinceau", plus > taille and toile.brush.size < taille)

	toile.primary_color = ROUGE
	_touche(toile, KEY_X)
	_verifier("X : échange les couleurs", toile.primary_color == BLANC and toile.secondary_color == ROUGE)

	doc.draw_line(Vector2(10, 10), Vector2(100, 10), _pinceau(), NOIR)
	_touche(toile, KEY_Z, true)
	var annule := doc.get_layer(0).is_empty()
	_touche(toile, KEY_Y, true)
	var retabli := not doc.get_layer(0).is_empty()
	_touche(toile, KEY_Z, true)
	_touche(toile, KEY_Z, true, true)
	_verifier("Ctrl+Z / Ctrl+Y / Ctrl+Maj+Z", annule and retabli and not doc.get_layer(0).is_empty())
	_touche(toile, KEY_A, true)
	var tout := doc.has_selection()
	_touche(toile, KEY_I, true, true)
	var inverse := not doc.has_selection()
	_touche(toile, KEY_A, true)
	_touche(toile, KEY_D, true)
	_verifier("Ctrl+A, Ctrl+Maj+I (inverse de tout = rien), Ctrl+D", tout and inverse and not doc.has_selection())
	_touche(toile, KEY_A, true)
	_touche(toile, KEY_DELETE)
	_verifier("Suppr : efface le contenu de la sélection", doc.get_layer(0).is_empty())

	var rotation := toile.view_rotation
	_touche(toile, KEY_EQUAL)
	_verifier("= : rotation de 15°", is_equal_approx(toile.view_rotation, rotation + 15.0))
	_touche(toile, KEY_1, true)
	_verifier("Ctrl+1 : taille réelle", toile.zoom == 1.0)
	_touche(toile, KEY_0, true)
	_verifier("Ctrl+0 : ajuster", toile.zoom > 1.2)

	toile.shortcuts_enabled = false
	_touche(toile, KEY_E)
	_verifier("shortcuts_enabled = false : touches ignorées", toile.tool == DrawingCanvas.TOOL_BRUSH)
	toile.queue_free()
	await _frames(1)


func _tester_toile_tactile() -> void:
	var toile := _toile()
	await _frames(1)
	var calque: DrawingLayer = toile.document.get_layer(0)
	_toucher(toile, 0, Vector2(20, 50), true)
	for i in range(1, 11):
		_glisser_doigt(toile, 0, toile.canvas_to_screen(Vector2(20 + i * 15, 50)))
	_toucher(toile, 0, Vector2(170, 50), false)
	_verifier("Un doigt : dessine", _alpha(calque, 100, 50) > 0.9 and toile.document.get_undo_count() == 1)

	# Deuxième doigt juste après le premier : le début de trait est annulé et un geste commence.
	var zoom := toile.zoom
	_toucher(toile, 0, Vector2(100, 120), true)
	_glisser_doigt(toile, 0, toile.canvas_to_screen(Vector2(110, 120)))
	var a := toile.canvas_to_screen(Vector2(110, 120)) # position du premier doigt
	var b := a + Vector2(100, 0)
	var touche_b := InputEventScreenTouch.new()
	touche_b.index = 1
	touche_b.position = b
	touche_b.pressed = true
	toile.get_viewport().push_input(touche_b, true)
	_glisser_doigt(toile, 1, a + Vector2(200, 0)) # écarter : zoom × 2
	_verifier("Deux doigts : le trait tout juste commencé est annulé (rien peint, pas d'historique)",
		_alpha(calque, 105, 120) == 0.0 and toile.document.get_undo_count() == 1)
	_verifier("Pincer : zoom selon l'écart des doigts", absf(toile.zoom - zoom * 2.0) < 0.01, "%.3f → %.3f" % [zoom, toile.zoom])
	var rotation := toile.view_rotation
	_glisser_doigt(toile, 1, a + Vector2(0, 200)) # quart de tour
	_verifier("Tourner deux doigts : rotation de la vue (touch_rotation)", absf(absf(toile.view_rotation - rotation) - 90.0) < 0.5, str(toile.view_rotation))
	touche_b.pressed = false
	toile.get_viewport().push_input(touche_b, true)
	_glisser_doigt(toile, 0, a + Vector2(30, 30))
	_toucher(toile, 0, Vector2(100, 120), false)
	_verifier("Après le geste, le doigt restant ne dessine pas", toile.document.get_undo_count() == 1 and not toile.is_dragging())
	toile.queue_free()
	await _frames(1)


# 🎛️ Atelier

func _tester_atelier() -> void:
	var atelier := DrawingStudio.new()
	atelier.size = Vector2(1100, 700)
	_ajouter(atelier)
	await _frames(2)
	var toile := atelier.get_canvas()
	_verifier("Atelier : toile interne, document 1280 × 720 créé, pinceaux préréglés assignés",
		toile != null and atelier.get_document() != null and atelier.get_document().get_size() == Vector2i(1280, 720)
		and toile.brush != null and toile.brush.resource_name == "Plume G" and toile.eraser != null and toile.eraser.blend_mode == DrawingBrush.BRUSH_BLEND_ERASE)
	_verifier("Atelier : enfants internes (jamais sauvegardés)", atelier.get_child_count() == 0 and atelier.get_child_count(true) > 0)
	_verifier("Atelier : la toile remplit l'espace entre les panneaux", toile.size.x > 400 and toile.size.y > 400, str(toile.size))

	atelier.new_document(Vector2i(64, 48), Color(1, 1, 1, 0))
	_verifier("new_document", atelier.get_document().get_size() == Vector2i(64, 48) and toile.document == atelier.get_document()
		and atelier.get_current_path().is_empty())
	var doc := atelier.get_document()
	doc.draw_line(Vector2(5, 24), Vector2(60, 24), toile.brush, ROUGE)
	doc.add_layer("Dessus")
	var enregistres := _espionner(atelier.document_saved)
	var ouverts := _espionner(atelier.document_opened)
	_verifier("save_as .tres : calques conservés, chemin courant", atelier.save_as("user://atelier_test.tres") == OK and enregistres.n == 1
		and atelier.get_current_path() == "user://atelier_test.tres")
	_verifier("save_as .png : image fusionnée", atelier.save_as("user://atelier_test.png") == OK)
	_verifier("open_file .png : un calque, pixels de l'image", atelier.open_file("user://atelier_test.png") == OK and ouverts.n == 1
		and atelier.get_document().get_layer_count() == 1 and atelier.get_document().get_layer(0).get_pixel(30, 24).is_equal_approx(ROUGE))
	_verifier("open_file .tres : deux calques", atelier.open_file("user://atelier_test.tres") == OK and atelier.get_document().get_layer_count() == 2
		and atelier.get_document().get_layer(1).layer_name == "Dessus")
	_verifier("open_file d'un fichier absent : erreur, document conservé", atelier.open_file("user://absent.png") != OK and atelier.get_document().get_layer_count() == 2)
	for chemin in ["user://atelier_test.tres", "user://atelier_test.png"]:
		DirAccess.remove_absolute(chemin)

	var pinceau := DrawingBrush.create_preset(DrawingBrush.PRESET_MARKER)
	var liste: Array[DrawingBrush] = [pinceau]
	atelier.brushes = liste
	_verifier("brushes : préréglages personnalisés, le premier est assigné", toile.brush == pinceau)
	atelier.show_file_buttons = false
	_verifier("show_file_buttons = false", not atelier.show_file_buttons)
	atelier.queue_free()
	await _frames(1)
