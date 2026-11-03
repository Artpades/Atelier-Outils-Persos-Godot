extends "res://🎥Démos de documentation/socle/sequences.gd"

## 🎭 Plans filmés de la page Changeur de visuels (widgets/changeur-de-visuels.html).


func titre_module() -> String:
	return "🎭 Changeur de visuels · VisualsSwapper"


func plans() -> Dictionary:
	return {
		"mode-emploi": _mode_emploi,
		"classe-VisualsSwapper": _classe_swapper,
		"classe-VisualsAsset": _classe_asset,
		"classe-VisualsRegister": _classe_register,
		"ex-3d": _ex_3d,
		"ex-boutique": _ex_boutique,
		"ex-interface": _ex_interface,
		"ex-fondu": _ex_fondu,
	}


func legendes() -> Dictionary:
	return {
		"mode-emploi": "Fleur de feu ramassée : <code>swap_by_name(\"feu\")</code> ; touché : <code>swap_by_name(\"normal\")</code>.",
		"classe-VisualsSwapper": "Un catalogue de trois apparences (normal, feu, glace), appliquées par leur nom.",
		"classe-VisualsAsset": "Une apparence modifie plusieurs nœuds d'un coup : le corps, le chapeau et l'arme.",
		"classe-VisualsRegister": "Selon le nœud visé : texture d'un Sprite2D, maillage d'un MeshInstance3D, ou scène qui remplace les enfants d'un Node3D.",
		"ex-3d": "Un Node3D conteneur : ses enfants sont remplacés par la scène <code>modeling_import</code> (ici trois modèles).",
		"ex-boutique": "Une boutique de skins : un bouton par VisualsAsset du catalogue.",
		"ex-interface": "Un même VisualsAsset par saison change les TextureRect d'une interface (<code>asset.apply($HUD)</code>).",
		"ex-fondu": "Un flash blanc (FXFade) masque le changement instantané d'apparence.",
	}


func schemas() -> Dictionary:
	return {
		"figure-1": ["inspecteur.svg", "Inspecteur d'un VisualsSwapper : trois VisualsAsset (normal, feu, glace) dans <code>visuals_assets</code>, l'un ouvert sur ses VisualsRegister."],
		"types": ["types.svg", "<code>swap_visuals</code> : ce que reçoit le nœud visé selon son type (texture, maillage ou scène importée)."],
	}


func reutilisations() -> Dictionary:
	return {
		"VisualsSwapper.visuals_assets": "classe-VisualsSwapper",
		"VisualsSwapper.swap": "ex-boutique",
		"VisualsSwapper.swap_by_name": "mode-emploi",
		"VisualsAsset.suit_type_name": "classe-VisualsSwapper",
		"VisualsAsset.visuals_registers": "classe-VisualsAsset",
		"VisualsAsset.apply": "ex-interface",
		"VisualsAsset.swap_visuals": "types",
		"VisualsRegister.node_visual": "classe-VisualsAsset",
		"VisualsRegister.texture": "classe-VisualsRegister",
		"VisualsRegister.mesh": "classe-VisualsRegister",
		"VisualsRegister.modeling_import": "ex-3d",
	}


## Texture de personnage : corps coloré, yeux, et un motif selon l'apparence.
func _texture(couleur: Color, motif: Color = Color.TRANSPARENT) -> Texture2D:
	var rects := [[Rect2i(14, 18, 8, 8), Color("1b1f2a")], [Rect2i(42, 18, 8, 8), Color("1b1f2a")]]
	if motif.a > 0.0:
		rects += [[Rect2i(0, 60, 64, 12), motif], [Rect2i(8, 0, 8, 10), motif], [Rect2i(48, 0, 8, 10), motif]]
	return Formes.dessin(Vector2i(64, 96), couleur, rects)


func _registre(chemin: String, texture: Texture2D = null) -> VisualsRegister:
	var r := VisualsRegister.new()
	r.node_visual = NodePath(chemin)
	r.texture = texture
	return r


func _asset(nom: String, registres: Array) -> VisualsAsset:
	var a := VisualsAsset.new()
	a.suit_type_name = nom
	a.visuals_registers = registres
	return a


