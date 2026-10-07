extends "../suite_tests_base.gd"

## Suite de tests automatiques du module de succès : conditions (drapeau, compteur, collection, script),
## Achievement et AchievementManager (évènements, méta-succès, sauvegarde, notifications).
## Erreurs attendues dans la console : id de succès en double, succès / condition introuvables,
## mauvais mot de passe de sauvegarde.

const ConditionClicsRapides := preload("condition_clics_rapides.gd")
const FICHIER_TEST := "user://tests_succes.cfg"


func _executer() -> void:
	_tester_condition_drapeau()
	_tester_compteur()
	_tester_collection()
	_tester_condition_script()
	_tester_succes_toutes()
	_tester_succes_une_seule()
	_tester_suivi_progression()
	_tester_deblocage_force()
	_tester_etat_succes()
	await _tester_gestionnaire()
	await _tester_meta_succes()
	await _tester_raccourcis_gestionnaire()
	await _tester_sauvegarde()
	await _tester_chargement_automatique()
	await _tester_notifications()
	_nettoyer_fichiers()


# 🚩 Conditions

func _tester_condition_drapeau() -> void:
	var condition := _drapeau(&"enemy_killed")
	_verifier("Drapeau : vide au départ", not condition.is_completed() and condition.get_progress() == 0.0 and condition.get_progress_text() == "0/1",
		condition.get_progress_text())
	_verifier("Drapeau : matches (nom d'évènement, n'importe quel id)", condition.matches(&"enemy_killed", "slime") and not condition.matches(&"coin", "slime"))

	condition.event_ids = PackedStringArray(["boss", "dragon"])
	_verifier("Drapeau : event_ids filtre les identifiants", condition.matches(&"enemy_killed", "boss") and not condition.matches(&"enemy_killed", "slime"))
	var remplie := _espionner(condition.completed)
	var change := _espionner(condition.changed)
	_verifier("Drapeau : notify d'un id non listé ignoré", not condition.notify(&"enemy_killed", "slime") and not condition.is_completed())
	_verifier("Drapeau : notify reconnu → rempli", condition.notify(&"enemy_killed", "dragon") and condition.is_completed() and condition.get_progress_text() == "1/1")
	_verifier("Drapeau : completed et changed émis une fois", remplie.n == 1 and change.n == 1, "completed %d, changed %d" % [remplie.n, change.n])
	_verifier("Drapeau : déjà rempli → notify ignoré", not condition.notify(&"enemy_killed", "dragon") and remplie.n == 1)

	condition.reset()
	_verifier("Drapeau : reset", not condition.is_completed() and change.n == 2)
	_verifier("Drapeau : sans event_name, aucun évènement ne correspond", not AchievementCondition.new().matches(&"", ""))
	condition.complete()
	var copie := _drapeau(&"enemy_killed")
	copie.set_state(condition.get_state())
	_verifier("Drapeau : get_state / set_state", copie.is_completed())


