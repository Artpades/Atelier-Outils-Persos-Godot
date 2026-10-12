extends Control

## 💻 Client qui rejoint un serveur distant (scène « Démo Serveur ») par Internet ou en réseau local.
##
## - Coller le CODE DE CONNEXION donné par le serveur : adresse, port, transport et empreinte du serveur.
##   Le client vérifie la signature du serveur : un faux serveur (adresse détournée) est refusé.
## - Identifiants selon le mode d'accès du serveur : compte (identifiant + mot de passe), invitation (souvent
##   déjà dans le code), mot de passe commun. Le mot de passe ne circule jamais : seule une preuve est envoyée.
## - L'appareil a une clé d'identité sauvegardée (user://client_multijoueur/identite.key) : le serveur le
##   reconnaît d'une connexion à l'autre (appareil invité ou validé, ou banni).
##
## Plusieurs clients sur une même machine : lancer avec « -- --profil=2 » pour une autre identité.

const Noms := preload("noms_multijoueur.gd")

@onready var session: NetworkSession = %Session
@onready var journal: RichTextLabel = %Journal
@onready var reglages: Array[Control] = [%Code, %Adresse, %Port, %Transport, %ChiffrementTransport, %Nom, %Identifiant, %MotDePasseCompte,
	%Invitation, %MotDePasseServeur, %ExigerBoutEnBout, %BoutonColler]

var dossier := "user://client_multijoueur/"


func _ready() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--profil="):
			dossier = "user://client_multijoueur_%s/" % argument.get_slice("=", 1).validate_filename()
	DirAccess.make_dir_recursive_absolute(dossier)
	%Transport.add_item("ENet (UDP)", NetworkSession.TRANSPORT_ENET)
	%Transport.add_item("WebSocket (TCP)", NetworkSession.TRANSPORT_WEBSOCKET)
	_charger()

	%BoutonColler.pressed.connect(func(): %Code.text = DisplayServer.clipboard_get().strip_edges(); _apercu_code())
	%Code.text_changed.connect(func(_t): _apercu_code())
	%BoutonConnecter.pressed.connect(_connecter)
	%BoutonDeconnecter.pressed.connect(session.close)
	%BoutonEnvoyer.pressed.connect(_envoyer)
	%Message.text_submitted.connect(func(_t): _envoyer())

	session.state_changed.connect(func(_e): _rafraichir())
	session.session_joined.connect(func(id): _journaliser("[color=green]🟢 Connecté (joueur #%d)[/color]" % id))
	session.waiting_for_approval.connect(func(delai): _journaliser("[color=yellow]⏳ Le serveur fait valider cet appareil par son administrateur (jusqu'à %d s)…[/color]" % delai))
	session.connection_failed.connect(func(raison, message): _journaliser("[color=red]❌ Connexion refusée (%s) : %s[/color]" % [Noms.raison(raison), message]))
	session.session_ended.connect(func(raison, message): _journaliser("[color=orange]🚪 Déconnecté (%s) : %s[/color]" % [Noms.raison(raison), message]))
	session.player_joined.connect(func(id, infos): _journaliser("👋 %s (#%d) est là" % [infos.name, id]); _rafraichir())
	session.player_left.connect(func(id, infos): _journaliser("👋 %s (#%d) est parti" % [infos.name, id]); _rafraichir())
	session.secure_link_established.connect(func(_id): _rafraichir())
	session.security_warning.connect(func(id, message): _journaliser("[color=red]⚠️ Sécurité (#%d) : %s[/color]" % [id, message]))
	session.message_received.connect(func(de, donnees, chiffre):
		var nom: String = "Serveur" if de == 1 and session.is_dedicated_session() else session.get_player_info(de).get("name", "#%d" % de)
		_journaliser("[b]%s[/b]%s : %s" % [nom, " 🔒" if chiffre else "", donnees]))
	_rafraichir()
	_apercu_code()


func _notification(quoi: int) -> void:
	# Fermeture de la fenêtre : avant le démontage de la scène (les %nœuds existent encore).
	if quoi == NOTIFICATION_WM_CLOSE_REQUEST:
		_sauver()


# 💾 Sauvegarde

func _charger() -> void:
	# Clé d'identité de l'appareil : générée une fois, puis réutilisée (le serveur reconnaît l'appareil).
	var chemin_cle := dossier + "identite.key"
	var cle := CryptoKey.new()
	if not (FileAccess.file_exists(chemin_cle) and cle.load(chemin_cle) == OK):
		cle = Crypto.new().generate_rsa(2048)
		cle.save(chemin_cle)
	session.identity_key = cle

	var config := ConfigFile.new()
	config.load(dossier + "reglages.cfg")
	%Code.text = config.get_value("client", "code", "")
	%Adresse.text = config.get_value("client", "adresse", "127.0.0.1")
	%Port.value = config.get_value("client", "port", 47500)
	%Transport.select(config.get_value("client", "transport", 0))
	%ChiffrementTransport.button_pressed = config.get_value("client", "tls", true)
	%Nom.text = config.get_value("client", "nom", session.player_name)
	%Identifiant.text = config.get_value("client", "identifiant", "")
	%ExigerBoutEnBout.button_pressed = config.get_value("client", "e2e", false)
	# Mots de passe et invitation : jamais sauvegardés.


