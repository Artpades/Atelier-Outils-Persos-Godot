extends Node2D

## 🕹️ Socle des aperçus interactifs de l'export web (demos/web/index.html?demo=<page>).
##
## Disposition : zone de jeu à gauche (ZONE, 712 × 648), panneau de commande à droite (panneau_apercu.gd),
## code GDScript équivalent en haut à gauche (afficher_code). Un aperçu dérivé surcharge titre(), aide() et
## construire(), et peut définir _jouer() : l'action principale, déclenchée par le mode rapport du lanceur.
## Pas d'emoji dans les textes : le web n'a pas de police de secours pour les afficher (les accents passent).
## Ne rien précharger hors de « 🎥Démos de documentation/ » : l'export web n'embarque que ce dossier.

const Formes := preload("res://🎥Démos de documentation/socle/formes.gd")
const Panneau := preload("res://🎥Démos de documentation/socle/panneau_apercu.gd")
const Traceur := preload("res://🎥Démos de documentation/socle/traceur.gd")

## Zone de jeu (à gauche du panneau).
const ZONE := Rect2(0, 0, 712, 648)
const CENTRE := Vector2(356, 340)

var panneau: Panneau
var interface: CanvasLayer
var _code: Label


func titre() -> String:
	return "Aperçu"


func aide() -> String:
	return ""


## Construit la scène et les commandes (panneau.ajouter_...).
func construire() -> void:
	pass


func _ready() -> void:
	RenderingServer.set_default_clear_color(Formes.FOND)
	interface = CanvasLayer.new()
	add_child(interface)
	_code = Label.new()
	_code.position = Vector2(16, 12)
	_code.add_theme_color_override("font_color", Formes.CODE)
	_code.add_theme_font_size_override("font_size", 14)
	interface.add_child(_code)
	panneau = Panneau.new()
	panneau.titre = titre()
	panneau.aide = aide()
	interface.add_child(panneau)
	construire()


## Code GDScript affiché en haut à gauche (ce que ferait un script pour obtenir la même chose).
func afficher_code(texte: String) -> void:
	_code.text = texte


func journaliser(message: String) -> void:
	panneau.journaliser(message)


# Raccourcis vers formes.gd, dans cet aperçu

func carre(position: Vector2, couleur: Color, cote: float = 80.0) -> Node2D:
	return Formes.carre(self, position, couleur, cote)


func disque(position: Vector2, couleur: Color, rayon: float = 40.0) -> Node2D:
	return Formes.disque(self, position, couleur, rayon)


func vue_2d(cadre: Rect2, fond: Color = Formes.FOND_VUE) -> SubViewport:
	return Formes.vue_2d(self, cadre, fond)


func vue_3d(cadre: Rect2) -> SubViewport:
	return Formes.vue_3d(self, cadre)


func etiquette(position: Vector2, texte: String, taille: int = 16, couleur: Color = Formes.TEXTE_DOUX) -> Label:
	return Formes.etiquette(self, position, texte, taille, couleur)


## Traceur de `mesure` (Callable -> float), large de `largeur`, centré sur `position`.
func traceur(position: Vector2, mesure: Callable, couleur: Color = Formes.VARIANTES[0], echelle: float = 1.5,
		largeur: float = 420.0) -> Traceur:
	var trace := Traceur.new()
	trace.mesure = mesure
	trace.couleur = couleur
	trace.echelle = echelle
	trace.largeur = largeur
	trace.position = position - Vector2(largeur / 2.0, 0)
	add_child(trace)
	return trace