func _tester_compteur() -> void:
	var compteur := _compteur(&"coin", 10.0)
	compteur.add(3)
	_verifier("Compteur : add et texte « 3/10 »", compteur.value == 3.0 and compteur.get_progress_text() == "3/10" and is_equal_approx(compteur.get_progress(), 0.3),
		compteur.get_progress_text())
	compteur.notify(&"coin", "", 2.5)
	_verifier("Compteur (ADD) : la quantité de l'évènement s'ajoute, texte décimal", compteur.value == 5.5 and compteur.get_progress_text() == "5.5/10", compteur.get_progress_text())
	var remplie := _espionner(compteur.completed)
	compteur.notify(&"coin", "", 20)
	_verifier("Compteur : rempli à la cible, texte plafonné", compteur.is_completed() and remplie.n == 1 and compteur.get_progress_text() == "10/10" and compteur.get_progress() == 1.0,
		compteur.get_progress_text())

	var record := _compteur(&"score", 1000.0, AchievementCounterCondition.VALUE_MODE_MAX)
	record.notify(&"score", "", 700)
	record.notify(&"score", "", 300)
	_verifier("Compteur (MAX) : garde la meilleure valeur", record.value == 700.0, str(record.value))

	var etat := _compteur(&"owned", 5.0, AchievementCounterCondition.VALUE_MODE_SET)
	etat.notify(&"owned", "", 4)
	etat.notify(&"owned", "", 2)
	_verifier("Compteur (SET) : la dernière valeur remplace", etat.value == 2.0 and not etat.is_completed())
	etat.notify(&"owned", "", 5)
	_verifier("Compteur : une fois rempli, les évènements suivants sont ignorés", etat.is_completed() and not etat.notify(&"owned", "", 1) and etat.value == 5.0)
	etat.value = 1
	_verifier("Compteur : une fois rempli, le reste si la valeur redescend", etat.is_completed() and etat.value == 1.0)

	var force := _compteur(&"x", 8.0)
	force.complete()
	_verifier("Compteur : complete() porte la valeur à la cible", force.is_completed() and force.value == 8.0)
	force.reset()
	_verifier("Compteur : reset remet la valeur à 0", not force.is_completed() and force.value == 0.0)

	var source := _compteur(&"coin", 10.0)
	source.add(4)
	var copie := _compteur(&"coin", 10.0)
	var change := _espionner(copie.changed)
	copie.set_state(source.get_state())
	_verifier("Compteur : set_state restaure la valeur sans émettre changed", copie.value == 4.0 and change.n == 0)

	var duplique: AchievementCounterCondition = source.duplicate()
	_verifier("Compteur : la valeur (état de jeu) n'est pas enregistrée dans la ressource", duplique.target == 10.0 and duplique.value == 0.0, str(duplique.value))


func _tester_collection() -> void:
	var collection := AchievementCollectionCondition.new()
	collection.event_name = &"gem"
	collection.required_ids = PackedStringArray(["rubis", "saphir", "topaze"])
	_verifier("Collection : 0/3 au départ", collection.get_progress_text() == "0/3" and collection.get_missing_ids().size() == 3)
	_verifier("Collection : un id hors liste est ignoré", not collection.notify(&"gem", "diamant") and collection.get_collected_ids().is_empty())
	collection.notify(&"gem", "rubis")
	collection.notify(&"gem", "rubis")
	_verifier("Collection : un id compte une seule fois", collection.get_collected_ids() == PackedStringArray(["rubis"]) and collection.has_collected("rubis"))
	_verifier("Collection : ids manquants", collection.get_missing_ids() == PackedStringArray(["saphir", "topaze"]))
	collection.collect("saphir")
	collection.notify(&"autre", "topaze")
	_verifier("Collection : mauvais évènement ignoré", not collection.is_completed() and collection.get_progress_text() == "2/3")
	collection.notify(&"gem", "topaze")
	_verifier("Collection : remplie quand tout est obtenu", collection.is_completed() and collection.get_progress() == 1.0)

	var copie := AchievementCollectionCondition.new()
	copie.required_ids = collection.required_ids
	copie.set_state(collection.get_state())
	_verifier("Collection : get_state / set_state", copie.is_completed() and copie.get_collected_ids().size() == 3)
	copie.reset()
	_verifier("Collection : reset", not copie.is_completed() and copie.get_collected_ids().is_empty())
	copie.complete()
	_verifier("Collection : complete() obtient tout", copie.get_missing_ids().is_empty() and copie.is_completed())


func _tester_condition_script() -> void:
	var condition: AchievementCondition = ConditionClicsRapides.new()
	condition.event_name = &"clic"
	condition.clics = 3
	condition.delai = 10.0
	_verifier("Script : _handle_event décide (évènement étranger refusé)", not condition.notify(&"autre"))
	condition.notify(&"clic")
	condition.notify(&"clic")
	_verifier("Script : pas encore rempli après 2 clics", not condition.is_completed())
	condition.notify(&"clic")
	_verifier("Script : rempli au 3e clic (complete() appelé par le script)", condition.is_completed())


# 🏆 Succès

