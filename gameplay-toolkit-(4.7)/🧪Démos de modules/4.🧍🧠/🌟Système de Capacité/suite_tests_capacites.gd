extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques du système de capacités : AttributeDefinition, AttributeModifier, GameplayEffect,
## Ability, AbilitySystem (mise à jour manuelle : update(delta)).
##
## Avertissement attendu dans la console : expression de magnitude invalide.


func _executer() -> void:
	_tester_attributs()
	_tester_tags()
	_tester_effet_instantane()
	_tester_effet_a_duree()
	_tester_effet_periodique()
	_tester_cumul()
	_tester_conditions_des_effets()
	_tester_expression_de_magnitude()
	_tester_activation_et_couts()
	_tester_recharge_et_charges()
	_tester_incantation_et_duree()
	_tester_tags_de_capacite()
	_tester_capacite_scriptee()
	await _tester_mise_a_jour_automatique()


# 🧱 Construction

func _attribut(nom: StringName, base: float, maximum: StringName = &"", min_valeur := 0.0) -> AttributeDefinition:
	var definition := AttributeDefinition.new()
	definition.attribute_name = nom
	definition.base_value = base
	definition.min_value = min_valeur
	definition.max_attribute = maximum
	return definition


## Système avec vie (≤ vie_max), vie_max 100, mana 50, force 10.
func _systeme() -> AbilitySystem:
	var systeme := AbilitySystem.new()
	systeme.manual_update = true
	systeme.attribute_definitions = [_attribut(&"vie_max", 100), _attribut(&"vie", 100, &"vie_max"), _attribut(&"mana", 50), _attribut(&"force", 10)]
	_ajouter(systeme)
	return systeme


func _modificateur(attribut: StringName, valeur: float, operation := AttributeModifier.OPERATION_ADD) -> AttributeModifier:
	var modificateur := AttributeModifier.new()
	modificateur.attribute = attribut
	modificateur.magnitude = valeur
	modificateur.operation = operation
	return modificateur


func _effet(modificateurs: Array, politique := GameplayEffect.DURATION_INSTANT, duree := 1.0) -> GameplayEffect:
	var effet := GameplayEffect.new()
	effet.duration_policy = politique
	effet.duration = duree
	effet.modifiers.assign(modificateurs)
	return effet


func _capacite(id: StringName, recharge := 0.0) -> Ability:
	var capacite := Ability.new()
	capacite.ability_id = id
	capacite.cooldown = recharge
	return capacite


# 📊 Attributs

func _tester_attributs() -> void:
	var systeme := _systeme()
	_verifier("Attributs : valeurs initiales", systeme.get_value(&"vie") == 100 and systeme.get_value(&"mana") == 50)
	_verifier("Attributs : liste dans l'ordre", systeme.get_attribute_names() == PackedStringArray(["vie_max", "vie", "mana", "force"]))
	systeme.add_to_base(&"vie", 50)
	_verifier("Attributs : bornés par max_attribute", systeme.get_value(&"vie") == 100)
	var changements := {"n": 0, "args": []} # signal à 3 arguments : _espionner n'en accepte que 2
	systeme.attribute_changed.connect(func(nom, ancienne, nouvelle):
		changements.n += 1
		changements.args = [nom, ancienne, nouvelle])
	var epuises := _espionner(systeme.attribute_depleted)
	systeme.add_to_base(&"vie", -150)
	_verifier("Attributs : bornés par min_value", systeme.get_value(&"vie") == 0)
	_verifier("Attributs : attribute_changed (ancienne, nouvelle valeur)", changements.n == 1 and changements.args == [&"vie", 100.0, 0.0], str(changements))
	_verifier("Attributs : attribute_depleted à zéro", epuises.n == 1 and epuises.args[0] == &"vie")
	systeme.set_base_value(&"vie", 80)
	systeme.set_base_value(&"vie_max", 60)
	_verifier("Attributs : baisse du maximum → la vie est rabotée", systeme.get_value(&"vie") == 60 and systeme.get_base_value(&"vie") == 60)
	_verifier("Attributs : attribut inconnu → 0", systeme.get_value(&"inconnu") == 0.0 and not systeme.has_attribute(&"inconnu"))


