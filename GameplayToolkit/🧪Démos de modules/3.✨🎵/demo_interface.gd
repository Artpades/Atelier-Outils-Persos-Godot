extends Control

## ✨ Démo interface et sensations : CursorChanger / CursorStyle, CustomTooltip / TooltipContent, FloatingDock,
## ScreenFader, et des effets (FXChangeSize, FXShake + FXAudio) appliqués à des boutons.
##
## - Le curseur global (flèche, rouge quand on clique) est remplacé au survol des boutons (main, chargement animé).
## - Chaque bouton et chaque icône du dock a une infobulle : texte BBCode, icône, vidéo, scène personnalisée...
## - Le panneau règle le dock, les infobulles, et lance des fondus d'écran (couleur, masques, texture).
## Le panneau est dans un CanvasLayer au-dessus du voile : il reste utilisable quand l'écran est couvert.

const Panneau := preload("../2.🧭🌍/panneau_demo.gd")

## Masques de transition du ScreenFader (niveaux de gris : le noir est couvert en premier).
@export var masque_volet: Texture2D
@export var masque_cercle: Texture2D
@export var masque_dissolution: Texture2D
## Texture affichée par le voile en mode « Texture ».
@export var texture_voile: Texture2D

@onready var panneau: Panneau = $Interface/Panneau
@onready var bouton_survol: Button = $Boutons/Survol
@onready var bouton_clic: Button = $Boutons/Clic
@onready var curseur_global: CursorChanger = $CurseurGlobal
@onready var dock: FloatingDock = $Dock
@onready var voile: ScreenFader = $Voile

var _duree_fondu := 0.6


func _ready() -> void:
	# Effets appliqués aux boutons
	bouton_survol.mouse_entered.connect($Boutons/Survol/PlayerFX.play)
	bouton_survol.mouse_exited.connect($Boutons/Survol/PlayerFX.stop)
	bouton_clic.pressed.connect($Boutons/Clic/PlayerFX.play)

	dock.hovered_item_changed.connect(func(item: Control): if item: panneau.journaliser("🛟 Survol : %s" % item.name))
	voile.faded_out.connect(func(): panneau.journaliser("🌈 Écran couvert"))
	voile.faded_in.connect(func(): panneau.journaliser("🌈 Écran découvert"))

	panneau.ajouter_section("🌈 Fondu d'écran")
	panneau.ajouter_options("Forme", PackedStringArray(["Couleur unie", "Volet", "Cercle", "Dissolution", "Texture"]), 0, _choisir_forme)
	var couleur := ColorPickerButton.new()
	couleur.color = voile.color
	couleur.custom_minimum_size.y = 28
	couleur.focus_mode = Control.FOCUS_NONE
	couleur.color_changed.connect(func(c: Color): voile.color = c)
	panneau.ajouter(couleur)
	panneau.ajouter_curseur("Durée (s)", 0.0, 2.0, 0.05, _duree_fondu, func(v: float): _duree_fondu = v)
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("Couvrir", func(): voile.fade_out(_duree_fondu), rangee)
	panneau.ajouter_bouton("Découvrir", func(): voile.fade_in(_duree_fondu), rangee)
	panneau.ajouter_bouton("Aller-retour", _aller_retour, rangee)

	panneau.ajouter_section("🛟 Dock")
	panneau.ajouter_curseur("Grossissement", 1.0, 3.0, 0.05, dock.magnification, func(v: float): dock.magnification = v)
	panneau.ajouter_curseur("Rayon d'influence", 40.0, 400.0, 5.0, dock.influence_radius, func(v: float): dock.influence_radius = v)
	panneau.ajouter_curseur("Espace au survol", 0.0, 40.0, 1.0, dock.hover_spacing, func(v: float): dock.hover_spacing = v)
	panneau.ajouter_curseur("Soulèvement", 0.0, 40.0, 1.0, dock.hover_lift, func(v: float): dock.hover_lift = v)
	panneau.ajouter_options("Agrandir par", PackedStringArray(["Taille", "Échelle"]), 0, func(i: int): dock.resize_mode = i)

	panneau.ajouter_section("👆 Curseur et 🗨️ infobulles")
	panneau.ajouter_case("Curseur personnalisé global", true, func(actif: bool): curseur_global.activate() if actif else curseur_global.deactivate())
	panneau.ajouter_curseur("Délai des infobulles", 0.0, 1.5, 0.05, 0.5, _regler_infobulles.bind("delay"))
	panneau.ajouter_case("Infobulles suivent la souris", true, _regler_infobulles.bind("follow_mouse"))


func _choisir_forme(index: int) -> void:
	voile.mask = [null, masque_volet, masque_cercle, masque_dissolution, null][index]
	voile.texture = texture_voile if index == 4 else null


func _aller_retour() -> void:
	voile.fade_out(_duree_fondu)
	await voile.faded_out
	await get_tree().create_timer(0.3).timeout
	voile.fade_in(_duree_fondu)


## Applique une valeur à toutes les infobulles de la scène (sauf la « Carte », réglée à part).
func _regler_infobulles(valeur: Variant, propriete: String) -> void:
	for infobulle in find_children("*", "CustomTooltip", true, false):
		if infobulle.get_parent().name != "Carte":
			infobulle.set(propriete, valeur)