func _tester_succes_toutes() -> void:
	var a := _drapeau(&"a")
	var b := _compteur(&"b", 2.0)
	var succes := _succes(&"tous", [a, b])
	var debloque := _espionner(succes.unlocked)
	var progres := _espionner(succes.progressed)

	succes.notify(&"a")
	_verifier("ALL : une condition sur deux → verrouillé, progressed émis", not succes.is_unlocked() and progres.n == 1 and is_equal_approx(progres.args[0], 0.5),
		"progressed %d (%s)" % [progres.n, str(progres.args)])
	succes.notify(&"b")
	succes.notify(&"b")
	_verifier("ALL : toutes remplies → débloqué une fois", succes.is_unlocked() and debloque.n == 1)
	_verifier("ALL : date de déblocage", succes.get_unlock_time() > 1_600_000_000.0, str(succes.get_unlock_time()))
	_verifier("Débloqué : notify ignoré", not succes.notify(&"b") and debloque.n == 1)

	var c := _drapeau(&"c")
	var direct := _succes(&"direct", [c])
	c.complete()
	_verifier("Condition remplie directement (sans notify) → succès débloqué", direct.is_unlocked())
	_verifier("Sans condition : jamais débloqué par notify", not _succes(&"vide", []).evaluate())


func _tester_succes_une_seule() -> void:
	var succes := _succes(&"une", [_drapeau(&"boss"), _compteur(&"fuite", 3.0)], Achievement.COMPLETION_ANY)
	succes.notify(&"fuite")
	_verifier("ANY : progression = meilleure condition (en %)", not succes.is_unlocked() and succes.get_progress_text() == "33%", succes.get_progress_text())
	succes.notify(&"boss")
	_verifier("ANY : une condition suffit", succes.is_unlocked() and succes.get_progress() == 1.0)


func _tester_suivi_progression() -> void:
	var simple := _succes(&"s", [_compteur(&"kill", 10.0)])
	simple.notify(&"kill", "", 4)
	_verifier("AUTO, une condition : sa progression (4/10)", simple.has_progress() and simple.get_progress_text() == "4/10", simple.get_progress_text())
	var drapeau := _succes(&"d", [_drapeau(&"x")])
	_verifier("AUTO, un seul drapeau : pas de barre", not drapeau.has_progress() and drapeau.get_progress_text() == "")

	var multiple := _succes(&"m", [_drapeau(&"x"), _drapeau(&"y"), _compteur(&"z", 5.0)])
	multiple.notify(&"x")
	_verifier("AUTO, plusieurs : conditions remplies (1/3)", multiple.get_progress_text() == "1/3" and is_equal_approx(multiple.get_progress(), 1.0 / 3.0), multiple.get_progress_text())

	var suivi := _compteur(&"z", 5.0)
	suivi.condition_name = &"compteur"
	var depuis := _succes(&"f", [_drapeau(&"x"), suivi])
	depuis.progress_tracker = Achievement.TRACKER_FROM_CONDITION
	depuis.tracked_condition = &"compteur"
	depuis.notify(&"z", "", 2)
	_verifier("FROM_CONDITION : progression de la condition nommée (2/5)", depuis.get_progress_text() == "2/5", depuis.get_progress_text())
	_verifier("get_condition par nom", depuis.get_condition(&"compteur") == suivi and depuis.get_condition(&"inconnue") == null)

	depuis.progress_tracker = Achievement.TRACKER_NONE
	_verifier("TRACKER_NONE : pas de barre, progression 0", not depuis.has_progress() and depuis.get_progress() == 0.0)
	depuis.unlock()
	_verifier("Débloqué : progression 1", depuis.get_progress() == 1.0)


