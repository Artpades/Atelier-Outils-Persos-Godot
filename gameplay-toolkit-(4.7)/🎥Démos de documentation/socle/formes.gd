extends RefCounted

## 🧱 Formes, vues et couleurs communes aux plans filmés (sequences.gd) et aux aperçus web (apercu.gd).
## Fonctions statiques : const Formes := preload("res://🎥Démos de documentation/socle/formes.gd").

const FOND := Color("1b1f2a")
const FOND_VUE := Color("232a3a")
const SOL := Color("3a4256")
const DECOR := Color("56607a")
const GRILLE := Color(1, 1, 1, 0.04)
const TEXTE := Color("e8ecf4")
const TEXTE_DOUX := Color("9aa4b8")
const CODE := Color("cfe3ff")
const ACCENT := Color("f2a33a")
const BON := Color("5ccf7a")
const MAUVAIS := Color("ef5f5f")
## Couleurs des objets comparés côte à côte (variantes d'un réglage).
const VARIANTES: Array[Color] = [Color("4ea1ff"), Color("f2a33a"), Color("c678dd"), Color("5ccf7a")]


## Polygone rectangulaire (pour Polygon2D) de taille `taille` centré sur l'origine.
static func rectangle(taille: Vector2) -> PackedVector2Array:
	var d := taille / 2.0
	return PackedVector2Array([Vector2(-d.x, -d.y), Vector2(d.x, -d.y), Vector2(d.x, d.y), Vector2(-d.x, d.y)])


## Carré coloré (Node2D portant un Polygon2D) centré sur `position`.
static func carre(parent: Node, position: Vector2, couleur: Color, cote: float = 80.0) -> Node2D:
	var noeud := Node2D.new()
	noeud.position = position
	var forme := Polygon2D.new()
	forme.polygon = rectangle(Vector2(cote, cote))
	forme.color = couleur
	noeud.add_child(forme)
	parent.add_child(noeud)
	return noeud


## Disque (Node2D dessiné) centré sur `position`.
static func disque(parent: Node, position: Vector2, couleur: Color, rayon: float = 40.0) -> Node2D:
	var noeud := Node2D.new()
	noeud.position = position
	noeud.draw.connect(func(): noeud.draw_circle(Vector2.ZERO, rayon, couleur, true, -1.0, true))
	parent.add_child(noeud)
	return noeud


## Bord (Panel) puis SubViewportContainer occupant `cadre`.
static func cadre_de_vue(parent: Node, cadre: Rect2) -> SubViewportContainer:
	var bord := Panel.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color.TRANSPARENT
	style.border_color = Color(1, 1, 1, 0.25)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_expand_margin_all(2)
	bord.add_theme_stylebox_override("panel", style)
	bord.position = cadre.position
	bord.size = cadre.size
	bord.mouse_filter = Control.MOUSE_FILTER_IGNORE
	parent.add_child(bord)
	var conteneur := SubViewportContainer.new()
	conteneur.stretch = true
	conteneur.position = cadre.position
	conteneur.size = cadre.size
	parent.add_child(conteneur)
	bord.move_to_front()
	conteneur.set_meta("bord", bord)
	return conteneur


## Écran de jeu 2D (SubViewport encadré, avec fond, grille et une Camera2D centrée sur l'origine) dans
## `cadre`. Renvoie le SubViewport : y ajouter les nœuds 2D ; sa caméra est vue.get_meta("camera").
static func vue_2d(parent: Node, cadre: Rect2, fond: Color = FOND_VUE) -> SubViewport:
	var conteneur := cadre_de_vue(parent, cadre)
	var vue := SubViewport.new()
	vue.physics_object_picking = true
	conteneur.add_child(vue)
	var arriere := CanvasLayer.new()
	arriere.layer = -1
	vue.add_child(arriere)
	var couleur := ColorRect.new()
	couleur.color = fond
	couleur.size = cadre.size
	couleur.mouse_filter = Control.MOUSE_FILTER_IGNORE
	arriere.add_child(couleur)
	var grille := Grille.new()
	grille.zone = Rect2(-3000, -3000, 6000, 6000)
	vue.add_child(grille)
	var camera := Camera2D.new()
	vue.add_child(camera)
	vue.set_meta("camera", camera)
	return vue


