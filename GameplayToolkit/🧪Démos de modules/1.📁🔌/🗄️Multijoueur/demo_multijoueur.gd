extends Control

## 🗄️ Démo du module Multijoueur (LanDiscovery, NetworkSession, SecureChannel).
##
## - Colonne de gauche : appareils à proximité qui exécutent un jeu fait avec ce module (système
##   d'exploitation, version, compatibilité, partie hébergée et sécurité), avec un bouton pour rejoindre.
## - Colonne du milieu : réglages de cet appareil (nom, mot de passe, transport, chiffrement), héberger /
##   quitter, résumé de sécurité et liste des joueurs avec leur empreinte de clé.
## - Colonne de droite : messages entre joueurs (🔒 = chiffré de bout en bout) et journal des signaux.
##
## Pour essayer seul : « Ajouter un appareil virtuel » crée un second appareil dans ce même programme. Il
## rejoint votre partie si vous hébergez, sinon il héberge la sienne (rejoignez-la depuis la liste). Il renvoie
## en écho les messages reçus. Pour un vrai test : lancer la scène sur deux ordinateurs du même réseau.

const JournalTests := preload("../../2.🧭🌍/journal_tests.gd")
const SuiteTests := preload("suite_tests_multijoueur.gd")

const NOMS_COMPATIBILITE := ["✅ compatible", "⬆️ autre version du jeu : mise à jour nécessaire", "🎮 autre jeu", "⬆️ autre version du module réseau"]
const NOMS_ETAT := ["⚪ Hors ligne", "🟡 Connexion…", "🟡 Authentification…", "🟢 Connecté", "🟢 Hôte de la partie"]
const Noms := preload("noms_multijoueur.gd")

@onready var decouverte: LanDiscovery = %Decouverte
@onready var session: NetworkSession = %Session
@onready var liste_appareils: VBoxContainer = %ListeAppareils
@onready var journal: RichTextLabel = %Journal
@onready var info_local: Label = %InfoLocal
@onready var etat: Label = %Etat
@onready var securite: RichTextLabel = %Securite
@onready var joueurs: RichTextLabel = %Joueurs
@onready var bouton_heberger: Button = %BoutonHeberger
@onready var bouton_quitter: Button = %BoutonQuitter
## Réglages verrouillés pendant une session.
@onready var reglages: Array[Control] = [%Nom, %MotDePasse, %Transport, %ChiffrementTransport, %BoutEnBout]

## Appareils virtuels : [branche, chemin] (chemin gardé pour rendre la branche au MultiplayerAPI principal).
var _bots: Array = []


func _ready() -> void:
	for bouton in find_children("*", "BaseButton"):
		bouton.focus_mode = Control.FOCUS_NONE
	%Transport.add_item("ENet (UDP)", NetworkSession.TRANSPORT_ENET)
	%Transport.add_item("WebSocket (TCP)", NetworkSession.TRANSPORT_WEBSOCKET)
	%Nom.text = session.player_name

	session.discovery = decouverte
	decouverte.device_found.connect(func(appareil): _journaliser("[color=gray]📡 Trouvé : %s (%s)[/color]" % [appareil.name, appareil.os]); _rafraichir_appareils())
	decouverte.device_updated.connect(func(_appareil): _rafraichir_appareils())
	decouverte.device_lost.connect(func(appareil): _journaliser("[color=gray]📡 Disparu : %s[/color]" % appareil.name); _rafraichir_appareils())

	session.state_changed.connect(func(_etat): _rafraichir_session())
	session.hosting_started.connect(func(port): _journaliser("[color=cyan]🏠 Partie hébergée sur le port %d[/color]" % port))
	session.session_joined.connect(func(id): _journaliser("[color=green]🟢 Partie rejointe (je suis le joueur #%d)[/color]" % id))
	session.connection_failed.connect(func(raison, message): _journaliser("[color=red]❌ Connexion refusée (%s) : %s[/color]" % [Noms.raison(raison), message]))
	session.session_ended.connect(func(raison, message): _journaliser("[color=orange]🚪 Session terminée (%s) : %s[/color]" % [Noms.raison(raison), message]))
	session.peer_rejected.connect(func(id, raison, _message): _journaliser("[color=orange]⛔ Joueur #%d refusé : %s[/color]" % [id, Noms.raison(raison)]))
	session.player_joined.connect(func(id, infos): _journaliser("👋 %s (#%d, %s) a rejoint" % [infos.name, id, infos.os]); _rafraichir_session())
	session.player_left.connect(func(id, infos): _journaliser("👋 %s (#%d) est parti" % [infos.name, id]); _rafraichir_session())
	session.player_updated.connect(func(_id, _infos): _rafraichir_session())
	session.secure_link_established.connect(func(id): _journaliser("[color=green]🛡️ Lien chiffré de bout en bout établi avec #%d[/color]" % id); _rafraichir_session())
	session.security_warning.connect(func(id, message): _journaliser("[color=red]⚠️ Sécurité (#%d) : %s[/color]" % [id, message]))
	session.message_received.connect(_sur_message)

	%BoutonHeberger.pressed.connect(_heberger)
	%BoutonQuitter.pressed.connect(session.close)
	%BoutonEnvoyer.pressed.connect(_envoyer)
	%Message.text_submitted.connect(func(_texte): _envoyer())
	%BoutonBot.pressed.connect(_ajouter_bot)
	%BoutonRetirerBots.pressed.connect(_retirer_bots)
	%BoutonTests.pressed.connect(_lancer_tests)

	decouverte.start_search()
	_rafraichir_appareils()
	_rafraichir_session()
	_journaliser("Bienvenue ! Hébergez une partie, ou rejoignez un appareil de la liste.")


