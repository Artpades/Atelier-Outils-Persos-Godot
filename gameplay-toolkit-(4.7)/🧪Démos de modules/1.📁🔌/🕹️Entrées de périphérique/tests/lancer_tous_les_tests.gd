extends SceneTree

## 🧪 Lance les suites de tests du module Entrées de périphérique (bibliothèque input_devices), sans interface.
##
## Depuis le dossier du projet Godot :
##   godot --headless --path . -s "res://🧪Démos de modules/1.📁🔌/🕹️Entrées de périphérique/tests/lancer_tous_les_tests.gd"
## Code de sortie : nombre total d'échecs (0 = tout est passé).

const JournalTests := preload("res://🧪Démos de modules/2.🧭🌍/journal_tests.gd")

const SUITES := {
	"Entrées de périphérique": preload("suite_tests_entrees.gd"),
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
