extends SceneTree

## 🧪 Lance les suites de tests des modules Entités & Algorithme (bibliothèque entity_algorithm), sans interface.
##
## Depuis le dossier du projet Godot :
##   godot --headless --path . -s "res://🧪Démos de modules/4.🧍🧠/lancer_tous_les_tests.gd"
## Code de sortie : nombre total d'échecs (0 = tout est passé).

const JournalTests := preload("../2.🧭🌍/journal_tests.gd")

const SUITES := {
	"Machine à états hiérarchique": preload("🤾Système de Comportement/suite_tests_hfsm.gd"),
	"Arbre de comportement": preload("🤾Système de Comportement/suite_tests_arbre.gd"),
	"Graphe de comportement": preload("🤾Système de Comportement/suite_tests_graphe.gd"),
	"Système de capacités": preload("🌟Système de Capacité/suite_tests_capacites.gd"),
	"Système de dialogue": preload("💬Système de Dialogue/suite_tests_dialogue.gd"),
	"Personnages": preload("🧑Personnage/suite_tests_personnages.gd"),
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
