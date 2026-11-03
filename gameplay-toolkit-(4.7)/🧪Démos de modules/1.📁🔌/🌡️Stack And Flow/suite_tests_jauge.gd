extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques de la Gauge (module Stack And Flow) : bornage, signaux de valeur,
## bornes min/max, ratio, seuils (ordre et cas limites), sauvegarde / rechargement.

const DOSSIER_TESTS := "user://tests_jauge/"


func _executer() -> void:
	_tester_valeurs_par_defaut()
	_tester_bornage()
	_tester_signal_valeur()
	_tester_add()
	_tester_bornes()
	_tester_ratio()
	_tester_seuils_descente()
	_tester_seuils_montee()
	_tester_seuils_limites()
	_tester_plein_et_vide()
	_tester_inspecteur()
	_tester_sauvegarde()
	await _frames(0)


# 🛠️ Utilitaires

## Jauge neuve [p_min ; p_max] à p_valeur, avec p_seuils.
func _jauge(p_min := 0.0, p_max := 100.0, p_valeur := 100.0, p_seuils := PackedFloat32Array()) -> Gauge:
	var jauge := Gauge.new()
	jauge.min_value = p_min
	jauge.max_value = p_max
	jauge.current_value = p_valeur
	jauge.thresholds = p_seuils
	return jauge


## Enregistre dans l'ordre les émissions des signaux de seuils, plein et vide : ["bas:50", "haut:75", "vide"...].
func _journal_seuils(jauge: Gauge) -> Array[String]:
	var journal: Array[String] = []
	jauge.threshold_crossed_down.connect(func(seuil: float): journal.append("bas:%d" % seuil))
	jauge.threshold_crossed_up.connect(func(seuil: float): journal.append("haut:%d" % seuil))
	jauge.filled.connect(func(): journal.append("plein"))
	jauge.depleted.connect(func(): journal.append("vide"))
	return journal


# 🧪 Tests

func _tester_valeurs_par_defaut() -> void:
	var jauge := Gauge.new()
	_verifier("Valeurs par défaut : 0 / 100 / 100", jauge.min_value == 0.0 and jauge.max_value == 100.0 and jauge.current_value == 100.0,
		"min %s, max %s, valeur %s" % [jauge.min_value, jauge.max_value, jauge.current_value])
	_verifier("Jauge par défaut pleine", jauge.is_full() and not jauge.is_empty())


func _tester_bornage() -> void:
	var jauge := _jauge(10.0, 50.0, 30.0)
	jauge.current_value = 80.0
	_verifier("Valeur trop haute bornée au maximum", jauge.current_value == 50.0, "valeur %s" % jauge.current_value)
	jauge.current_value = -5.0
	_verifier("Valeur trop basse bornée au minimum", jauge.current_value == 10.0, "valeur %s" % jauge.current_value)


func _tester_signal_valeur() -> void:
	var jauge := _jauge(0.0, 100.0, 40.0)
	var espion := _espionner(jauge.current_value_changed)
	jauge.current_value = 60.0
	_verifier("current_value_changed(nouvelle, précédente)", espion.n == 1 and espion.args == [60.0, 40.0], "émissions %d, args %s" % [espion.n, espion.args])
	_verifier("get_previous_value", jauge.get_previous_value() == 40.0, "précédente %s" % jauge.get_previous_value())

	jauge.current_value = 60.0
	_verifier("Pas de signal si la valeur ne change pas", espion.n == 1, "émissions %d" % espion.n)

	jauge.current_value = 100.0
	jauge.current_value = 500.0
	_verifier("Pas de signal si la valeur bornée ne change pas", espion.n == 2, "émissions %d" % espion.n)

	var espion_ressource := _espionner(jauge.changed)
	jauge.current_value = 10.0
	_verifier("Resource.changed émis au changement de valeur", espion_ressource.n == 1, "émissions %d" % espion_ressource.n)


func _tester_add() -> void:
	var jauge := _jauge(0.0, 100.0, 10.0)
	var applique := jauge.add(-30.0)
	_verifier("add() renvoie la variation réellement appliquée (retrait)", applique == -10.0 and jauge.current_value == 0.0, "variation %s, valeur %s" % [applique, jauge.current_value])
	applique = jauge.add(25.0)
	_verifier("add() renvoie la variation réellement appliquée (ajout)", applique == 25.0 and jauge.current_value == 25.0, "variation %s, valeur %s" % [applique, jauge.current_value])
	applique = jauge.add(500.0)
	_verifier("add() au-delà du maximum", applique == 75.0 and jauge.is_full(), "variation %s" % applique)


func _tester_bornes() -> void:
	var jauge := _jauge(0.0, 100.0, 80.0)
	var espion_min := _espionner(jauge.min_value_changed)
	var espion_max := _espionner(jauge.max_value_changed)

	jauge.max_value = 50.0
	_verifier("Baisser le maximum reborne la valeur", jauge.current_value == 50.0, "valeur %s" % jauge.current_value)
	_verifier("max_value_changed émis", espion_max.n == 1 and espion_max.args[0] == 50.0, "émissions %d" % espion_max.n)

	jauge.min_value = 70.0
	_verifier("Un minimum au-dessus du maximum relève le maximum", jauge.max_value == 70.0 and jauge.current_value == 70.0,
		"max %s, valeur %s" % [jauge.max_value, jauge.current_value])
	_verifier("min_value_changed et max_value_changed émis", espion_min.n == 1 and espion_max.n == 2, "min %d, max %d" % [espion_min.n, espion_max.n])

	jauge.max_value = 20.0
	_verifier("Un maximum sous le minimum abaisse le minimum", jauge.min_value == 20.0 and jauge.current_value == 20.0,
		"min %s, valeur %s" % [jauge.min_value, jauge.current_value])

	jauge.max_value = 20.0
	_verifier("Pas de signal si le maximum ne change pas", espion_max.n == 3, "émissions %d" % espion_max.n)


