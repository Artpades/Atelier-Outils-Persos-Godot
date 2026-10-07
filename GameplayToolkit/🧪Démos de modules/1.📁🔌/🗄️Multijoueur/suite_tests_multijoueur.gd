extends "../../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques du module Multijoueur : chiffrement (SecureChannel), découverte des
## appareils (LanDiscovery) et sessions (NetworkSession : poignée de main, refus, messages, chiffrement
## du transport et de bout en bout, expulsion, découverte d'une partie hébergée).
##
## Chaque « appareil » simulé est une branche avec son propre SceneMultiplayer : hôte et clients
## tournent dans le même processus, reliés par 127.0.0.1.
## Les refus (mauvais mot de passe, partie complète...) peuvent afficher des avertissements dans la
## console : c'est attendu, seul le statut affiché compte.

const JEU := "jeu_test_multijoueur"
const PORT_DECOUVERTE := 47700

## Prochain port de session : un port neuf par test, pour ne pas dépendre de la libération du précédent.
var _port := 47810
var _branches: Array[Node] = []


func _executer() -> void:
	_tester_canal_aller_retour()
	_tester_canal_alteration()
	_tester_canal_rejeu()
	_tester_derivation_cle()
	_tester_empreinte()
	await _tester_decouverte()
	await _tester_decouverte_plusieurs_instances()
	await _tester_session_simple()
	await _tester_relais_entre_clients()
	await _tester_refus()
	await _tester_partie_complete()
	await _tester_bout_en_bout()
	await _tester_exigence_bout_en_bout()
	await _tester_transport_dtls()
	await _tester_websocket_tls()
	await _tester_donnees_et_expulsion()
	await _tester_fermeture_hote()
	await _tester_decouverte_de_partie()
	await _nettoyer()


# 🛠️ Utilitaires

## Un appareil simulé : une branche avec son propre MultiplayerAPI et une NetworkSession réglée pour les tests.
func _session(nom: String, reglages := {}) -> NetworkSession:
	var branche := Node.new()
	branche.name = "%s_%d" % [nom, _branches.size()]
	_ajouter(branche)
	branche.get_tree().set_multiplayer(SceneMultiplayer.new(), branche.get_path())
	_branches.append(branche)

	var session := NetworkSession.new()
	session.name = "Session"
	session.player_name = nom
	session.game_id = JEU
	session.game_version = "1.0"
	session.connection_timeout = 4.0
	for cle in reglages:
		session.set(cle, reglages[cle])
	branche.add_child(session)
	return session


## Ferme toutes les sessions et rend leur branche au MultiplayerAPI principal.
func _nettoyer() -> void:
	for branche in _branches:
		if is_instance_valid(branche):
			var chemin := branche.get_path()
			branche.free()
			_bac_a_sable.get_tree().set_multiplayer(null, chemin)
	_branches.clear()
	await _frames(3)


func _prochain_port() -> int:
	_port += 1
	return _port


## Attend que p_condition soit vraie (au plus p_delai secondes). Renvoie la condition finale.
func _attendre(condition: Callable, delai := 4.0) -> bool:
	var limite := Time.get_ticks_msec() + int(delai * 1000.0)
	while not condition.call() and Time.get_ticks_msec() < limite:
		await _frames(1)
	return condition.call()


## Héberge sur un port neuf et fait rejoindre chaque client. Renvoie vrai si tous sont connectés.
func _connecter(hote: NetworkSession, clients: Array) -> bool:
	var port := _prochain_port()
	if hote.host(port) != OK:
		return false
	for client in clients:
		client.join("127.0.0.1", port)
	return await _attendre(func(): return clients.all(func(c): return c.get_state() == NetworkSession.STATE_CONNECTED) \
		and hote.get_player_count() == clients.size() + 1)


## Enregistre les messages reçus : [[expéditeur, données, chiffré], ...].
func _boite(session: NetworkSession) -> Array:
	var boite := []
	session.message_received.connect(func(de: int, donnees, chiffre: bool): boite.append([de, donnees, chiffre]))
	return boite


## Enregistre les fins de session ou échecs de connexion : [[raison, message], ...].
func _fins(session: NetworkSession) -> Array:
	var fins := []
	session.connection_failed.connect(func(raison: int, message: String): fins.append([raison, message]))
	session.session_ended.connect(func(raison: int, message: String): fins.append([raison, message]))
	return fins