## Écran de jeu 2D « libre » : SubViewport hors de tout conteneur, affiché par un TextureRect dans `cadre`.
## Sa position de souris (get_mouse_position) est celle des évènements qu'on lui pousse (push_input), et non
## la vraie souris de la fenêtre : à utiliser pour filmer un nœud qui lit get_local_mouse_position().
## Renvoie le SubViewport (meta "cadre" : son rectangle à l'écran).
static func vue_libre(parent: Node, cadre: Rect2, fond: Color = FOND_VUE) -> SubViewport:
	var vue := SubViewport.new()
	vue.size = Vector2i(cadre.size)
	vue.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	vue.set_meta("cadre", cadre)
	parent.add_child(vue)
	var couleur := ColorRect.new()
	couleur.color = fond
	couleur.size = cadre.size
	couleur.mouse_filter = Control.MOUSE_FILTER_IGNORE
	vue.add_child(couleur)
	var image := TextureRect.new()
	image.texture = vue.get_texture()
	image.position = cadre.position
	image.size = cadre.size
	image.mouse_filter = Control.MOUSE_FILTER_IGNORE
	parent.add_child(image)
	var bord := Panel.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color.TRANSPARENT
	style.border_color = Color(1, 1, 1, 0.25)
	style.set_border_width_all(2)
	style.set_corner_radius_all(6)
	style.set_expand_margin_all(2)
	bord.add_theme_stylebox_override("panel", style)
	bord.position = cadre.position
	bord.size = cadre.size
	bord.mouse_filter = Control.MOUSE_FILTER_IGNORE
	parent.add_child(bord)
	return vue


## Vue 3D (SubViewport avec son monde, une lumière et une caméra) dans `cadre`.
## Renvoie le SubViewport : y ajouter les nœuds 3D ; sa caméra est get_camera_3d().
static func vue_3d(parent: Node, cadre: Rect2) -> SubViewport:
	var conteneur := cadre_de_vue(parent, cadre)
	var vue := SubViewport.new()
	vue.own_world_3d = true
	vue.msaa_3d = Viewport.MSAA_4X
	vue.physics_object_picking = true
	conteneur.add_child(vue)
	var environnement := WorldEnvironment.new()
	environnement.environment = Environment.new()
	environnement.environment.background_mode = Environment.BG_COLOR
	environnement.environment.background_color = FOND_VUE
	environnement.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environnement.environment.ambient_light_color = Color(0.55, 0.6, 0.7)
	vue.add_child(environnement)
	var soleil := DirectionalLight3D.new()
	soleil.rotation_degrees = Vector3(-50, 30, 0)
	soleil.shadow_enabled = true
	vue.add_child(soleil)
	var camera := Camera3D.new()
	camera.position = Vector3(0, 2.2, 5.5)
	camera.rotation_degrees.x = -18
	vue.add_child(camera)
	return vue


## Pavé 3D coloré.
static func cube(parent: Node, position: Vector3, couleur: Color, taille: Vector3 = Vector3.ONE) -> MeshInstance3D:
	var maillage := BoxMesh.new()
	maillage.size = taille
	maillage.material = matiere(couleur)
	var instance := MeshInstance3D.new()
	instance.mesh = maillage
	instance.position = position
	parent.add_child(instance)
	return instance


## Sphère 3D colorée.
static func sphere(parent: Node, position: Vector3, couleur: Color, rayon: float = 0.5) -> MeshInstance3D:
	var maillage := SphereMesh.new()
	maillage.radius = rayon
	maillage.height = rayon * 2.0
	maillage.material = matiere(couleur)
	var instance := MeshInstance3D.new()
	instance.mesh = maillage
	instance.position = position
	parent.add_child(instance)
	return instance


static func matiere(couleur: Color) -> StandardMaterial3D:
	var resultat := StandardMaterial3D.new()
	resultat.albedo_color = couleur
	if couleur.a < 1.0:
		resultat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	return resultat


