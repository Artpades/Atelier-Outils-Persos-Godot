extends SceneTree

## 🧪 Lance toutes les suites de tests des modules Widgets & Spatialité, sans interface.
##
## Depuis le dossier du projet Godot :
##   godot --headless --path . -s "res://🧪Démos de modules/2.🧭🌍/lancer_tous_les_tests.gd"
## Code de sortie : nombre total d'échecs (0 = tout est passé).

const JournalTests := preload("journal_tests.gd")

const SUITES := {
	"Caméras": preload("🎞️Caméras/suite_tests_cameras.gd"),
	"Générateur d'objet": preload("🏭Générateur d'objet/suite_tests_generateur.gd"),
	"Gestionnaire de scènes": preload("🎬Gestionnaire de scènes/suite_tests_gestionnaire_scenes.gd"),
	"Changeur de visuels": preload("🎭Changeur de visuels/suite_tests_visuels.gd"),
}


func _initialize() -> void:
	_lancer.call_deferred()


func _lancer() -> void:
	var hote := Node.new()
	hote.name = "HoteTests"
	root.add_child(hote)

	var total_echecs := 0
	for titre in SUITES:
		var suite: RefCounted = SUITES[titre].new()
		# Sans scène courante à préserver, les tests qui changent de scène peuvent tourner.
		if "autoriser_changement_de_scene" in suite:
			suite.autoriser_changement_de_scene = true
		var resultats: Array[Dictionary] = await suite.executer(hote)
		JournalTests.formater(titre, resultats)
		total_echecs += JournalTests.echecs(resultats)

	print_rich("[b]Total : %s[/b]" % ("[color=green]aucun échec[/color]" if total_echecs == 0 else "[color=red]%d échec(s)[/color]" % total_echecs))
	quit(total_echecs)
