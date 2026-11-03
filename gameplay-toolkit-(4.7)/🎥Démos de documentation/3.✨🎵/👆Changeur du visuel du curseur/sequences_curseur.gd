extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 👆 Plans filmés de la page Changeur du visuel du curseur (effets/changeur-du-visuel-du-curseur.html).
##
## --write-movie ne filme pas le curseur du système : une souris simulée (_Souris) dessine l'image du
## CursorChanger au sommet de la pile (is_current(), style ou pressed_style, get_frame_at), à son hotspot.
## Survol : la souris émet elle-même mouse_entered / mouse_exited sur les cibles (les signaux qu'écoute
## CursorChanger) ; le picking du moteur est coupé, la vraie souris de la fenêtre filmée le perturbant.
## Clic : Input.parse_input_event (état du bouton lu par CursorChanger).

const Curseurs := preload("res://🎥Démos de documentation/socle/curseurs.gd")


## Souris simulée : position en coordonnées de la scène, évènements réels, image du curseur courant.
class _Souris extends Node2D:
	var changeurs: Array[CursorChanger] = []
	var bouton_enfonce := false
	## Cibles survolables : [nœud, Callable(position de la souris) -> bool].
	var cibles: Array = []
	var _dedans := {}
	var _temps := {}
	var _systeme := Curseurs.fleche()

	func _process(_delta: float) -> void:
		queue_redraw()

	func courant() -> CursorChanger:
		for changeur in changeurs:
			if is_instance_valid(changeur) and changeur.is_current():
				return changeur
		return null

	func _draw() -> void:
		var changeur := courant()
		var style: CursorStyle = null
		if changeur:
			style = changeur.pressed_style if bouton_enfonce and changeur.pressed_style else changeur.style
		if style == null or style.frames.is_empty():
			draw_texture(_systeme, Vector2.ZERO)
			return
		if not _temps.has(style):
			_temps[style] = Time.get_ticks_msec()
		var image := style.get_frame_at((Time.get_ticks_msec() - _temps[style]) / 1000.0)
		draw_texture(image, -style.hotspot)

	## Émet mouse_entered / mouse_exited sur les cibles dont la souris entre ou sort.
	func envoyer_mouvement() -> void:
		for cible in cibles:
			var noeud: Node = cible[0]
			var dedans: bool = cible[1].call(global_position)
			if dedans != _dedans.get(noeud, false):
				_dedans[noeud] = dedans
				noeud.emit_signal("mouse_entered" if dedans else "mouse_exited")

	## Clic : état du bouton dans Input (lu par CursorChanger) et évènement envoyé au viewport.
	func cliquer(appuye: bool) -> void:
		bouton_enfonce = appuye
		var evenement := InputEventMouseButton.new()
		evenement.button_index = MOUSE_BUTTON_LEFT
		evenement.button_mask = MOUSE_BUTTON_MASK_LEFT if appuye else 0
		evenement.pressed = appuye
		evenement.position = get_viewport().get_final_transform() * global_position
		evenement.global_position = evenement.position
		Input.parse_input_event(evenement)


var _souris: _Souris
var _pile: Label
## Agrandissement du curseur dessiné, pour qu'il se lise dans la vidéo (la position cliquée ne change pas).
const ZOOM_CURSEUR := 1.6


