extends Node

## 🎥 Socle des séquences filmées de la documentation : une séquence (un « plan ») = une vidéo.
##
## Un module déclare ses plans dans un script qui étend celui-ci et surcharge plans() :
##   func plans() -> Dictionary:
##       return {"mode-emploi": _mode_emploi, "FXShake.duration": _duree}
## Chaque plan est une coroutine : il construit sa scène sous `scene` (2D) ou dans une vue_3d(), l'anime,
## et la vidéo s'arrête à son retour. Les noms de plans sont ceux de la page : « mode-emploi »,
## « classe-<Classe> », « <Classe>.<membre> » (tableaux d'API), l'id des exemples (« ex-... ») et
## « figure-N » (remplace le N-ième emplacement « Image à insérer » de la page).
## Les légendes (legendes()) et les schémas (schemas()) sont insérés dans la page par
## « generer_medias.py integrer », lancé aussi après « videos ».
##
## Filmé par Documentation/generer_medias.py (capture.tscn + --write-movie), en 1152×648 logiques.

const Formes := preload("res://🎥Démos de documentation/socle/formes.gd")
const TAILLE := Vector2(1152, 648)
const FOND := Formes.FOND
const TEXTE := Formes.TEXTE
const TEXTE_DOUX := Formes.TEXTE_DOUX
const ACCENT := Formes.ACCENT
const BON := Formes.BON
const MAUVAIS := Formes.MAUVAIS
## Couleurs des objets comparés côte à côte (variantes d'un réglage).
const VARIANTES := Formes.VARIANTES

## Contenu 2D du plan en cours (dessous de l'interface).
var scene: Node2D

var _interface: CanvasLayer
var _titre: Label
var _sous_titre: Label
var _legende: Label
var _code: PanelContainer
var _code_texte: Label
var _police_code: Font


## Plans du module : { nom : Callable (coroutine sans argument) }.
func plans() -> Dictionary:
	return {}


## Légendes affichées sous les vidéos dans la page (HTML permis) : { nom du plan : "légende" }.
func legendes() -> Dictionary:
	return {}


## Schémas SVG (membres sans effet visible, captures d'éditeur...) : { clé : ["fichier.svg", "légende"] }.
## Clé : « Classe » ou « Classe.membre » (carte / ligne d'API), ou « figure-N » (N-ième emplacement
## « Image à insérer » de la page). Fichiers dans le dossier « schemas/ » à côté du script de plans ;
## dessin en currentColor et var(--accent) pour suivre le thème de la documentation.
func schemas() -> Dictionary:
	return {}


## Lignes d'API montrées par un plan ou un schéma déjà fait : { "Classe.membre" : "plan ou clé de schéma" }
## ou { ... : ["plan ou clé de schéma", "légende propre"] } (évite de filmer deux fois la même chose).
func reutilisations() -> Dictionary:
	return {}


## Titre du module affiché en haut à gauche de chaque vidéo.
func titre_module() -> String:
	return ""


func _ready() -> void:
	RenderingServer.set_default_clear_color(FOND)
	var mono := SystemFont.new()
	mono.font_names = PackedStringArray(["JetBrains Mono", "DejaVu Sans Mono", "monospace"])
	_police_code = mono

	scene = Node2D.new()
	scene.name = "Scene"
	add_child(scene)
	var grille := Formes.Grille.new()
	grille.z_index = -100
	scene.add_child(grille)

	_interface = CanvasLayer.new()
	_interface.layer = 10
	add_child(_interface)

	_titre = _etiquette(_interface, Vector2(28, 18), 26, TEXTE)
	_titre.text = titre_module()
	_sous_titre = _etiquette(_interface, Vector2(28, 54), 18, ACCENT)

	_legende = _etiquette(_interface, Vector2(0, TAILLE.y - 64), 22, TEXTE)
	_legende.size = Vector2(TAILLE.x, 40)
	_legende.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER

	_code = PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0, 0, 0, 0.55)
	style.border_color = Color(1, 1, 1, 0.12)
	style.set_border_width_all(1)
	style.set_corner_radius_all(8)
	style.set_content_margin_all(12)
	_code.add_theme_stylebox_override("panel", style)
	_code.position = Vector2(TAILLE.x - 28, 18)
	_code.visible = false
	_interface.add_child(_code)
	_code_texte = Label.new()
	_code_texte.add_theme_font_override("font", _police_code)
	_code_texte.add_theme_font_size_override("font_size", 16)
	_code_texte.add_theme_color_override("font_color", Color("cfe3ff"))
	_code.add_child(_code_texte)


