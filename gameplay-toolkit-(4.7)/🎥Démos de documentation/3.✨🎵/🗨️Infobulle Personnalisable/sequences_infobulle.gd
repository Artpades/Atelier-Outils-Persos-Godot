extends "res://🎥Démos de documentation/3.✨🎵/sequences_effets.gd"

## 🗨️ Plans filmés de la page Infobulle personnalisable (effets/infobulle-personnalisable.html).
##
## CustomTooltip se place à la position de la souris (get_mouse_position) : les démos sont dans une vue_libre
## (SubViewport hors conteneur) qui reçoit directement les mouvements simulés.

const VIDEO := "res://🎥Démos de documentation/3.✨🎵/🗨️Infobulle Personnalisable/double_saut.ogv"

var _vue: SubViewport
var _fleche: Node2D


func titre_module() -> String:
	return "🗨️ Infobulle · CustomTooltip, TooltipContent"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-CustomTooltip": _classe_tooltip,
		"classe-TooltipContent": _classe_contenu,
		"CustomTooltip.delay": _delay,
		"CustomTooltip.fade_duration": _fade_duration,
		"CustomTooltip.follow_mouse": _follow_mouse,
		"CustomTooltip.offset": _offset,
		"TooltipContent.max_width": _max_width,
		"ex-monde": _ex_monde,
		"ex-inventaire": _ex_inventaire,
		"ex-video": _ex_video,
		"ex-style": _ex_style,
		"ex-tuto": _ex_tuto,
		"ex-pause": _ex_pause,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Survol d'une case d'inventaire : après <code>delay</code>, l'infobulle apparaît en fondu près de la souris, la suit, et passe à gauche près du bord droit.",
		"classe-CustomTooltip": "Une infobulle par bouton ; à droite, les signaux <code>tooltip_shown</code> / <code>tooltip_hidden</code>.",
		"classe-TooltipContent": "Les éléments d'un contenu : titre, puis icône, puis texte BBCode, puis scène personnalisée ; les éléments vides sont omis.",
		"CustomTooltip.delay": "<code>delay</code> 0, 0,5 (défaut) et 1,5 s : le temps de survol avant l'apparition.",
		"CustomTooltip.fade_duration": "<code>fade_duration</code> 0, 0,15 (défaut) et 0,6 s : le fondu d'apparition et de disparition.",
		"CustomTooltip.follow_mouse": "true : l'infobulle suit la souris · false : elle reste là où elle est apparue.",
		"CustomTooltip.offset": "<code>offset</code> (16, 16) (défaut) puis (40, -70) : le décalage par rapport à la pointe de la souris.",
		"TooltipContent.max_width": "<code>max_width</code> 160 puis 320 (défaut) : le texte revient à la ligne au-delà.",
		"ex-monde": "Un coffre du monde (Area2D) avec son infobulle.",
		"ex-inventaire": "Le contenu change pendant l'affichage (objet suivant), puis <code>content = null</code> masque l'infobulle.",
		"ex-video": "Une infobulle de compétence avec une vidéo (VideoStreamTheora) qui boucle.",
		"ex-style": "Le même <code>panel_style</code> (StyleBoxFlat doré) appliqué à toutes les infobulles du groupe.",
		"ex-tuto": "Astuce de tutoriel : <code>show_tooltip()</code> sans survol, puis <code>hide_tooltip()</code> 4 s plus tard.",
		"ex-pause": "Jeu en pause : l'infobulle en PROCESS_MODE_ALWAYS s'affiche, l'autre jamais.",
	}


func schemas() -> Dictionary:
	return {
		"panneau": ["panneau.svg", "Structure d'une infobulle : un CanvasLayer (<code>layer</code>) qui porte le PanelContainer (<code>get_panel()</code>, <code>panel_style</code>, <code>tooltip_theme</code>) et ses éléments, empilés dans l'ordre."],
	}


func reutilisations() -> Dictionary:
	return {
		"CustomTooltip.content": "classe-TooltipContent",
		"CustomTooltip.target": "ex-monde",
		"CustomTooltip.panel_style": "ex-style",
		"CustomTooltip.layer": "panneau",
		"CustomTooltip.tooltip_theme": "panneau",
		"CustomTooltip.get_panel": "panneau",
		"CustomTooltip.is_tooltip_visible": "classe-CustomTooltip",
		"CustomTooltip.show_tooltip": "ex-tuto",
		"CustomTooltip.hide_tooltip": "ex-tuto",
		"CustomTooltip.tooltip_shown": "classe-CustomTooltip",
		"CustomTooltip.tooltip_hidden": "classe-CustomTooltip",
		"TooltipContent.title": "classe-TooltipContent",
		"TooltipContent.icon": "classe-TooltipContent",
		"TooltipContent.icon_max_size": "classe-TooltipContent",
		"TooltipContent.text": "classe-TooltipContent",
		"TooltipContent.custom_scene": "classe-TooltipContent",
		"TooltipContent.video": "ex-video",
		"TooltipContent.video_size": "ex-video",
		"TooltipContent.video_loop": "ex-video",
		"TooltipContent.video_muted": "ex-video",
	}


