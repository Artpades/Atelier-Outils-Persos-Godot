extends "suite_tests_multijoueur.gd"

## Suite de tests du mode serveur (jeu à distance) : règles d'accès (ServerAccessPolicy), code de
## connexion, serveur dédié, identité du serveur et des appareils, comptes, invitations, validation
## manuelle, bannissement, blocage après échecs, et classement des adresses IP (PortForwarder).
##
## Réutilise les utilitaires de suite_tests_multijoueur.gd (sessions simulées sur 127.0.0.1).
## L'ouverture réelle d'un port sur la box (UPnP) n'est PAS testée : elle modifierait la box de la machine.

const DOSSIER_TESTS := "user://tests_serveur/"


func _executer() -> void:
	_port = 47900
	_tester_comptes()
	await _tester_invitations()
	_tester_appareils_et_bans()
	_tester_sauvegarde_regles()
	_tester_classement_adresses()
	await _tester_code_de_connexion()
	await _tester_serveur_dedie()
	await _tester_identite_serveur()
	await _tester_acces_par_comptes()
	await _tester_acces_par_invitation()
	await _tester_validation_manuelle()
	await _tester_bannissement()
	await _tester_blocage_apres_echecs()
	await _nettoyer()


func _regles(mode := ServerAccessPolicy.ACCESS_OPEN) -> ServerAccessPolicy:
	var regles := ServerAccessPolicy.new()
	regles.access_mode = mode
	return regles


# 🚪 Règles d'accès (ServerAccessPolicy)

func _tester_comptes() -> void:
	var regles := _regles(ServerAccessPolicy.ACCESS_ACCOUNTS)
	_verifier("Compte sans mot de passe refusé", regles.add_account("alice", "") != OK)
	_verifier("Création d'un compte", regles.add_account("Alice", "secret", ServerAccessPolicy.ROLE_MODERATOR) == OK)
	_verifier("Identifiant insensible à la casse et aux espaces", regles.has_account(" alice ") and regles.has_account("ALICE"))
	_verifier("Rôle du compte", regles.get_account_role("alice") == ServerAccessPolicy.ROLE_MODERATOR)
	_verifier("Nom affiché conservé", regles.get_account_names() == PackedStringArray(["Alice"]), str(regles.get_account_names()))
	var stocke: Dictionary = regles.accounts["alice"]
	_verifier("Mot de passe jamais stocké en clair", not var_to_str(regles.accounts).contains("secret") and stocke.key.size() == 32)
	regles.set_account_role("alice", ServerAccessPolicy.ROLE_ADMIN)
	_verifier("Changement de rôle", regles.get_account_role("alice") == ServerAccessPolicy.ROLE_ADMIN)
	regles.remove_account("alice")
	_verifier("Suppression d'un compte", not regles.has_account("alice"))
	_verifier("Graine propre à chaque serveur", regles.get_account_seed() != ServerAccessPolicy.new().get_account_seed())


func _tester_invitations() -> void:
	var regles := _regles(ServerAccessPolicy.ACCESS_INVITATIONS)
	var code := regles.create_invite(2, 24.0, ServerAccessPolicy.ROLE_SPECTATOR)
	var morceaux := code.split("-")
	_verifier("Format du code d'invitation (XXXX-XXXX-XXXX)", morceaux.size() == 3 and Array(morceaux).all(func(m): return m.length() == 4), code)
	_verifier("Code reconnu sans tirets et en minuscules", regles.has_invite(code.replace("-", "").to_lower()))
	_verifier("Pas de caractères ambigus (0, O, 1, I)", not code.contains("0") and not code.contains("O") and not code.contains("1") and not code.contains("I"), code)
	var liste := regles.get_invites()
	_verifier("Liste des invitations", liste.size() == 1 and liste[0].code == code and liste[0].uses_left == 2 and liste[0].role == ServerAccessPolicy.ROLE_SPECTATOR, str(liste))
	regles.revoke_invite(code)
	_verifier("Révocation", not regles.has_invite(code) and regles.get_invites().is_empty())

	var courte := regles.create_invite(1, 0.0003) # environ 1 seconde
	_verifier("Invitation valide avant expiration", regles.has_invite(courte))
	await _secondes(2.2)
	_verifier("Invitation expirée", not regles.has_invite(courte) and regles.get_invites().is_empty())
	var illimitee := regles.create_invite(0, 0.0)
	_verifier("Invitation sans limite d'usage ni de durée", regles.get_invites()[0].uses_left == -1 and regles.get_invites()[0].expires_unix == 0 and regles.has_invite(illimitee))


