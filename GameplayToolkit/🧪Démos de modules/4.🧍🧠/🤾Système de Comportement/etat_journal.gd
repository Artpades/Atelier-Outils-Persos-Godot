extends State

## État de test : note ses entrées (« +Nom ») et sorties (« -Nom ») dans journal (tableau partagé entre
## les états d'une même machine), compte ses mises à jour et consomme les événements listés.
## rediriger_vers : chemin d'un état vers lequel transiter dès l'entrée (test des transitions en chaîne).

var journal: Array = []
var evenements_consommes: Array[StringName] = []
var rediriger_vers: String = ""
var mises_a_jour := 0
var dernier_evenement: StringName


func _on_enter() -> void:
	journal.append("+" + name)
	if rediriger_vers:
		transition_to(rediriger_vers)


func _on_exit() -> void:
	journal.append("-" + name)


func _on_update(_delta: float) -> void:
	mises_a_jour += 1


func _on_event(event: StringName, _data: Variant) -> bool:
	dernier_evenement = event
	return event in evenements_consommes
