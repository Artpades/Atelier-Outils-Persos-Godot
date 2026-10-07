extends SceneTree

## 🧪 Lance la suite de tests du module Multijoueur (bibliothèque multiplayer), sans interface.
##
## Depuis le dossier du projet Godot :
##   godot --headless --path . --quit-after 6000 -s "res://🧪Démos de modules/1.📁🔌/🗄️Multijoueur/lancer_tous_les_tests.gd"
## Code de sortie : nombre total d'échecs (0 = tout est passé).
## Utilise les ports UDP/TCP 47700-47702 (découverte) et 47811 et suivants (sessions) sur 127.0.0.1.

const JournalTests := preload("../../2.🧭🌍/journal_tests.gd")

const SUITES := {
	"Multijoueur (SecureChannel, LanDiscovery, NetworkSession)": preload("suite_tests_multijoueur.gd"),
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