func _decouverte(reglages := {}) -> LanDiscovery:
	var decouverte := LanDiscovery.new()
	decouverte.autostart = false
	decouverte.port = PORT_DECOUVERTE
	decouverte.port_range = 3
	decouverte.query_interval = 0.2
	decouverte.device_timeout = 2.0
	decouverte.game_id = JEU
	decouverte.game_version = "1.0"
	for cle in reglages:
		decouverte.set(cle, reglages[cle])
	_ajouter(decouverte)
	return decouverte


# 🔐 Chiffrement (SecureChannel)

func _tester_canal_aller_retour() -> void:
	var emetteur := SecureChannel.new()
	var cle := emetteur.generate_key()
	var recepteur := SecureChannel.new()
	recepteur.set_key(cle)
	_verifier("Clé de 64 octets", cle.size() == SecureChannel.KEY_SIZE, "taille %d" % cle.size())

	var texte := "Bonjour à toi ! 🔐".to_utf8_buffer()
	var paquet := emetteur.encrypt(texte)
	_verifier("Le paquet chiffré ne contient pas le texte clair", paquet.hex_encode().find(texte.hex_encode()) == -1)
	var dechiffre := recepteur.decrypt(paquet)
	_verifier("Aller-retour chiffrement / déchiffrement", dechiffre == texte and recepteur.get_last_error() == OK,
		"erreur %s" % error_string(recepteur.get_last_error()))

	var vide := recepteur.decrypt(emetteur.encrypt(PackedByteArray()))
	_verifier("Message vide accepté", vide.is_empty() and recepteur.get_last_error() == OK, error_string(recepteur.get_last_error()))
	_verifier("Deux chiffrements du même texte diffèrent (IV aléatoire)", emetteur.encrypt(texte) != emetteur.encrypt(texte))


func _tester_canal_alteration() -> void:
	var emetteur := SecureChannel.new()
	var recepteur := SecureChannel.new()
	recepteur.set_key(emetteur.generate_key())
	var paquet := emetteur.encrypt("ordre secret".to_utf8_buffer())

	var modifie := paquet.duplicate()
	modifie[30] = modifie[30] ^ 1
	_verifier("Paquet modifié rejeté (HMAC)", recepteur.decrypt(modifie).is_empty() and recepteur.get_last_error() == ERR_UNAUTHORIZED,
		error_string(recepteur.get_last_error()))

	var autre := SecureChannel.new()
	autre.generate_key()
	_verifier("Mauvaise clé rejetée", autre.decrypt(paquet).is_empty() and autre.get_last_error() == ERR_UNAUTHORIZED,
		error_string(autre.get_last_error()))
	_verifier("Paquet tronqué rejeté", recepteur.decrypt(paquet.slice(0, 20)).is_empty() and recepteur.get_last_error() == ERR_INVALID_DATA,
		error_string(recepteur.get_last_error()))
	var sans_cle := SecureChannel.new()
	_verifier("Déchiffrer sans clé échoue", sans_cle.decrypt(paquet).is_empty() and sans_cle.get_last_error() == ERR_UNCONFIGURED,
		error_string(sans_cle.get_last_error()))


func _tester_canal_rejeu() -> void:
	var emetteur := SecureChannel.new()
	var recepteur := SecureChannel.new()
	recepteur.set_key(emetteur.generate_key())
	var paquets := []
	for i in 5:
		paquets.append(emetteur.encrypt(PackedByteArray([i])))

	recepteur.decrypt(paquets[0])
	recepteur.decrypt(paquets[0])
	_verifier("Rejeu d'un paquet rejeté", recepteur.get_last_error() == ERR_ALREADY_EXISTS, error_string(recepteur.get_last_error()))

	# Désordre (canal non fiable) : 3 puis 1 puis 2 sont acceptés, une seule fois chacun.
	var ordre_ok := recepteur.decrypt(paquets[3]) == PackedByteArray([3]) and recepteur.decrypt(paquets[1]) == PackedByteArray([1]) \
		and recepteur.decrypt(paquets[2]) == PackedByteArray([2])
	_verifier("Paquets arrivés dans le désordre acceptés", ordre_ok)
	recepteur.decrypt(paquets[1])
	_verifier("Rejeu dans la fenêtre rejeté", recepteur.get_last_error() == ERR_ALREADY_EXISTS, error_string(recepteur.get_last_error()))

	for i in 70:
		emetteur.encrypt(PackedByteArray())
	recepteur.decrypt(emetteur.encrypt(PackedByteArray()))
	recepteur.decrypt(paquets[4])
	_verifier("Paquet trop ancien (hors fenêtre de 64) rejeté", recepteur.get_last_error() == ERR_ALREADY_EXISTS, error_string(recepteur.get_last_error()))
	_verifier("Compteur d'envoi", emetteur.get_sent_count() == 76, "envoyés %d" % emetteur.get_sent_count())