## Personnage : racine Node2D, Sprite2D « Corps » et VisualsSwapper au catalogue normal / feu / glace.
func _personnage(position: Vector2) -> Node2D:
	var racine := Node2D.new()
	racine.position = position
	scene.add_child(racine)
	var corps := Sprite2D.new()
	corps.name = "Corps"
	corps.texture = _texture(VARIANTES[0])
	racine.add_child(corps)
	var swapper := VisualsSwapper.new()
	swapper.name = "VisualsSwapper"
	swapper.visuals_assets = [_asset("normal", [_registre("../Corps", _texture(VARIANTES[0]))]),
		_asset("feu", [_registre("../Corps", _texture(Color("ef5f5f"), Color("ffd166")))]),
		_asset("glace", [_registre("../Corps", _texture(Color("9fd8ff"), Color.WHITE))])]
	racine.add_child(swapper)
	return racine


# ⚙️ Mode d'emploi

func _mode_emploi() -> void:
	sujet("Mode d'emploi : une transformation au ramassage")
	var heros := _personnage(Vector2(250, 360))
	var swapper: VisualsSwapper = heros.get_node("VisualsSwapper")
	arbre(Vector2(40, 140), PackedStringArray(["Personnage", "  Corps (Sprite2D)", "  VisualsSwapper"]))
	code("func _on_fleur_de_feu_ramassee() -> void:\n\t$VisualsSwapper.swap_by_name(\"feu\")\nfunc _on_degats() -> void:\n\t$VisualsSwapper.swap_by_name(\"normal\")")
	for tour in 2:
		heros.position.x = 250
		var fleur := disque(Vector2(600, 380), Color("ffd166"), 16)
		var ennemi := carre(Vector2(950, 380), MAUVAIS, 50)
		await deplacer(heros, Vector2(580, 360), 1.0)
		fleur.queue_free()
		legende("Fleur de feu : swap_by_name(\"feu\")")
		swapper.swap_by_name("feu")
		await secondes(0.8)
		await deplacer(heros, Vector2(910, 360), 0.9)
		legende("Touché : swap_by_name(\"normal\")")
		swapper.swap_by_name("normal")
		ennemi.queue_free()
		await secondes(0.8)


# 💻 API

func _classe_swapper() -> void:
	sujet("VisualsSwapper : un catalogue d'apparences")
	var heros := _personnage(Vector2(576, 340))
	heros.scale = Vector2(1.6, 1.6)
	var swapper: VisualsSwapper = heros.get_node("VisualsSwapper")
	etiquette(Vector2(576, 140), "visuals_assets = [normal, feu, glace]", 18, ACCENT)
	for nom in ["feu", "glace", "normal", "glace", "inconnu", "feu"]:
		await secondes(1.0)
		code("$VisualsSwapper.swap_by_name(\"%s\")" % nom)
		legende("swap_by_name(\"%s\")%s" % [nom, " : nom inconnu, rien ne change" if nom == "inconnu" else ""])
		swapper.swap_by_name(nom)


func _classe_asset() -> void:
	sujet("VisualsAsset : plusieurs changements ensemble")
	var racine := Node2D.new()
	racine.position = Vector2(576, 350)
	racine.scale = Vector2(1.5, 1.5)
	scene.add_child(racine)
	var corps := Sprite2D.new()
	corps.name = "Corps"
	corps.texture = _texture(VARIANTES[0])
	racine.add_child(corps)
	var chapeau := Sprite2D.new()
	chapeau.name = "Chapeau"
	chapeau.position = Vector2(0, -58)
	chapeau.texture = Formes.dessin(Vector2i(50, 18), Color("6b7590"))
	racine.add_child(chapeau)
	var arme := Sprite2D.new()
	arme.name = "Arme"
	arme.position = Vector2(46, 10)
	arme.texture = Formes.dessin(Vector2i(10, 60), Color("cfd6e6"))
	racine.add_child(arme)
	var swapper := VisualsSwapper.new()
	racine.add_child(swapper)
	var chevalier := _asset("chevalier", [_registre("../Corps", _texture(Color("9aa4b8"), Color("6b7590"))), _registre("../Chapeau", Formes.dessin(Vector2i(54, 30), Color("cfd6e6"))),
		_registre("../Arme", Formes.dessin(Vector2i(12, 80), Color("e8ecf4")))])
	var mage := _asset("mage", [_registre("../Corps", _texture(Color("c678dd"), Color("ffd166"))), _registre("../Chapeau", Formes.dessin(Vector2i(40, 44), Color("6b2fa3"))),
		_registre("../Arme", Formes.dessin(Vector2i(8, 90), Color("8a5a3c")))])
	code("var chevalier := VisualsAsset.new()\nchevalier.visuals_registers = [corps, chapeau, arme]\n$VisualsSwapper.swap(chevalier)")
	for asset in [chevalier, mage, chevalier, mage]:
		await secondes(1.2)
		legende("swap(%s) : trois VisualsRegister appliqués ensemble" % asset.suit_type_name)
		swapper.swap(asset)