func _tester_appareils_et_bans() -> void:
	var regles := _regles()
	regles.remember_device("AAAA BBBB", "PC de Bob", ServerAccessPolicy.ROLE_MODERATOR)
	_verifier("Appareil reconnu avec son rôle", regles.is_device_known("AAAA BBBB") and regles.get_device_role("AAAA BBBB") == ServerAccessPolicy.ROLE_MODERATOR)
	_verifier("Liste des appareils", regles.get_known_devices().size() == 1 and regles.get_known_devices()[0].name == "PC de Bob")
	regles.ban_device("AAAA BBBB")
	_verifier("Bannir oublie l'appareil", regles.is_device_banned("AAAA BBBB") and not regles.is_device_known("AAAA BBBB"))
	regles.unban_device("AAAA BBBB")
	_verifier("Débannir", not regles.is_device_banned("AAAA BBBB"))
	regles.ban_address("203.0.113.7")
	_verifier("Bannir une adresse", regles.is_address_banned("203.0.113.7") and not regles.is_address_banned("203.0.113.8"))
	var modifications := _espionner(regles.changed)
	regles.unban_address("203.0.113.7")
	_verifier("Resource.changed émis à chaque modification (sauvegarde automatique possible)", modifications.n == 1)


func _tester_sauvegarde_regles() -> void:
	DirAccess.make_dir_recursive_absolute(DOSSIER_TESTS)
	var regles := _regles(ServerAccessPolicy.ACCESS_ACCOUNTS)
	regles.require_approval = true
	regles.add_account("bob", "pw", ServerAccessPolicy.ROLE_ADMIN)
	var code := regles.create_invite(3)
	regles.remember_device("CCCC DDDD", "Tablette", ServerAccessPolicy.ROLE_PLAYER)
	regles.ban_device("EEEE FFFF")
	var chemin := DOSSIER_TESTS + "acces.tres"
	_verifier("Sauvegarde des règles (.tres)", ResourceSaver.save(regles, chemin) == OK)
	var relu := ResourceLoader.load(chemin, "", ResourceLoader.CACHE_MODE_IGNORE) as ServerAccessPolicy
	_verifier("Règles relues", relu != null and relu.access_mode == ServerAccessPolicy.ACCESS_ACCOUNTS and relu.require_approval)
	if relu:
		_verifier("Comptes, invitations, appareils et bans relus", relu.has_account("bob") and relu.get_account_role("bob") == ServerAccessPolicy.ROLE_ADMIN \
			and relu.has_invite(code) and relu.is_device_known("CCCC DDDD") and relu.is_device_banned("EEEE FFFF"))
		_verifier("Graine des comptes conservée (mots de passe toujours valides)", relu.get_account_seed() == regles.get_account_seed())
	DirAccess.remove_absolute(chemin)
	DirAccess.remove_absolute(DOSSIER_TESTS)


func _tester_classement_adresses() -> void:
	var cas := {
		"8.8.8.8": PortForwarder.ADDRESS_PUBLIC, "203.0.113.5": PortForwarder.ADDRESS_PUBLIC,
		"192.168.1.20": PortForwarder.ADDRESS_PRIVATE, "10.0.0.3": PortForwarder.ADDRESS_PRIVATE,
		"172.16.4.1": PortForwarder.ADDRESS_PRIVATE, "172.32.0.1": PortForwarder.ADDRESS_PUBLIC,
		"100.64.1.1": PortForwarder.ADDRESS_CGNAT, "100.127.255.1": PortForwarder.ADDRESS_CGNAT, "100.128.0.1": PortForwarder.ADDRESS_PUBLIC,
		"127.0.0.1": PortForwarder.ADDRESS_LOOPBACK, "::1": PortForwarder.ADDRESS_LOOPBACK,
		"fd12::1": PortForwarder.ADDRESS_PRIVATE, "fe80::1": PortForwarder.ADDRESS_PRIVATE, "2001:db8::1": PortForwarder.ADDRESS_PUBLIC,
		"::ffff:192.168.0.1": PortForwarder.ADDRESS_PRIVATE, "pas une ip": PortForwarder.ADDRESS_INVALID,
	}
	var erreurs := []
	for adresse in cas:
		if PortForwarder.classify_address(adresse) != cas[adresse]:
			erreurs.append("%s → %d" % [adresse, PortForwarder.classify_address(adresse)])
	_verifier("Classement des adresses (publique, privée, CGNAT, boucle locale)", erreurs.is_empty(), str(erreurs))
	var locales := PortForwarder.get_local_addresses()
	_verifier("Adresses locales : IPv4 privées uniquement", Array(locales).all(func(a): return PortForwarder.classify_address(a) == PortForwarder.ADDRESS_PRIVATE), str(locales))