func _exit_tree() -> void:
	# Les branches sont libérées avec la scène : il reste à oublier leur MultiplayerAPI.
	for bot in _bots:
		get_tree().set_multiplayer(null, bot[1])
	_bots.clear()


# ⚙️ Réglages et session

func _appliquer_reglages(cible: NetworkSession) -> void:
	cible.password = %MotDePasse.text
	cible.transport = %Transport.get_selected_id()
	cible.transport_encryption = %ChiffrementTransport.button_pressed
	cible.end_to_end_encryption = %BoutEnBout.button_pressed


func _heberger() -> void:
	session.player_name = %Nom.text
	decouverte.device_name = %Nom.text
	_appliquer_reglages(session)
	if %BoutEnBout.button_pressed or %ChiffrementTransport.button_pressed:
		_journaliser("[color=gray]🔑 Génération des clés RSA…[/color]")
		await get_tree().process_frame
	var erreur := session.host()
	if erreur != OK:
		_journaliser("[color=red]❌ Impossible d'héberger : %s[/color]" % session.get_last_message())


func _rejoindre(appareil: Dictionary) -> void:
	session.player_name = %Nom.text
	_appliquer_reglages(session)
	_journaliser("➡️ Connexion à %s (%s:%d)…" % [appareil.name, appareil.address, appareil.session.port])
	await get_tree().process_frame
	session.join_device(appareil)


func _envoyer() -> void:
	var texte: String = %Message.text.strip_edges()
	if texte.is_empty() or not session.is_online():
		return
	if session.get_player_count() < 2:
		_journaliser("[color=gray]Personne d'autre dans la partie.[/color]")
		return
	session.send_message(texte)
	_journaliser("[b]Moi[/b]%s : %s" % [" 🔒" if session.is_end_to_end_active() else "", texte])
	%Message.clear()


func _sur_message(de: int, donnees, chiffre: bool) -> void:
	var nom: String = session.get_player_info(de).get("name", "#%d" % de)
	_journaliser("[b]%s[/b]%s : %s" % [nom, " 🔒" if chiffre else "", str(donnees)])


# 🖥️ Affichage

func _rafraichir_appareils() -> void:
	var local := decouverte.get_local_device_info()
	info_local.text = "Cet appareil : %s — %s %s (%s) · port de découverte %d" % [local.name, local.os, local.os_version, local.distribution, decouverte.get_bound_port()]

	for enfant in liste_appareils.get_children():
		liste_appareils.remove_child(enfant)
		enfant.queue_free()
	var appareils := decouverte.get_devices()
	if appareils.is_empty():
		var vide := Label.new()
		vide.text = "Recherche en cours… aucun appareil pour l'instant."
		vide.modulate = Color(1, 1, 1, 0.6)
		liste_appareils.add_child(vide)
	for appareil in appareils:
		liste_appareils.add_child(_ligne_appareil(appareil))


func _ligne_appareil(appareil: Dictionary) -> Control:
	var cadre := PanelContainer.new()
	var colonnes := HBoxContainer.new()
	cadre.add_child(colonnes)

	var texte := RichTextLabel.new()
	texte.bbcode_enabled = true
	texte.fit_content = true
	texte.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	texte.add_theme_font_size_override("normal_font_size", 12)
	texte.add_theme_font_size_override("bold_font_size", 12)
	var lignes := [
		"[b]%s[/b] — %s %s" % [appareil.name, appareil.os, appareil.os_version],
		"[color=gray]%s · %s · %s[/color]" % [appareil.distribution, appareil.model, appareil.address],
		"🎮 %s %s — %s" % [appareil.game_id, "v" + appareil.game_version if appareil.game_version else "(sans version)", NOMS_COMPATIBILITE[appareil.compatibility]],
	]
	var partie: Dictionary = appareil.session
	if partie.get("hosting", false):
		var securite := []
		if partie.get("password_required", false):
			securite.append("🔑 mot de passe")
		if partie.get("transport_encryption", false):
			securite.append("🔒 transport")
		if partie.get("end_to_end_encryption", false):
			securite.append("🛡️ bout en bout")
		lignes.append("🏠 Partie %s : %d/%d joueurs · %s %s" % [partie.get("transport", ""), partie.get("players", 0), partie.get("max_players", 0),
			", ".join(securite) if not securite.is_empty() else "sans protection", "· %d ms" % appareil.latency_ms if appareil.latency_ms >= 0 else ""])
	else:
		lignes.append("[color=gray]N'héberge pas de partie[/color]")
	texte.text = "\n".join(lignes)
	colonnes.add_child(texte)

	var bouton := Button.new()
	bouton.text = "Rejoindre"
	bouton.focus_mode = Control.FOCUS_NONE
	bouton.disabled = not appareil.can_join or session.is_online()
	bouton.pressed.connect(_rejoindre.bind(appareil))
	colonnes.add_child(bouton)
	return cadre


