extends RefCounted

## Suite de tests automatiques du module de sauvegarde : couvre SaveConfig, SaveData, SaveSerializer,
## SaveFileAccess, SaveManager et SaveDataBinder.
##
## Travaille dans un dossier isolé (DOSSIER_TESTS), vidé avant et après exécution.
## Chaque résultat est un Dictionary { statut, nom, detail } avec statut ∈ Statut.
##
## Certains tests provoquent volontairement des erreurs dans la console (clé erronée, fichier corrompu,
## SaveData nul) : c'est attendu, seul le statut affiché compte.

enum Statut { REUSSI, ECHEC, AVERTISSEMENT }

const DOSSIER_TESTS := "user://demo_saves/tests/"
const CLE := "DemoGameplayToolkitCle32Octets!!"
const MAUVAISE_CLE := "MauvaiseCleMauvaiseCleMauvaise!!"

var _resultats: Array[Dictionary] = []
var _config: SaveConfig


func executer() -> Array[Dictionary]:
	_resultats.clear()
	_config = SaveConfig.new()
	_config.save_directory = DOSSIER_TESTS
	_vider_dossier()

	_tester_save_config()
	_tester_save_data()
	_tester_serializer()
	_tester_file_access_clair()
	_tester_file_access_chiffre()
	_tester_save_manager()
	_tester_binder_node()
	_tester_binder_resource()
	_tester_binder_cible_liberee()
	_tester_types()

	_vider_dossier()
	return _resultats


# ⚙️ SaveConfig

func _tester_save_config() -> void:
	var defaut := SaveConfig.new()
	_verifier("SaveConfig : valeurs par défaut",
		defaut.save_directory == "user://saves/" and defaut.extension == ".json" and defaut.encrypted_extension == ".sav",
		"%s | %s | %s" % [defaut.save_directory, defaut.extension, defaut.encrypted_extension])

	var perso := SaveConfig.new()
	perso.save_directory = "user://autre/"
	perso.extension = ".txt"
	perso.encrypted_extension = ".bin"
	_verifier("SaveConfig : setters / getters",
		perso.save_directory == "user://autre/" and perso.extension == ".txt" and perso.encrypted_extension == ".bin")


# 🗃️ SaveData

func _tester_save_data() -> void:
	var data := SaveData.new()
	_verifier("SaveData : SaveConfig instancié à la construction", data.file_config != null)
	_verifier("SaveData : chiffré par défaut", data.is_encrypted)
	_verifier("SaveData : clé par défaut de 32 octets", data.encryption_key.to_utf8_buffer().size() == 32,
		"%d octets" % data.encryption_key.to_utf8_buffer().size())

	data.system_id = "test"
	data.data = {"a": 1}
	_verifier("SaveData : setters / getters", data.system_id == "test" and data.data == {"a": 1})


# 🧾 SaveSerializer

func _tester_serializer() -> void:
	var source := _creer_save_data("serializer", false)
	var json := SaveSerializer.serialize(source)
	_verifier("SaveSerializer : serialize produit un JSON avec system_id et data",
		json.contains("\"system_id\"") and json.contains("\"data\""), json)

	var relu := SaveSerializer.deserialize(json)
	_verifier("SaveSerializer : aller-retour",
		relu != null and relu.system_id == source.system_id and relu.data == source.data)

	_verifier("SaveSerializer : JSON invalide → null", SaveSerializer.deserialize("{ pas du json") == null)
	_verifier("SaveSerializer : JSON non dictionnaire → null", SaveSerializer.deserialize("[1, 2, 3]") == null)

	var sans_data := SaveSerializer.deserialize("{\"system_id\": \"vide\"}")
	_verifier("SaveSerializer : clé data absente → dictionnaire vide",
		sans_data != null and sans_data.system_id == "vide" and sans_data.data.is_empty())


# 📁 SaveFileAccess (clair)

func _tester_file_access_clair() -> void:
	var source := _creer_save_data("acces_clair", false)
	var erreur := SaveFileAccess.write(source, _config)
	var chemin := DOSSIER_TESTS + "acces_clair.json"
	_verifier("SaveFileAccess : write (clair) renvoie OK", erreur == OK, error_string(erreur))
	_verifier("SaveFileAccess : fichier .json créé", FileAccess.file_exists(chemin), chemin)
	_verifier("SaveFileAccess : contenu lisible en clair",
		FileAccess.get_file_as_string(chemin).contains("acces_clair"))

	var relu := SaveFileAccess.read("acces_clair", _config, false)
	_verifier("SaveFileAccess : read (clair)", relu != null and relu.data == source.data)

	_verifier("SaveFileAccess : read d'un fichier inexistant → null",
		SaveFileAccess.read("inexistant", _config, false) == null)

	SaveFileAccess.delete("acces_clair", _config, false)
	_verifier("SaveFileAccess : delete supprime le fichier", not FileAccess.file_exists(chemin))