# 🔗 Code de connexion

func _tester_code_de_connexion() -> void:
	var serveur := _session("Serveur", {"transport_encryption": true, "dedicated_server": true})
	serveur.host(_prochain_port())
	var code := serveur.create_connection_code("203.0.113.5", "abcd-efgh-jkmn")
	var lu := NetworkSession.parse_connection_code(code)
	_verifier("Code de connexion lisible", code.begins_with("gtk://203.0.113.5:%d?" % _port), code)
	_verifier("Code décodé : adresse, port, transport, chiffrement, empreinte, invitation",
		lu.address == "203.0.113.5" and lu.port == _port and lu.transport == NetworkSession.TRANSPORT_ENET and lu.transport_encryption \
		and lu.fingerprint == serveur.get_server_fingerprint() and lu.invite == "ABCDEFGHJKMN", str(lu))
	var v6 := NetworkSession.parse_connection_code("gtk://[2001:db8::1]:4000?t=websocket")
	_verifier("Code IPv6 + WebSocket", v6.address == "2001:db8::1" and v6.port == 4000 and v6.transport == NetworkSession.TRANSPORT_WEBSOCKET, str(v6))
	var invalides := ["", "http://x:1", "gtk://sansport", "gtk://hote:99999", "gtk://:4000"]
	_verifier("Codes invalides rejetés", invalides.all(func(c): return NetworkSession.parse_connection_code(c).is_empty()))

	var client := _session("Client")
	var fins := _fins(client)
	_verifier("join_with_code : code invalide signalé", client.join_with_code("n'importe quoi") != OK and fins.size() == 1)
	var local := serveur.create_connection_code("127.0.0.1")
	client.transport_encryption = false
	client.join_with_code(local)
	_verifier("join_with_code : connecté (transport chiffré annoncé par le code)",
		await _attendre(func(): return client.get_state() == NetworkSession.STATE_CONNECTED, 4.0) and client.is_transport_encrypted(), client.get_last_message())
	await _nettoyer()


# 🖥️ Serveur dédié

