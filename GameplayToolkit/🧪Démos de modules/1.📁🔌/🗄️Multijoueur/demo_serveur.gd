extends Control

## 🖥️ Serveur multijoueur pour des appareils distants (Internet) : NetworkSession + ServerAccessPolicy + PortForwarder.
##
## - Démarrer en « serveur dédié » (ce PC sert sans jouer) ou « héberger et jouer ».
## - Rendre le serveur joignable : ouvrir le port sur la box (UPnP), trouver l'IP publique, puis partager
##   le CODE DE CONNEXION (adresse, port, empreinte du serveur, invitation éventuelle) à coller dans la
##   scène « Démo Client distant ».
## - Protocoles d'accès : mode ouvert / comptes / invitations, validation manuelle des nouveaux appareils,
##   rôles, expulsion, bannissement (appareil ou adresse IP), blocage automatique après des échecs.
##
## Tout est sauvegardé dans user://serveur_multijoueur/ : règles d'accès (acces.tres), clé d'identité du
## serveur (son empreinte reste la même d'un lancement à l'autre, les codes déjà partagés restent valides),
## certificat de transport et réglages.
##
## Sans fenêtre (serveur loué, Raspberry Pi...) :
##   godot --headless --path . "res://🧪Démos de modules/1.📁🔌/🗄️Multijoueur/Démo Serveur.tscn" -- --port=47500 --acces=invitations --invitation=3 --upnp
## Arguments : --port=N --transport=enet|websocket --mot-de-passe=... --acces=ouvert|comptes|invitations
##   --compte=nom:motdepasse[:role 0-3] --invitation=N (crée une invitation à N usages) --jouer (pas dédié)
##   --sans-tls --e2e --upnp --ip-publique --adresse=mon.domaine.fr

const Noms := preload("noms_multijoueur.gd")
const DOSSIER := "user://serveur_multijoueur/"

@onready var session: NetworkSession = %Session
@onready var redirection: PortForwarder = %Redirection
@onready var journal: RichTextLabel = %Journal
@onready var etat: Label = %Etat
@onready var etat_internet: RichTextLabel = %EtatInternet
@onready var reglages_serveur: Array[Control] = [%Mode, %NomServeur, %Port, %Transport, %JoueursMax, %MotDePasse, %ChiffrementTransport, %BoutEnBout]

var regles: ServerAccessPolicy
var _sans_interface := false
var _alertes_internet: Array[String] = []


func _ready() -> void:
	DirAccess.make_dir_recursive_absolute(DOSSIER)
	_sans_interface = DisplayServer.get_name() == "headless" or OS.get_cmdline_user_args().has("--serveur")

	Noms.remplir(%Mode, ["🖥️ Serveur dédié (ne joue pas)", "🎮 Héberger et jouer"])
	%Transport.add_item("ENet (UDP)", NetworkSession.TRANSPORT_ENET)
	%Transport.add_item("WebSocket (TCP)", NetworkSession.TRANSPORT_WEBSOCKET)
	Noms.remplir(%ModeAcces, Noms.MODES_ACCES)
	for option in [%RoleDefaut, %CompteRole, %InvitationRole]:
		Noms.remplir(option, Noms.ROLES, ServerAccessPolicy.ROLE_PLAYER)
	for i in %Onglets.get_tab_count():
		%Onglets.set_tab_title(i, ["🚪 Accès", "⏳ Demandes", "👥 Connectés", "🚫 Bans et appareils"][i])

	_charger()
	_connecter_signaux()
	_rafraichir_tout()
	_journaliser("Règles et clés chargées depuis [code]%s[/code]." % ProjectSettings.globalize_path(DOSSIER))
	if _sans_interface:
		_demarrer_sans_interface()


func _notification(quoi: int) -> void:
	# Fermeture de la fenêtre : avant le démontage de la scène (les %nœuds existent encore).
	if quoi == NOTIFICATION_WM_CLOSE_REQUEST:
		_sauver_reglages()