func _tester_deblocage_force() -> void:
	var drapeau := _drapeau(&"x")
	var compteur := _compteur(&"y", 4.0)
	var succes := _succes(&"force", [drapeau, compteur])
	var debloque := _espionner(succes.unlocked)
	succes.unlock()
	_verifier("unlock() : débloqué une fois, conditions remplies", succes.is_unlocked() and debloque.n == 1 and drapeau.is_completed() and compteur.value == 4.0,
		"unlocked %d" % debloque.n)

	var icone := PlaceholderTexture2D.new()
	var verrouillee := PlaceholderTexture2D.new()
	succes.icon = icone
	succes.locked_icon = verrouillee
	_verifier("get_current_icon : icône normale une fois débloqué", succes.get_current_icon() == icone)

	var progres := _espionner(succes.progressed)
	succes.reset()
	_verifier("reset() : reverrouillé, conditions remises à zéro, progressed(0)",
		not succes.is_unlocked() and succes.get_unlock_time() == 0.0 and not drapeau.is_completed() and compteur.value == 0.0 and progres.n == 1 and progres.args[0] == 0.0)
	_verifier("get_current_icon : icône verrouillée tant que verrouillé", succes.get_current_icon() == verrouillee)
	_verifier("Après reset : redébloquable", (func(): drapeau.complete(); compteur.add(4); return succes.is_unlocked()).call())


func _tester_etat_succes() -> void:
	var nomme := _compteur(&"kill", 10.0)
	nomme.condition_name = &"tueur"
	var source := _succes(&"etat", [nomme, _drapeau(&"x")])
	source.notify(&"kill", "", 6)
	source.notify(&"x")
	var etat := source.get_state()
	_verifier("get_state : clé = nom de la condition, sinon son rang", (etat.conditions as Dictionary).has("tueur") and (etat.conditions as Dictionary).has("1"), str(etat))

	var cible_compteur := _compteur(&"kill", 10.0)
	cible_compteur.condition_name = &"tueur"
	var cible := _succes(&"etat", [cible_compteur, _drapeau(&"x")])
	var signaux := _espionner(cible.progressed)
	var debloque := _espionner(cible.unlocked)
	cible.set_state(etat)
	_verifier("set_state : conditions restaurées sans signal", cible_compteur.value == 6.0 and cible.get_completed_condition_count() == 1 and signaux.n == 0)

	source.notify(&"kill", "", 4)
	cible.set_state(source.get_state())
	_verifier("set_state : déblocage restauré (date comprise) sans signal", cible.is_unlocked() and cible.get_unlock_time() == source.get_unlock_time() and debloque.n == 0)
	var copie: Achievement = source.duplicate(true)
	_verifier("L'état de jeu n'est pas enregistré dans la ressource", not copie.is_unlocked())


# 🧩 Gestionnaire

func _tester_gestionnaire() -> void:
	var gestionnaire := _gestionnaire()
	var tueur := _succes(&"tueur", [_compteur(&"enemy_killed", 3.0)], Achievement.COMPLETION_ALL, Achievement.CATEGORY_SLAYER)
	var gemme := _succes(&"gemme", [_drapeau(&"gem")], Achievement.COMPLETION_ALL, Achievement.CATEGORY_COLLECTOR)
	tueur.conditions[0].event_ids = PackedStringArray(["5"])
	gestionnaire.achievements = [tueur, gemme] as Array[Achievement]
	await _frames(1)

	_verifier("get_achievement / has_achievement", gestionnaire.get_achievement(&"tueur") == tueur and gestionnaire.has_achievement(&"gemme") and not gestionnaire.has_achievement(&"x"))
	_verifier("get_achievements_in_category", gestionnaire.get_achievements_in_category(Achievement.CATEGORY_SLAYER) == [tueur])
	gestionnaire.register_achievement(_succes(&"tueur", [])) # erreur attendue : id en double
	_verifier("register_achievement : id en double refusé", gestionnaire.get_achievement_count() == 2)

	var debloque := _espionner(gestionnaire.achievement_unlocked)
	var progres := _espionner(gestionnaire.achievement_progressed)
	var evenements := _espionner(gestionnaire.event_notified)
	gestionnaire.notify(&"enemy_killed", 5)
	_verifier("notify : identifiant numérique comparé en texte (5 → \"5\")", tueur.get_progress_text() == "1/3", tueur.get_progress_text())
	_verifier("event_notified émis", evenements.n == 1 and evenements.args[0] == &"enemy_killed" and evenements.args[1] == "5", str(evenements.args))
	_verifier("achievement_progressed relayé (succès, progression)", progres.n == 1 and progres.args[0] == tueur and is_equal_approx(progres.args[1], 1.0 / 3.0), str(progres.args))

	gestionnaire.notify(&"enemy_killed", 5, 2)
	_verifier("achievement_unlocked émis avec le succès", debloque.n == 1 and debloque.args[0] == tueur and gestionnaire.is_unlocked(&"tueur"))
	_verifier("Compteurs : 1 débloqué sur 2 (50 %)", gestionnaire.get_unlocked_count() == 1 and gestionnaire.get_completion_ratio() == 0.5
		and gestionnaire.get_unlocked_achievements() == [tueur])

	gestionnaire.unregister_achievement(gemme)
	gestionnaire.notify(&"gem")
	_verifier("unregister_achievement : ne reçoit plus les évènements", gestionnaire.get_achievement_count() == 1 and not gemme.is_unlocked())
	gemme.notify(&"gem")
	_verifier("unregister_achievement : son déblocage n'est plus signalé", gemme.is_unlocked() and debloque.n == 1, "débloqués signalés : %d" % debloque.n)
	gestionnaire.queue_free()