func _tester_serveur_dedie() -> void:
	var serveur := _session("Serveur", {"dedicated_server": true, "end_to_end_encryption": true, "max_players": 2})
	var c1 := _session("C1")
	var c2 := _session("C2")
	var c3 := _session("C3")
	var port := _prochain_port()
	serveur.host(port)
	_verifier("Serveur dédié : aucun joueur au démarrage", serveur.is_host() and serveur.get_player_count() == 0)
	c1.join("127.0.0.1", port)
	c2.join("127.0.0.1", port)
	var connectes := await _attendre(func(): return c1.get_state() == NetworkSession.STATE_CONNECTED and c2.get_state() == NetworkSession.STATE_CONNECTED, 5.0)
	_verifier("Deux clients connectés au serveur dédié", connectes)
	if not connectes:
		await _nettoyer()
		return
	var id1 := c1.get_local_peer_id()
	var id2 := c2.get_local_peer_id()
	_verifier("Le serveur n'est pas un joueur", await _attendre(func(): return c1.get_player_count() == 2 and not c1.has_player(1) and serveur.get_player_count() == 2, 2.0),
		"joueurs c1 %s, serveur %s" % [c1.get_player_ids(), serveur.get_player_ids()])
	_verifier("Les clients savent que le serveur est dédié", c1.is_dedicated_session() and c1.get_security_summary().dedicated)
	_verifier("max_players compte les clients seulement : 3e client refusé", await _refuse(c3, port, NetworkSession.REASON_SESSION_FULL))

	await _attendre(func(): return c1.has_secure_link(id2) and c1.has_secure_link(1) and serveur.has_secure_link(id1), 4.0)
	var boite_c2 := _boite(c2)
	var boite_serveur := _boite(serveur)
	var boite_c1 := _boite(c1)
	c1.send_message("à C2", id2)
	c1.send_message("au serveur", 1)
	serveur.send_message("annonce du serveur")
	await _attendre(func(): return boite_c2.size() == 2 and boite_serveur.size() == 1 and boite_c1.size() == 1, 3.0)
	_verifier("Client → client chiffré, le serveur ne le reçoit pas", boite_c2.any(func(m): return m[1] == "à C2" and m[2]) and boite_serveur.size() == 1, "%s / %s" % [boite_c2, boite_serveur])
	_verifier("Client → serveur dédié (chiffré)", boite_serveur.size() == 1 and boite_serveur[0] == [id1, "au serveur", true], str(boite_serveur))
	_verifier("Serveur → tous les clients (chiffré)", boite_c1.size() == 1 and boite_c1[0] == [1, "annonce du serveur", true] and boite_c2.any(func(m): return m[0] == 1), str(boite_c1))
	_verifier("Adresse IP d'un client vue par le serveur", serveur.get_peer_address(id1) == "127.0.0.1", serveur.get_peer_address(id1))
	await _nettoyer()


## Lance une connexion de p_client vers p_port et vérifie qu'elle est refusée avec p_raison.
func _refuse(client: NetworkSession, port: int, raison: int, delai := 4.0) -> bool:
	var fins := _fins(client)
	client.join("127.0.0.1", port)
	await _attendre(func(): return not fins.is_empty(), delai)
	if fins.size() != 1 or fins[0][0] != raison:
		_resultats.append({"statut": Statut.ECHEC, "nom": "  ↳ refus attendu %d" % raison, "detail": str(fins)})
		return false
	return true


func _tester_identite_serveur() -> void:
	var serveur := _session("Serveur", {"access_policy": _regles()})
	serveur.host(_prochain_port())
	var bon := _session("Bon", {"expected_server_fingerprint": serveur.get_server_fingerprint()})
	bon.join("127.0.0.1", _port)
	_verifier("Empreinte du serveur conforme : connecté et serveur vérifié",
		await _attendre(func(): return bon.get_state() == NetworkSession.STATE_CONNECTED, 4.0) and bon.get_security_summary().server_verified \
		and bon.get_server_fingerprint() == serveur.get_server_fingerprint(), bon.get_last_message())

	var mefiant := _session("Mefiant", {"expected_server_fingerprint": "0000 1111 2222 3333 4444 5555 6666 7777"})
	_verifier("Empreinte différente (faux serveur) : connexion annulée", await _refuse(mefiant, _port, NetworkSession.REASON_SECURITY_MISMATCH))
	var naif := _session("Naif")
	naif.join("127.0.0.1", _port)
	await _attendre(func(): return naif.get_state() == NetworkSession.STATE_CONNECTED, 4.0)
	_verifier("Sans empreinte attendue : connecté, mais serveur non vérifié", naif.is_online() and not naif.get_security_summary().server_verified)
	_verifier("L'appareil client est identifié par sa clé", not naif.get_local_fingerprint().is_empty() \
		and serveur.get_player_info(naif.get_local_peer_id()).fingerprint == naif.get_local_fingerprint())
	await _nettoyer()


# 🔑 Accès

