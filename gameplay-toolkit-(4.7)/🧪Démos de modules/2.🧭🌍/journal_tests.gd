extends RefCounted

## Mise en forme commune des résultats d'une suite (suite_tests_base.gd) : texte BBCode pour un
## RichTextLabel, et bilan dans la console (seuls les échecs et avertissements y sont détaillés).

const SuiteBase := preload("suite_tests_base.gd")


## Renvoie le bilan + le détail en BBCode, et l'affiche dans la console.
static func formater(titre: String, resultats: Array[Dictionary]) -> String:
	var compte := {SuiteBase.Statut.REUSSI: 0, SuiteBase.Statut.ECHEC: 0, SuiteBase.Statut.AVERTISSEMENT: 0}
	var lignes := PackedStringArray()

	for resultat in resultats:
		compte[resultat.statut] += 1
		var detail: String = resultat.detail
		match resultat.statut:
			SuiteBase.Statut.REUSSI:
				lignes.append("[color=green]✔[/color] %s" % resultat.nom)
			SuiteBase.Statut.ECHEC:
				lignes.append("[color=red]✘ %s[/color]%s" % [resultat.nom, " — " + detail if detail else ""])
			SuiteBase.Statut.AVERTISSEMENT:
				lignes.append("[color=yellow]⚠ %s[/color] — %s" % [resultat.nom, detail])

	var bilan := "[b]🧪 %s : [color=green]%d réussis[/color], [color=red]%d échecs[/color], [color=yellow]%d avertissements[/color][/b]" % [
		titre, compte[SuiteBase.Statut.REUSSI], compte[SuiteBase.Statut.ECHEC], compte[SuiteBase.Statut.AVERTISSEMENT]]

	print_rich(bilan)
	for ligne in lignes:
		if not ligne.begins_with("[color=green]"):
			print_rich("    " + ligne)

	return bilan + "\n" + "\n".join(lignes)


## Nombre d'échecs d'une liste de résultats.
static func echecs(resultats: Array[Dictionary]) -> int:
	return resultats.filter(func(r): return r.statut == SuiteBase.Statut.ECHEC).size()