func _tester_meta_succes() -> void:
	var gestionnaire := _gestionnaire()
	var a := _succes(&"a", [_drapeau(&"ev_a")])
	var b := _succes(&"b", [_drapeau(&"ev_b")])
	var c := _succes(&"c", [_drapeau(&"ev_c")])
	var compte_deux := _succes(&"deux_succes", [_compteur(&"achievement_unlocked", 2.0)])
	var duo := AchievementCollectionCondition.new()
	duo.event_name = &"achievement_unlocked"
	duo.required_ids = PackedStringArray(["a", "c"])
	var a_et_c := _succes(&"a_et_c", [duo])
	gestionnaire.achievements = [a, b, c, compte_deux, a_et_c] as Array[Achievement]
	await _frames(1)

	var debloque := _espionner(gestionnaire.achievement_unlocked)
	gestionnaire.notify(&"ev_a")
	_verifier("Méta : « achievement_unlocked » émis à chaque déblocage", compte_deux.get_progress_text() == "1/2" and duo.has_collected("a"), compte_deux.get_progress_text())
	gestionnaire.notify(&"ev_b")
	_verifier("Méta (compteur) : 2 succès débloqués → débloqué en cascade", compte_deux.is_unlocked())
	_verifier("Méta (collection) : attend précisément a et c", not a_et_c.is_unlocked())
	gestionnaire.notify(&"ev_c")
	_verifier("Méta (collection) : a et c débloqués → débloqué", a_et_c.is_unlocked() and debloque.n == 5, "%d déblocages" % debloque.n)
	gestionnaire.queue_free()


func _tester_raccourcis_gestionnaire() -> void:
	var gestionnaire := _gestionnaire()
	var drapeau := _drapeau(&"jamais")
	drapeau.condition_name = &"porte"
	var compteur := _compteur(&"jamais", 3.0)
	compteur.condition_name = &"cles"
	var succes := _succes(&"donjon", [drapeau, compteur])
	gestionnaire.achievements = [succes] as Array[Achievement]
	await _frames(1)

	gestionnaire.complete_condition(&"donjon", &"porte")
	_verifier("complete_condition", drapeau.is_completed() and not succes.is_unlocked())
	gestionnaire.add_progress(&"donjon", &"cles", 2)
	_verifier("add_progress", compteur.value == 2.0)
	gestionnaire.add_progress(&"donjon", &"porte") # erreur attendue : pas un compteur
	gestionnaire.complete_condition(&"inconnu", &"porte") # erreur attendue : succès introuvable
	gestionnaire.add_progress(&"donjon", &"cles")
	_verifier("add_progress jusqu'à la cible → débloqué", succes.is_unlocked())
	gestionnaire.reset_achievement(&"donjon")
	_verifier("reset_achievement", not succes.is_unlocked() and compteur.value == 0.0)
	gestionnaire.unlock(&"donjon")
	_verifier("unlock par id", gestionnaire.is_unlocked(&"donjon"))
	gestionnaire.reset_all()
	_verifier("reset_all", gestionnaire.get_unlocked_count() == 0)
	gestionnaire.queue_free()