func titre_module() -> String:
	return "👆 Changeur du curseur · CursorChanger, CursorStyle"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-CursorChanger": _classe_changer,
		"classe-CursorStyle": _classe_style,
		"CursorChanger.activation_mode": _activation_mode,
		"CursorChanger.pressed_style": _pressed_style,
		"CursorChanger.target": _target,
		"CursorChanger.activate": _activate,
		"CursorStyle.frames_per_second": _fps,
		"CursorStyle.hotspot": _hotspot,
		"ex-chargement": _ex_chargement,
		"ex-visee": _ex_visee,
		"ex-3d": _ex_3d,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Curseur « épée » permanent ; au survol du coffre (Area2D), un second CursorChanger en mode Hover affiche la main, fermée pendant le clic.",
		"classe-CursorChanger": "La pile des curseurs : le dernier activé dessine le curseur ; quand il se retire, le précédent revient.",
		"classe-CursorStyle": "Un style fixe (épée) et un style animé (sablier, quatre images), avec leur hotspot en rouge.",
		"CursorChanger.activation_mode": "ALWAYS : dès l'entrée dans l'arbre · HOVER : au survol de la cible · MANUAL : à <code>activate()</code>.",
		"CursorChanger.pressed_style": "Tant que <code>pressed_button</code> (gauche) est enfoncé, <code>pressed_style</code> remplace <code>style</code>.",
		"CursorChanger.target": "Le CursorChanger est sous le coffre, mais <code>target</code> vise la serrure : seul son survol change le curseur.",
		"CursorChanger.activate": "<code>activate()</code> / <code>deactivate()</code> en mode Manual, avec <code>is_active()</code> et <code>is_current()</code> de chaque CursorChanger.",
		"CursorStyle.frames_per_second": "Le même sablier à 3 et à 12 images par seconde.",
		"CursorStyle.hotspot": "Le point rouge est le pixel qui clique : hotspot (0, 0) sur un viseur clique à côté de la cible, (8, 8) en plein centre.",
		"ex-chargement": "Sablier animé (mode Manual) activé pendant le chargement, puis retiré : l'épée revient.",
		"ex-visee": "<code>set_mode_visee(true)</code> active le viseur par-dessus le curseur de base, <code>false</code> le retire.",
		"ex-3d": "Un levier 3D (StaticBody3D) avec un CursorChanger en mode Hover : la main apparaît à son survol, sans script.",
	}


func schemas() -> Dictionary:
	return {
		"ex-bouton": ["formes.svg", "Chaque forme système a son image : un style à la forme Arrow ne change pas la main affichée au-dessus d'un Button (Pointing Hand)."],
	}


func reutilisations() -> Dictionary:
	return {
		"CursorChanger.style": "classe-CursorStyle",
		"CursorChanger.pressed_button": "CursorChanger.pressed_style",
		"CursorChanger.deactivate": "CursorChanger.activate",
		"CursorChanger.is_active": "CursorChanger.activate",
		"CursorChanger.is_current": "CursorChanger.activate",
		"CursorStyle.frames": "classe-CursorStyle",
		"CursorStyle.get_frame_at": "CursorStyle.frames_per_second",
		"CursorStyle.shape": "ex-bouton",
	}


func _style(images: Array, hotspot: Vector2 = Vector2.ZERO, fps: float = 10.0) -> CursorStyle:
	var style := CursorStyle.new()
	style.frames = images
	style.hotspot = hotspot
	style.frames_per_second = fps
	return style


func _sablier(fps: float = 6.0) -> CursorStyle:
	return _style([Curseurs.sablier(0), Curseurs.sablier(1), Curseurs.sablier(2), Curseurs.sablier(3)], Vector2(10, 14), fps)


func _changeur(parent: Node, mode: int, style: CursorStyle, nom: String = "") -> CursorChanger:
	var changeur := CursorChanger.new()
	changeur.activation_mode = mode
	changeur.style = style
	if nom:
		changeur.name = nom
	parent.add_child(changeur)
	_souris.changeurs.append(changeur)
	return changeur


## Prépare la souris simulée (et le picking physique de la scène) et l'affichage de la pile.
func _preparer(position: Vector2 = Vector2(980, 560)) -> void:
	get_viewport().physics_object_picking = false
	_souris = _Souris.new()
	_souris.z_index = 100
	_souris.scale = Vector2.ONE * ZOOM_CURSEUR
	_souris.position = position
	scene.add_child(_souris)
	_pile = etiquette(Vector2(576, 545), "", 16, ACCENT)
	get_tree().process_frame.connect(_maj_pile)


func _maj_pile() -> void:
	if not is_instance_valid(_pile):
		return
	var noms := PackedStringArray()
	for changeur in _souris.changeurs:
		if is_instance_valid(changeur) and changeur.is_active():
			noms.append(String(changeur.name) + (" ◀ dessine" if changeur.is_current() else ""))
	_pile.text = "Pile active : " + (", ".join(noms) if noms else "(vide : curseur du système)")