# 🏷️ Tags

func _tester_tags() -> void:
	var systeme := _systeme()
	var changements := _espionner(systeme.tag_changed)
	systeme.add_tag("etat.etourdi")
	systeme.add_tag("etat.etourdi")
	_verifier("Tags : présent, hiérarchique", systeme.has_tag("etat.etourdi") and systeme.has_tag("etat") and not systeme.has_tag("eta"))
	systeme.remove_tag("etat.etourdi")
	_verifier("Tags : compté (deux ajouts, un retrait → encore présent)", systeme.has_tag("etat.etourdi"))
	systeme.remove_tag("etat.etourdi")
	_verifier("Tags : retiré", not systeme.has_tag("etat"))
	_verifier("Tags : tag_changed seulement aux changements de présence", changements.n == 2, str(changements.n))
	systeme.add_tag("a")
	systeme.add_tag("b.c")
	_verifier("Tags : has_any / has_all", systeme.has_any_tags(["x", "b"]) and systeme.has_all_tags(["a", "b"]) and not systeme.has_all_tags(["a", "x"]))


# ✨ Effets

func _tester_effet_instantane() -> void:
	var systeme := _systeme()
	var degats := _effet([_modificateur(&"vie", -30)])
	_verifier("Instantané : handle 0", systeme.apply_effect(degats) == 0)
	_verifier("Instantané : modifie la base", systeme.get_base_value(&"vie") == 70 and systeme.get_value(&"vie") == 70)
	_verifier("Instantané : pas d'effet actif", systeme.get_active_effects().is_empty())
	systeme.apply_effect(_effet([_modificateur(&"force", 2, AttributeModifier.OPERATION_MULTIPLY)]))
	_verifier("Instantané : multiplication de la base", systeme.get_base_value(&"force") == 20)


func _tester_effet_a_duree() -> void:
	var systeme := _systeme()
	var bonus := _effet([_modificateur(&"force", 5), _modificateur(&"force", 2, AttributeModifier.OPERATION_MULTIPLY)], GameplayEffect.DURATION_TIMED, 2.0)
	bonus.granted_tags = ["buff.force"]
	var retraits := _espionner(systeme.effect_removed)
	var handle := systeme.apply_effect(bonus)
	_verifier("Durée : handle positif", handle > 0)
	_verifier("Durée : (base + ajout) × multiplicateur", systeme.get_value(&"force") == 30 and systeme.get_base_value(&"force") == 10, str(systeme.get_value(&"force")))
	_verifier("Durée : tag accordé", systeme.has_tag("buff.force"))
	systeme.update(1.5)
	_verifier("Durée : encore actif, temps restant", systeme.has_effect(bonus) and is_equal_approx(systeme.get_effect_remaining(handle), 0.5))
	systeme.update(0.6)
	_verifier("Durée : retiré à l'expiration", not systeme.has_effect(bonus) and systeme.get_value(&"force") == 10 and not systeme.has_tag("buff"))
	_verifier("Durée : effect_removed", retraits.n == 1 and retraits.args[1] == handle)

	var infini := _effet([_modificateur(&"mana", 999, AttributeModifier.OPERATION_OVERRIDE)], GameplayEffect.DURATION_INFINITE)
	handle = systeme.apply_effect(infini)
	systeme.update(100.0)
	_verifier("Infini : valeur imposée, jamais expiré", systeme.get_value(&"mana") == 999 and systeme.has_effect(infini))
	systeme.remove_effect(handle)
	_verifier("Infini : remove_effect", systeme.get_value(&"mana") == 50)


