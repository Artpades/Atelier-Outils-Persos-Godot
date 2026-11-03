extends RefCounted

## Base des suites de tests automatiques des modules Widgets & Spatialité.
##
## Une suite dérivée implémente _executer() : c'est une coroutine, elle peut attendre des frames ou des
## secondes (tweens, ajouts différés...). Les nœuds de test sont créés dans un bac à sable (_ajouter),
## libéré en fin de suite.
## Chaque résultat est un Dictionary { statut, nom, detail } avec statut ∈ Statut.
##
## Certains tests provoquent volontairement des erreurs ou avertissements dans la console (caméra nulle,
## pool épuisé, chemin invalide...) : c'est attendu, seul le statut affiché compte.

enum Statut { REUSSI, ECHEC, AVERTISSEMENT }

var _resultats: Array[Dictionary] = []
var _bac_a_sable: Node
var _espions: Array = []


## Lance la suite sous p_hote (qui doit être dans l'arbre) et renvoie ses résultats. À appeler avec await.
func executer(hote: Node) -> Array[Dictionary]:
	_resultats.clear()
	_bac_a_sable = Node.new()
	_bac_a_sable.name = "BacASableTests"
	hote.add_child(_bac_a_sable)

	await _executer()

	_deconnecter_espions()
	_bac_a_sable.queue_free()
	await hote.get_tree().process_frame
	return _resultats


## À implémenter par chaque suite (coroutine).
func _executer() -> void:
	await _frames(0)


# ✅ Résultats

func _verifier(nom: String, condition: bool, detail: String = "") -> void:
	_resultats.append({"statut": Statut.REUSSI if condition else Statut.ECHEC, "nom": nom, "detail": "" if condition else detail})


## Limitation connue plutôt qu'échec : signalée sans faire échouer la suite.
func _avertir_si(nom: String, condition: bool, detail: String) -> void:
	_resultats.append({"statut": Statut.REUSSI if condition else Statut.AVERTISSEMENT, "nom": nom, "detail": "" if condition else detail})


# 🛠️ Utilitaires

## Ajoute p_noeud au bac à sable (ou à p_parent) et le renvoie.
func _ajouter(noeud: Node, parent: Node = null) -> Node:
	(parent if parent else _bac_a_sable).add_child(noeud)
	return noeud


func _frames(nombre: int = 1) -> void:
	for i in nombre:
		await _bac_a_sable.get_tree().process_frame


func _secondes(duree: float) -> void:
	await _bac_a_sable.get_tree().create_timer(duree).timeout


## Empaquette p_racine (et ses enfants) en PackedScene, puis libère p_racine.
func _empaqueter(racine: Node) -> PackedScene:
	for enfant in racine.find_children("*", "", true, false):
		enfant.owner = racine
	var scene := PackedScene.new()
	scene.pack(racine)
	racine.free()
	return scene


## Compte les émissions d'un signal (jusqu'à 4 arguments) : lire compteur.n et compteur.args (deux premiers
## arguments de la dernière émission). Déconnecté automatiquement en fin de suite (les singletons survivent à la suite).
func _espionner(sig: Signal) -> Dictionary:
	var compteur := {"n": 0, "args": []}
	var enregistrer := func(a = null, b = null, _c = null, _d = null) -> void:
		compteur.n += 1
		compteur.args = [a, b]
	sig.connect(enregistrer)
	_espions.append([sig, enregistrer])
	return compteur


func _deconnecter_espions() -> void:
	for espion in _espions:
		var sig: Signal = espion[0]
		# L'objet émetteur a pu être libéré par le test : sa connexion a disparu avec lui.
		if is_instance_valid(sig.get_object()) and sig.is_connected(espion[1]):
			sig.disconnect(espion[1])
	_espions.clear()