## Joue le plan `nom` (attendre son retour : la vidéo peut alors s'arrêter).
func jouer(nom: String) -> bool:
	var liste := plans()
	if not liste.has(nom):
		push_error("Plan inconnu : %s (plans : %s)" % [nom, ", ".join(PackedStringArray(liste.keys()))])
		return false
	await liste[nom].call()
	await secondes(0.4)
	return true


# 🧾 Textes à l'écran

## Sous-titre (sous le nom du module) : ce que montre la vidéo.
func sujet(texte: String) -> void:
	_sous_titre.text = texte


## Légende en bas de l'écran (vide : masquée).
func legende(texte: String) -> void:
	_legende.text = texte


## Bulle de code en haut à droite (vide : masquée) : la ligne de script qui produit ce qu'on voit.
func code(texte: String) -> void:
	_code.visible = not texte.is_empty()
	_code_texte.text = texte
	_code.reset_size()
	_code.position.x = TAILLE.x - 28 - _code.size.x
	# Bulle qui toucherait le titre ou le sujet : en dessous plutôt qu'à côté
	var fin_textes := maxf(_titre.position.x + _titre.get_minimum_size().x, _sous_titre.position.x + _sous_titre.get_minimum_size().x)
	_code.position.y = 92.0 if _code.position.x < fin_textes + 24.0 else 18.0


## Étiquette fixe dans la scène 2D (centrée sur `position`).
func etiquette(position: Vector2, texte: String, taille: int = 18, couleur: Color = TEXTE_DOUX) -> Label:
	var label := _etiquette(scene, Vector2.ZERO, taille, couleur)
	label.text = texte
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.size = Vector2(640, label.get_minimum_size().y)
	label.position = position - label.size / 2.0
	return label


## Arborescence de nœuds dessinée à l'écran (une ligne par nœud, indentation par 2 espaces).
func arbre(position: Vector2, lignes: PackedStringArray) -> Label:
	var label := _etiquette(scene, position, 17, Color("cfe3ff"))
	label.add_theme_font_override("font", _police_code)
	var texte := ""
	for ligne in lignes:
		var profondeur := ligne.length() - ligne.lstrip(" ").length()
		texte += ("   ".repeat(profondeur / 2 - 1) + "└─ " if profondeur > 0 else "") + ligne.strip_edges() + "\n"
	label.text = texte
	return label


# 🧱 Formes (voir formes.gd)

## Carré coloré (Node2D) centré sur `position` : la cible la plus simple d'un effet 2D.
func carre(position: Vector2, couleur: Color, cote: float = 80.0) -> Node2D:
	return Formes.carre(scene, position, couleur, cote)


## Disque coloré (Node2D) centré sur `position`.
func disque(position: Vector2, couleur: Color, rayon: float = 40.0) -> Node2D:
	return Formes.disque(scene, position, couleur, rayon)


## Écran de jeu 2D encadré dans `cadre` ; sa Camera2D : vue.get_meta("camera").
func vue_2d(cadre: Rect2, fond: Color = Formes.FOND_VUE) -> SubViewport:
	return Formes.vue_2d(scene, cadre, fond)


## Vue 3D encadrée dans `cadre` ; sa caméra : get_camera_3d().
func vue_3d(cadre: Rect2) -> SubViewport:
	return Formes.vue_3d(scene, cadre)


## Pavé 3D coloré.
func cube(parent: Node, position: Vector3, couleur: Color, taille: Vector3 = Vector3.ONE) -> MeshInstance3D:
	return Formes.cube(parent, position, couleur, taille)


# 🖱️ Souris simulée. Évènements envoyés au viewport principal (les SubViewportContainer les transmettent ;
# poussé directement dans un SubViewport en conteneur, un évènement n'atteint pas ses Control dans la fenêtre
# filmée), ou à une vue_libre. get_mouse_position() d'un viewport de la fenêtre (même dans un conteneur) est
# la vraie souris : pour un nœud qui la lit, utiliser une vue_libre.

## Position dans la scène (viewport principal) d'un point `local` du canvas d'un nœud, même dans un SubViewport.
func position_ecran(noeud: CanvasItem, local: Vector2 = Vector2.ZERO) -> Vector2:
	var point := noeud.get_global_transform_with_canvas() * local
	var vue := noeud.get_viewport()
	while vue is SubViewport and vue.get_parent() is SubViewportContainer:
		var conteneur: SubViewportContainer = vue.get_parent()
		point = conteneur.get_global_transform_with_canvas() * (point / float(conteneur.stretch_shrink))
		vue = conteneur.get_viewport()
	return point


