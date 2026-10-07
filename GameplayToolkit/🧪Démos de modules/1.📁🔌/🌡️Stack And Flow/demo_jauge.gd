extends Control

## 🌡️ Démo de la Gauge (module Stack And Flow).
##
## - Au lancement, exécute la suite de tests automatiques (suite_tests_jauge.gd) et affiche le bilan.
## - Partie interactive : une jauge de vie (ressource configurée dans la scène, seuils à 25 / 50 / 75 %)
##   reliée à une barre. Infliger des dégâts, soigner, remplir, vider, changer le maximum, et observer
##   dans le journal les signaux émis (valeur, seuils franchis, plein / vide).

const JournalTests := preload("../../2.🧭🌍/journal_tests.gd")
const SuiteTests := preload("suite_tests_jauge.gd")

## Jauge affichée par la démo.
@export var jauge: Gauge

@onready var barre: ProgressBar = %Barre
@onready var etiquette: Label = %Etiquette
@onready var slider_max: HSlider = %SliderMax
@onready var etiquette_max: Label = %EtiquetteMax
@onready var journal: RichTextLabel = %Journal

var _style_barre := StyleBoxFlat.new()


func _ready() -> void:
	if jauge == null:
		jauge = Gauge.new()

	for bouton in find_children("*", "BaseButton"):
		bouton.focus_mode = Control.FOCUS_NONE
	barre.add_theme_stylebox_override("fill", _style_barre)

	jauge.current_value_changed.connect(_sur_valeur_changee)
	jauge.max_value_changed.connect(func(valeur: float): _journaliser("[color=gray]📏 max_value_changed(%.0f)[/color]" % valeur))
	jauge.threshold_crossed_down.connect(func(seuil: float): _journaliser("[color=orange]📉 threshold_crossed_down(%.0f %%)[/color]" % seuil))
	jauge.threshold_crossed_up.connect(func(seuil: float): _journaliser("[color=cyan]📈 threshold_crossed_up(%.0f %%)[/color]" % seuil))
	jauge.filled.connect(func(): _journaliser("[color=green][b]💚 filled[/b][/color]"))
	jauge.depleted.connect(func(): _journaliser("[color=red][b]💀 depleted[/b][/color]"))
	jauge.changed.connect(_rafraichir)

	%BoutonDegats.pressed.connect(func(): _journaliser("⚔️ add(-15) → variation appliquée : %.0f" % jauge.add(-15.0)))
	%BoutonSoin.pressed.connect(func(): _journaliser("🧪 add(+10) → variation appliquée : %+.0f" % jauge.add(10.0)))
	%BoutonRemplir.pressed.connect(jauge.fill)
	%BoutonVider.pressed.connect(jauge.deplete)
	%BoutonTests.pressed.connect(_lancer_tests)
	slider_max.value = jauge.max_value
	slider_max.value_changed.connect(func(valeur: float): jauge.max_value = valeur)

	_rafraichir()
	_lancer_tests()


func _sur_valeur_changee(nouvelle: float, precedente: float) -> void:
	_journaliser("current_value_changed(%.0f, %.0f)" % [nouvelle, precedente])


func _rafraichir() -> void:
	barre.min_value = jauge.min_value
	barre.max_value = jauge.max_value
	barre.value = jauge.current_value
	etiquette.text = "PV : %.0f / %.0f  (%.0f %%)  ·  seuils : %s" % [
		jauge.current_value, jauge.max_value, jauge.get_percent(), ", ".join(Array(jauge.thresholds).map(func(s): return "%.0f %%" % s))]
	etiquette_max.text = "Maximum : %.0f" % jauge.max_value

	var ratio := jauge.get_ratio()
	_style_barre.bg_color = Color.FIREBRICK if ratio <= 0.25 else Color.ORANGE if ratio <= 0.5 else Color.SEA_GREEN


func _journaliser(texte: String) -> void:
	journal.append_text(texte + "\n")


# 🧪 Tests automatiques

func _lancer_tests() -> void:
	var resultats: Array[Dictionary] = await SuiteTests.new().executer(self)
	_journaliser(JournalTests.formater("Tests de la jauge", resultats))