func _rafraichir_session() -> void:
	etat.text = NOMS_ETAT[session.get_state()]
	var hors_ligne := session.get_state() == NetworkSession.STATE_OFFLINE
	bouton_heberger.disabled = not hors_ligne
	bouton_quitter.disabled = hors_ligne
	for reglage in reglages:
		if reglage is LineEdit:
			reglage.editable = hors_ligne
		else:
			reglage.disabled = not hors_ligne

	var resume := session.get_security_summary()
	var lignes := []
	if session.is_online():
		lignes.append("🔒 Transport : %s" % ("chiffré" + (" et hôte vérifié" if resume.transport_verified else " (hôte non vérifié)") if resume.transport_encryption else "[color=orange]en clair[/color]"))
		lignes.append("🛡️ Bout en bout : %s" % ("actif (%d lien(s))" % resume.secure_links.size() if resume.end_to_end else "[color=orange]inactif[/color]"))
		lignes.append("🔑 Mot de passe : %s" % ("oui" if resume.password_protected else "non"))
		if not session.get_local_fingerprint().is_empty():
			lignes.append("Mon empreinte : [code]%s[/code]" % session.get_local_fingerprint())
	securite.text = "\n".join(lignes)

	var lignes_joueurs := []
	var local_id := session.get_local_peer_id()
	for id in session.get_player_ids():
		var infos := session.get_player_info(id)
		var marques := ""
		if id == 1:
			marques += " 🏠"
		if id == local_id:
			marques += " (moi)"
		if session.has_secure_link(id):
			marques += " 🛡️"
		lignes_joueurs.append("• [b]%s[/b]%s — #%d\n   %s %s · %s" % [infos.name, marques, id, infos.os, infos.os_version, infos.model])
		if id != local_id and not session.get_peer_fingerprint(id).is_empty():
			lignes_joueurs.append("   [color=gray]empreinte [code]%s[/code][/color]" % session.get_peer_fingerprint(id))
	joueurs.text = "\n".join(lignes_joueurs) if not lignes_joueurs.is_empty() else "[color=gray]Aucune partie en cours.[/color]"
	_rafraichir_appareils()


func _journaliser(texte: String) -> void:
	journal.append_text(texte + "\n")


# 🤖 Appareils virtuels

func _ajouter_bot() -> void:
	var numero := _bots.size() + 1
	var branche := Node.new()
	branche.name = "AppareilVirtuel%d" % numero
	add_child(branche)
	# Son propre MultiplayerAPI : pour le réseau, c'est un appareil distinct.
	get_tree().set_multiplayer(SceneMultiplayer.new(), branche.get_path())
	_bots.append([branche, branche.get_path()])

	var nom := "🤖 Appareil virtuel %d" % numero
	var decouverte_bot := LanDiscovery.new()
	decouverte_bot.device_name = nom
	branche.add_child(decouverte_bot) # autostart : annonce sur le port de découverte libre suivant

	var bot := NetworkSession.new()
	bot.player_name = nom
	bot.player_data = {"virtuel": true}
	bot.port = session.port + numero
	bot.discovery = decouverte_bot
	_appliquer_reglages(bot)
	branche.add_child(bot)
	bot.message_received.connect(func(de: int, donnees, chiffre: bool):
		bot.send_message("écho%s de « %s »" % [" chiffré" if chiffre else "", donnees], de))

	_journaliser("[color=gray]🔑 %s prépare ses clés…[/color]" % nom)
	await get_tree().process_frame
	if session.is_host():
		# Il fait confiance au certificat de notre hôte : transport chiffré ET hôte vérifié.
		bot.trusted_certificate = session.transport_certificate
		bot.transport = session.transport
		bot.join("127.0.0.1", session.port)
		_journaliser("%s rejoint votre partie." % nom)
	else:
		bot.host()
		_journaliser("%s héberge une partie : rejoignez-la depuis la liste (même mot de passe et mêmes réglages)." % nom)


func _retirer_bots() -> void:
	for bot in _bots:
		if is_instance_valid(bot[0]):
			bot[0].free()
		get_tree().set_multiplayer(null, bot[1])
	_bots.clear()


# 🧪 Tests automatiques

func _lancer_tests() -> void:
	%BoutonTests.disabled = true
	_journaliser("[color=gray]🧪 Tests en cours (ports 47700+ et 47811+, sans toucher à la partie de la démo)…[/color]")
	var resultats: Array[Dictionary] = await SuiteTests.new().executer(self)
	_journaliser(JournalTests.formater("Tests du module Multijoueur", resultats))
	%BoutonTests.disabled = false
