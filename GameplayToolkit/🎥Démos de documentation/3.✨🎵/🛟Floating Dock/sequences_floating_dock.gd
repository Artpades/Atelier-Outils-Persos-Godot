extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 🛟 Plans filmés de la page Floating Dock (effets/floating-dock.html).
##
## FloatingDock lit get_local_mouse_position() : dans la fenêtre filmée ce serait la vraie souris. Le dock est
## donc placé dans une vue_libre (SubViewport hors conteneur) qui reçoit directement les évènements simulés.

const COULEURS := [Color("4ea1ff"), Color("f2a33a"), Color("c678dd"), Color("5ccf7a"), Color("ef5f5f"), Color("ffd166"), Color("56d6e0")]

var _fleche: Node2D
var _vues: Array[SubViewport] = []


func titre_module() -> String:
	return "🛟 Floating Dock · FloatingDock"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-FloatingDock": _classe,
		"FloatingDock.magnification": _magnification,
		"FloatingDock.influence_radius": _influence_radius,
		"FloatingDock.hover_spacing": _hover_spacing,
		"FloatingDock.hover_lift": _hover_lift,
		"FloatingDock.item_anchor": _item_anchor,
		"FloatingDock.resize_mode": _resize_mode,
		"FloatingDock.smoothing": _smoothing,
		"FloatingDock.hovered_item_changed": _hovered_item_changed,
		"ex-vertical": _ex_vertical,
		"ex-texte": _ex_texte,
		"ex-manette": _ex_manette,
		"ex-raccourcis": _ex_raccourcis,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Un FloatingDock ancré en bas, sept icônes : elles grossissent sous le pointeur et le nom de l'outil survolé s'affiche.",
		"classe-FloatingDock": "Les éléments proches du pointeur grossissent et s'écartent, en douceur.",
		"FloatingDock.magnification": "1,3, 1,8 (défaut) et 2,5 : l'échelle de l'élément sous le pointeur.",
		"FloatingDock.influence_radius": "80, 160 (défaut) et 320 px : jusqu'où les voisins grossissent encore.",
		"FloatingDock.hover_spacing": "0 et 24 px : l'écart ajouté autour des éléments grossis.",
		"FloatingDock.hover_lift": "0 et 14 px : les éléments influencés « sautent » vers le haut.",
		"FloatingDock.item_anchor": "BEGIN, CENTER et END (défaut) : le bord collé, donc le sens du grossissement.",
		"FloatingDock.resize_mode": "RESIZE_SIZE agrandit la taille (le texte garde sa police) ; RESIZE_SCALE met tout à l'échelle.",
		"FloatingDock.smoothing": "4, 12 (défaut) et 0 (immédiat) : la rapidité de l'animation.",
		"FloatingDock.hovered_item_changed": "<code>hovered_item_changed</code> à chaque changement d'élément survolé, puis avec null en quittant le dock.",
		"ex-vertical": "Dock vertical collé à gauche, qui grossit vers la droite, avec <code>hover_lift</code>.",
		"ex-texte": "Des boutons de menu avec texte : en RESIZE_SCALE le texte grossit avec eux.",
		"ex-manette": "Sans souris, <code>follow_focus</code> grossit l'élément qui a le focus : ici déplacé avec les flèches.",
		"ex-raccourcis": "Chaque bouton du dock lance son raccourci au clic.",
	}


func reutilisations() -> Dictionary:
	return {
		"FloatingDock.item_size": "classe-FloatingDock",
		"FloatingDock.separation": "classe-FloatingDock",
		"FloatingDock.vertical": "ex-vertical",
		"FloatingDock.follow_focus": "ex-manette",
		"FloatingDock.hover_margin": ["classe-FloatingDock", "<code>hover_margin</code> : le pointeur compte encore comme au-dessus du dock un peu au-delà de ses bords, le grossissement ne s'arrête pas net."],
		"FloatingDock.get_hovered_item": "FloatingDock.hovered_item_changed",
		"FloatingDock.get_item_influence": ["FloatingDock.hovered_item_changed", "<code>get_item_influence(i)</code> : 0 au repos, 1 au grossissement maximal (barres sous les icônes)."],
	}