# 💾 Sauvegarde

func _charger() -> void:
	# Règles d'accès : créées au premier lancement, sauvegardées à chaque modification.
	var chemin := DOSSIER + "acces.tres"
	regles = ResourceLoader.load(chemin, "", ResourceLoader.CACHE_MODE_IGNORE) as ServerAccessPolicy if FileAccess.file_exists(chemin) else null
	if regles == null:
		regles = ServerAccessPolicy.new()
		ResourceSaver.save(regles, chemin)
	regles.changed.connect(func(): ResourceSaver.save(regles, chemin); _rafraichir_tout())
	session.access_policy = regles

	# Clé d'identité : l'empreinte du serveur (dans les codes de connexion) reste stable.
	session.identity_key = _charger_cle(DOSSIER + "identite.key")
	var cle_transport := DOSSIER + "transport.key"
	var certificat := DOSSIER + "transport.crt"
	if FileAccess.file_exists(cle_transport) and FileAccess.file_exists(certificat):
		session.transport_key = _charger_cle(cle_transport)
		var x509 := X509Certificate.new()
		if x509.load(certificat) == OK:
			session.transport_certificate = x509

	var config := ConfigFile.new()
	config.load(DOSSIER + "reglages.cfg")
	%Mode.select(config.get_value("serveur", "mode", 0))
	%NomServeur.text = config.get_value("serveur", "nom", "Serveur de " + session.player_name)
	%Port.value = config.get_value("serveur", "port", 47500)
	%Transport.select(config.get_value("serveur", "transport", 0))
	%JoueursMax.value = config.get_value("serveur", "joueurs_max", 16)
	# Mot de passe commun stocké en clair dans user:// (fichier local du serveur) pour un redémarrage sans saisie.
	%MotDePasse.text = config.get_value("serveur", "mot_de_passe", "")
	%ChiffrementTransport.button_pressed = config.get_value("serveur", "tls", true)
	%BoutEnBout.button_pressed = config.get_value("serveur", "e2e", false)
	%AdressePublique.text = config.get_value("serveur", "adresse", "")


func _charger_cle(chemin: String) -> CryptoKey:
	var cle := CryptoKey.new()
	if FileAccess.file_exists(chemin) and cle.load(chemin) == OK:
		return cle
	cle = Crypto.new().generate_rsa(2048)
	cle.save(chemin)
	return cle


func _sauver_reglages() -> void:
	var config := ConfigFile.new()
	config.set_value("serveur", "mode", %Mode.selected)
	config.set_value("serveur", "nom", %NomServeur.text)
	config.set_value("serveur", "port", int(%Port.value))
	config.set_value("serveur", "transport", %Transport.selected)
	config.set_value("serveur", "joueurs_max", int(%JoueursMax.value))
	config.set_value("serveur", "mot_de_passe", %MotDePasse.text)
	config.set_value("serveur", "tls", %ChiffrementTransport.button_pressed)
	config.set_value("serveur", "e2e", %BoutEnBout.button_pressed)
	config.set_value("serveur", "adresse", %AdressePublique.text)
	config.save(DOSSIER + "reglages.cfg")


# ▶️ Démarrage