func _preparer(cadre: Rect2 = Rect2(76, 110, 1000, 450)) -> void:
	_vue = vue_libre(cadre)
	_fleche = pointeur()
	_fleche.position = cadre.get_center() + Vector2(0, 180)


## Bouton (ou case) dans la vue, centré sur `position` (coordonnées de la vue).
func _case(position: Vector2, texte: String, couleur: Color = Formes.DECOR, taille: Vector2 = Vector2(90, 90)) -> Button:
	var b := Button.new()
	b.text = texte
	b.size = taille
	b.position = position - taille / 2.0
	b.focus_mode = Control.FOCUS_NONE
	var style := StyleBoxFlat.new()
	style.bg_color = couleur
	style.set_corner_radius_all(8)
	b.add_theme_stylebox_override("normal", style)
	b.add_theme_stylebox_override("hover", style)
	_vue.add_child(b)
	return b


func _infobulle(cible: Node, contenu: TooltipContent, reglages: Dictionary = {}) -> CustomTooltip:
	var infobulle := CustomTooltip.new()
	infobulle.content = contenu
	for propriete in reglages:
		infobulle.set(propriete, reglages[propriete])
	cible.add_child(infobulle)
	return infobulle


func _contenu(titre: String, texte: String = "", icone: Texture2D = null) -> TooltipContent:
	var contenu := TooltipContent.new()
	contenu.title = titre
	contenu.text = texte
	contenu.icon = icone
	return contenu


func _icone(couleur: Color) -> Texture2D:
	return Formes.dessin(Vector2i(64, 64), couleur, [[Rect2i(14, 14, 36, 36), couleur.lightened(0.45)], [Rect2i(26, 4, 12, 56), couleur.darkened(0.3)]])


## Déplace le pointeur vers `vers` (coordonnées de la vue) en envoyant les mouvements à la vue.
func _aller(vers: Vector2, duree: float = 0.6) -> void:
	var cadre: Rect2 = _vue.get_meta("cadre")
	var depart := _fleche.position
	var arrivee := vers + cadre.position
	var t := 0.0
	while t < duree:
		await images()
		t += get_process_delta_time()
		_fleche.position = depart.lerp(arrivee, smoothstep(0.0, 1.0, minf(t / duree, 1.0)))
		souris_mouvement(_fleche.position, _vue)


## Petits mouvements autour de la position actuelle (la souris « vit » pendant le survol).
func _rester(duree: float, amplitude: float = 6.0) -> void:
	var centre := _fleche.position
	var t := 0.0
	while t < duree:
		await images()
		t += get_process_delta_time()
		_fleche.position = centre + Vector2(sin(t * 3.0), cos(t * 2.3)) * amplitude
		souris_mouvement(_fleche.position, _vue)


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : une infobulle sur une case d'inventaire")
	_preparer()
	var objets := [["Épée de feu", "[color=orange]+12[/color] dégâts de feu", Color("ef5f5f")], ["Bouclier", "Bloque [b]40 %[/b] des dégâts", Color("4ea1ff")],
		["Potion", "Rend [color=green]50 PV[/color]", Color("5ccf7a")], ["Anneau", "[i]Une lueur étrange...[/i]", Color("c678dd")]]
	var cases: Array[Button] = []
	for i in objets.size():
		var c := _case(Vector2(260 + i * 160, 150 + (300 if i == 3 else 0)) if i < 3 else Vector2(930, 150), "", objets[i][2])
		_infobulle(c, _contenu(objets[i][0], objets[i][1], _icone(objets[i][2])))
		cases.append(c)
	code("infos.title = \"Épée de feu\"\ninfos.icon = preload(\"res://icones/epee.png\")\ninfos.text = \"[color=orange]+12[/color] dégâts de feu\"\n$Case/CustomTooltip.content = infos")
	for i in [0, 1, 3]:
		await _aller(cases[i].position + cases[i].size / 2.0, 0.6)
		legende("Survol : apparition après delay (0,5 s), en fondu" if i == 0 else ("Près du bord droit, l'infobulle passe à gauche de la souris" if i == 3 else "L'infobulle suit la souris"))
		await _rester(2.0, 20.0)
	await _aller(Vector2(500, 380), 0.6)
	await secondes(0.8)