## Déplace la souris simulée vers `vers` en envoyant les mouvements à chaque image.
func _aller(vers: Vector2, duree: float = 0.5) -> void:
	var depart := _souris.position
	var t := 0.0
	while t < duree:
		await images()
		t += get_process_delta_time()
		_souris.position = depart.lerp(vers, smoothstep(0.0, 1.0, minf(t / duree, 1.0)))
		_souris.envoyer_mouvement()


func _clic(duree: float = 0.4) -> void:
	_souris.cliquer(true)
	await secondes(duree)
	_souris.cliquer(false)


## Zone survolable (Area2D + rectangle visible) centrée sur `position`.
func _zone(position: Vector2, taille: Vector2, couleur: Color, nom: String) -> Area2D:
	var zone := Area2D.new()
	zone.name = nom
	zone.position = position
	zone.input_pickable = true
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = taille
	zone.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(taille)
	dessin.color = couleur
	zone.add_child(dessin)
	scene.add_child(zone)
	etiquette(position + Vector2(0, taille.y / 2.0 + 18), nom, 16)
	_survolable(zone, Rect2(position - taille / 2.0, taille))
	return zone


## Déclare `noeud` survolable par la souris simulée dans le rectangle `zone` (coordonnées de la scène).
func _survolable(noeud: Node, zone: Rect2) -> void:
	_souris.cibles.append([noeud, func(p: Vector2) -> bool: return zone.has_point(p)])


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : un curseur de base et un curseur au survol")
	_preparer()
	arbre(Vector2(40, 140), PackedStringArray(["Niveau", "  CursorChanger  (Always, épée)", "  Coffre (Area2D)", "    CursorChanger  (Hover, main)"]))
	var base := _changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.epee()], Vector2(2, 2)), "Épée")
	var coffre := _zone(Vector2(760, 320), Vector2(160, 110), Color("8a5a3c"), "Coffre")
	var main := _changeur(coffre, CursorChanger.ACTIVATION_HOVER, _style([Curseurs.main_ouverte()], Vector2(12, 2)), "Main")
	main.pressed_style = _style([Curseurs.main_fermee()], Vector2(12, 8))
	code("style.frames = [preload(\"res://curseurs/epee.png\")]\nstyle.hotspot = Vector2(2, 2)")
	legende("1. CursorChanger permanent à la racine : le curseur du jeu devient une épée")
	await _aller(Vector2(450, 420), 1.0)
	await secondes(0.6)
	legende("2-3. Au survol du coffre, le CursorChanger « Hover » passe au sommet de la pile")
	await _aller(Vector2(750, 320), 0.8)
	await secondes(0.8)
	legende("4. pressed_style : la main se ferme pendant le clic")
	await _clic(0.6)
	await secondes(0.4)
	legende("En sortant, il se retire : l'épée revient toute seule")
	await _aller(Vector2(470, 200), 0.8)
	await secondes(1.0)
	base.name = "Épée"


# 💻 API

func _classe_changer() -> void:
	sujet("CursorChanger : une pile de curseurs")
	_preparer(Vector2(576, 330))
	var base := _changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.epee()], Vector2(2, 2)), "Épée (Always)")
	var visee := _changeur(scene, CursorChanger.ACTIVATION_MANUAL, _style([Curseurs.viseur()], Vector2(16, 16)), "Viseur (Manual)")
	var sablier := _changeur(scene, CursorChanger.ACTIVATION_MANUAL, _sablier(), "Sablier (Manual)")
	await _aller(Vector2(576, 300), 0.3)
	for etape in [["Viseur.activate()", visee, true], ["Sablier.activate()", sablier, true], ["Sablier.deactivate()", sablier, false], ["Viseur.deactivate()", visee, false]]:
		await secondes(1.2)
		code(etape[0])
		legende(etape[0] + (" : passe au sommet de la pile" if etape[2] else " : le précédent reprend la main"))
		if etape[2]:
			etape[1].activate()
		else:
			etape[1].deactivate()
	await secondes(1.4)
	base.name = "Épée (Always)"


