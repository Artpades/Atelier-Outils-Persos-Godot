extends PanelContainer

## Panneau de commande commun aux démos Widgets & Spatialité, ancré à droite de l'écran.
##
## La scène de démo ne contient que ce PanelContainer (titre et aide dans l'inspecteur) : la démo y ajoute
## ses commandes depuis son _ready (ajouter_bouton, ajouter_case...). En bas, un journal affiche les
## événements et le résultat des tests automatiques (lancer_tests).
##
## Les commandes ne prennent pas le focus : les flèches restent disponibles pour le jeu (CameraPanner...).

const JournalTests := preload("journal_tests.gd")

@export var titre: String = "Démo"
@export_multiline var aide: String = ""
@export var largeur: float = 440.0

var _contenu: VBoxContainer
var _journal: RichTextLabel


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_RIGHT_WIDE)
	offset_left = -largeur

	var marges := MarginContainer.new()
	for cote in ["left", "top", "right", "bottom"]:
		marges.add_theme_constant_override("margin_" + cote, 12)
	add_child(marges)

	_contenu = VBoxContainer.new()
	_contenu.add_theme_constant_override("separation", 6)
	marges.add_child(_contenu)

	var etiquette_titre := Label.new()
	etiquette_titre.text = titre
	etiquette_titre.add_theme_font_size_override("font_size", 20)
	_contenu.add_child(etiquette_titre)

	_journal = RichTextLabel.new()
	_journal.bbcode_enabled = true
	_journal.scroll_following = true
	_journal.selection_enabled = true
	_journal.size_flags_vertical = Control.SIZE_EXPAND_FILL
	_journal.custom_minimum_size.y = 160
	# Le journal reste en dernier : les commandes sont insérées avant lui.
	_contenu.add_child(_journal)

	if aide:
		var etiquette_aide := ajouter_etiquette(aide)
		etiquette_aide.modulate.a = 0.7


# 🧩 Commandes

func ajouter_section(texte: String) -> Label:
	_inserer(HSeparator.new())
	var etiquette := Label.new()
	etiquette.text = texte
	etiquette.add_theme_font_size_override("font_size", 16)
	return _inserer(etiquette)


## Ajoute un contrôle quelconque (barre de progression...).
func ajouter(controle: Control) -> Control:
	return _inserer(controle)


func ajouter_etiquette(texte: String = "") -> Label:
	var etiquette := Label.new()
	etiquette.text = texte
	etiquette.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	return _inserer(etiquette)


## Rangée horizontale de boutons (ajouter_bouton(..., rangee)).
func ajouter_rangee() -> HBoxContainer:
	var rangee := HBoxContainer.new()
	return _inserer(rangee)


func ajouter_bouton(texte: String, action: Callable, rangee: HBoxContainer = null) -> Button:
	var bouton := Button.new()
	bouton.text = texte
	bouton.focus_mode = Control.FOCUS_NONE
	bouton.pressed.connect(action)
	if rangee:
		bouton.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		rangee.add_child(bouton)
		return bouton
	return _inserer(bouton)


func ajouter_case(texte: String, valeur: bool, action: Callable) -> CheckBox:
	var case := CheckBox.new()
	case.text = texte
	case.button_pressed = valeur
	case.focus_mode = Control.FOCUS_NONE
	case.toggled.connect(action)
	return _inserer(case)


func ajouter_curseur(texte: String, minimum: float, maximum: float, pas: float, valeur: float, action: Callable) -> HSlider:
	var rangee := ajouter_rangee()
	var etiquette := Label.new()
	etiquette.custom_minimum_size.x = 150
	rangee.add_child(etiquette)
	var curseur := HSlider.new()
	curseur.min_value = minimum
	curseur.max_value = maximum
	curseur.step = pas
	curseur.value = valeur
	curseur.focus_mode = Control.FOCUS_NONE
	curseur.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	curseur.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	rangee.add_child(curseur)
	var maj_etiquette := func(v: float) -> void: etiquette.text = "%s : %s" % [texte, str(snappedf(v, pas))]
	maj_etiquette.call(valeur)
	curseur.value_changed.connect(maj_etiquette)
	curseur.value_changed.connect(action)
	return curseur


func ajouter_options(texte: String, options: PackedStringArray, selection: int, action: Callable) -> OptionButton:
	var rangee := ajouter_rangee()
	var etiquette := Label.new()
	etiquette.text = texte
	etiquette.custom_minimum_size.x = 150
	rangee.add_child(etiquette)
	var liste := OptionButton.new()
	for option in options:
		liste.add_item(option)
	liste.select(selection)
	liste.focus_mode = Control.FOCUS_NONE
	liste.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	liste.item_selected.connect(action)
	rangee.add_child(liste)
	return liste


# 📰 Journal et tests

func journaliser(message: String) -> void:
	_journal.append_text(message + "\n")


func vider_journal() -> void:
	_journal.clear()


## Exécute une suite (script dérivé de suite_tests_base.gd) sous p_hote et affiche son bilan.
func lancer_tests(titre_suite: String, suite: GDScript, hote: Node) -> void:
	journaliser("[i]🧪 %s : tests en cours...[/i]" % titre_suite)
	var resultats: Array[Dictionary] = await suite.new().executer(hote)
	journaliser(JournalTests.formater(titre_suite, resultats) + "\n")


func _inserer(controle: Control) -> Control:
	_contenu.add_child(controle)
	_contenu.move_child(controle, _journal.get_index())
	return controle