## Texture unie (ou en dégradé vertical vers `bas`) de taille `taille`, pour Sprite2D / TextureRect.
static func texture(taille: Vector2i, haut: Color, bas: Color = Color.TRANSPARENT) -> Texture2D:
	var degrade := Gradient.new()
	degrade.colors = PackedColorArray([haut, bas if bas.a > 0.0 else haut])
	var resultat := GradientTexture2D.new()
	resultat.gradient = degrade
	resultat.width = taille.x
	resultat.height = taille.y
	resultat.fill_from = Vector2(0, 0)
	resultat.fill_to = Vector2(0, 1)
	return resultat


## Image (ImageTexture) de taille `taille`, fond `fond`, avec des rectangles colorés : [[Rect2i, Color], ...].
static func dessin(taille: Vector2i, fond: Color, rectangles: Array = []) -> ImageTexture:
	var image := Image.create(taille.x, taille.y, false, Image.FORMAT_RGBA8)
	image.fill(fond)
	for r in rectangles:
		image.fill_rect(r[0], r[1])
	return ImageTexture.create_from_image(image)


## Image en pixel art : une chaîne par ligne, un caractère par pixel (« . » ou espace : transparent), couleurs
## de `palette` ({"x": Color}), agrandie `echelle` fois (sans lissage).
static func pixel_art(lignes: PackedStringArray, palette: Dictionary, echelle: int = 2) -> ImageTexture:
	var largeur := 0
	for ligne in lignes:
		largeur = maxi(largeur, ligne.length())
	var image := Image.create(largeur, lignes.size(), false, Image.FORMAT_RGBA8)
	for y in lignes.size():
		for x in lignes[y].length():
			var c := lignes[y][x]
			if palette.has(c):
				image.set_pixel(x, y, palette[c])
	image.resize(largeur * echelle, lignes.size() * echelle, Image.INTERPOLATE_NEAREST)
	return ImageTexture.create_from_image(image)


## Masque de transition (niveaux de gris) : "iris" (bords d'abord), "volet" (gauche → droite),
## "losanges", "dissolution" (bruit).
static func masque(sorte: String) -> Texture2D:
	match sorte:
		"dissolution":
			var bruit := NoiseTexture2D.new()
			bruit.width = 256
			bruit.height = 256
			bruit.noise = FastNoiseLite.new()
			bruit.noise.frequency = 0.03
			return bruit
		"losanges":
			var image := Image.create(256, 256, false, Image.FORMAT_L8)
			for y in 256:
				for x in 256:
					var cx := absf(fmod(x, 64.0) - 32.0) + absf(fmod(y, 64.0) - 32.0)
					image.set_pixel(x, y, Color.from_hsv(0, 0, 1.0 - cx / 64.0))
			return ImageTexture.create_from_image(image)
	var degrade := Gradient.new()
	var texture := GradientTexture2D.new()
	texture.gradient = degrade
	if sorte == "iris":
		degrade.colors = PackedColorArray([Color.WHITE, Color.BLACK])
		texture.fill = GradientTexture2D.FILL_RADIAL
		texture.fill_from = Vector2(0.5, 0.5)
		texture.fill_to = Vector2(1.0, 0.5)
	else:
		degrade.colors = PackedColorArray([Color.BLACK, Color.WHITE])
		texture.fill_from = Vector2(0, 0.5)
		texture.fill_to = Vector2(1, 0.5)
	return texture


## Label stylé (contour sombre) ajouté à `parent`.
static func etiquette(parent: Node, position: Vector2, texte: String, taille: int = 18, couleur: Color = TEXTE) -> Label:
	var label := Label.new()
	label.position = position
	label.text = texte
	label.add_theme_font_size_override("font_size", taille)
	label.add_theme_color_override("font_color", couleur)
	label.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.6))
	label.add_theme_constant_override("outline_size", 4)
	parent.add_child(label)
	return label


class Grille extends Node2D:
	var zone := Rect2(0, 0, 1152, 648)

	func _draw() -> void:
		for x in range(int(zone.position.x), int(zone.end.x) + 1, 48):
			draw_line(Vector2(x, zone.position.y), Vector2(x, zone.end.y), Color(1, 1, 1, 0.04))
		for y in range(int(zone.position.y), int(zone.end.y) + 1, 48):
			draw_line(Vector2(zone.position.x, y), Vector2(zone.end.x, y), Color(1, 1, 1, 0.04))