func _classe_style() -> void:
	sujet("CursorStyle : image fixe ou animée, et hotspot")
	var styles := [_style([Curseurs.epee()], Vector2(2, 2)), _sablier(6.0)]
	var noms := ["frames = [épée] · hotspot (2, 2)", "frames = 4 images · 6 images/s"]
	var sprites: Array[Sprite2D] = []
	for i in 2:
		var sprite := Sprite2D.new()
		sprite.centered = false
		sprite.scale = Vector2(4, 4)
		sprite.position = Vector2(300 + i * 430, 230)
		scene.add_child(sprite)
		sprites.append(sprite)
		var point := disque(sprite.position + styles[i].hotspot * 4.0, MAUVAIS, 6)
		point.z_index = 5
		etiquette(Vector2(364 + i * 430, 440), noms[i], 17, VARIANTES[i])
	code("var style := CursorStyle.new()\nstyle.frames = [image_1, image_2, image_3, image_4]\nstyle.frames_per_second = 6.0\nstyle.hotspot = Vector2(10, 14)")
	legende("En rouge : le hotspot, le pixel qui clique")
	var debut := Time.get_ticks_msec()
	while Time.get_ticks_msec() - debut < 6000:
		var t := (Time.get_ticks_msec() - debut) / 1000.0
		for i in 2:
			sprites[i].texture = styles[i].get_frame_at(t)
		await images()


func _activation_mode() -> void:
	sujet("activation_mode : quand le curseur est appliqué")
	_preparer(Vector2(200, 560))
	_changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.epee()], Vector2(2, 2)), "Épée (ALWAYS)")
	var zone := _zone(Vector2(420, 300), Vector2(200, 140), Formes.DECOR, "Zone (HOVER)")
	_changeur(zone, CursorChanger.ACTIVATION_HOVER, _style([Curseurs.main_ouverte()], Vector2(12, 2)), "Main (HOVER)")
	var manuel := _changeur(scene, CursorChanger.ACTIVATION_MANUAL, _style([Curseurs.viseur()], Vector2(16, 16)), "Viseur (MANUAL)")
	legende("ALWAYS : actif dès l'entrée dans l'arbre")
	code("activation_mode = ACTIVATION_ALWAYS")
	await _aller(Vector2(220, 300), 0.8)
	await secondes(0.8)
	legende("HOVER : actif seulement pendant le survol de la cible")
	code("activation_mode = ACTIVATION_HOVER")
	await _aller(Vector2(420, 300), 0.8)
	await secondes(1.0)
	await _aller(Vector2(800, 300), 0.8)
	legende("MANUAL : actif entre activate() et deactivate()")
	code("activation_mode = ACTIVATION_MANUAL\n$Viseur.activate()")
	await secondes(0.5)
	manuel.activate()
	await _aller(Vector2(900, 360), 1.0)
	manuel.deactivate()
	code("$Viseur.deactivate()")
	await secondes(1.2)


func _pressed_style() -> void:
	sujet("pressed_style et pressed_button")
	_preparer(Vector2(576, 320))
	var main := _changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.main_ouverte()], Vector2(12, 2)), "Main")
	main.pressed_style = _style([Curseurs.main_fermee()], Vector2(12, 8))
	main.pressed_button = MOUSE_BUTTON_LEFT
	code("main.pressed_style = style_main_fermee\nmain.pressed_button = MOUSE_BUTTON_LEFT")
	var objet := carre(Vector2(576, 320), Color("8a5a3c"), 70)
	for i in 3:
		await _aller(Vector2(560 + i * 20, 330), 0.4)
		legende("Bouton gauche enfoncé : la main se ferme (et attrape)")
		_souris.cliquer(true)
		var depart := _souris.position
		await _aller(depart + Vector2(-200 + i * 180, -60), 0.7)
		objet.position = _souris.position
		legende("Relâché : la main s'ouvre")
		_souris.cliquer(false)
		await secondes(0.6)