# 💻 API

func _classe_tooltip() -> void:
	sujet("CustomTooltip : infobulle au survol")
	_preparer(Rect2(76, 110, 680, 450))
	var journal := etiquette(Vector2(950, 300), "", 18, ACCENT)
	var lignes := PackedStringArray()
	var cases: Array[Button] = []
	for i in 3:
		var c := _case(Vector2(160 + i * 180, 140), ["Jouer", "Options", "Quitter"][i], Formes.DECOR, Vector2(150, 56))
		var infobulle := _infobulle(c, _contenu(["Jouer", "Options", "Quitter"][i], ["Lancer une partie", "Son, image, touches", "Retour au bureau"][i]))
		var nom: String = ["Jouer", "Options", "Quitter"][i]
		infobulle.tooltip_shown.connect(func():
			lignes.append("%s : tooltip_shown" % nom)
			journal.text = "\n".join(lignes.slice(-8)))
		infobulle.tooltip_hidden.connect(func():
			lignes.append("%s : tooltip_hidden" % nom)
			journal.text = "\n".join(lignes.slice(-8)))
		cases.append(c)
	code("$Bouton/CustomTooltip.content = infos")
	for i in [0, 1, 2]:
		await _aller(cases[i].position + cases[i].size / 2.0, 0.5)
		await _rester(1.6)
	await _aller(Vector2(340, 380), 0.5)
	await secondes(1.0)


func _classe_contenu() -> void:
	sujet("TooltipContent : titre, icône, texte, scène")
	_preparer()
	var contenus: Array[TooltipContent] = [_contenu("Titre seul"), _contenu("Avec icône", "", _icone(Color("f2a33a"))),
		_contenu("Avec texte", "Texte [b]BBCode[/b] : [color=orange]couleurs[/color], [i]italique[/i]...", _icone(Color("4ea1ff")))]
	var scene_perso := PackedScene.new()
	var barres := HBoxContainer.new()
	for i in 3:
		var barre := ProgressBar.new()
		barre.custom_minimum_size = Vector2(70, 14)
		barre.value = [80, 45, 60][i]
		barre.show_percentage = false
		barres.add_child(barre)
		barre.owner = barres
	scene_perso.pack(barres)
	var complet := _contenu("Avec scène", "custom_scene : statistiques", _icone(Color("5ccf7a")))
	complet.custom_scene = scene_perso
	contenus.append(complet)
	var cases: Array[Button] = []
	for i in 4:
		var c := _case(Vector2(150 + i * 220, 120), ["title", "+ icon", "+ text", "+ custom_scene"][i], Formes.DECOR, Vector2(160, 56))
		_infobulle(c, contenus[i], {"delay": 0.2})
		cases.append(c)
	code("infos.title = \"...\"\ninfos.icon = icone        # icon_max_size (64, 64)\ninfos.text = \"[b]BBCode[/b]\"\ninfos.custom_scene = preload(\"res://stats.tscn\")")
	for i in 4:
		await _aller(cases[i].position + cases[i].size / 2.0, 0.5)
		await _rester(1.6)


## Trois boutons comparant `valeurs` de la propriété `propriete` du CustomTooltip.
func _comparer(propriete: String, valeurs: Array, textes: Array, duree_survol: float = 2.0) -> void:
	_preparer()
	var cases: Array[Button] = []
	for i in valeurs.size():
		var c := _case(Vector2(200 + i * 300, 120), textes[i], VARIANTES[i].darkened(0.3), Vector2(200, 56))
		_infobulle(c, _contenu("Infobulle", "%s = %s" % [propriete, textes[i]]), {propriete: valeurs[i]})
		cases.append(c)
	for i in valeurs.size():
		await _aller(cases[i].position + cases[i].size / 2.0, 0.5)
		await _rester(duree_survol, 4.0)
		await _aller(cases[i].position + Vector2(cases[i].size.x / 2.0, 200), 0.4)
		await secondes(0.6)


func _delay() -> void:
	sujet("delay : temps de survol avant l'apparition")
	code("$CustomTooltip.delay = 0.5")
	await _comparer("delay", [0.0, 0.5, 1.5], ["0", "0,5 (défaut)", "1,5"])


func _fade_duration() -> void:
	sujet("fade_duration : fondu d'apparition et de disparition")
	code("$CustomTooltip.fade_duration = 0.15")
	await _comparer("fade_duration", [0.0, 0.15, 0.6], ["0", "0,15 (défaut)", "0,6"], 1.6)