func _tester_effet_periodique() -> void:
	var systeme := _systeme()
	var poison := _effet([_modificateur(&"vie", -5)], GameplayEffect.DURATION_TIMED, 3.0)
	poison.period = 1.0
	poison.effect_tags = ["debuff.poison"]
	systeme.apply_effect(poison)
	_verifier("Périodique : rien avant la première période", systeme.get_value(&"vie") == 100)
	systeme.update(1.0)
	systeme.update(1.0)
	_verifier("Périodique : base modifiée à chaque période", systeme.get_value(&"vie") == 90, str(systeme.get_value(&"vie")))
	systeme.update(1.0)
	_verifier("Périodique : 3 périodes puis expiré", systeme.get_value(&"vie") == 85 and not systeme.has_effect(poison), str(systeme.get_value(&"vie")))
	poison.execute_period_on_apply = true
	systeme.apply_effect(poison)
	_verifier("Périodique : execute_period_on_apply", systeme.get_value(&"vie") == 80)
	_verifier("remove_effects_with_tag (effect_tags)", systeme.remove_effects_with_tag("debuff") == 1 and not systeme.has_effect(poison))


func _tester_cumul() -> void:
	var systeme := _systeme()
	var rage := _effet([_modificateur(&"force", 2)], GameplayEffect.DURATION_TIMED, 1.0)
	rage.stacking = GameplayEffect.STACKING_STACK
	rage.max_stacks = 3
	var premier := systeme.apply_effect(rage)
	systeme.update(0.8)
	var handles := []
	for i in 4:
		handles.append(systeme.apply_effect(rage))
	_verifier("Cumul : même handle", handles.all(func(h): return h == premier))
	_verifier("Cumul : plafonné à max_stacks", systeme.get_effect_stacks(rage) == 3 and systeme.get_value(&"force") == 16, str(systeme.get_value(&"force")))
	systeme.update(0.8)
	_verifier("Cumul : durée relancée", systeme.has_effect(rage))

	var separe := _effet([_modificateur(&"force", 1)], GameplayEffect.DURATION_INFINITE)
	systeme.apply_effect(separe)
	systeme.apply_effect(separe)
	_verifier("Séparé : deux instances", systeme.get_active_effects().filter(func(e): return e.effect == separe).size() == 2)


func _tester_conditions_des_effets() -> void:
	var systeme := _systeme()
	var soin := _effet([_modificateur(&"vie", 10)])
	soin.blocked_by_tags = ["etat.mort"]
	systeme.add_to_base(&"vie", -50)
	systeme.add_tag("etat.mort")
	var bloques := _espionner(systeme.effect_blocked)
	_verifier("Bloqué : -1 et effect_blocked", systeme.apply_effect(soin) == -1 and bloques.n == 1)
	systeme.remove_tag("etat.mort")
	var renforcement := _effet([_modificateur(&"force", 1)])
	renforcement.required_tags = ["classe.guerrier"]
	_verifier("Requis : refusé sans le tag", systeme.apply_effect(renforcement) == -1)
	var purge := _effet([], GameplayEffect.DURATION_INSTANT)
	purge.remove_effects_with_tags = ["debuff"]
	var poison := _effet([_modificateur(&"vie", -1)], GameplayEffect.DURATION_INFINITE)
	poison.granted_tags = ["debuff.poison"]
	systeme.apply_effect(poison)
	systeme.apply_effect(purge)
	_verifier("remove_effects_with_tags : purge des effets portant le tag", not systeme.has_effect(poison) and not systeme.has_tag("debuff"))


func _tester_expression_de_magnitude() -> void:
	var source := _systeme()
	var cible := _systeme()
	source.set_base_value(&"force", 12)
	var coup := AttributeModifier.new()
	coup.attribute = &"vie"
	coup.magnitude_expression = "-source.get_value('force') * 2 * level"
	var effet := _effet([coup])
	source.apply_effect_to(effet, cible, 1.5)
	_verifier("Expression : dépend de la source et du niveau", cible.get_value(&"vie") == 64, str(cible.get_value(&"vie")))
	var niveau := _modificateur(&"force", 2)
	niveau.magnitude_per_level = 3
	cible.apply_effect(_effet([niveau]), null, 3)
	_verifier("magnitude_per_level", cible.get_value(&"force") == 18, str(cible.get_value(&"force")))
	coup.magnitude_expression = "source.)"
	_verifier("Expression invalide : 0 et erreur", coup.compute_magnitude(source, cible, 1) == 0 and coup.get_error() != "")


