extends BTAction

## Action de test : renvoie les statuts de resultats dans l'ordre (le dernier se répète ensuite) et compte
## ses passages dans _on_enter / _tick / _on_exit.

var resultats: Array[int] = [SUCCESS]
var ticks := 0
var entrees := 0
var sorties := 0


func _on_enter() -> void:
	entrees += 1


func _on_exit() -> void:
	sorties += 1


func _tick(_delta: float) -> int:
	var resultat := resultats[mini(ticks, resultats.size() - 1)]
	ticks += 1
	return resultat