func _tester_acces_par_comptes() -> void:
	var regles := _regles(ServerAccessPolicy.ACCESS_ACCOUNTS)
	regles.add_account("alice", "mot de passe", ServerAccessPolicy.ROLE_MODERATOR)
	var serveur := _session("Serveur", {"access_policy": regles, "dedicated_server": true})
	serveur.host(_prochain_port())
	var port := _port

	var alice := _session("Alice", {"account_name": "Alice", "account_password": "mot de passe"})
	alice.join("127.0.0.1", port)
	_verifier("Compte valide : connecté", await _attendre(func(): return alice.get_state() == NetworkSession.STATE_CONNECTED, 4.0), alice.get_last_message())
	if alice.is_online():
		var infos := serveur.get_player_info(alice.get_local_peer_id())
		_verifier("Rôle du compte attribué par le serveur", infos.role == ServerAccessPolicy.ROLE_MODERATOR and infos.username == "Alice", str(infos))
		_verifier("Le client voit son rôle", alice.get_player_info(alice.get_local_peer_id()).role == ServerAccessPolicy.ROLE_MODERATOR)
	_verifier("Mauvais mot de passe : ACCESS_DENIED",
		await _refuse(_session("Intrus", {"account_name": "alice", "account_password": "faux"}), port, NetworkSession.REASON_ACCESS_DENIED))
	_verifier("Compte inconnu : ACCESS_DENIED",
		await _refuse(_session("Inconnu", {"account_name": "mallory", "account_password": "x"}), port, NetworkSession.REASON_ACCESS_DENIED))
	_verifier("Sans identifiant : ACCESS_DENIED", await _refuse(_session("Anonyme"), port, NetworkSession.REASON_ACCESS_DENIED))
	await _nettoyer()


func _tester_acces_par_invitation() -> void:
	var regles := _regles(ServerAccessPolicy.ACCESS_INVITATIONS)
	var code := regles.create_invite(1, 1.0, ServerAccessPolicy.ROLE_SPECTATOR)
	var serveur := _session("Serveur", {"access_policy": regles})
	serveur.host(_prochain_port())
	var port := _port

	var invite := _session("Invite")
	invite.join_with_code(serveur.create_connection_code("127.0.0.1", code))
	_verifier("Code d'invitation valide : connecté", await _attendre(func(): return invite.get_state() == NetworkSession.STATE_CONNECTED, 4.0), invite.get_last_message())
	_verifier("Rôle de l'invitation (spectateur)", serveur.get_player_info(invite.get_local_peer_id()).get("role", -1) == ServerAccessPolicy.ROLE_SPECTATOR)
	_verifier("Invitation à usage unique consommée", not regles.has_invite(code))
	_verifier("Appareil invité mémorisé", regles.is_device_known(invite.get_local_fingerprint()))

	_verifier("Même code sur un autre appareil : ACCESS_DENIED",
		await _refuse(_session("Resquilleur", {"invite_code": code}), port, NetworkSession.REASON_ACCESS_DENIED))
	_verifier("Sans invitation : ACCESS_DENIED", await _refuse(_session("SansCode"), port, NetworkSession.REASON_ACCESS_DENIED))

	invite.close()
	invite.invite_code = ""
	await _frames(2)
	invite.join("127.0.0.1", port)
	_verifier("Appareil invité qui revient : accepté sans nouveau code",
		await _attendre(func(): return invite.get_state() == NetworkSession.STATE_CONNECTED, 4.0), invite.get_last_message())
	await _nettoyer()


