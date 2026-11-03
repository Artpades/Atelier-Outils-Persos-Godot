extends SceneTree

## 🧪 Lance les suites de tests du module Mouvement & Physique (bibliothèque movement_physics), sans interface.
##
## Depuis le dossier du projet Godot :
##   godot --headless --path . -s "res://🧪Démos de modules/5.🌀🫗/lancer_tous_les_tests.gd"
## Code de sortie : nombre total d'échecs (0 = tout est passé).

const JournalTests := preload("../2.🧭🌍/journal_tests.gd")

const SUITES := {
	"Mouvement & Physique": preload("suite_tests_mouvement.gd"),
}


func _initialize() -> void:
	_lancer.call_deferred()


func _lancer() -> void:
	var hote := Node.new()
	hote.name = "HoteTests"
	root.add_child(hote)

	var total_echecs := 0
	for titre in SUITES:
		var resultats: Array[Dictionary] = await SUITES[titre].new().executer(hote)
		JournalTests.formater(titre, resultats)
		total_echecs += JournalTests.echecs(resultats)

	print_rich("[b]Total : %s[/b]" % ("[color=green]aucun échec[/color]" if total_echecs == 0 else "[color=red]%d échec(s)[/color]" % total_echecs))
	quit(total_echecs)