func _tester_derivation_cle() -> void:
	# Vecteurs de test PBKDF2-HMAC-SHA256 (RFC 7914, § 11 et vecteurs publiés).
	var une := SecureChannel.derive_key("password", "salt".to_utf8_buffer(), 1, 32).hex_encode()
	_verifier("PBKDF2 : vecteur de test (1 itération)", une == "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b", une)
	var deux := SecureChannel.derive_key("password", "salt".to_utf8_buffer(), 2, 32).hex_encode()
	_verifier("PBKDF2 : vecteur de test (2 itérations)", deux == "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43", deux)

	var sel := SecureChannel.random_bytes(16)
	var cle := SecureChannel.derive_key("mot de passe", sel, 100)
	_verifier("Dérivation déterministe, 64 octets par défaut", cle == SecureChannel.derive_key("mot de passe", sel, 100) and cle.size() == 64)
	_verifier("Autre sel, autre clé", cle != SecureChannel.derive_key("mot de passe", SecureChannel.random_bytes(16), 100))
	var canal := SecureChannel.new()
	_verifier("Une clé dérivée sert de clé de canal", canal.set_key(cle) == OK and canal.has_key())


func _tester_empreinte() -> void:
	var empreinte := SecureChannel.fingerprint("clé publique".to_utf8_buffer())
	var groupes := empreinte.split(" ")
	_verifier("Empreinte : 8 groupes de 4 caractères hexadécimaux", groupes.size() == 8 and Array(groupes).all(func(g): return g.length() == 4 and g.is_valid_hex_number()), empreinte)
	_verifier("Empreinte stable", empreinte == SecureChannel.fingerprint("clé publique".to_utf8_buffer()))
	_verifier("Empreinte différente pour une autre donnée", empreinte != SecureChannel.fingerprint("autre clé".to_utf8_buffer()))


# 📡 Découverte (LanDiscovery)

func _tester_decouverte() -> void:
	var annonceur := _decouverte({"device_name": "Annonceur"})
	var chercheur := _decouverte({"advertise": false})
	var trouves := []
	var perdus := []
	var mis_a_jour := []
	chercheur.device_found.connect(func(appareil): trouves.append(appareil))
	chercheur.device_lost.connect(func(appareil): perdus.append(appareil))
	chercheur.device_updated.connect(func(appareil): mis_a_jour.append(appareil))

	_verifier("L'annonceur ouvre le premier port de la plage", annonceur.start() == OK and annonceur.get_bound_port() == PORT_DECOUVERTE,
		"port %d" % annonceur.get_bound_port())
	chercheur.start_search()
	_verifier("Le chercheur trouve l'annonceur", await _attendre(func(): return trouves.size() >= 1, 3.0), "trouvés %d" % trouves.size())
	if trouves.is_empty():
		annonceur.queue_free()
		chercheur.queue_free()
		return
	var appareil: Dictionary = trouves[0]
	_verifier("Un seul appareil (pas le chercheur lui-même)", chercheur.get_device_count() == 1, "appareils %d" % chercheur.get_device_count())
	_verifier("Nom et système d'exploitation annoncés", appareil.name == "Annonceur" and appareil.os == OS.get_name() and appareil.has("model") \
		and appareil.has("distribution") and appareil.has("engine_version"), str(appareil))
	_verifier("Même jeu, même version : compatible", appareil.compatibility == LanDiscovery.COMPATIBLE, "compatibilité %s" % appareil.compatibility)
	_verifier("N'héberge rien : can_join faux", not appareil.can_join and not appareil.session.hosting)
	_verifier("Latence mesurée", appareil.latency_ms >= 0, "latence %s" % appareil.latency_ms)

	annonceur.set_session_info({"hosting": true, "port": 47999, "transport": "enet", "players": 1, "max_players": 4})
	_verifier("Partie hébergée : mise à jour immédiate, can_join vrai",
		await _attendre(func(): return mis_a_jour.size() >= 1 and mis_a_jour[-1].can_join, 1.0), "mises à jour %d" % mis_a_jour.size())
	_verifier("get_joinable_devices", chercheur.get_joinable_devices().size() == 1)

	annonceur.set_session_info({"hosting": true, "port": 47999, "transport": "enet", "players": 4, "max_players": 4})
	_verifier("Partie complète : can_join faux", await _attendre(func(): return not chercheur.get_devices()[0].can_join, 1.0))

	annonceur.game_version = "1.1"
	annonceur.set_session_info({"hosting": true, "players": 1, "max_players": 4})
	_verifier("Autre version : VERSION_MISMATCH (mise à jour nécessaire)",
		await _attendre(func(): return chercheur.get_devices()[0].compatibility == LanDiscovery.VERSION_MISMATCH, 1.5))

	annonceur.game_id = "autre_jeu"
	_verifier("Autre jeu : OTHER_GAME", await _attendre(func(): return chercheur.get_devices()[0].compatibility == LanDiscovery.OTHER_GAME, 1.5))
	chercheur.include_other_games = false
	chercheur.stop()
	chercheur.start_search()
	await _secondes(0.6)
	_verifier("include_other_games = false : autre jeu masqué", chercheur.get_device_count() == 0, "appareils %d" % chercheur.get_device_count())
	chercheur.include_other_games = true
	annonceur.game_id = JEU
	_verifier("Appareil retrouvé", await _attendre(func(): return chercheur.get_device_count() == 1, 1.5))

	perdus.clear()
	var avant := Time.get_ticks_msec()
	annonceur.stop()
	_verifier("Arrêt de l'annonceur : device_lost aussitôt (sans attendre l'expiration)",
		await _attendre(func(): return perdus.size() == 1, 1.0) and Time.get_ticks_msec() - avant < 1000, "perdus %d" % perdus.size())
	annonceur.queue_free()
	chercheur.queue_free()