func _follow_mouse() -> void:
	sujet("follow_mouse : suivre la souris ?")
	_preparer()
	for i in 2:
		var c := _case(Vector2(270 + i * 460, 200), ["follow_mouse = true", "follow_mouse = false"][i], VARIANTES[i].darkened(0.3), Vector2(340, 160))
		_infobulle(c, _contenu("Infobulle", "follow_mouse = %s" % (i == 0)), {"follow_mouse": i == 0, "delay": 0.2})
		code("$CustomTooltip.follow_mouse = false")
		await _aller(c.position + Vector2(60, 40), 0.5)
		await secondes(0.5)
		for point in [Vector2(280, 40), Vector2(300, 130), Vector2(60, 130), Vector2(170, 80)]:
			await _aller(c.position + point, 0.5)
		await _aller(c.position + Vector2(170, 260), 0.4)
		await secondes(0.4)


func _offset() -> void:
	sujet("offset : décalage par rapport à la souris")
	code("$CustomTooltip.offset = Vector2(16, 16)")
	await _comparer("offset", [Vector2(16, 16), Vector2(40, -70)], ["(16, 16) (défaut)", "(40, -70)"])


func _max_width() -> void:
	sujet("max_width : largeur avant le retour à la ligne")
	_preparer()
	var texte := "Une épée forgée dans le cœur d'un volcan. Inflige des dégâts de feu et éclaire les cavernes sombres."
	var cases: Array[Button] = []
	for i in 2:
		var contenu := _contenu("Épée de feu", texte, _icone(Color("ef5f5f")))
		contenu.max_width = [160.0, 320.0][i]
		var c := _case(Vector2(200 + i * 360, 120), "max_width = %d" % contenu.max_width, VARIANTES[i].darkened(0.3), Vector2(220, 56))
		_infobulle(c, contenu, {"delay": 0.2})
		cases.append(c)
	code("infos.max_width = 320.0")
	for c in cases:
		await _aller(c.position + c.size / 2.0, 0.5)
		await _rester(2.2)


# 📌 Exemples

func _ex_monde() -> void:
	sujet("Exemple : une infobulle sur un objet du monde")
	_preparer()
	var coffre := Area2D.new()
	coffre.position = Vector2(500, 250)
	coffre.input_pickable = true
	var forme := CollisionShape2D.new()
	forme.shape = RectangleShape2D.new()
	forme.shape.size = Vector2(160, 110)
	coffre.add_child(forme)
	var dessin := Polygon2D.new()
	dessin.polygon = Formes.rectangle(Vector2(160, 110))
	dessin.color = Color("8a5a3c")
	coffre.add_child(dessin)
	_vue.add_child(coffre)
	_infobulle(coffre, _contenu("Coffre ancien", "Verrouillé. Il faut une [b]clé de bronze[/b]."))
	code("# Coffre (Area2D, input_pickable = true)\n# └─ CustomTooltip\ninfos.title = \"Coffre ancien\"")
	# Survol émis par le script (le picking physique de la capture n'est pas fiable) : mêmes signaux.
	var dedans := [false]
	var suivi := func():
		var interieur := Rect2(420, 195, 160, 110).has_point(_fleche.position - _vue.get_meta("cadre").position)
		if interieur != dedans[0]:
			dedans[0] = interieur
			coffre.emit_signal("mouse_entered" if interieur else "mouse_exited")
	get_tree().process_frame.connect(suivi)
	for i in 2:
		await _aller(Vector2(480, 240), 0.8)
		await _rester(2.0, 25.0)
		await _aller(Vector2(200 + i * 600, 400), 0.7)
		await secondes(0.8)


func _ex_inventaire() -> void:
	sujet("Exemple : mettre à jour une case d'inventaire")
	_preparer()
	var c := _case(Vector2(500, 200), "", Color("ef5f5f"))
	var infobulle := _infobulle(c, _contenu("Épée de feu", "Lame brûlante\n[color=gray]Valeur : 120 or[/color]", _icone(Color("ef5f5f"))), {"delay": 0.2})
	code("$CustomTooltip.content = infos   # relu à chaque modification\n$CustomTooltip.content = null    # vider_case()")
	await _aller(Vector2(500, 200), 0.6)
	legende("afficher_objet(épée)")
	await _rester(1.6)
	legende("afficher_objet(bouclier) : l'infobulle se reconstruit pendant l'affichage")
	infobulle.content = _contenu("Bouclier", "Bloque les flèches\n[color=gray]Valeur : 80 or[/color]", _icone(Color("4ea1ff")))
	await _rester(2.0)
	legende("vider_case() : content = null, l'infobulle disparaît")
	infobulle.content = null
	await _rester(1.6)