func _connecter_signaux() -> void:
	%BoutonDemarrer.pressed.connect(_demarrer)
	%BoutonArreter.pressed.connect(func(): session.close(); redirection.close_port())
	%BoutonUpnp.pressed.connect(_ouvrir_port)
	%BoutonIpPublique.pressed.connect(func(): _journaliser("🔎 Recherche de l'adresse publique…"); redirection.find_public_address())
	%BoutonCopierCode.pressed.connect(_copier_code)
	%BoutonEnvoyer.pressed.connect(_envoyer)
	%Message.text_submitted.connect(func(_t): _envoyer())

	%ModeAcces.item_selected.connect(func(i): regles.access_mode = i)
	%RoleDefaut.item_selected.connect(func(i): regles.default_role = i)
	%Validation.toggled.connect(func(actif): regles.require_approval = actif)
	%BoutonAjouterCompte.pressed.connect(_ajouter_compte)
	%BoutonCreerInvitation.pressed.connect(func():
		var code := regles.create_invite(int(%InvitationUsages.value), %InvitationDuree.value, %InvitationRole.get_selected_id())
		_journaliser("✉️ Invitation créée : [b]%s[/b]" % code))

	session.state_changed.connect(func(_e): _rafraichir_tout())
	session.hosting_started.connect(func(port): _journaliser("[color=cyan]▶️ Serveur en ligne sur le port %d (%s)[/color]" % [port, %Transport.text]))
	session.session_ended.connect(func(raison, message): _journaliser("[color=orange]⏹️ Serveur arrêté (%s) %s[/color]" % [Noms.raison(raison), message]))
	session.player_joined.connect(func(id, infos): _journaliser("👋 %s (#%d, %s, %s) connecté depuis %s" % [infos.name, id, infos.os, Noms.role(infos.get("role", 1)), session.get_peer_address(id)]); _rafraichir_tout())
	session.player_left.connect(func(id, infos): _journaliser("👋 %s (#%d) déconnecté" % [infos.name, id]); _rafraichir_tout())
	session.peer_rejected.connect(func(id, raison, message): _journaliser("[color=orange]⛔ #%d refusé (%s) : %s[/color]" % [id, Noms.raison(raison), message]); _rafraichir_tout())
	session.access_requested.connect(_sur_demande)
	session.message_received.connect(func(de, donnees, chiffre): _journaliser("[b]%s[/b]%s : %s" % [session.get_player_info(de).get("name", "#%d" % de), " 🔒" if chiffre else "", donnees]))
	session.security_warning.connect(func(id, message): _journaliser("[color=red]⚠️ Sécurité (#%d) : %s[/color]" % [id, message]))

	redirection.port_opened.connect(func(adresse, port):
		_journaliser("[color=green]🔓 Port %d ouvert sur la box (adresse vue par la box : %s)[/color]" % [port, adresse])
		if %AdressePublique.text.is_empty() and PortForwarder.classify_address(adresse) == PortForwarder.ADDRESS_PUBLIC:
			%AdressePublique.text = adresse
		_rafraichir_internet())
	redirection.port_failed.connect(func(message): _journaliser("[color=orange]🔒 Ouverture du port impossible : %s[/color]" % message); _rafraichir_internet())
	redirection.port_closed.connect(func(): _journaliser("🔒 Port refermé sur la box."); _rafraichir_internet())
	redirection.public_address_found.connect(func(adresse):
		_journaliser("🌍 Adresse publique : [b]%s[/b]" % adresse)
		if %AdressePublique.text.is_empty():
			%AdressePublique.text = adresse
		_rafraichir_internet())
	redirection.public_address_failed.connect(func(message): _journaliser("[color=orange]%s[/color]" % message))
	redirection.nat_warning.connect(func(message): _alertes_internet.append(message); _journaliser("[color=red]⚠️ %s[/color]" % message); _rafraichir_internet())


func _demarrer() -> void:
	session.dedicated_server = %Mode.selected == 0
	session.player_name = %NomServeur.text
	session.port = int(%Port.value)
	session.transport = %Transport.get_selected_id()
	session.max_players = int(%JoueursMax.value)
	session.password = %MotDePasse.text
	session.transport_encryption = %ChiffrementTransport.button_pressed
	session.end_to_end_encryption = %BoutEnBout.button_pressed
	_sauver_reglages()
	if session.transport_encryption and session.transport_certificate == null:
		_journaliser("[color=gray]🔑 Génération du certificat du serveur…[/color]")
		await get_tree().process_frame

	if session.host() != OK:
		_journaliser("[color=red]❌ %s[/color]" % session.get_last_message())
		return
	if session.transport_certificate and not FileAccess.file_exists(DOSSIER + "transport.crt"):
		session.transport_key.save(DOSSIER + "transport.key")
		session.transport_certificate.save(DOSSIER + "transport.crt")
	redirection.port = session.port
	redirection.forward_udp = session.transport == NetworkSession.TRANSPORT_ENET
	redirection.forward_tcp = session.transport == NetworkSession.TRANSPORT_WEBSOCKET
	_mettre_a_jour_code(false)


