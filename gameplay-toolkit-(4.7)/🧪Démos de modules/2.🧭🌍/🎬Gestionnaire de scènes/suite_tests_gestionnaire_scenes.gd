extends "../suite_tests_base.gd"

## Suite de tests automatiques du module gestionnaire de scènes : SceneManager.
##
## Un chargement complet CHANGE la scène courante : depuis la démo, il détruirait la démo elle-même.
## Ces tests ne tournent donc que si autoriser_changement_de_scene est vrai (lanceur sans interface
## lancer_tous_les_tests.gd) ; dans la démo, le cycle complet se teste à la main (bouton « Charger »).

const DESTINATION := "res://🧪Démos de modules/2.🧭🌍/🎬Gestionnaire de scènes/Scène de destination.tscn"

## Active les tests qui changent la scène courante (puis la déchargent).
var autoriser_changement_de_scene := false

var _gestionnaire: SceneManager


func _executer() -> void:
	await _tester_singleton()
	if _gestionnaire == null:
		return
	_tester_proprietes()
	await _tester_chemin_vide()
	await _tester_chemin_inexistant()
	await _tester_ressource_pas_une_scene()
	if autoriser_changement_de_scene:
		await _tester_cycle_complet()
		await _tester_changement_exterieur()
	else:
		_avertir_si("Cycle complet de chargement (change la scène courante)", false,
			"non exécuté depuis la démo : utiliser le bouton « Charger la scène de destination », ou lancer_tous_les_tests.gd")


# 🛠️ Singleton (ManagerBuilder)

func _tester_singleton() -> void:
	_gestionnaire = SceneManager.get_instance()
	_verifier("SceneManager : get_instance renvoie une instance", _gestionnaire != null,
		"nul : le module tool_editor (ManagerBuilder) est-il chargé ?")
	if _gestionnaire == null:
		return
	_verifier("SceneManager : get_instance renvoie toujours la même instance", SceneManager.get_instance() == _gestionnaire)
	await _frames()
	_verifier("SceneManager : singleton sous la racine (survit aux changements de scène)",
		_gestionnaire.get_parent() == _gestionnaire.get_tree().root)

	var doublon := SceneManager.new()
	_ajouter(doublon)
	_verifier("SceneManager : un second SceneManager est détruit (doublon)", doublon.is_queued_for_deletion())
	await _frames()
	_verifier("SceneManager : le singleton reste le premier", SceneManager.get_instance() == _gestionnaire)


func _tester_proprietes() -> void:
	_verifier("SceneManager : aucun chargement en cours au repos", not _gestionnaire.is_loading())
	var initial := _gestionnaire.log_activated
	_gestionnaire.log_activated = not initial
	_verifier("SceneManager : log_activated (setter / getter)", _gestionnaire.log_activated == not initial)
	_gestionnaire.log_activated = initial


# ❌ Requêtes refusées ou en échec

func _tester_chemin_vide() -> void:
	var debut := _espionner(_gestionnaire.loading_started)
	_gestionnaire.load_scene_async("")
	_verifier("Chemin vide : refusé, loading_started non émis", debut.n == 0 and not _gestionnaire.is_loading())
	await _frames()


func _tester_chemin_inexistant() -> void:
	var scene_courante := _bac_a_sable.get_tree().current_scene
	var fin := _espionner(_gestionnaire.load_finished)
	_gestionnaire.load_scene_async("res://inexistant/aucune_scene.tscn")
	await _attendre_fin_chargement()
	_verifier("Chemin inexistant : le chargement ne reste pas bloqué", not _gestionnaire.is_loading())
	await _frames(3)
	_verifier("Chemin inexistant : pas de changement de scène ni de load_finished",
		_bac_a_sable.get_tree().current_scene == scene_courante and fin.n == 0)


func _tester_ressource_pas_une_scene() -> void:
	var scene_courante := _bac_a_sable.get_tree().current_scene
	var fin := _espionner(_gestionnaire.load_finished)
	_gestionnaire.load_scene_async("res://icon.svg")
	await _attendre_fin_chargement()
	_verifier("Ressource qui n'est pas une scène : le chargement ne reste pas bloqué", not _gestionnaire.is_loading())
	await _frames(3)
	_verifier("Ressource qui n'est pas une scène : pas de changement de scène",
		_bac_a_sable.get_tree().current_scene == scene_courante and fin.n == 0)


# ✅ Cycle complet (change la scène courante)

func _tester_cycle_complet() -> void:
	var arbre := _bac_a_sable.get_tree()
	var debut := _espionner(_gestionnaire.loading_started)
	var fin := _espionner(_gestionnaire.load_finished)
	var progressions: Array[float] = []
	var enregistrer := func(pourcentage: float) -> void: progressions.append(pourcentage)
	_gestionnaire.loading_progress.connect(enregistrer)

	_gestionnaire.load_scene_async(DESTINATION)
	_verifier("Cycle complet : loading_started émis, chargement en cours", debut.n == 1 and _gestionnaire.is_loading())
	_gestionnaire.load_scene_async(DESTINATION)
	_verifier("Cycle complet : une seconde requête pendant le chargement est ignorée", debut.n == 1)

	await _attendre_fin_chargement()
	for i in 30:
		if fin.n > 0:
			break
		await _frames()
	_gestionnaire.loading_progress.disconnect(enregistrer)

	var croissante := true
	for i in range(1, progressions.size()):
		croissante = croissante and progressions[i] >= progressions[i - 1]
	_verifier("Cycle complet : loading_progress croissant, jusqu'à 100",
		not progressions.is_empty() and croissante and is_equal_approx(progressions[-1], 100.0), str(progressions))
	_verifier("Cycle complet : get_progress_percent vaut 100 à la fin", is_equal_approx(_gestionnaire.get_progress_percent(), 100.0))
	_verifier("Cycle complet : load_finished émis une fois", fin.n == 1, "émissions : %d" % fin.n)
	_verifier("Cycle complet : la scène de destination est la scène courante",
		arbre.current_scene != null and arbre.current_scene.scene_file_path == DESTINATION, str(arbre.current_scene))
	_verifier("Cycle complet : plus de chargement en cours", not _gestionnaire.is_loading())

	arbre.unload_current_scene()
	await _frames()


func _tester_changement_exterieur() -> void:
	var arbre := _bac_a_sable.get_tree()
	var fin := _espionner(_gestionnaire.load_finished)
	var racine := Node.new()
	racine.name = "SceneExterieure"
	arbre.change_scene_to_packed(_empaqueter(racine))
	await arbre.scene_changed
	await _frames()
	_verifier("Changement de scène fait par un autre code : load_finished non émis", fin.n == 0)
	arbre.unload_current_scene()
	await _frames()


# 🛠️ Utilitaires

## Attend la fin du chargement en cours (au plus 5 s).
func _attendre_fin_chargement() -> void:
	var limite := Time.get_ticks_msec() + 5000
	while _gestionnaire.is_loading() and Time.get_ticks_msec() < limite:
		await _frames()