func _sauver() -> void:
	var config := ConfigFile.new()
	config.set_value("client", "code", %Code.text)
	config.set_value("client", "adresse", %Adresse.text)
	config.set_value("client", "port", int(%Port.value))
	config.set_value("client", "transport", %Transport.selected)
	config.set_value("client", "tls", %ChiffrementTransport.button_pressed)
	config.set_value("client", "nom", %Nom.text)
	config.set_value("client", "identifiant", %Identifiant.text)
	config.set_value("client", "e2e", %ExigerBoutEnBout.button_pressed)
	config.save(dossier + "reglages.cfg")


# 🔌 Connexion

func _apercu_code() -> void:
	var code: String = %Code.text.strip_edges()
	var lu := NetworkSession.parse_connection_code(code)
	for controle in [%Adresse, %Port, %Transport, %ChiffrementTransport]:
		controle.modulate.a = 0.4 if not lu.is_empty() else 1.0
	if lu.is_empty():
		return
	%Adresse.text = lu.address
	%Port.value = lu.port
	%Transport.select(lu.transport)
	%ChiffrementTransport.button_pressed = lu.transport_encryption


func _connecter() -> void:
	session.player_name = %Nom.text
	session.account_name = %Identifiant.text.strip_edges()
	session.account_password = %MotDePasseCompte.text
	session.password = %MotDePasseServeur.text
	session.invite_code = %Invitation.text.strip_edges()
	session.end_to_end_encryption = %ExigerBoutEnBout.button_pressed
	_sauver()

	var code: String = %Code.text.strip_edges()
	if not code.is_empty():
		session.transport_encryption = false # le code dit si le serveur chiffre ; join_with_code s'y adapte
		_journaliser("➡️ Connexion avec le code de connexion…")
		await get_tree().process_frame
		session.join_with_code(code)
		return
	# Sans code : aucune empreinte attendue, le serveur n'est pas authentifié.
	session.expected_server_fingerprint = ""
	session.transport = %Transport.get_selected_id()
	session.transport_encryption = %ChiffrementTransport.button_pressed
	_journaliser("➡️ Connexion à %s:%d… [color=orange](sans code : identité du serveur non vérifiée)[/color]" % [%Adresse.text, int(%Port.value)])
	await get_tree().process_frame
	session.join(%Adresse.text.strip_edges(), int(%Port.value))


func _envoyer() -> void:
	var texte: String = %Message.text.strip_edges()
	if texte.is_empty() or not session.is_online():
		return
	session.send_message(texte)
	_journaliser("[b]Moi[/b]%s : %s" % [" 🔒" if session.is_end_to_end_active() else "", texte])
	%Message.clear()


# 🖼️ Affichage

func _rafraichir() -> void:
	# La session (enfant) émet encore ses signaux pendant la fermeture de la scène, quand les %nœuds
	# ne se résolvent plus : rien à afficher alors.
	if get_node_or_null("%Etat") == null:
		return
	var hors_ligne := session.get_state() == NetworkSession.STATE_OFFLINE
	%Etat.text = Noms.ETATS[session.get_state()].replace("Serveur en ligne", "Hôte")
	%BoutonConnecter.disabled = not hors_ligne
	%BoutonDeconnecter.disabled = hors_ligne
	for reglage in reglages:
		if reglage is LineEdit or reglage is SpinBox:
			reglage.editable = hors_ligne
		else:
			reglage.disabled = not hors_ligne

	var lignes := ["Mon appareil : [code]%s[/code]" % session.get_local_fingerprint()]
	if session.is_online():
		var resume := session.get_security_summary()
		lignes.append("[color=green]✅ Serveur authentifié (empreinte conforme au code)[/color]" if resume.server_verified
			else "[color=orange]⚠️ Serveur non vérifié (connexion sans code)[/color]")
		lignes.append("Empreinte du serveur : [code]%s[/code]" % session.get_server_fingerprint())
		lignes.append("🔒 Transport : %s" % ("chiffré" if resume.transport_encryption else "[color=orange]en clair[/color]"))
		lignes.append("🛡️ Bout en bout : %s" % ("actif" if resume.end_to_end else "inactif"))
		lignes.append("🖥️ %s" % ("Serveur dédié" if resume.dedicated else "Hôte joueur"))
		lignes.append("Mon rôle : %s" % Noms.role(session.get_player_info(session.get_local_peer_id()).get("role", 1)))
	%Securite.text = "\n".join(lignes)

	var joueurs := []
	for id in session.get_player_ids():
		var infos := session.get_player_info(id)
		joueurs.append("• [b]%s[/b]%s%s — %s\n   %s %s · %s" % [infos.name, " (moi)" if id == session.get_local_peer_id() else "", " 🛡️" if session.has_secure_link(id) else "",
			Noms.role(infos.get("role", 1)), infos.os, infos.os_version, infos.model])
	%Joueurs.text = "\n".join(joueurs) if not joueurs.is_empty() else "[color=gray]Pas connecté.[/color]"


func _journaliser(texte: String) -> void:
	journal.append_text(texte + "\n")