func _tester_decouverte_plusieurs_instances() -> void:
	var a := _decouverte({"device_name": "A"})
	var b := _decouverte({"device_name": "B"})
	var chercheur := _decouverte({"advertise": false})
	a.start()
	b.start()
	_verifier("Deux instances sur la même machine : ports différents de la plage",
		a.get_bound_port() == PORT_DECOUVERTE and b.get_bound_port() == PORT_DECOUVERTE + 1, "%d / %d" % [a.get_bound_port(), b.get_bound_port()])
	chercheur.start_search()
	_verifier("Le chercheur trouve les deux", await _attendre(func(): return chercheur.get_device_count() == 2, 3.0), "appareils %d" % chercheur.get_device_count())
	_verifier("Identifiants d'appareil distincts", a.get_device_id() != b.get_device_id())

	var trouve_par_a := []
	a.device_found.connect(func(appareil): trouve_par_a.append(appareil.name))
	a.start_search()
	_verifier("Un annonceur peut aussi chercher (trouve B, pas lui-même)",
		await _attendre(func(): return trouve_par_a.has("B"), 3.0) and not trouve_par_a.has("A"), str(trouve_par_a))
	for n in [a, b, chercheur]:
		n.queue_free()
	await _frames(2)


# 🌐 Sessions (NetworkSession)

