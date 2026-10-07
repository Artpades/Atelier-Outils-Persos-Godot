extends SceneTree

## 🧪 Lance seulement la suite du Système de dessin, sans interface (la suite fait aussi partie de
## « lancer_tous_les_tests.gd » de Widgets & Spatialité).
##   godot --headless --path . -s "res://🧪Démos de modules/2.🧭🌍/✏️Système de dessin/lancer_tests_dessin.gd"

const JournalTests := preload("../journal_tests.gd")
const Suite := preload("suite_tests_dessin.gd")


func _initialize() -> void:
	_lancer.call_deferred()


func _lancer() -> void:
	var hote := Node.new()
	root.add_child(hote)
	var resultats: Array[Dictionary] = await Suite.new().executer(hote)
	JournalTests.formater("Système de dessin", resultats)
	quit(JournalTests.echecs(resultats))
