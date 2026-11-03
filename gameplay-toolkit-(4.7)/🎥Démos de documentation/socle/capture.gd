extends Node

## 🎬 Scène filmée par Documentation/generer_medias.py :
##   <godot> --path . --write-movie <fichier>.avi --fixed-fps 30 --resolution 960x540 capture.tscn -- --page=effets/secousse --plan=<nom>
## Joue le plan puis quitte (fin de la vidéo). --lister : écrit « PLANS: {plans, legendes, schemas} » (JSON)
## et quitte, sans filmer.

const CATALOGUE := "res://🎥Démos de documentation/catalogue.json"


func _ready() -> void:
	var arguments := {}
	for argument in OS.get_cmdline_user_args():
		var morceaux := argument.trim_prefix("--").split("=", true, 1)
		arguments[morceaux[0]] = morceaux[1] if morceaux.size() > 1 else ""

	var catalogue: Dictionary = JSON.parse_string(FileAccess.get_file_as_string(CATALOGUE))
	var page: String = arguments.get("page", "")
	if not catalogue.has(page) or not catalogue[page].has("sequences"):
		push_error("Page absente du catalogue (ou sans séquences) : '%s'" % page)
		get_tree().quit(1)
		return

	var sequences: Node = load(catalogue[page]["sequences"]).new()
	if arguments.has("lister"):
		var dossier: String = sequences.get_script().resource_path.get_base_dir()
		var schemas := {}
		for cle in sequences.schemas():
			var schema: Array = sequences.schemas()[cle]
			schemas[cle] = [ProjectSettings.globalize_path(dossier.path_join("schemas").path_join(schema[0])), schema[1]]
		print("PLANS: ", JSON.stringify({"plans": sequences.plans().keys(), "legendes": sequences.legendes(), "schemas": schemas,
			"reutilisations": sequences.reutilisations()}))
		sequences.free()
		get_tree().quit()
		return

	add_child(sequences)
	var reussi: bool = await sequences.jouer(arguments.get("plan", ""))
	get_tree().quit(0 if reussi else 1)
