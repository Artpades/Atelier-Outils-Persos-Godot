extends Control

## 💾 Démo du module de sauvegarde.
##
## - Au lancement, exécute la suite de tests automatiques (suite_tests_sauvegarde.gd) et affiche le bilan.
## - Partie interactive : un joueur (cible Node) et des paramètres (cible Resource, chiffrés), chacun relié
##   à un SaveDataBinder configuré dans la scène. Déplacer le joueur (flèches / ZQSD), modifier ses valeurs,
##   puis Sauvegarder / Charger / Supprimer, et observer le fichier écrit sur disque.

const SuiteTests := preload("suite_tests_sauvegarde.gd")

@onready var binder_joueur: SaveDataBinder = $BinderJoueur
@onready var binder_parametres: SaveDataBinder = $BinderParametres
@onready var joueur: ColorRect = %Joueur
@onready var infos: Label = %Infos
@onready var champ_pseudo: LineEdit = %ChampPseudo
@onready var case_chiffrement: CheckBox = %CaseChiffrement
@onready var slider_volume: HSlider = %SliderVolume
@onready var case_plein_ecran: CheckBox = %CasePleinEcran
@onready var journal: RichTextLabel = %Journal


func _ready() -> void:
	# Filet de sécurité si le NodePath de la scène n'a pas été résolu vers node_target.
	if binder_joueur.node_target == null:
		binder_joueur.node_target = joueur

	for bouton in find_children("*", "BaseButton"):
		bouton.focus_mode = Control.FOCUS_NONE

	%BoutonPiece.pressed.connect(func(): joueur.pieces += 1)
	%BoutonNiveau.pressed.connect(func(): joueur.niveau += 1)
	%BoutonDegats.pressed.connect(func(): joueur.pv = maxf(joueur.pv - 10.0, 0.0))
	champ_pseudo.text_changed.connect(func(texte: String): joueur.pseudo = texte)
	case_chiffrement.toggled.connect(func(actif: bool): binder_joueur.save_data.is_encrypted = actif)
	slider_volume.value_changed.connect(func(valeur: float): _parametres().volume = valeur)
	case_plein_ecran.toggled.connect(func(actif: bool): _parametres().plein_ecran = actif)

	%BoutonSauvegarder.pressed.connect(_sauvegarder)
	%BoutonCharger.pressed.connect(_charger)
	%BoutonSupprimer.pressed.connect(_supprimer)
	%BoutonFichiers.pressed.connect(_afficher_fichiers)
	%BoutonTests.pressed.connect(_lancer_tests)

	case_chiffrement.button_pressed = binder_joueur.save_data.is_encrypted
	_rafraichir_interface()
	_lancer_tests()


func _process(_delta: float) -> void:
	infos.text = "Pseudo : %s\nPièces : %d\nNiveau : %d\nPV : %.0f\nPosition : (%.0f, %.0f)\n\nVolume : %.2f · Plein écran : %s" % [
		joueur.pseudo, joueur.pieces, joueur.niveau, joueur.pv, joueur.pos_x, joueur.pos_y,
		_parametres().volume, "oui" if _parametres().plein_ecran else "non"]


# 💾 Actions

func _sauvegarder() -> void:
	binder_joueur.save()
	binder_parametres.save()
	_journaliser("[color=green]💾 Sauvegarde :[/color] joueur → %s, paramètres → %s" % [
		_chemin(binder_joueur.save_data), _chemin(binder_parametres.save_data)])
	_afficher_fichiers()


func _charger() -> void:
	var existe_joueur := FileAccess.file_exists(_chemin(binder_joueur.save_data))
	var existe_parametres := FileAccess.file_exists(_chemin(binder_parametres.save_data))
	binder_joueur.load()
	binder_parametres.load()
	_rafraichir_interface()
	_journaliser("[color=cyan]📖 Chargement :[/color] joueur %s, paramètres %s" % [
		"restauré" if existe_joueur else "[color=yellow]sans sauvegarde[/color]",
		"restaurés" if existe_parametres else "[color=yellow]sans sauvegarde[/color]"])


func _supprimer() -> void:
	binder_joueur.delete()
	binder_parametres.delete()
	_journaliser("[color=orange]🗑️ Suppression :[/color] %s, %s" % [
		_chemin(binder_joueur.save_data), _chemin(binder_parametres.save_data)])


## Affiche le contenu brut des fichiers : texte JSON en clair, octets en hexadécimal si chiffré.
func _afficher_fichiers() -> void:
	for binder: SaveDataBinder in [binder_joueur, binder_parametres]:
		var chemin := _chemin(binder.save_data)
		if not FileAccess.file_exists(chemin):
			_journaliser("[color=gray]📄 %s : aucun fichier[/color]" % chemin)
			continue
		var octets := FileAccess.get_file_as_bytes(chemin)
		var apercu := octets.slice(0, 48).hex_encode() + ("…" if octets.size() > 48 else "") \
			if binder.save_data.is_encrypted else octets.get_string_from_utf8()
		_journaliser("📄 [b]%s[/b] (%d octets, %s)\n[code]%s[/code]" % [
			ProjectSettings.globalize_path(chemin), octets.size(),
			"chiffré" if binder.save_data.is_encrypted else "clair", apercu])


# 🧪 Tests automatiques

func _lancer_tests() -> void:
	var resultats := SuiteTests.new().executer()
	var compte := {SuiteTests.Statut.REUSSI: 0, SuiteTests.Statut.ECHEC: 0, SuiteTests.Statut.AVERTISSEMENT: 0}
	var lignes := PackedStringArray()

	for resultat in resultats:
		compte[resultat.statut] += 1
		var detail: String = resultat.detail
		match resultat.statut:
			SuiteTests.Statut.REUSSI:
				lignes.append("[color=green]✔[/color] %s" % resultat.nom)
			SuiteTests.Statut.ECHEC:
				lignes.append("[color=red]✘ %s[/color]%s" % [resultat.nom, " — " + detail if detail else ""])
			SuiteTests.Statut.AVERTISSEMENT:
				lignes.append("[color=yellow]⚠ %s[/color] — %s" % [resultat.nom, detail])

	var bilan := "[b]🧪 Tests du module de sauvegarde : [color=green]%d réussis[/color], [color=red]%d échecs[/color], [color=yellow]%d avertissements[/color][/b]" % [
		compte[SuiteTests.Statut.REUSSI], compte[SuiteTests.Statut.ECHEC], compte[SuiteTests.Statut.AVERTISSEMENT]]
	_journaliser(bilan + "\n" + "\n".join(lignes))
	print_rich(bilan)
	for ligne in lignes:
		if not ligne.begins_with("[color=green]"):
			print_rich("    " + ligne)


# 🛠️ Utilitaires

func _parametres() -> Resource:
	return binder_parametres.resource_target


func _chemin(save_data: SaveData) -> String:
	var config := save_data.file_config
	return config.save_directory + save_data.system_id + (config.encrypted_extension if save_data.is_encrypted else config.extension)


func _rafraichir_interface() -> void:
	champ_pseudo.text = joueur.pseudo
	slider_volume.set_value_no_signal(_parametres().volume)
	case_plein_ecran.set_pressed_no_signal(_parametres().plein_ecran)


func _journaliser(message: String) -> void:
	journal.append_text(message + "\n\n")