func _ouvrir_port() -> void:
	redirection.port = int(%Port.value)
	redirection.forward_udp = %Transport.get_selected_id() == NetworkSession.TRANSPORT_ENET
	redirection.forward_tcp = %Transport.get_selected_id() == NetworkSession.TRANSPORT_WEBSOCKET
	_alertes_internet.clear()
	_journaliser("🔓 Recherche de la box (UPnP)…")
	redirection.open_port()
	_rafraichir_internet()


# 🔗 Code de connexion

## Adresse mise dans le code : celle saisie, sinon l'IP publique connue, sinon l'IP locale (même réseau).
func _adresse_a_partager() -> String:
	if not %AdressePublique.text.strip_edges().is_empty():
		return %AdressePublique.text.strip_edges()
	for adresse in [redirection.get_public_address(), redirection.get_external_address()]:
		if PortForwarder.classify_address(adresse) == PortForwarder.ADDRESS_PUBLIC:
			return adresse
	var locales := PortForwarder.get_local_addresses()
	return locales[0] if not locales.is_empty() else "127.0.0.1"


func _mettre_a_jour_code(avec_invitation: bool) -> String:
	if not session.is_host():
		%CodeConnexion.text = ""
		return ""
	var invitation := ""
	if avec_invitation and regles.access_mode == ServerAccessPolicy.ACCESS_INVITATIONS:
		invitation = regles.create_invite(int(%InvitationUsages.value), %InvitationDuree.value, %InvitationRole.get_selected_id())
	var code := session.create_connection_code(_adresse_a_partager(), invitation)
	%CodeConnexion.text = code
	return code


func _copier_code() -> void:
	var code := _mettre_a_jour_code(%AvecInvitation.button_pressed)
	if code.is_empty():
		_journaliser("Démarrez d'abord le serveur.")
		return
	DisplayServer.clipboard_set(code)
	_journaliser("📋 Code copié : [code]%s[/code]" % code)
	if PortForwarder.classify_address(_adresse_a_partager()) != PortForwarder.ADDRESS_PUBLIC:
		_journaliser("[color=orange]ℹ️ Ce code contient une adresse locale : il ne marche que pour un appareil du même réseau. Trouvez l'IP publique (ou saisissez un nom de domaine) pour des appareils distants.[/color]")


# 🚪 Accès

func _ajouter_compte() -> void:
	var nom: String = %CompteNom.text.strip_edges()
	if regles.add_account(nom, %CompteMotDePasse.text, %CompteRole.get_selected_id()) != OK:
		_journaliser("[color=orange]Identifiant et mot de passe obligatoires.[/color]")
		return
	_journaliser("👤 Compte « %s » enregistré (%s)." % [nom, Noms.role(%CompteRole.get_selected_id())])
	%CompteNom.clear()
	%CompteMotDePasse.clear()


func _sur_demande(id: int, demande: Dictionary) -> void:
	_journaliser("[color=yellow]⏳ Demande d'accès : %s (%s %s) depuis %s — onglet Demandes[/color]" % [demande.name, demande.os, demande.os_version, demande.address])
	%Onglets.current_tab = 1
	_rafraichir_tout()
	if _sans_interface:
		_journaliser("[color=orange]Sans fenêtre, impossible de valider à la main : refusé. Désactivez la validation manuelle ou utilisez des invitations.[/color]")
		session.deny_peer(id, "Validation manuelle indisponible sur ce serveur.")