# 🌟 Capacités

func _tester_activation_et_couts() -> void:
	var lanceur := _systeme()
	var cible_noeud := Node.new()
	_ajouter(cible_noeud)
	var cible := _systeme()
	cible.reparent(cible_noeud)
	var boule := _capacite(&"boule_de_feu")
	boule.costs = {&"mana": 20}
	boule.requires_target = true
	boule.effects_on_target.assign([_effet([_modificateur(&"vie", -25)])])
	lanceur.grant_ability(boule)
	_verifier("Capacité : accordée", lanceur.has_ability(&"boule_de_feu") and lanceur.get_ability_ids() == PackedStringArray(["boule_de_feu"]))
	var echecs := _espionner(lanceur.ability_failed)
	_verifier("Capacité : cible requise", not lanceur.activate(&"boule_de_feu") and echecs.args == [&"boule_de_feu", "no_target"], str(echecs.args))
	var activations := _espionner(lanceur.ability_activated)
	var fins := _espionner(lanceur.ability_ended)
	_verifier("Capacité : activée sur le nœud parent de la cible", lanceur.activate(&"boule_de_feu", cible_noeud))
	_verifier("Capacité : coût payé, effet appliqué à la cible", lanceur.get_value(&"mana") == 30 and cible.get_value(&"vie") == 75)
	_verifier("Capacité : instantanée → terminée aussitôt", activations.n == 1 and fins.n == 1 and fins.args[1] == false and not lanceur.is_ability_active(&"boule_de_feu"))
	lanceur.activate(&"boule_de_feu", cible_noeud)
	_verifier("Capacité : coût insuffisant", lanceur.get_activation_failure(&"boule_de_feu", cible_noeud) == "cost" and not lanceur.activate(&"boule_de_feu", cible_noeud))
	_verifier("Capacité : inconnue", lanceur.get_activation_failure(&"rien") == "unknown")


func _tester_recharge_et_charges() -> void:
	var systeme := _systeme()
	var dash := _capacite(&"dash", 2.0)
	dash.max_charges = 2
	systeme.grant_ability(dash)
	_verifier("Charges : deux activations d'affilée", systeme.activate(&"dash") and systeme.activate(&"dash"))
	_verifier("Charges : troisième refusée (cooldown)", systeme.get_activation_failure(&"dash") == "cooldown" and systeme.get_cooldown_remaining(&"dash") == 2.0)
	var pretes := _espionner(systeme.cooldown_ready)
	systeme.update(2.0)
	_verifier("Recharge : une charge rendue", systeme.get_charges(&"dash") == 1 and pretes.n == 1)
	systeme.update(2.0)
	_verifier("Recharge : toutes les charges", systeme.get_charges(&"dash") == 2 and pretes.n == 2)
	systeme.activate(&"dash")
	systeme.reset_cooldown(&"dash")
	_verifier("reset_cooldown", systeme.get_charges(&"dash") == 2)


func _tester_incantation_et_duree() -> void:
	var systeme := _systeme()
	var bouclier := _capacite(&"bouclier")
	bouclier.cast_time = 0.5
	bouclier.active_duration = 1.0
	bouclier.activation_owned_tags = ["action.incantation"]
	bouclier.effects_on_self.assign([_effet([_modificateur(&"vie_max", 50)], GameplayEffect.DURATION_TIMED, 3.0)])
	systeme.grant_ability(bouclier)
	var engages := _espionner(systeme.ability_committed)
	systeme.activate(&"bouclier")
	_verifier("Incantation : active, tag accordé, effets pas encore appliqués", systeme.is_ability_active(&"bouclier") and systeme.has_tag("action") and engages.n == 0)
	systeme.update(0.5)
	_verifier("Incantation : effets appliqués après cast_time", engages.n == 1 and systeme.get_value(&"vie_max") == 150)
	systeme.update(0.6)
	_verifier("Durée active : encore active", systeme.is_ability_active(&"bouclier"))
	systeme.update(0.5)
	_verifier("Durée active : terminée, tag retiré", not systeme.is_ability_active(&"bouclier") and not systeme.has_tag("action"))

	# Annulation pendant l'incantation : aucun effet.
	systeme.reset_cooldown(&"bouclier")
	var fins := _espionner(systeme.ability_ended)
	systeme.activate(&"bouclier")
	systeme.cancel_ability(&"bouclier")
	_verifier("Annulation : ability_ended(cancelled = true), pas d'effet", fins.args == [&"bouclier", true] and engages.n == 1)

	var canal := _capacite(&"canal")
	canal.active_duration = -1
	systeme.grant_ability(canal)
	systeme.activate(&"canal")
	systeme.update(50.0)
	_verifier("active_duration = -1 : jusqu'à end_ability", systeme.is_ability_active(&"canal"))
	systeme.end_ability(&"canal")
	_verifier("end_ability", not systeme.is_ability_active(&"canal"))