func _classe_register() -> void:
	sujet("VisualsRegister : texture, maillage ou scène")
	var racine := Node2D.new()
	scene.add_child(racine)
	var sprite := Sprite2D.new()
	sprite.name = "Sprite"
	sprite.position = Vector2(200, 330)
	sprite.texture = _texture(VARIANTES[0])
	racine.add_child(sprite)
	etiquette(Vector2(200, 440), "Sprite2D → texture", 16)
	var vue := vue_3d(Rect2(380, 180, 360, 250))
	var maillage := MeshInstance3D.new()
	maillage.name = "Maillage"
	maillage.mesh = BoxMesh.new()
	maillage.material_override = Formes.matiere(VARIANTES[1])
	maillage.position = Vector3(0, 0.8, 0)
	vue.add_child(maillage)
	etiquette(Vector2(560, 460), "MeshInstance3D → mesh", 16)
	var vue2 := vue_3d(Rect2(780, 180, 330, 250))
	var conteneur := Node3D.new()
	conteneur.name = "Modele"
	conteneur.position = Vector3(0, 0.6, 0)
	vue2.add_child(conteneur)
	Formes.sphere(conteneur, Vector3.ZERO, VARIANTES[2], 0.6)
	etiquette(Vector2(945, 460), "Node3D → modeling_import", 16)
	var scenes := [_scene_modele(Color("5ccf7a"), "cone"), _scene_modele(Color("ffd166"), "cube")]
	code("registre.node_visual = ^\"../Sprite\"\nregistre.texture / .mesh / .modeling_import")
	var formes := [SphereMesh.new(), CylinderMesh.new(), PrismMesh.new()]
	for i in 3:
		await secondes(1.3)
		VisualsAsset.swap_visuals(racine, _registre("Sprite", _texture([Color("ef5f5f"), Color("9fd8ff"), VARIANTES[0]][i], Color("ffd166") if i == 0 else Color.TRANSPARENT)))
		var r := VisualsRegister.new()
		r.node_visual = NodePath(".")
		r.mesh = formes[i]
		VisualsAsset.swap_visuals(maillage, r)
		var m := VisualsRegister.new()
		m.node_visual = NodePath(".")
		m.modeling_import = scenes[i % 2]
		VisualsAsset.swap_visuals(conteneur, m)
		legende("Matériau surchargé du MeshInstance3D conservé ; les enfants du Node3D sont remplacés")
	await secondes(1.2)


## Petite scène 3D empaquetée (pour modeling_import).
func _scene_modele(couleur: Color, forme: String) -> PackedScene:
	var racine := Node3D.new()
	racine.name = "Modele"
	var instance := MeshInstance3D.new()
	instance.mesh = CylinderMesh.new() if forme == "cone" else BoxMesh.new()
	if forme == "cone":
		instance.mesh.top_radius = 0.0
	instance.mesh.material = Formes.matiere(couleur)
	racine.add_child(instance)
	instance.owner = racine
	var paquet := PackedScene.new()
	paquet.pack(racine)
	racine.free()
	return paquet


# 📌 Exemples

func _ex_3d() -> void:
	sujet("Exemple : changer le modèle 3D complet")
	var vue := vue_3d(Rect2(176, 130, 800, 400))
	Formes.cube(vue, Vector3(0, -0.1, 0), Formes.SOL, Vector3(10, 0.2, 10))
	var joueur := Node3D.new()
	joueur.name = "Joueur"
	vue.add_child(joueur)
	var modele := Node3D.new()
	modele.name = "Modele"
	modele.position = Vector3(0, 0.8, 0)
	joueur.add_child(modele)
	Formes.sphere(modele, Vector3.ZERO, VARIANTES[0], 0.7)
	var swapper := VisualsSwapper.new()
	joueur.add_child(swapper)
	code("robot.node_visual = ^\"../Modele\"\nrobot.modeling_import = preload(\"res://modeles/robot.glb\")\n$VisualsSwapper.swap(tenue)")
	var modeles := [[_scene_modele(Color("9aa4b8"), "cube"), "robot"], [_scene_modele(Color("5ccf7a"), "cone"), "sapin"], [_scene_modele(Color("ffd166"), "cube"), "coffre"]]
	var t := 0.0
	for m in modeles:
		var r := VisualsRegister.new()
		r.node_visual = NodePath("../Modele")
		r.modeling_import = m[0]
		await secondes(1.3)
		legende("swap(%s)" % m[1])
		swapper.swap(_asset(m[1], [r]))
	await secondes(1.0)