# 🖼️ Affichage

func _rafraichir_tout() -> void:
	# La session (enfant) émet encore ses signaux pendant la fermeture de la scène, quand les %nœuds
	# ne se résolvent plus : rien à afficher alors.
	if get_node_or_null("%Etat") == null:
		return
	var en_ligne := session.is_host()
	etat.text = Noms.ETATS[session.get_state()] + ("  ·  %d / %d joueurs" % [session.get_player_count(), session.max_players] if en_ligne else "")
	%BoutonDemarrer.disabled = en_ligne
	%BoutonArreter.disabled = not en_ligne
	for reglage in reglages_serveur:
		if reglage is LineEdit:
			reglage.editable = not en_ligne
		elif reglage is SpinBox:
			reglage.editable = not en_ligne
		else:
			reglage.disabled = en_ligne
	%Empreinte.text = "Empreinte du serveur : %s" % session.get_server_fingerprint() if session.identity_key else ""
	if not en_ligne:
		%CodeConnexion.text = ""

	%ModeAcces.select(regles.access_mode)
	%RoleDefaut.select(regles.default_role)
	%Validation.set_pressed_no_signal(regles.require_approval)
	_remplir_comptes()
	_remplir_invitations()
	_remplir_demandes()
	_remplir_joueurs()
	_remplir_appareils()
	_rafraichir_internet()


func _rafraichir_internet() -> void:
	# La session (enfant) émet encore ses signaux pendant la fermeture de la scène, quand les %nœuds
	# ne se résolvent plus : rien à afficher alors.
	if get_node_or_null("%Etat") == null:
		return
	var locales := PortForwarder.get_local_addresses()
	%Adresses.text = "IP locale : %s\nIP publique : %s" % [", ".join(locales) if not locales.is_empty() else "?",
		redirection.get_public_address() if redirection.get_public_address() else "inconnue (bouton « Mon IP publique »)"]
	var lignes := []
	match redirection.get_status():
		PortForwarder.STATUS_WORKING: lignes.append("⏳ Recherche de la box…")
		PortForwarder.STATUS_OPEN: lignes.append("[color=green]🔓 Port %d ouvert sur la box[/color]" % int(%Port.value))
		PortForwarder.STATUS_FAILED: lignes.append("[color=orange]🔒 %s Ouvrez le port %d (%s) à la main dans l'interface de la box, vers %s.[/color]" % [
			redirection.get_last_message(), int(%Port.value), "UDP" if %Transport.get_selected_id() == 0 else "TCP", locales[0] if not locales.is_empty() else "ce PC"])
		_: lignes.append("Port non ouvert sur la box : les appareils distants ne peuvent pas encore joindre ce serveur.")
	for alerte in _alertes_internet:
		lignes.append("[color=red]⚠️ %s[/color]" % alerte)
	etat_internet.text = "\n".join(lignes)


## Ligne d'une liste : texte (BBCode) + boutons [[libellé, Callable], ...] et contrôles supplémentaires.
func _ligne(texte: String, boutons: Array = [], controles: Array = []) -> Control:
	var cadre := PanelContainer.new()
	var colonne := VBoxContainer.new()
	cadre.add_child(colonne)
	var etiquette := RichTextLabel.new()
	etiquette.bbcode_enabled = true
	etiquette.fit_content = true
	etiquette.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	for taille in ["normal_font_size", "bold_font_size", "mono_font_size"]:
		etiquette.add_theme_font_size_override(taille, 12)
	etiquette.text = texte
	colonne.add_child(etiquette)
	if boutons.is_empty() and controles.is_empty():
		return cadre
	# Boutons sous le texte, alignés à droite : le texte garde toute la largeur.
	var ligne := HBoxContainer.new()
	ligne.alignment = BoxContainer.ALIGNMENT_END
	colonne.add_child(ligne)
	for controle in controles:
		ligne.add_child(controle)
	for bouton in boutons:
		var b := Button.new()
		b.text = bouton[0]
		b.focus_mode = Control.FOCUS_NONE
		b.pressed.connect(bouton[1])
		ligne.add_child(b)
	return cadre