func _tester_tags_de_capacite() -> void:
	var systeme := _systeme()
	var attaque := _capacite(&"attaque")
	attaque.activation_blocked_tags = ["etat.etourdi"]
	var soin := _capacite(&"soin")
	soin.activation_required_tags = ["classe.pretre"]
	var charge := _capacite(&"charge")
	charge.active_duration = -1
	charge.ability_tags = ["mouvement.charge"]
	var arret := _capacite(&"arret")
	arret.cancel_abilities_with_tags = ["mouvement"]
	for capacite in [attaque, soin, charge, arret]:
		systeme.grant_ability(capacite)
	systeme.add_tag("etat.etourdi")
	_verifier("Tags bloquants", systeme.get_activation_failure(&"attaque") == "blocked")
	_verifier("Tags requis", systeme.get_activation_failure(&"soin") == "missing_tags")
	systeme.activate(&"charge")
	systeme.activate(&"arret")
	_verifier("cancel_abilities_with_tags", not systeme.is_ability_active(&"charge"))
	_verifier("Déjà active", systeme.activate(&"charge") and systeme.get_activation_failure(&"charge") == "active")


func _tester_capacite_scriptee() -> void:
	var script := GDScript.new()
	script.source_code = """extends Ability
var journal: Array = []
func _can_activate(system: Node, _target: Node) -> bool:
	return system.get_value(&"force") >= 10
func _on_activate(_system: Node, _target: Node) -> void:
	journal.append("activate")
func _on_commit(_system: Node, _target: Node) -> void:
	journal.append("commit")
func _on_end(_system: Node, cancelled: bool) -> void:
	journal.append("end %s" % cancelled)
"""
	script.reload()
	var capacite := _capacite(&"scriptee")
	capacite.set_script(script)
	var systeme := _systeme()
	systeme.grant_ability(capacite)
	_verifier("Script : _can_activate vrai → activée", systeme.activate(&"scriptee"))
	_verifier("Script : _on_activate, _on_commit, _on_end", capacite.journal == ["activate", "commit", "end false"], str(capacite.journal))
	systeme.set_base_value(&"force", 5)
	_verifier("Script : _can_activate faux → refusée", systeme.get_activation_failure(&"scriptee") == "script")


func _tester_mise_a_jour_automatique() -> void:
	var systeme := AbilitySystem.new()
	systeme.attribute_definitions = [_attribut(&"vie", 10)]
	systeme.initial_tags = ["vivant"]
	var regen := _effet([_modificateur(&"vie", 1)], GameplayEffect.DURATION_INFINITE)
	regen.period = 0.05
	systeme.initial_effects = [regen]
	_ajouter(systeme)
	_verifier("_ready : configuration appliquée", systeme.has_tag("vivant") and systeme.has_effect(regen))
	await _secondes(0.3)
	_verifier("_process : effets mis à jour sans appel manuel", systeme.get_value(&"vie") > 12, str(systeme.get_value(&"vie")))
	var noeud := Node.new()
	noeud.add_child(systeme.duplicate())
	_ajouter(noeud)
	_verifier("find_on : AbilitySystem enfant", AbilitySystem.find_on(noeud) != null and AbilitySystem.find_on(systeme) == systeme)