func _tester_session_simple() -> void:
	var hote := _session("Hote", {"player_data": {"couleur": "rouge"}})
	var client := _session("Client")
	var arrivees_hote := []
	hote.player_joined.connect(func(id: int, infos: Dictionary): arrivees_hote.append([id, infos]))
	var arrivees_client := []
	client.player_joined.connect(func(id: int, infos: Dictionary): arrivees_client.append(id))
	var rejoint := _espionner(client.session_joined)

	_verifier("Hors ligne au départ", hote.get_state() == NetworkSession.STATE_OFFLINE and not hote.is_online() and hote.get_local_peer_id() == 0)
	var connecte := await _connecter(hote, [client])
	_verifier("Le client rejoint l'hôte", connecte, "état client %d, joueurs hôte %d" % [client.get_state(), hote.get_player_count()])
	if not connecte:
		await _nettoyer()
		return
	var id_client := client.get_local_peer_id()
	_verifier("session_joined(identifiant local)", rejoint.n == 1 and rejoint.args[0] == id_client)
	_verifier("player_joined côté hôte : lui-même puis le client", arrivees_hote.size() == 2 and arrivees_hote[0][0] == 1 and arrivees_hote[1][0] == id_client,
		str(arrivees_hote.map(func(a): return a[0])))
	_verifier("player_joined côté client : tous les joueurs, lui compris", arrivees_client.has(1) and arrivees_client.has(id_client), str(arrivees_client))

	var infos_client: Dictionary = hote.get_player_info(id_client)
	_verifier("L'hôte connaît le nom et le système du client", infos_client.name == "Client" and infos_client.os == OS.get_name() \
		and infos_client.has("os_version") and infos_client.game_version == "1.0", str(infos_client))
	var infos_hote: Dictionary = client.get_player_info(1)
	_verifier("Le client connaît l'hôte et ses données", infos_hote.name == "Hote" and infos_hote.data.couleur == "rouge", str(infos_hote))
	_verifier("is_host / get_local_peer_id", hote.is_host() and hote.get_local_peer_id() == 1 and not client.is_host())
	_verifier("Pas de chiffrement demandé : résumé de sécurité", not hote.get_security_summary().end_to_end and not client.is_transport_encrypted())

	var boite_client := _boite(client)
	var boite_hote := _boite(hote)
	hote.send_message({"type": "bonjour", "valeur": 42})
	client.send_message("réponse", 1)
	await _attendre(func(): return boite_client.size() == 1 and boite_hote.size() == 1, 2.0)
	_verifier("Message de l'hôte reçu (non chiffré)", boite_client.size() == 1 and boite_client[0][0] == 1 and boite_client[0][1].valeur == 42 and not boite_client[0][2], str(boite_client))
	_verifier("Message du client reçu par l'hôte", boite_hote.size() == 1 and boite_hote[0] == [id_client, "réponse", false], str(boite_hote))
	_verifier("S'envoyer un message à soi-même : erreur", hote.send_message("x", 1) != OK)

	var depart := []
	hote.player_left.connect(func(id: int, _infos): depart.append(id))
	var fins_client := _fins(client)
	client.close()
	_verifier("Le client quitte : player_left côté hôte", await _attendre(func(): return depart.has(id_client), 2.0), str(depart))
	_verifier("Fermeture locale : session_ended(REASON_CLOSED)", fins_client.size() == 1 and fins_client[0][0] == NetworkSession.REASON_CLOSED, str(fins_client))
	_verifier("Le client revient hors ligne", client.get_state() == NetworkSession.STATE_OFFLINE and client.get_player_count() == 0)
	await _nettoyer()


func _tester_relais_entre_clients() -> void:
	var hote := _session("Hote")
	var c1 := _session("C1")
	var c2 := _session("C2")
	if not await _connecter(hote, [c1, c2]):
		_verifier("Trois joueurs connectés", false)
		await _nettoyer()
		return
	_verifier("Chaque client voit les trois joueurs", await _attendre(func(): return c1.get_player_count() == 3 and c2.get_player_count() == 3, 2.0),
		"%d / %d" % [c1.get_player_count(), c2.get_player_count()])
	var boite_c2 := _boite(c2)
	var boite_hote := _boite(hote)
	c1.send_message("à C2 seulement", c2.get_local_peer_id())
	c1.send_message("à tous sauf l'hôte", -1)
	await _attendre(func(): return boite_c2.size() == 2, 2.0)
	_verifier("Message entre deux clients relayé par l'hôte", boite_c2.size() == 2 and boite_c2[0][0] == c1.get_local_peer_id(), str(boite_c2))
	_verifier("L'hôte ne reçoit pas ce qui ne lui est pas destiné", boite_hote.is_empty(), str(boite_hote))
	await _nettoyer()


func _tester_refus() -> void:
	var cas := [
		["Mauvais mot de passe", {"password": "secret"}, {"password": "faux"}, NetworkSession.REASON_WRONG_PASSWORD],
		["Mot de passe absent", {"password": "secret"}, {}, NetworkSession.REASON_WRONG_PASSWORD],
		["Autre version du jeu", {}, {"game_version": "2.0"}, NetworkSession.REASON_VERSION_MISMATCH],
		["Autre jeu", {}, {"game_id": "autre_jeu"}, NetworkSession.REASON_GAME_MISMATCH],
	]
	for c in cas:
		var hote := _session("Hote", c[1])
		var client := _session("Client", c[2])
		var fins := _fins(client)
		var refus := _espionner(hote.peer_rejected)
		hote.host(_prochain_port())
		client.join("127.0.0.1", _port)
		await _attendre(func(): return not fins.is_empty(), 3.0)
		_verifier("%s : refusé avec la bonne raison" % c[0], fins.size() == 1 and fins[0][0] == c[3] and not fins[0][1].is_empty(), str(fins))
		_verifier("%s : l'hôte signale le refus, aucun joueur ajouté" % c[0], refus.n == 1 and hote.get_player_count() == 1, "refus %d, joueurs %d" % [refus.n, hote.get_player_count()])
		await _nettoyer()

	var hote := _session("Hote", {"password": "secret", "allow_version_mismatch": true})
	var client := _session("Client", {"password": "secret", "game_version": "1.1"})
	_verifier("Bon mot de passe et allow_version_mismatch : accepté", await _connecter(hote, [client]))
	_verifier("Partie protégée signalée", client.get_security_summary().password_protected)
	await _nettoyer()

	var seul := _session("Seul", {"connection_timeout": 1.0})
	var fins_seul := _fins(seul)
	seul.join("127.0.0.1", _prochain_port())
	_verifier("Hôte absent : échec de connexion signalé", await _attendre(func(): return not fins_seul.is_empty(), 3.0) \
		and fins_seul[0][0] in [NetworkSession.REASON_TIMEOUT, NetworkSession.REASON_CONNECTION_FAILED], str(fins_seul))
	await _nettoyer()