# 💾 Sauvegarde

func _tester_sauvegarde() -> void:
	_nettoyer_fichiers()
	var source := _gestionnaire_complet()
	source.save_path = FICHIER_TEST
	await _frames(1)
	source.notify(&"kill", "", 4)
	source.notify(&"gem", "rubis")
	source.notify(&"boss")
	_verifier("save_achievements : OK", source.save_achievements() == OK and FileAccess.file_exists(FICHIER_TEST))

	var cible := _gestionnaire_complet()
	cible.save_path = FICHIER_TEST
	await _frames(1)
	var debloque := _espionner(cible.achievement_unlocked)
	_verifier("load_achievements : OK", cible.load_achievements() == OK)
	_verifier("Rechargé : compteur, collection et déblocage restaurés, sans signal",
		cible.get_achievement(&"tueur").get_progress_text() == "4/10" and cible.get_achievement(&"gemmes").get_progress_text() == "1/2"
		and cible.is_unlocked(&"boss") and debloque.n == 0,
		"%s, %s" % [cible.get_achievement(&"tueur").get_progress_text(), cible.get_achievement(&"gemmes").get_progress_text()])
	_verifier("Rechargé : date de déblocage conservée",
		is_equal_approx(cible.get_achievement(&"boss").get_unlock_time(), source.get_achievement(&"boss").get_unlock_time()))

	var vide := _gestionnaire_complet()
	vide.save_path = "user://tests_succes_absent.cfg"
	_verifier("Fichier absent : OK, rien ne change", vide.load_achievements() == OK and vide.get_unlocked_count() == 0)

	source.save_password = "secret"
	source.save_achievements()
	var texte := FileAccess.get_file_as_bytes(FICHIER_TEST).get_string_from_ascii()
	_verifier("Mot de passe : fichier chiffré (illisible)", not texte.contains("tueur"))
	var chiffre := _gestionnaire_complet()
	chiffre.save_path = FICHIER_TEST
	chiffre.save_password = "secret"
	_verifier("Mot de passe : rechargé avec le bon mot de passe", chiffre.load_achievements() == OK and chiffre.is_unlocked(&"boss"))
	var pirate := _gestionnaire_complet()
	pirate.save_path = FICHIER_TEST
	pirate.save_password = "faux"
	_verifier("Mot de passe : mauvais mot de passe refusé", pirate.load_achievements() != OK and not pirate.is_unlocked(&"boss")) # erreur attendue

	var etat := source.get_state()
	var externe := _gestionnaire_complet()
	externe.get_achievement(&"tueur").notify(&"kill", "", 9)
	externe.set_state(etat)
	_verifier("get_state / set_state (sauvegarde externe), succès absent remis à zéro",
		externe.is_unlocked(&"boss") and externe.get_achievement(&"tueur").get_progress_text() == "4/10")
	externe.set_state({})
	_verifier("set_state({}) : tout reverrouillé", externe.get_unlocked_count() == 0 and externe.get_achievement(&"tueur").get_progress_text() == "0/10")

	for gestionnaire in [source, cible, vide, chiffre, pirate, externe]:
		gestionnaire.queue_free()
	await _frames(1)


func _tester_chargement_automatique() -> void:
	_nettoyer_fichiers()
	var premier := _gestionnaire_complet(false)
	premier.save_path = FICHIER_TEST
	premier.auto_save = true
	premier.auto_load = true
	_ajouter(premier)
	premier.notify(&"boss")
	_verifier("auto_save : sauvegardé au déblocage", FileAccess.file_exists(FICHIER_TEST))
	premier.notify(&"kill", "", 7)
	premier.queue_free()
	await _frames(1)

	var second := _gestionnaire_complet(false)
	second.save_path = FICHIER_TEST
	second.auto_load = true
	second.auto_save = false
	_ajouter(second)
	_verifier("auto_save à la sortie + auto_load au _ready", second.is_unlocked(&"boss") and second.get_achievement(&"tueur").get_progress_text() == "7/10",
		second.get_achievement(&"tueur").get_progress_text())
	second.queue_free()
	await _frames(1)