func _tester_ratio() -> void:
	var jauge := _jauge(50.0, 150.0, 75.0)
	_verifier("get_ratio sur une plage décalée", is_equal_approx(jauge.get_ratio(), 0.25), "ratio %s" % jauge.get_ratio())
	_verifier("get_percent", is_equal_approx(jauge.get_percent(), 25.0), "pourcentage %s" % jauge.get_percent())
	jauge.set_ratio(0.5)
	_verifier("set_ratio", jauge.current_value == 100.0, "valeur %s" % jauge.current_value)
	jauge.set_ratio(2.0)
	_verifier("set_ratio au-delà de 1 borné", jauge.is_full(), "valeur %s" % jauge.current_value)

	var nulle := _jauge(10.0, 10.0, 10.0)
	_verifier("Ratio 0 sur une plage nulle (pas de division par zéro)", nulle.get_ratio() == 0.0, "ratio %s" % nulle.get_ratio())


func _tester_seuils_descente() -> void:
	var jauge := _jauge(0.0, 200.0, 200.0, PackedFloat32Array([50.0, 25.0, 75.0]))
	var journal := _journal_seuils(jauge)

	jauge.current_value = 160.0 # 80 %
	_verifier("Pas de seuil franchi au-dessus de 75 %", journal.is_empty(), str(journal))

	jauge.current_value = 20.0 # 10 % : traverse 75, 50 puis 25
	_verifier("Grand saut vers le bas : seuils signalés dans l'ordre décroissant", journal == ["bas:75", "bas:50", "bas:25"], str(journal))

	journal.clear()
	jauge.current_value = 10.0
	_verifier("Pas de seuil re-signalé en restant en dessous", journal.is_empty(), str(journal))


func _tester_seuils_montee() -> void:
	var jauge := _jauge(0.0, 100.0, 0.0, PackedFloat32Array([75.0, 25.0, 50.0]))
	var journal := _journal_seuils(jauge)

	jauge.current_value = 60.0
	_verifier("Grand saut vers le haut : seuils signalés dans l'ordre croissant", journal == ["haut:25", "haut:50"], str(journal))

	journal.clear()
	jauge.current_value = 75.0
	_verifier("Atteindre exactement un seuil en montant le franchit", journal == ["haut:75"], str(journal))


func _tester_seuils_limites() -> void:
	var jauge := _jauge(0.0, 30.0, 15.0, PackedFloat32Array([0.0, 100.0]))
	var journal := _journal_seuils(jauge)

	jauge.deplete()
	_verifier("Seuil 0 % franchi en vidant la jauge", journal == ["bas:0", "vide"], str(journal))

	journal.clear()
	jauge.fill()
	_verifier("Seuil 100 % franchi en remplissant la jauge", journal == ["haut:100", "plein"], str(journal))

	# Plage où min + 100 % × (max - min) n'est pas exactement max en flottant.
	var fine := _jauge(0.1, 0.3, 0.1, PackedFloat32Array([100.0]))
	var journal_fin := _journal_seuils(fine)
	fine.fill()
	_verifier("Seuil 100 % atteint malgré les arrondis", journal_fin.has("haut:100"), str(journal_fin))


func _tester_plein_et_vide() -> void:
	var jauge := _jauge(0.0, 100.0, 50.0)
	var journal := _journal_seuils(jauge)
	jauge.add(-20.0)
	jauge.add(10.0)
	_verifier("Ni plein ni vide au milieu", journal.is_empty(), str(journal))
	jauge.add(-1000.0)
	jauge.add(-5.0)
	_verifier("depleted émis une seule fois", journal == ["vide"], str(journal))
	_verifier("is_empty", jauge.is_empty() and not jauge.is_full())


func _tester_inspecteur() -> void:
	var jauge := _jauge(-20.0, 40.0, 0.0)
	var propriete: Dictionary = {}
	for p in jauge.get_property_list():
		if p.name == "current_value":
			propriete = p
	_verifier("current_value affiché en curseur borné min/max dans l'inspecteur",
		propriete.get("hint") == PROPERTY_HINT_RANGE and Array(str(propriete.get("hint_string")).split_floats(",")).slice(0, 2) == [-20.0, 40.0], str(propriete))


## Régression du script C# : les bornes étaient chargées après la valeur, une jauge [0 ; 200] à 150
## revenait à 100 au rechargement.
func _tester_sauvegarde() -> void:
	DirAccess.make_dir_recursive_absolute(DOSSIER_TESTS)
	var chemin := DOSSIER_TESTS + "jauge.tres"
	var jauge := _jauge(10.0, 200.0, 150.0, PackedFloat32Array([30.0]))
	var erreur := ResourceSaver.save(jauge, chemin)
	_verifier("Sauvegarde .tres", erreur == OK, error_string(erreur))

	var rechargee := ResourceLoader.load(chemin, "", ResourceLoader.CACHE_MODE_IGNORE) as Gauge
	_verifier("Rechargement : bornes et valeur intactes", rechargee != null and rechargee.min_value == 10.0
		and rechargee.max_value == 200.0 and rechargee.current_value == 150.0,
		"rechargée : %s" % str([rechargee.min_value, rechargee.max_value, rechargee.current_value] if rechargee else null))
	_verifier("Rechargement : seuils intacts", rechargee != null and rechargee.thresholds == PackedFloat32Array([30.0]))

	DirAccess.remove_absolute(chemin)
	DirAccess.remove_absolute(DOSSIER_TESTS)