func _tester_partie_complete() -> void:
	var hote := _session("Hote", {"max_players": 2})
	var c1 := _session("C1")
	var c2 := _session("C2")
	var fins := _fins(c2)
	_verifier("Premier client accepté (2 places)", await _connecter(hote, [c1]))
	c2.join("127.0.0.1", _port)
	await _attendre(func(): return not fins.is_empty(), 3.0)
	_verifier("Client en trop : REASON_SESSION_FULL", fins.size() == 1 and fins[0][0] == NetworkSession.REASON_SESSION_FULL, str(fins))
	await _nettoyer()


func _tester_bout_en_bout() -> void:
	var hote := _session("Hote", {"end_to_end_encryption": true})
	var c1 := _session("C1")  # n'exige rien : adopte le chiffrement de l'hôte
	var c2 := _session("C2", {"end_to_end_encryption": true})
	if not await _connecter(hote, [c1, c2]):
		_verifier("Partie chiffrée de bout en bout : connexions", false)
		await _nettoyer()
		return
	var id1 := c1.get_local_peer_id()
	var id2 := c2.get_local_peer_id()
	_verifier("Chiffrement de bout en bout actif chez tous", hote.is_end_to_end_active() and c1.is_end_to_end_active() and c2.is_end_to_end_active())
	var liens_ok := await _attendre(func(): return c1.has_secure_link(id2) and c2.has_secure_link(id1) and c1.has_secure_link(1) and hote.has_secure_link(id2), 4.0)
	_verifier("Liens chiffrés établis entre chaque paire", liens_ok,
		"c1→c2 %s, c2→c1 %s, c1→hôte %s" % [c1.has_secure_link(id2), c2.has_secure_link(id1), c1.has_secure_link(1)])

	_verifier("Empreinte de C1 vue par C2 = empreinte locale de C1", c2.get_peer_fingerprint(id1) == c1.get_local_fingerprint() and not c1.get_local_fingerprint().is_empty(),
		"%s / %s" % [c2.get_peer_fingerprint(id1), c1.get_local_fingerprint()])
	_verifier("Empreinte dans les infos du joueur", hote.get_player_info(id1).fingerprint == c1.get_local_fingerprint())
	_verifier("Empreintes distinctes d'un joueur à l'autre", c1.get_local_fingerprint() != c2.get_local_fingerprint())

	var boite_c2 := _boite(c2)
	var boite_hote := _boite(hote)
	var paquets_vus_par_hote := []
	hote.multiplayer.peer_packet.connect(func(_id, paquet: PackedByteArray): paquets_vus_par_hote.append(paquet))
	c1.send_message({"secret": "code 1234"}, id2)
	await _attendre(func(): return boite_c2.size() == 1, 2.0)
	_verifier("Message de bout en bout reçu et marqué chiffré", boite_c2.size() == 1 and boite_c2[0][1].secret == "code 1234" and boite_c2[0][2], str(boite_c2))
	_verifier("L'hôte relaie sans recevoir le message", boite_hote.is_empty(), str(boite_hote))

	c2.send_message("diffusion chiffrée")
	await _attendre(func(): return boite_hote.size() == 1, 2.0)
	_verifier("Diffusion : chiffrée pour chaque destinataire", boite_hote.size() == 1 and boite_hote[0][2], str(boite_hote))
	_verifier("Aucun paquet vu par l'hôte ne contient le texte clair", not paquets_vus_par_hote.any(func(p: PackedByteArray): return p.hex_encode().contains("diffusion".to_utf8_buffer().hex_encode())))

	# Un intermédiaire qui injecte un message en clair dans une partie chiffrée est ignoré.
	var alertes := []
	c2.security_warning.connect(func(id: int, message: String): alertes.append([id, message]))
	var injecte := PackedByteArray([0xC7, 3])
	injecte.append_array(var_to_bytes("message injecté"))
	c1.multiplayer.send_bytes(injecte, id2)
	await _attendre(func(): return not alertes.is_empty(), 2.0)
	_verifier("Message en clair injecté : rejeté avec alerte", alertes.size() == 1 and boite_c2.size() == 1, "alertes %s, reçus %d" % [alertes, boite_c2.size()])
	await _nettoyer()