## Vue contenant un FloatingDock (ancré en bas au centre) de `nombre` icônes ; renvoie le dock.
func _dock(cadre: Rect2, reglages: Dictionary = {}, nombre: int = 7) -> FloatingDock:
	var vue := vue_libre(cadre)
	_vues.append(vue)
	var interface := Control.new()
	interface.size = cadre.size
	interface.mouse_filter = Control.MOUSE_FILTER_IGNORE
	vue.add_child(interface)
	var dock := FloatingDock.new()
	for propriete in reglages:
		dock.set(propriete, reglages[propriete])
	for i in nombre:
		var icone := TextureRect.new()
		icone.texture = Formes.dessin(Vector2i(64, 64), COULEURS[i % COULEURS.size()], [[Rect2i(16, 16, 32, 32), COULEURS[i % COULEURS.size()].lightened(0.4)]])
		icone.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		icone.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		icone.tooltip_text = ["Pinceau", "Gomme", "Seau", "Texte", "Pipette", "Loupe", "Calques"][i % 7]
		dock.add_child(icone)
	interface.add_child(dock)
	if dock.vertical:
		dock.set_anchors_and_offsets_preset(Control.PRESET_CENTER_LEFT, Control.PRESET_MODE_MINSIZE, 20)
	else:
		dock.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM, Control.PRESET_MODE_MINSIZE, 20)
	return dock


## Mouvement simulé envoyé à toutes les vues libres (chacune le reçoit dans ses coordonnées).
func _souris(position: Vector2) -> void:
	for vue in _vues:
		souris_mouvement(position, vue)


func _preparer_fleche() -> void:
	_fleche = pointeur()
	_fleche.position = Vector2(576, 620)


## Balaye le dock (position de scène) de gauche à droite et retour.
func _balayer(debut: Vector2, fin: Vector2, duree: float = 2.2) -> void:
	for cible in [debut, fin, debut]:
		var depart := _fleche.position
		var t := 0.0
		var d := duree if depart.distance_to(cible) > 200 else 0.5
		while t < d:
			await images()
			t += get_process_delta_time()
			_fleche.position = depart.lerp(cible, smoothstep(0.0, 1.0, minf(t / d, 1.0)))
			_souris(_fleche.position)


## Dock de comparaison par colonne (vues côte à côte) : la souris passe sur chacun.
func _comparer(propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {}) -> void:
	var n := valeurs.size()
	var hauteur := minf(150.0, 430.0 / n - 20.0)
	var docks: Array[FloatingDock] = []
	for i in n:
		var cadre := Rect2(76, 110 + i * (hauteur + 34), 1000, hauteur)
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		reglages["item_size"] = Vector2(44, 44)
		docks.append(_dock(cadre, reglages))
		etiquette(Vector2(150, cadre.position.y + 16), textes[i], 15, VARIANTES[i % VARIANTES.size()])
	_preparer_fleche()
	for i in n:
		var y := 110 + i * (hauteur + 34) + hauteur - 44
		await _balayer(Vector2(380, y), Vector2(780, y), 1.6)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : un dock en bas de l'écran")
	var dock := _dock(Rect2(76, 150, 1000, 380))
	var nom := etiquette(Vector2(576, 230), "", 26, ACCENT)
	dock.hovered_item_changed.connect(func(element): nom.text = element.tooltip_text if element else "")
	code("$Dock.hovered_item_changed.connect(func(element):\n\t$NomOutil.text = element.tooltip_text if element else \"\")")
	_preparer_fleche()
	legende("item_size 64 · magnification 1,8 · influence_radius 160")
	await _balayer(Vector2(330, 480), Vector2(820, 480), 3.0)
	await deplacer(_fleche, Vector2(576, 300), 0.5)
	_souris(_fleche.position)
	legende("Le pointeur quitte le dock : tout revient au repos")
	await secondes(1.2)


# 💻 API