# 🔐 SaveFileAccess (chiffré)

func _tester_file_access_chiffre() -> void:
	var source := _creer_save_data("acces_chiffre", true)
	var erreur := SaveFileAccess.write(source, _config)
	var chemin := DOSSIER_TESTS + "acces_chiffre.sav"
	_verifier("SaveFileAccess : write (chiffré) renvoie OK", erreur == OK, error_string(erreur))
	_verifier("SaveFileAccess : fichier .sav créé", FileAccess.file_exists(chemin), chemin)

	var octets := FileAccess.get_file_as_bytes(chemin)
	_verifier("SaveFileAccess : taille = IV (16) + blocs de 16",
		octets.size() >= 32 and octets.size() % 16 == 0, "%d octets" % octets.size())
	_verifier("SaveFileAccess : contenu non lisible en clair",
		not octets.get_string_from_ascii().contains("acces_chiffre"))

	var relu := SaveFileAccess.read("acces_chiffre", _config, true, CLE)
	_verifier("SaveFileAccess : read (chiffré, bonne clé)", relu != null and relu.data == source.data)

	var mauvais := SaveFileAccess.read("acces_chiffre", _config, true, MAUVAISE_CLE)
	_verifier("SaveFileAccess : read avec une mauvaise clé → null", mauvais == null)

	_verifier("SaveFileAccess : read chiffré sur fichier clair absent → null",
		SaveFileAccess.read("acces_chiffre", _config, false) == null)

	# Fichier tronqué : moins de 32 octets, le déchiffrement doit refuser.
	var corrompu := FileAccess.open(DOSSIER_TESTS + "corrompu.sav", FileAccess.WRITE)
	corrompu.store_string("trop court")
	corrompu.close()
	_verifier("SaveFileAccess : fichier chiffré corrompu → null",
		SaveFileAccess.read("corrompu", _config, true, CLE) == null)

	SaveFileAccess.delete("acces_chiffre", _config, true)
	_verifier("SaveFileAccess : delete (chiffré)", not FileAccess.file_exists(chemin))


# 💾 SaveManager

func _tester_save_manager() -> void:
	for chiffre in [false, true]:
		var mode := "chiffré" if chiffre else "clair"
		var source := _creer_save_data("manager_" + mode, chiffre)

		SaveManager.save(source, _config)
		var charge := SaveManager.load(source, _config)
		_verifier("SaveManager : save puis load (%s)" % mode, charge != null and charge.data == source.data)

		SaveManager.delete(source, _config)
		_verifier("SaveManager : delete puis load → null (%s)" % mode, SaveManager.load(source, _config) == null)


# 🔗 SaveDataBinder → Node

func _tester_binder_node() -> void:
	var cible := Node2D.new()
	cible.name = "Cible"
	cible.rotation = 1.25
	cible.visible = false

	var binder := _creer_binder("binder_node")
	binder.node_target = cible
	binder.tracked_properties = PackedStringArray(["name", "rotation", "visible"])
	_verifier("SaveDataBinder : node_target assigné", binder.node_target == cible)

	binder.pull_to_save_data()
	var data: Dictionary = binder.save_data.data
	_verifier("SaveDataBinder : pull_to_save_data (Node)",
		data.get("name") == &"Cible" and is_equal_approx(data.get("rotation", 0.0), 1.25) and data.get("visible") == false,
		str(data))

	binder.save()
	cible.rotation = 0.0
	cible.visible = true
	cible.name = "Modifiee"
	binder.load()
	_verifier("SaveDataBinder : save puis load restaure la cible (Node)",
		is_equal_approx(cible.rotation, 1.25) and not cible.visible and cible.name == &"Cible",
		"rotation=%s visible=%s name=%s" % [cible.rotation, cible.visible, cible.name])

	binder.save_data.data = {"rotation": 2.0, "inconnue": 99}
	binder.push_from_save_data()
	_verifier("SaveDataBinder : push ignore les clés absentes / non suivies",
		is_equal_approx(cible.rotation, 2.0) and not cible.visible)

	binder.delete()
	_verifier("SaveDataBinder : delete supprime le fichier",
		not FileAccess.file_exists(DOSSIER_TESTS + "binder_node.json"))

	binder.free()
	cible.free()


# 🔗 SaveDataBinder → Resource (chiffré)