func _ex_boutique() -> void:
	sujet("Exemple : une boutique de skins")
	var heros := _personnage(Vector2(330, 340))
	heros.scale = Vector2(1.5, 1.5)
	var swapper: VisualsSwapper = heros.get_node("VisualsSwapper")
	var liste := VBoxContainer.new()
	liste.position = Vector2(700, 230)
	scene.add_child(liste)
	var boutons: Array[Button] = []
	for skin: VisualsAsset in swapper.visuals_assets:
		var b := Button.new()
		b.text = skin.suit_type_name.capitalize()
		b.custom_minimum_size = Vector2(220, 50)
		b.pressed.connect(func(): swapper.swap(skin))
		liste.add_child(b)
		boutons.append(b)
	code("for skin: VisualsAsset in $Joueur/VisualsSwapper.visuals_assets:\n\tbouton.pressed.connect(func(): $Joueur/VisualsSwapper.swap(skin))")
	var fleche := pointeur()
	fleche.position = Vector2(1000, 560)
	await images(2)
	for i in [1, 2, 0, 1]:
		var cible := boutons[i].get_global_rect().get_center()
		await deplacer(fleche, cible, 0.5)
		await souris_clic(cible, 0.12)
		legende("Skin choisi : %s" % boutons[i].text)
		await secondes(0.7)


func _ex_interface() -> void:
	sujet("Exemple : les icônes d'une interface selon la saison")
	var hud := Control.new()
	hud.name = "HUD"
	hud.position = Vector2(326, 200)
	scene.add_child(hud)
	var fond := TextureRect.new()
	fond.name = "Fond"
	fond.size = Vector2(500, 220)
	hud.add_child(fond)
	var icone := TextureRect.new()
	icone.name = "Icone"
	icone.position = Vector2(200, 50)
	icone.size = Vector2(100, 100)
	hud.add_child(icone)
	var saisons := {
		"ete": [Color("ffd166"), Color("ef8f3a")], "automne": [Color("c47a3d"), Color("8a5a3c")], "hiver": [Color("dff2ff"), Color("9fd8ff")], "printemps": [Color("b7e4a8"), Color("ff9ecb")]}
	code("var asset: VisualsAsset = load(\"res://saisons/%s.tres\" % saison)\nasset.apply($HUD)   # chemins depuis $HUD")
	for saison in ["ete", "automne", "hiver", "printemps"]:
		var asset := _asset(saison, [_registre("Fond", Formes.dessin(Vector2i(500, 220), saisons[saison][0])), _registre("Icone", Formes.dessin(Vector2i(100, 100), saisons[saison][1], [[Rect2i(30, 30, 40, 40), Color.WHITE]]))])
		legende("appliquer_saison(\"%s\")" % saison)
		asset.apply(hud)
		await secondes(1.4)


func _ex_fondu() -> void:
	sujet("Exemple : changer d'apparence avec un flash")
	var heros := _personnage(Vector2(576, 340))
	heros.scale = Vector2(1.6, 1.6)
	var swapper: VisualsSwapper = heros.get_node("VisualsSwapper")
	var joueur := PlayerFX.new()
	heros.add_child(joueur)
	code("flash.color = Color(3, 3, 3)\n$PlayerFX.play()\nawait get_tree().create_timer(0.08).timeout\n$VisualsSwapper.swap_by_name(nom)\n$PlayerFX.stop()")
	for nom in ["feu", "glace", "normal", "feu"]:
		await secondes(1.0)
		var flash := FXFade.new()
		flash.color = Color(3, 3, 3)
		flash.duration = 0.08
		joueur.special_effects = [flash]
		joueur.play()
		await secondes(0.08)
		legende("transformer(\"%s\")" % nom)
		swapper.swap_by_name(nom)
		joueur.stop()