func _target() -> void:
	sujet("target : le nœud dont le survol active le curseur")
	_preparer(Vector2(300, 520))
	_changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.epee()], Vector2(2, 2)), "Épée")
	var coffre := _zone(Vector2(576, 300), Vector2(280, 170), Color("8a5a3c"), "Coffre")
	var serrure := Area2D.new()
	serrure.name = "Serrure"
	serrure.position = Vector2(0, 10)
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(46, 46)
	serrure.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(46, 46))
	dessin.color = Color("ffd166")
	serrure.add_child(dessin)
	coffre.add_child(serrure)
	_survolable(serrure, Rect2(Vector2(576, 310) - Vector2(23, 23), Vector2(46, 46)))
	var main := _changeur(coffre, CursorChanger.ACTIVATION_HOVER, _style([Curseurs.main_ouverte()], Vector2(12, 2)), "Main")
	main.target = serrure
	arbre(Vector2(40, 140), PackedStringArray(["Coffre (Area2D)", "  Serrure (Area2D)", "  CursorChanger"]))
	code("$Coffre/CursorChanger.target = $Coffre/Serrure")
	legende("Survol du coffre : rien ne change")
	await _aller(Vector2(480, 260), 0.8)
	await secondes(0.8)
	legende("Survol de la serrure (target) : la main apparaît")
	await _aller(Vector2(576, 310), 0.6)
	await secondes(1.0)
	await _aller(Vector2(680, 250), 0.6)
	await secondes(0.8)


func _activate() -> void:
	sujet("activate(), deactivate(), is_active(), is_current()")
	_preparer(Vector2(576, 330))
	var etats := etiquette(Vector2(576, 140), "", 17, TEXTE)
	var changeurs := [_changeur(scene, CursorChanger.ACTIVATION_MANUAL, _style([Curseurs.epee()], Vector2(2, 2)), "A"),
		_changeur(scene, CursorChanger.ACTIVATION_MANUAL, _style([Curseurs.viseur()], Vector2(16, 16)), "B")]
	get_tree().process_frame.connect(func():
		if is_instance_valid(etats):
			etats.text = "A : is_active() = %s, is_current() = %s\nB : is_active() = %s, is_current() = %s" % [
				changeurs[0].is_active(), changeurs[0].is_current(), changeurs[1].is_active(), changeurs[1].is_current()])
	await _aller(Vector2(576, 320), 0.3)
	for etape in [["A.activate()", 0, true], ["B.activate()", 1, true], ["A.activate()  # repasse devant", 0, true], ["A.deactivate()", 0, false], ["B.deactivate()", 1, false]]:
		await secondes(1.1)
		code(etape[0])
		legende(etape[0])
		if etape[2]:
			changeurs[etape[1]].activate()
		else:
			changeurs[etape[1]].deactivate()
	await secondes(1.2)


func _fps() -> void:
	sujet("frames_per_second : vitesse de l'animation")
	var styles := [_sablier(3.0), _sablier(12.0)]
	var sprites: Array[Sprite2D] = []
	for i in 2:
		var sprite := Sprite2D.new()
		sprite.scale = Vector2(4, 4)
		sprite.position = Vector2(380 + i * 400, 280)
		scene.add_child(sprite)
		sprites.append(sprite)
		etiquette(Vector2(380 + i * 400, 400), ["3 images/s", "12 images/s"][i], 18, VARIANTES[i])
	code("style.frames_per_second = 12.0\nstyle.get_frame_at(temps)  # image à afficher")
	var debut := Time.get_ticks_msec()
	while Time.get_ticks_msec() - debut < 6000:
		for i in 2:
			sprites[i].texture = styles[i].get_frame_at((Time.get_ticks_msec() - debut) / 1000.0)
		await images()


func _hotspot() -> void:
	sujet("hotspot : le pixel qui clique")
	_preparer(Vector2(300, 500))
	var visee := _changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.viseur()], Vector2.ZERO), "Viseur")
	var cible := disque(Vector2(576, 300), MAUVAIS, 26)
	var impact := disque(Vector2(-100, -100), Color.WHITE, 5)
	impact.z_index = 120
	for hotspot in [Vector2.ZERO, Vector2(16, 16)]:
		visee.style = _style([Curseurs.viseur()], hotspot)
		code("style.hotspot = Vector2(%d, %d)" % [hotspot.x, hotspot.y])
		legende("hotspot (0, 0) : le clic part du coin de l'image, à côté de la cible" if hotspot == Vector2.ZERO else "hotspot (8, 8) × 2 : le clic part du centre du viseur")
		var vise: Vector2 = Vector2(576, 300) - (Vector2(16, 16) - hotspot)
		await _aller(vise, 0.9)
		await secondes(0.4)
		impact.position = _souris.position
		await _clic(0.2)
		await secondes(1.4)


# 📌 Exemples

