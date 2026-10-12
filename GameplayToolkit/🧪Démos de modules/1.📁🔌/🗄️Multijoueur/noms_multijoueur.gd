extends RefCounted

## Libellés partagés par les démos du module Multijoueur (index = valeur de l'énumération).

## NetworkSession.Reason
const RAISONS := ["", "fermée", "connexion impossible", "hôte perdu", "délai dépassé", "mauvais mot de passe", "autre jeu",
	"autre version", "partie complète", "sécurité incompatible", "poignée de main invalide", "expulsé", "accès refusé", "banni",
	"trop de tentatives"]
## NetworkSession.State
const ETATS := ["⚪ Hors ligne", "🟡 Connexion…", "🟡 Authentification…", "🟢 Connecté", "🟢 Serveur en ligne"]
## ServerAccessPolicy.Role
const ROLES := ["👁️ Spectateur", "🎮 Joueur", "🛡️ Modérateur", "👑 Administrateur"]
## ServerAccessPolicy.AccessMode
const MODES_ACCES := ["🔓 Ouvert", "👤 Comptes", "✉️ Invitations"]


static func raison(valeur: int) -> String:
	return RAISONS[valeur] if valeur >= 0 and valeur < RAISONS.size() else "raison %d" % valeur


static func role(valeur: int) -> String:
	return ROLES[valeur] if valeur >= 0 and valeur < ROLES.size() else "?"


## Remplit un OptionButton avec des libellés (identifiant = index).
static func remplir(option: OptionButton, libelles: Array, choisi := 0) -> void:
	option.clear()
	for i in libelles.size():
		option.add_item(libelles[i], i)
	option.select(choisi)