func _tester_validation_manuelle() -> void:
	var regles := _regles()
	regles.require_approval = true
	var serveur := _session("Serveur", {"access_policy": regles})
	var demandes := []
	serveur.access_requested.connect(func(id: int, demande: Dictionary): demandes.append([id, demande]))
	serveur.host(_prochain_port())
	var port := _port

	var client := _session("Client")
	var attente := _espionner(client.waiting_for_approval)
	client.join("127.0.0.1", port)
	await _attendre(func(): return demandes.size() == 1 and attente.n == 1, 4.0)
	_verifier("Demande d'accès reçue par le serveur (nom, adresse, empreinte)", demandes.size() == 1 and demandes[0][1].name == "Client" \
		and demandes[0][1].address == "127.0.0.1" and not demandes[0][1].fingerprint.is_empty(), str(demandes))
	_verifier("Le client est prévenu qu'il attend la validation", attente.n == 1 and client.get_state() == NetworkSession.STATE_AUTHENTICATING)
	_verifier("get_pending_requests", serveur.get_pending_requests().size() == 1)
	await _secondes(4.5) # plus que connection_timeout (4 s) : l'attente de validation ne doit pas expirer
	_verifier("L'attente dépasse le délai de connexion normal", client.get_state() == NetworkSession.STATE_AUTHENTICATING, "état %d" % client.get_state())
	if demandes.is_empty():
		await _nettoyer()
		return
	serveur.approve_peer(demandes[0][0], true, ServerAccessPolicy.ROLE_MODERATOR)
	_verifier("Validé : connecté avec le rôle choisi", await _attendre(func(): return client.get_state() == NetworkSession.STATE_CONNECTED, 3.0) \
		and serveur.get_player_info(client.get_local_peer_id()).role == ServerAccessPolicy.ROLE_MODERATOR)

	var refuse := _session("Refuse")
	var fins := _fins(refuse)
	refuse.join("127.0.0.1", port)
	await _attendre(func(): return demandes.size() == 2, 4.0)
	serveur.deny_peer(demandes[-1][0], "Pas ce soir")
	await _attendre(func(): return not fins.is_empty(), 3.0)
	_verifier("Refusé : ACCESS_DENIED avec le message", fins.size() == 1 and fins[0] == [NetworkSession.REASON_ACCESS_DENIED, "Pas ce soir"], str(fins))

	client.close()
	await _frames(2)
	client.join("127.0.0.1", port)
	_verifier("Appareil validé et mémorisé : revient sans nouvelle validation",
		await _attendre(func(): return client.get_state() == NetworkSession.STATE_CONNECTED, 4.0) and demandes.size() == 2, "demandes %d" % demandes.size())
	await _nettoyer()


func _tester_bannissement() -> void:
	var serveur := _session("Serveur", {"access_policy": _regles(), "dedicated_server": true})
	serveur.host(_prochain_port())
	var port := _port
	var fautif := _session("Fautif")
	fautif.join("127.0.0.1", port)
	if not await _attendre(func(): return fautif.get_state() == NetworkSession.STATE_CONNECTED, 4.0):
		_verifier("Connexion avant bannissement", false, fautif.get_last_message())
		await _nettoyer()
		return
	var fins := _fins(fautif)
	serveur.ban(fautif.get_local_peer_id(), "Comportement abusif")
	await _attendre(func(): return not fins.is_empty(), 3.0)
	_verifier("Banni : REASON_BANNED avec le message", fins.size() == 1 and fins[0] == [NetworkSession.REASON_BANNED, "Comportement abusif"], str(fins))
	_verifier("Empreinte de l'appareil bannie", serveur.access_policy.is_device_banned(fautif.get_local_fingerprint()))
	await _frames(5)
	_verifier("L'appareil banni ne peut plus revenir", await _refuse(fautif, port, NetworkSession.REASON_BANNED))

	var autre := _session("Autre")
	autre.join("127.0.0.1", port)
	_verifier("Un autre appareil de la même adresse reste accepté", await _attendre(func(): return autre.get_state() == NetworkSession.STATE_CONNECTED, 4.0))
	serveur.ban(autre.get_local_peer_id(), "", true)
	await _frames(30)
	_verifier("Bannir l'adresse : tout appareil de cette adresse refusé",
		serveur.access_policy.is_address_banned("127.0.0.1") and await _refuse(_session("Voisin"), port, NetworkSession.REASON_BANNED))
	await _nettoyer()


func _tester_blocage_apres_echecs() -> void:
	var regles := _regles()
	regles.max_failed_attempts = 2
	regles.block_duration = 30.0
	var serveur := _session("Serveur", {"access_policy": regles, "password": "bon"})
	serveur.host(_prochain_port())
	var port := _port
	_verifier("1er mauvais mot de passe", await _refuse(_session("Essai1", {"password": "x"}), port, NetworkSession.REASON_WRONG_PASSWORD))
	_verifier("2e mauvais mot de passe", await _refuse(_session("Essai2", {"password": "y"}), port, NetworkSession.REASON_WRONG_PASSWORD))
	_verifier("Adresse bloquée ensuite, même avec le bon mot de passe",
		await _refuse(_session("Essai3", {"password": "bon"}), port, NetworkSession.REASON_TOO_MANY_ATTEMPTS))
	await _nettoyer()