func _ex_chargement() -> void:
	sujet("Exemple : un sablier animé pendant un chargement")
	_preparer(Vector2(560, 300))
	_changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.epee()], Vector2(2, 2)), "Épée")
	var sablier := _changeur(scene, CursorChanger.ACTIVATION_MANUAL, _sablier(8.0), "Sablier")
	var barre := ProgressBar.new()
	barre.position = Vector2(376, 400)
	barre.size = Vector2(400, 26)
	scene.add_child(barre)
	code("sablier.activate()\nResourceLoader.load_threaded_request(chemin)\n# ... attendre la fin du chargement\nsablier.deactivate()")
	await _aller(Vector2(576, 300), 0.3)
	for i in 2:
		barre.value = 0
		await secondes(0.8)
		legende("charger() : le sablier passe au-dessus de tous les curseurs")
		sablier.activate()
		while barre.value < 100:
			barre.value += 1.2
			await images()
		sablier.deactivate()
		legende("Chargement terminé : deactivate(), le curseur précédent revient")
		await secondes(1.2)


func _ex_visee() -> void:
	sujet("Exemple : un curseur de visée selon l'état du jeu")
	_preparer(Vector2(400, 300))
	_changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.epee()], Vector2(2, 2)), "Épée")
	var visee := _changeur(scene, CursorChanger.ACTIVATION_MANUAL, _style([Curseurs.viseur()], Vector2(16, 16)), "CurseurVisee")
	visee.pressed_style = _style([Curseurs.viseur(true)], Vector2(16, 16))
	var etat := etiquette(Vector2(576, 150), "", 20, ACCENT)
	code("func set_mode_visee(actif: bool) -> void:\n\tif actif: $CurseurVisee.activate()\n\telse: $CurseurVisee.deactivate()")
	for i in 2:
		etat.text = "Mode exploration"
		await _aller(Vector2(500, 330), 0.6)
		await secondes(0.4)
		etat.text = "set_mode_visee(true)"
		visee.activate()
		for cible in [Vector2(700, 280), Vector2(800, 380)]:
			await _aller(cible, 0.5)
			await _clic(0.15)
		etat.text = "set_mode_visee(false)"
		visee.deactivate()
		await secondes(0.8)


func _ex_3d() -> void:
	sujet("Exemple : changer le curseur au survol d'un objet 3D")
	_preparer(Vector2(250, 300))
	_changeur(scene, CursorChanger.ACTIVATION_ALWAYS, _style([Curseurs.epee()], Vector2(2, 2)), "Épée")
	var vue := vue_3d(Rect2(126, 110, 900, 400))
	cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(12, 0.2, 8))
	var levier := StaticBody3D.new()
	levier.name = "Levier"
	levier.position = Vector3(0, 0.75, 0)
	var forme := CollisionShape3D.new()
	forme.shape = BoxShape3D.new()
	forme.shape.size = Vector3(0.6, 1.5, 0.6)
	levier.add_child(forme)
	cube(levier, Vector3.ZERO, ACCENT, Vector3(0.3, 1.5, 0.3))
	vue.add_child(levier)
	vue.physics_object_picking = false
	_changeur(levier, CursorChanger.ACTIVATION_HOVER, _style([Curseurs.main_ouverte()], Vector2(12, 2)), "Main (Hover)")
	# Zone survolée : le levier projeté par la caméra, dans le cadre de la vue (126, 110)
	var camera := vue.get_camera_3d()
	_souris.cibles.append([levier, func(p: Vector2) -> bool:
		var haut := camera.unproject_position(levier.global_position + Vector3(0, 0.75, 0))
		var bas := camera.unproject_position(levier.global_position - Vector3(0, 0.75, 0))
		return Rect2(Vector2(126, 110) + Vector2(minf(haut.x, bas.x) - 25, haut.y), Vector2(absf(haut.x - bas.x) + 50, bas.y - haut.y)).has_point(p)])
	code("# Levier (StaticBody3D + CollisionShape3D)\n# └─ CursorChanger (Hover, style = main)\n# Rien à scripter")
	for i in 2:
		legende("Survol du levier : la main apparaît")
		await _aller(Vector2(576, 300), 0.9)
		await secondes(1.0)
		legende("Hors du levier : l'épée revient")
		await _aller(Vector2(300 + i * 500, 420), 0.9)
		await secondes(0.8)