func _classe() -> void:
	sujet("FloatingDock : grossissement façon dock")
	_dock(Rect2(76, 150, 1000, 380))
	_preparer_fleche()
	code("$Dock.item_size = Vector2(64, 64)\n$Dock.separation = 8.0")
	await _balayer(Vector2(330, 480), Vector2(820, 480), 2.6)


func _magnification() -> void:
	sujet("magnification : échelle sous le pointeur")
	code("$Dock.magnification = 1.8")
	await _comparer("magnification", [1.3, 1.8, 2.5], ["1,3", "1,8 (défaut)", "2,5"])


func _influence_radius() -> void:
	sujet("influence_radius : portée du grossissement")
	code("$Dock.influence_radius = 160.0")
	await _comparer("influence_radius", [80.0, 160.0, 320.0], ["80 px", "160 px (défaut)", "320 px"])


func _hover_spacing() -> void:
	sujet("hover_spacing : écart autour des éléments grossis")
	code("$Dock.hover_spacing = 12.0")
	await _comparer("hover_spacing", [0.0, 24.0], ["0", "24 px"])


func _hover_lift() -> void:
	sujet("hover_lift : les éléments « sautent »")
	code("$Dock.hover_lift = 14.0")
	await _comparer("hover_lift", [0.0, 14.0], ["0 (défaut)", "14 px"])


func _item_anchor() -> void:
	sujet("item_anchor : bord collé, sens du grossissement")
	code("$Dock.item_anchor = FloatingDock.ANCHOR_END")
	await _comparer("item_anchor", [FloatingDock.ANCHOR_BEGIN, FloatingDock.ANCHOR_CENTER, FloatingDock.ANCHOR_END],
		["ANCHOR_BEGIN", "ANCHOR_CENTER", "ANCHOR_END (défaut)"])


func _smoothing() -> void:
	sujet("smoothing : rapidité de l'animation")
	code("$Dock.smoothing = 12.0")
	await _comparer("smoothing", [4.0, 12.0, 0.0], ["4", "12 (défaut)", "0 : immédiat"])


func _resize_mode() -> void:
	sujet("resize_mode : taille ou échelle")
	await _menus_texte()


## Deux menus de boutons texte, RESIZE_SIZE puis RESIZE_SCALE, survolés l'un après l'autre.
func _menus_texte() -> void:
	var docks: Array[FloatingDock] = []
	for i in 2:
		var cadre := Rect2(76, 120 + i * 220, 1000, 190)
		var vue := vue_libre(cadre)
		_vues.append(vue)
		var dock := FloatingDock.new()
		dock.resize_mode = [FloatingDock.RESIZE_SIZE, FloatingDock.RESIZE_SCALE][i]
		dock.item_size = Vector2(130, 40)
		dock.magnification = 1.4
		for texte in ["Jouer", "Options", "Crédits", "Quitter"]:
			var b := Button.new()
			b.text = texte
			b.focus_mode = Control.FOCUS_NONE
			dock.add_child(b)
		vue.add_child(dock)
		dock.position = Vector2(500 - 290, 110)
		docks.append(dock)
		etiquette(Vector2(200, cadre.position.y + 18), ["RESIZE_SIZE (défaut)", "RESIZE_SCALE"][i], 16, VARIANTES[i])
	_preparer_fleche()
	code("$Menu.resize_mode = FloatingDock.RESIZE_SCALE")
	for i in 2:
		await _balayer(Vector2(320, 120 + i * 220 + 130), Vector2(830, 120 + i * 220 + 130), 1.8)