func _ex_video() -> void:
	sujet("Exemple : montrer une capacité en vidéo")
	_preparer()
	var c := _case(Vector2(300, 150), "Double saut", Color("c678dd").darkened(0.3), Vector2(200, 60))
	var aide := _contenu("Double saut", "Appuyez de nouveau sur [b]Saut[/b] en l'air.")
	aide.video = load(VIDEO)
	aide.video_size = Vector2(240, 135)
	_infobulle(c, aide, {"delay": 0.3})
	code("aide.video = preload(\"res://videos/double_saut.ogv\")\naide.video_size = Vector2(240, 135)")
	await _aller(Vector2(300, 150), 0.6)
	await _rester(6.0, 8.0)


func _ex_style() -> void:
	sujet("Exemple : un style commun à toutes les infobulles")
	_preparer()
	var fond := StyleBoxFlat.new()
	fond.bg_color = Color(0.08, 0.06, 0.12, 0.95)
	fond.border_color = Color(0.8, 0.65, 0.3)
	fond.set_border_width_all(2)
	fond.set_corner_radius_all(6)
	fond.set_content_margin_all(10)
	var cases: Array[Button] = []
	for i in 3:
		var c := _case(Vector2(220 + i * 250, 140), ["Force", "Agilité", "Magie"][i], VARIANTES[i].darkened(0.3), Vector2(160, 56))
		var infobulle := _infobulle(c, _contenu(["Force", "Agilité", "Magie"][i], "Niveau [b]%d[/b]" % [12, 8, 15][i]), {"delay": 0.2})
		infobulle.add_to_group("infobulles")
		cases.append(c)
	code("for infobulle in get_tree().get_nodes_in_group(\"infobulles\"):\n\tinfobulle.panel_style = fond")
	for i in 3:
		if i == 1:
			legende("panel_style appliqué à tout le groupe")
			for infobulle in get_tree().get_nodes_in_group("infobulles"):
				infobulle.panel_style = fond
		await _aller(cases[i].position + cases[i].size / 2.0, 0.5)
		await _rester(1.6)


func _ex_tuto() -> void:
	sujet("Exemple : une infobulle sans survol (tutoriel)")
	_preparer()
	var inventaire := _case(Vector2(500, 200), "Inventaire", Formes.DECOR, Vector2(200, 70))
	var infobulle := _infobulle(inventaire, _contenu("Astuce", "Ouvrez l'[b]inventaire[/b] pour équiper l'épée."), {"follow_mouse": false})
	code("$Inventaire/CustomTooltip.follow_mouse = false\n$Inventaire/CustomTooltip.show_tooltip()\nawait get_tree().create_timer(4.0).timeout\n$Inventaire/CustomTooltip.hide_tooltip()")
	await _aller(Vector2(640, 260), 0.6)
	await secondes(0.5)
	legende("montrer_astuce() : show_tooltip() à la position de la souris, sans survol")
	infobulle.show_tooltip()
	await _rester(1.0, 10.0)
	await _aller(Vector2(820, 380), 1.0)
	await secondes(2.0)
	legende("hide_tooltip()")
	infobulle.hide_tooltip()
	await secondes(1.0)


func _ex_pause() -> void:
	sujet("Exemple : des infobulles dans un menu de pause")
	_preparer()
	var cases: Array[Button] = []
	for i in 2:
		var c := _case(Vector2(300 + i * 400, 150), ["PROCESS_MODE_ALWAYS", "PROCESS_MODE_INHERIT"][i], VARIANTES[i].darkened(0.3), Vector2(260, 60))
		var infobulle := _infobulle(c, _contenu("Options", "Mode %s" % ["Always", "Inherit"][i]), {"delay": 0.3})
		if i == 0:
			infobulle.process_mode = Node.PROCESS_MODE_ALWAYS
		cases.append(c)
	var etat := etiquette(Vector2(576, 100), "get_tree().paused = true", 20, MAUVAIS)
	code("$MenuPause/Options/CustomTooltip.process_mode = Node.PROCESS_MODE_ALWAYS")
	get_tree().paused = true
	_vue.process_mode = Node.PROCESS_MODE_ALWAYS
	for c in cases:
		await _aller(c.position + c.size / 2.0, 0.5)
		await _rester(2.0)
	get_tree().paused = false
	etat.text = ""