# 🔔 Notifications

func _tester_notifications() -> void:
	var gestionnaire := _gestionnaire()
	gestionnaire.show_notifications = true
	gestionnaire.notification_duration = 0.1
	var succes := _succes(&"toast", [_drapeau(&"go")])
	succes.display_name = "Grillé !"
	succes.icon = PlaceholderTexture2D.new()
	var muet := _succes(&"muet", [_drapeau(&"chut")])
	gestionnaire.achievements = [succes, muet] as Array[Achievement]
	await _frames(1)

	var boite := gestionnaire.get_notification_container()
	_verifier("Conteneur de notifications créé au _ready", boite != null and boite.get_child_count() == 0)
	gestionnaire.notify(&"go")
	var toast: Control = boite.get_child(0) if boite.get_child_count() > 0 else null
	_verifier("Déblocage : une notification avec le nom et l'icône",
		toast != null and toast.get_meta("achievement_id") == &"toast" and toast.find_child("Name", true, false).text == "Grillé !"
		and toast.find_children("*", "TextureRect", true, false).size() == 1)

	gestionnaire.notification_position = AchievementManager.NOTIFICATION_POSITION_BOTTOM_LEFT
	_verifier("notification_position : conteneur ancré en bas à gauche", boite.anchor_left == 0.0 and boite.anchor_top == 1.0)

	gestionnaire.show_notifications = false
	gestionnaire.notify(&"chut")
	_verifier("show_notifications = false : pas de notification", boite.get_child_count() == 1 and muet.is_unlocked())

	await _secondes(1.0)
	_verifier("La notification disparaît après sa durée", boite.get_child_count() == 0, "%d restante(s)" % boite.get_child_count())
	gestionnaire.queue_free()
	await _frames(1)


# 🛠️ Utilitaires

func _drapeau(evenement: StringName) -> AchievementCondition:
	var condition := AchievementCondition.new()
	condition.event_name = evenement
	return condition


func _compteur(evenement: StringName, cible: float, mode := AchievementCounterCondition.VALUE_MODE_ADD) -> AchievementCounterCondition:
	var compteur := AchievementCounterCondition.new()
	compteur.event_name = evenement
	compteur.target = cible
	compteur.value_mode = mode
	return compteur


func _succes(id: StringName, conditions: Array, mode := Achievement.COMPLETION_ALL, categorie := Achievement.CATEGORY_NONE) -> Achievement:
	var succes := Achievement.new()
	succes.id = id
	succes.display_name = String(id)
	succes.category = categorie
	succes.completion_mode = mode
	var liste: Array[AchievementCondition] = []
	liste.assign(conditions)
	succes.conditions = liste
	return succes


## Gestionnaire de test : sans sauvegarde ni chargement automatiques, sans notification.
func _gestionnaire() -> AchievementManager:
	var gestionnaire := AchievementManager.new()
	gestionnaire.auto_load = false
	gestionnaire.auto_save = false
	gestionnaire.show_notifications = false
	_ajouter(gestionnaire)
	return gestionnaire


## Gestionnaire avec trois succès (compteur, collection, drapeau) neufs ; ajouté au bac à sable si p_ajouter.
func _gestionnaire_complet(ajouter := true) -> AchievementManager:
	var gestionnaire := AchievementManager.new()
	gestionnaire.auto_load = false
	gestionnaire.auto_save = false
	gestionnaire.show_notifications = false
	var gemmes := AchievementCollectionCondition.new()
	gemmes.event_name = &"gem"
	gemmes.required_ids = PackedStringArray(["rubis", "saphir"])
	gestionnaire.achievements = [_succes(&"tueur", [_compteur(&"kill", 10.0)]), _succes(&"gemmes", [gemmes]),
		_succes(&"boss", [_drapeau(&"boss")])] as Array[Achievement]
	if ajouter:
		_ajouter(gestionnaire)
	return gestionnaire


func _nettoyer_fichiers() -> void:
	if FileAccess.file_exists(FICHIER_TEST):
		DirAccess.remove_absolute(FICHIER_TEST)