func _vider(liste: Container, vide := "") -> void:
	for enfant in liste.get_children():
		liste.remove_child(enfant)
		enfant.queue_free()
	if not vide.is_empty():
		var etiquette := Label.new()
		etiquette.text = vide
		etiquette.modulate = Color(1, 1, 1, 0.55)
		etiquette.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		liste.add_child(etiquette)


func _remplir_comptes() -> void:
	var noms := regles.get_account_names()
	_vider(%ListeComptes, "Aucun compte." if noms.is_empty() else "")
	for nom in noms:
		%ListeComptes.add_child(_ligne("[b]%s[/b] — %s" % [nom, Noms.role(regles.get_account_role(nom))], [["Supprimer", regles.remove_account.bind(nom)]]))


func _remplir_invitations() -> void:
	var invitations := regles.get_invites()
	_vider(%ListeInvitations, "Aucune invitation valide." if invitations.is_empty() else "")
	for invitation in invitations:
		var usages := "illimitée" if invitation.uses_left < 0 else "%d usage(s)" % invitation.uses_left
		var expire := "sans limite" if invitation.expires_unix == 0 else "jusqu'au " + Time.get_datetime_string_from_unix_time(invitation.expires_unix + int(Time.get_time_zone_from_system().bias) * 60, true)
		%ListeInvitations.add_child(_ligne("[b]%s[/b] — %s · %s · %s" % [invitation.code, usages, expire, Noms.role(invitation.role)], [
			["📋", func():
				if session.is_host():
					DisplayServer.clipboard_set(session.create_connection_code(_adresse_a_partager(), invitation.code))
					_journaliser("📋 Code de connexion avec l'invitation %s copié." % invitation.code)
				else:
					DisplayServer.clipboard_set(invitation.code)],
			["Révoquer", regles.revoke_invite.bind(invitation.code)]]))


func _remplir_demandes() -> void:
	var demandes := session.get_pending_requests()
	_vider(%ListeDemandes, "Aucune demande en attente. Activez « Valider à la main » dans l'onglet Accès pour approuver chaque nouvel appareil." if demandes.is_empty() else "")
	for demande in demandes:
		var role := OptionButton.new()
		Noms.remplir(role, Noms.ROLES, demande.role)
		%ListeDemandes.add_child(_ligne("[b]%s[/b]%s\n%s %s · %s · [color=gray]%s[/color]" % [demande.name, " (%s)" % demande.username if demande.has("username") else "",
			demande.os, demande.os_version, demande.address, demande.fingerprint], [
			["✅ Accepter", func(): session.approve_peer(demande.peer_id, true, role.get_selected_id())],
			["❌ Refuser", func(): session.deny_peer(demande.peer_id)]], [role]))


func _remplir_joueurs() -> void:
	var ids := session.get_player_ids()
	_vider(%ListeJoueurs, "Personne n'est connecté." if ids.is_empty() else "")
	for id in ids:
		var infos := session.get_player_info(id)
		var boutons := []
		if id != session.get_local_peer_id():
			boutons = [["Expulser", session.kick.bind(id, "")], ["Bannir", session.ban.bind(id, "", false)], ["Bannir l'IP", session.ban.bind(id, "", true)]]
		%ListeJoueurs.add_child(_ligne("[b]%s[/b] #%d — %s%s\n%s %s · %s · %s" % [infos.name, id, Noms.role(infos.get("role", 1)),
			" 🛡️" if session.has_secure_link(id) else "", infos.os, infos.os_version, session.get_peer_address(id) if id != 1 else "ce PC",
			"[color=gray]%s[/color]" % infos.get("fingerprint", "")], boutons))