func _tester_exigence_bout_en_bout() -> void:
	var hote := _session("Hote")
	var client := _session("Client", {"end_to_end_encryption": true})
	var fins := _fins(client)
	hote.host(_prochain_port())
	client.join("127.0.0.1", _port)
	await _attendre(func(): return not fins.is_empty(), 3.0)
	_verifier("Client qui exige le bout en bout chez un hôte sans : refus (pas de rétrogradation)",
		fins.size() == 1 and fins[0][0] == NetworkSession.REASON_SECURITY_MISMATCH, str(fins))
	await _nettoyer()


func _tester_transport_dtls() -> void:
	var hote := _session("Hote", {"transport_encryption": true})
	var port := _prochain_port()
	hote.host(port)
	_verifier("Certificat auto-signé généré par l'hôte", hote.transport_certificate != null and hote.transport_key != null)

	var client := _session("Client", {"transport_encryption": true, "trusted_certificate": hote.transport_certificate})
	client.join("127.0.0.1", port)
	var connecte := await _attendre(func(): return client.get_state() == NetworkSession.STATE_CONNECTED, 4.0)
	_verifier("ENet + DTLS avec certificat vérifié : connecté", connecte, "état %d, raison %s" % [client.get_state(), client.get_last_message()])
	var resume: Dictionary = client.get_security_summary()
	_verifier("Transport chiffré et hôte authentifié", resume.transport_encryption and resume.transport_verified, str(resume))

	var boite := _boite(hote)
	client.send_message("par DTLS")
	_verifier("Message transmis par DTLS", await _attendre(func(): return boite.size() == 1, 2.0))

	var sans_verif := _session("SansVerif", {"transport_encryption": true})
	sans_verif.join("127.0.0.1", port)
	_verifier("DTLS sans certificat de confiance : chiffré mais hôte non vérifié",
		await _attendre(func(): return sans_verif.get_state() == NetworkSession.STATE_CONNECTED, 4.0) and not sans_verif.get_security_summary().transport_verified)

	var autre_cert := Crypto.new().generate_self_signed_certificate(Crypto.new().generate_rsa(2048), "CN=gameplaytoolkit.local")
	var mefiant := _session("Mefiant", {"transport_encryption": true, "trusted_certificate": autre_cert, "connection_timeout": 2.0})
	var fins := _fins(mefiant)
	mefiant.join("127.0.0.1", port)
	await _attendre(func(): return not fins.is_empty(), 4.0)
	_verifier("Certificat de l'hôte différent de celui attendu : connexion refusée", fins.size() == 1 and mefiant.get_state() == NetworkSession.STATE_OFFLINE, str(fins))
	await _nettoyer()


func _tester_websocket_tls() -> void:
	var hote := _session("Hote", {"transport": NetworkSession.TRANSPORT_WEBSOCKET})
	var client := _session("Client", {"transport": NetworkSession.TRANSPORT_WEBSOCKET})
	_verifier("WebSocket : connexion", await _connecter(hote, [client]))
	var boite := _boite(client)
	hote.send_message("par WebSocket")
	_verifier("WebSocket : message transmis", await _attendre(func(): return boite.size() == 1, 2.0))
	await _nettoyer()

	var hote_tls := _session("HoteTLS", {"transport": NetworkSession.TRANSPORT_WEBSOCKET, "transport_encryption": true, "end_to_end_encryption": true})
	var port := _prochain_port()
	hote_tls.host(port)
	var client_tls := _session("ClientTLS", {"transport": NetworkSession.TRANSPORT_WEBSOCKET, "transport_encryption": true,
		"trusted_certificate": hote_tls.transport_certificate})
	client_tls.join("127.0.0.1", port)
	_verifier("WebSocket sécurisé (wss://) + bout en bout : connecté",
		await _attendre(func(): return client_tls.get_state() == NetworkSession.STATE_CONNECTED, 4.0), client_tls.get_last_message())
	var boite_tls := _boite(hote_tls)
	client_tls.send_message("doublement chiffré")
	_verifier("wss:// + bout en bout : message reçu chiffré", await _attendre(func(): return boite_tls.size() == 1 and boite_tls[0][2], 3.0), str(boite_tls))
	await _nettoyer()