func _hovered_item_changed() -> void:
	sujet("hovered_item_changed, get_hovered_item(), get_item_influence()")
	var dock := _dock(Rect2(76, 110, 1000, 300))
	var barres := Node2D.new()
	scene.add_child(barres)
	barres.draw.connect(func():
		for i in dock.get_child_count():
			var x := 220.0 + i * 110.0
			barres.draw_rect(Rect2(x, 470, 80, 14), Color(0, 0, 0, 0.35))
			barres.draw_rect(Rect2(x, 470, 80 * dock.get_item_influence(i), 14), COULEURS[i])
			barres.draw_string(ThemeDB.fallback_font, Vector2(x, 462), "influence %d" % i, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(1, 1, 1, 0.6)))
	get_tree().process_frame.connect(barres.queue_redraw)
	var journal := etiquette(Vector2(576, 530), "", 18, ACCENT)
	dock.hovered_item_changed.connect(func(element): journal.text = "hovered_item_changed(%s)" % (element.tooltip_text if element else "null"))
	code("$Dock.get_item_influence(i)   # 0 à 1")
	_preparer_fleche()
	await _balayer(Vector2(330, 360), Vector2(820, 360), 3.0)
	await deplacer(_fleche, Vector2(576, 180), 0.5)
	_souris(_fleche.position)
	await secondes(1.2)


# 📌 Exemples

func _ex_vertical() -> void:
	sujet("Exemple : un dock vertical sur le bord gauche")
	var dock := _dock(Rect2(76, 110, 1000, 440), {"vertical": true, "item_anchor": FloatingDock.ANCHOR_BEGIN, "hover_lift": 8.0, "item_size": Vector2(40, 40), "magnification": 1.6})
	code("$Dock.vertical = true\n$Dock.item_anchor = FloatingDock.ANCHOR_BEGIN\n$Dock.hover_lift = 8.0")
	_preparer_fleche()
	await _balayer(Vector2(120, 160), Vector2(120, 520), 2.6)
	dock.name = "Dock"


func _ex_texte() -> void:
	sujet("Exemple : des boutons avec du texte")
	await _menus_texte()


func _ex_manette() -> void:
	sujet("Exemple : au clavier ou à la manette (follow_focus)")
	var dock := _dock(Rect2(76, 150, 1000, 380))
	for icone in dock.get_children():
		icone.focus_mode = Control.FOCUS_ALL
	code("func ouvrir_menu() -> void:\n\t$Dock.get_child(0).grab_focus()")
	var touche := etiquette(Vector2(576, 260), "", 24, ACCENT)
	await secondes(0.6)
	dock.get_child(0).grab_focus()
	touche.text = "grab_focus()"
	var precedent := 0
	for i in [1, 2, 3, 4, 5, 6, 5, 4, 3]:
		await secondes(0.5)
		touche.text = "→ (flèche droite)" if i > precedent else "← (flèche gauche)"
		dock.get_child(i).grab_focus()
		precedent = i
	await secondes(0.8)


func _ex_raccourcis() -> void:
	sujet("Exemple : relier le dock à une barre de raccourcis")
	var cadre := Rect2(76, 150, 1000, 380)
	var vue := vue_libre(cadre)
	_vues.append(vue)
	var dock := FloatingDock.new()
	var journal := etiquette(Vector2(576, 260), "", 22, ACCENT)
	for i in 6:
		var b := TextureButton.new()
		b.texture_normal = Formes.dessin(Vector2i(64, 64), COULEURS[i], [[Rect2i(20, 20, 24, 24), COULEURS[i].lightened(0.4)]])
		b.ignore_texture_size = true
		b.stretch_mode = TextureButton.STRETCH_KEEP_ASPECT_CENTERED
		b.focus_mode = Control.FOCUS_NONE
		b.pressed.connect(func(): journal.text = "Raccourci %d" % i)
		dock.add_child(b)
	vue.add_child(dock)
	dock.position = Vector2(500 - 240, 280)
	code("for i in $Dock.get_child_count():\n\t($Dock.get_child(i) as BaseButton).pressed.connect(utiliser_raccourci.bind(i))")
	_preparer_fleche()
	for i in [0, 3, 5, 1]:
		var cible: Vector2 = dock.get_child(i).get_global_rect().get_center() + cadre.position
		var depart := _fleche.position
		var t := 0.0
		while t < 0.6:
			await images()
			t += get_process_delta_time()
			_fleche.position = depart.lerp(cible, smoothstep(0.0, 1.0, minf(t / 0.6, 1.0)))
			_souris(_fleche.position)
		await secondes(0.3)
		await souris_clic(_fleche.position, 0.1, vue)
		await secondes(0.6)