## Centre d'un Control, en position de scène.
func centre_controle(controle: Control) -> Vector2:
	return position_ecran(controle, controle.size / 2.0)


## Écran de jeu 2D « libre » (voir Formes.vue_libre) : sa position de souris suit les évènements simulés.
func vue_libre(cadre: Rect2, fond: Color = Formes.FOND_VUE) -> SubViewport:
	return Formes.vue_libre(scene, cadre, fond)


## Déplacement de souris simulé jusqu'à `position` (scène). `vue` : une vue_libre qui reçoit l'évènement
## directement (en coordonnées locales) ; sinon le viewport principal.
func souris_mouvement(position: Vector2, vue: SubViewport = null) -> void:
	var evenement := InputEventMouseMotion.new()
	evenement.position = position - (vue.get_meta("cadre").position if vue else Vector2.ZERO)
	evenement.global_position = evenement.position
	(vue if vue else get_viewport()).push_input(evenement, true)


## Clic gauche simulé (appui puis relâchement) à `position` (scène), dans `vue` si elle est donnée.
func souris_clic(position: Vector2, duree: float = 0.08, vue: SubViewport = null) -> void:
	souris_mouvement(position, vue)
	for appuye in [true, false]:
		var evenement := InputEventMouseButton.new()
		evenement.button_index = MOUSE_BUTTON_LEFT
		evenement.button_mask = MOUSE_BUTTON_MASK_LEFT if appuye else 0
		evenement.pressed = appuye
		evenement.position = position - (vue.get_meta("cadre").position if vue else Vector2.ZERO)
		evenement.global_position = evenement.position
		(vue if vue else get_viewport()).push_input(evenement, true)
		if appuye:
			await secondes(duree)
		else:
			await images()


## Explosion (disque qui grandit et s'efface) dans `parent` à `position`.
func explosion(parent: Node, position: Vector2, rayon: float = 60.0, couleur: Color = Color(1, 0.75, 0.3)) -> void:
	var cercle := Node2D.new()
	cercle.position = position
	cercle.draw.connect(func(): cercle.draw_circle(Vector2.ZERO, rayon, couleur))
	parent.add_child(cercle)
	cercle.scale = Vector2.ONE * 0.2
	var animation := cercle.create_tween().set_parallel()
	animation.tween_property(cercle, "scale", Vector2.ONE, 0.25).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	animation.tween_property(cercle, "modulate:a", 0.0, 0.45)
	animation.chain().tween_callback(cercle.queue_free)


## Bouton d'interface (hors conteneur) centré sur `position`.
func bouton(position: Vector2, texte: String, taille: Vector2 = Vector2(180, 56)) -> Button:
	var resultat := Button.new()
	resultat.text = texte
	resultat.add_theme_font_size_override("font_size", 22)
	resultat.size = taille
	resultat.position = position - taille / 2.0
	resultat.pivot_offset = taille / 2.0
	scene.add_child(resultat)
	return resultat


## Pointeur de souris simulé (flèche dessinée) déplacé en `duree` secondes vers `vers`.
func pointeur() -> Node2D:
	var fleche := Polygon2D.new()
	fleche.polygon = PackedVector2Array([Vector2(0, 0), Vector2(0, 26), Vector2(7, 20), Vector2(12, 31), Vector2(17, 29), Vector2(12, 18), Vector2(21, 18)])
	fleche.color = Color.WHITE
	var contour := Line2D.new()
	contour.points = fleche.polygon
	contour.closed = true
	contour.width = 1.5
	contour.default_color = Color.BLACK
	fleche.add_child(contour)
	fleche.z_index = 50
	scene.add_child(fleche)
	return fleche


func deplacer(noeud: Node2D, vers: Vector2, duree: float = 0.5) -> void:
	var animation := noeud.create_tween()
	animation.tween_property(noeud, "position", vers, duree).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	await animation.finished


# ⏱️ Temps

func secondes(duree: float) -> void:
	await get_tree().create_timer(duree, true, false, true).timeout


func images(nombre: int = 1) -> void:
	for i in nombre:
		await get_tree().process_frame


## Vide la scène 2D et les textes (entre deux parties d'un même plan).
func effacer() -> void:
	for enfant in scene.get_children():
		if not enfant is Formes.Grille:
			enfant.queue_free()
	legende("")
	code("")


func _etiquette(parent: Node, position: Vector2, taille: int, couleur: Color) -> Label:
	var label := Label.new()
	label.position = position
	label.add_theme_font_size_override("font_size", taille)
	label.add_theme_color_override("font_color", couleur)
	label.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.6))
	label.add_theme_constant_override("outline_size", 4)
	parent.add_child(label)
	return label
