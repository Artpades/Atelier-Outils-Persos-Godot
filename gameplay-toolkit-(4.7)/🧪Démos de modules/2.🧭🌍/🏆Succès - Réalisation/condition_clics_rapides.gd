extends AchievementCondition

## Condition personnalisée écrite en GDScript : remplie quand `clics` évènements event_name arrivent en moins
## de `delai` secondes. Montre comment redéfinir _handle_event() pour une logique propre au jeu.

@export var clics: int = 5
@export var delai: float = 1.5

var _instants: Array[float] = []


## Reçoit chaque évènement transmis au succès : renvoie vrai s'il concerne cette condition.
func _handle_event(event: StringName, _id: String, _amount: float) -> bool:
	if event != event_name:
		return false
	var maintenant := Time.get_ticks_msec() / 1000.0
	_instants.append(maintenant)
	_instants.assign(_instants.filter(func(instant: float) -> bool: return maintenant - instant <= delai))
	if _instants.size() >= clics:
		complete()
	return true