func _remplir_appareils() -> void:
	_vider(%ListeAppareils)
	_titre(%ListeAppareils, "🚫 Appareils bannis")
	for empreinte in regles.banned_devices:
		%ListeAppareils.add_child(_ligne("[code]%s[/code]" % empreinte, [["Débannir", regles.unban_device.bind(empreinte)]]))
	_titre(%ListeAppareils, "🚫 Adresses bannies")
	for adresse in regles.banned_addresses:
		%ListeAppareils.add_child(_ligne(adresse, [["Débannir", regles.unban_address.bind(adresse)]]))
	_titre(%ListeAppareils, "✅ Appareils reconnus (invités ou validés)")
	for appareil in regles.get_known_devices():
		%ListeAppareils.add_child(_ligne("[b]%s[/b] — %s\n[color=gray]%s[/color]" % [appareil.name, Noms.role(appareil.role), appareil.fingerprint],
			[["Oublier", regles.forget_device.bind(appareil.fingerprint)], ["Bannir", regles.ban_device.bind(appareil.fingerprint)]]))


func _titre(liste: Container, texte: String) -> void:
	var etiquette := Label.new()
	etiquette.text = texte
	etiquette.add_theme_font_size_override("font_size", 15)
	liste.add_child(etiquette)


func _envoyer() -> void:
	var texte: String = %Message.text.strip_edges()
	if texte.is_empty() or not session.is_host():
		return
	session.send_message("📢 " + texte)
	_journaliser("[b]Serveur[/b] : %s" % texte)
	%Message.clear()


func _journaliser(texte: String) -> void:
	journal.append_text(texte + "\n")
	if _sans_interface:
		print_rich(texte)


# 🧾 Sans fenêtre (serveur dédié en ligne de commande)

func _demarrer_sans_interface() -> void:
	var args := {}
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--"):
			var morceaux: PackedStringArray = argument.substr(2).split("=", true, 1)
			args[morceaux[0]] = morceaux[1] if morceaux.size() > 1 else ""

	%Mode.select(1 if args.has("jouer") else 0)
	if args.has("port"):
		%Port.value = int(args.port)
	if args.has("transport"):
		%Transport.select(1 if args.transport == "websocket" else 0)
	if args.has("mot-de-passe"):
		%MotDePasse.text = args["mot-de-passe"]
	if args.has("sans-tls"):
		%ChiffrementTransport.button_pressed = false
	if args.has("e2e"):
		%BoutEnBout.button_pressed = true
	if args.has("adresse"):
		%AdressePublique.text = args.adresse
	if args.has("acces"):
		regles.access_mode = {"ouvert": 0, "comptes": 1, "invitations": 2}.get(args.acces, regles.access_mode)
	if args.has("compte"):
		var compte: PackedStringArray = args.compte.split(":")
		if compte.size() >= 2:
			regles.add_account(compte[0], compte[1], int(compte[2]) if compte.size() > 2 else ServerAccessPolicy.ROLE_PLAYER)

	await _demarrer()
	if not session.is_host():
		get_tree().quit(1)
		return
	_journaliser("Accès : %s%s" % [Noms.MODES_ACCES[regles.access_mode], " + validation manuelle" if regles.require_approval else ""])
	if args.has("upnp"):
		_ouvrir_port()
		while redirection.get_status() == PortForwarder.STATUS_WORKING:
			await get_tree().process_frame
	if args.has("ip-publique"):
		redirection.find_public_address()
		var limite := Time.get_ticks_msec() + 12000
		while redirection.get_public_address().is_empty() and Time.get_ticks_msec() < limite:
			await get_tree().process_frame
	var invitation := ""
	if args.has("invitation"):
		invitation = regles.create_invite(max(1, int(args.invitation)), 24.0)
	var code := session.create_connection_code(_adresse_a_partager(), invitation)
	%CodeConnexion.text = code
	_journaliser("[b]Code de connexion :[/b] %s" % code)