func _tester_binder_resource() -> void:
	var cible := preload("parametres_demo_sauvegarde.gd").new()
	cible.volume = 0.3
	cible.langue = "en"
	cible.plein_ecran = true

	var binder := _creer_binder("binder_resource", true)
	binder.resource_target = cible
	binder.tracked_properties = PackedStringArray(["volume", "langue", "plein_ecran"])
	binder.save()
	_verifier("SaveDataBinder : save (Resource, chiffré) crée le .sav",
		FileAccess.file_exists(DOSSIER_TESTS + "binder_resource.sav"))

	cible.volume = 1.0
	cible.langue = "fr"
	cible.plein_ecran = false
	binder.load()
	_verifier("SaveDataBinder : load restaure la cible (Resource)",
		is_equal_approx(cible.volume, 0.3) and cible.langue == "en" and cible.plein_ecran)

	# Un node_target défini est prioritaire sur resource_target.
	var noeud := Node.new()
	binder.node_target = noeud
	binder.tracked_properties = PackedStringArray(["name"])
	noeud.name = "Prioritaire"
	binder.pull_to_save_data()
	_verifier("SaveDataBinder : node_target prioritaire sur resource_target",
		binder.save_data.data.get("name") == &"Prioritaire")

	binder.delete()
	binder.free()
	noeud.free()


# 🔗 SaveDataBinder : cible libérée

func _tester_binder_cible_liberee() -> void:
	var cible := Node.new()
	var binder := _creer_binder("binder_libere")
	binder.node_target = cible
	binder.tracked_properties = PackedStringArray(["name"])
	cible.free()

	_verifier("SaveDataBinder : node_target libéré → null", binder.node_target == null)
	binder.pull_to_save_data()
	_verifier("SaveDataBinder : pull sans cible ne modifie pas les données", binder.save_data.data.is_empty())
	binder.free()


# 🔢 Conservation des types après passage par le JSON

func _tester_types() -> void:
	var source := SaveData.new()
	source.system_id = "types"
	source.is_encrypted = false
	source.data = {
		"entier": 42,
		"flottant": 1.5,
		"booleen": true,
		"texte": "Accents é à ç et emoji 🎮",
		"tableau": [1.5, "deux", true],
		"dictionnaire": {"cle": "valeur"},
		"vecteur": Vector2(3, 4),
		"couleur": Color.RED,
	}
	SaveManager.save(source, _config)
	var relu := SaveManager.load(source, _config)
	if relu == null:
		_verifier("Types : relecture", false)
		return
	var d: Dictionary = relu.data

	_verifier("Types : bool conservé", d.get("booleen") is bool and d.get("booleen"))
	_verifier("Types : String UTF-8 conservée", d.get("texte") == source.data["texte"], str(d.get("texte")))
	_verifier("Types : float conservé", is_equal_approx(d.get("flottant", 0.0), 1.5))
	_verifier("Types : Array conservé", d.get("tableau") == [1.5, "deux", true], str(d.get("tableau")))
	_verifier("Types : Dictionary conservé", d.get("dictionnaire") == {"cle": "valeur"})
	_avertir_si("Types : int relu en int", d.get("entier") is int,
		"relu en %s (%s) : le JSON ne distingue pas int et float" % [type_string(typeof(d.get("entier"))), d.get("entier")])
	_avertir_si("Types : Vector2 relu en Vector2", d.get("vecteur") is Vector2,
		"relu en %s (%s) : JSON.stringify écrit les types Godot en texte" % [type_string(typeof(d.get("vecteur"))), d.get("vecteur")])
	_avertir_si("Types : Color relue en Color", d.get("couleur") is Color,
		"relue en %s (%s)" % [type_string(typeof(d.get("couleur"))), d.get("couleur")])

	SaveManager.delete(source, _config)


# 🛠️ Utilitaires

func _creer_save_data(system_id: String, chiffre: bool) -> SaveData:
	var save_data := SaveData.new()
	save_data.system_id = system_id
	save_data.is_encrypted = chiffre
	save_data.encryption_key = CLE
	save_data.file_config = _config
	# Pas d'int : le JSON les relit en float et == compare les types (voir _tester_types).
	save_data.data = {"score": 1234.5, "nom": "Testeur", "inventaire": ["épée", "potion"]}
	return save_data


func _creer_binder(system_id: String, chiffre: bool = false) -> SaveDataBinder:
	var binder := SaveDataBinder.new()
	var save_data := _creer_save_data(system_id, chiffre)
	save_data.data = {}
	binder.save_data = save_data
	return binder


func _verifier(nom: String, condition: bool, detail: String = "") -> void:
	_resultats.append({"statut": Statut.REUSSI if condition else Statut.ECHEC, "nom": nom, "detail": detail})


## Limitation connue plutôt qu'échec : signalée sans faire échouer la suite.
func _avertir_si(nom: String, condition: bool, detail: String) -> void:
	_resultats.append({"statut": Statut.REUSSI if condition else Statut.AVERTISSEMENT, "nom": nom, "detail": "" if condition else detail})


func _vider_dossier() -> void:
	var dossier := DirAccess.open(DOSSIER_TESTS)
	if dossier == null:
		return
	for fichier in dossier.get_files():
		dossier.remove(fichier)