func _tester_donnees_et_expulsion() -> void:
	var hote := _session("Hote")
	var c1 := _session("C1")
	var c2 := _session("C2")
	if not await _connecter(hote, [c1, c2]):
		_verifier("Connexions (données / expulsion)", false)
		await _nettoyer()
		return
	var id1 := c1.get_local_peer_id()
	var maj_c2 := []
	c2.player_updated.connect(func(id: int, infos: Dictionary): maj_c2.append([id, infos.data]))
	c1.update_player_data({"score": 7})
	_verifier("Données d'un client diffusées aux autres", await _attendre(func(): return maj_c2.size() == 1 and maj_c2[0] == [id1, {"score": 7}], 2.0), str(maj_c2))
	_verifier("Données mises à jour chez l'hôte", hote.get_player_info(id1).data == {"score": 7})

	var fins := _fins(c1)
	var departs := []
	c2.player_left.connect(func(id: int, _infos): departs.append(id))
	_verifier("Un client ne peut pas expulser", c2.kick(id1) != OK)
	hote.kick(id1, "Triche détectée")
	await _attendre(func(): return not fins.is_empty() and departs.has(id1), 3.0)
	_verifier("Expulsion : REASON_KICKED avec le message", fins.size() == 1 and fins[0] == [NetworkSession.REASON_KICKED, "Triche détectée"], str(fins))
	_verifier("Les autres voient le départ", departs.has(id1) and not hote.has_player(id1), str(departs))
	await _nettoyer()


func _tester_fermeture_hote() -> void:
	var hote := _session("Hote")
	var client := _session("Client")
	if not await _connecter(hote, [client]):
		_verifier("Connexion (fermeture de l'hôte)", false)
		await _nettoyer()
		return
	var fins := _fins(client)
	var departs := []
	client.player_left.connect(func(id: int, _infos): departs.append(id))
	hote.close()
	await _attendre(func(): return not fins.is_empty(), 3.0)
	_verifier("Hôte fermé : REASON_HOST_LOST chez le client", fins.size() == 1 and fins[0][0] == NetworkSession.REASON_HOST_LOST, str(fins))
	_verifier("player_left pour chaque joueur à la fin de session", departs.has(1) and departs.size() == 2, str(departs))
	await _nettoyer()


func _tester_decouverte_de_partie() -> void:
	var hote := _session("Hote", {"password": "secret", "end_to_end_encryption": true, "max_players": 4})
	var decouverte_hote := _decouverte()
	hote.discovery = decouverte_hote
	decouverte_hote.start()
	var chercheur := _decouverte({"advertise": false})
	chercheur.start_search()
	hote.host(_prochain_port())

	var trouve := await _attendre(func(): return chercheur.get_joinable_devices().size() == 1, 3.0)
	_verifier("La partie hébergée apparaît dans la découverte", trouve, str(chercheur.get_devices()))
	if trouve:
		var appareil: Dictionary = chercheur.get_joinable_devices()[0]
		var s: Dictionary = appareil.session
		_verifier("Annonce : port, joueurs, mot de passe, chiffrement", s.port == _port and s.players == 1 and s.max_players == 4 \
			and s.password_required and s.end_to_end_encryption and not s.transport_encryption and s.host_name == "Hote", str(s))

		var client := _session("Client", {"password": "secret"})
		client.join_device(appareil)
		_verifier("join_device rejoint la partie trouvée", await _attendre(func(): return client.get_state() == NetworkSession.STATE_CONNECTED, 4.0), client.get_last_message())
		_verifier("Nombre de joueurs annoncé mis à jour", await _attendre(func(): return chercheur.get_devices()[0].session.players == 2, 1.5))

		var exigeant := _session("Exigeant", {"transport_encryption": true})
		var fins := _fins(exigeant)
		_verifier("join_device refuse un hôte sans chiffrement du transport si on l'exige",
			exigeant.join_device(chercheur.get_devices()[0]) == ERR_UNAUTHORIZED and fins.size() == 1 and fins[0][0] == NetworkSession.REASON_SECURITY_MISMATCH, str(fins))

	hote.close()
	_verifier("Fin de partie : la découverte ne l'annonce plus", await _attendre(func(): return chercheur.get_joinable_devices().is_empty(), 1.5))
	decouverte_hote.queue_free()
	chercheur.queue_free()
	await _nettoyer()
