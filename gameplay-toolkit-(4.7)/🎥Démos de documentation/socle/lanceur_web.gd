extends Control

## 🌐 Scène principale de l'export web de la documentation (application/run/main_scene.web).
## Ouvre l'aperçu de la page demandée : index.html?demo=effets/secousse (ou --demo=effets/secousse hors web).
## Sans paramètre : liste des aperçus disponibles.
## Auto-contrôle de l'export : &rapport=1 lance l'action principale de l'aperçu (_jouer) puis envoie une
## capture PNG et l'état des extensions en POST sur ./rapport (serveur de test local).

const CATALOGUE := "res://🎥Démos de documentation/catalogue.json"


func _ready() -> void:
	var catalogue: Dictionary = JSON.parse_string(FileAccess.get_file_as_string(CATALOGUE))
	var demande := _demo_demandee()
	if catalogue.has(demande) and catalogue[demande].has("apercu"):
		if _parametre("rapport").is_empty():
			get_tree().change_scene_to_file.call_deferred(catalogue[demande]["apercu"])
		else:
			_rapport(catalogue[demande]["apercu"])
		return

	var liste := VBoxContainer.new()
	liste.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	add_child(liste)
	var titre := Label.new()
	titre.text = "Aperçus interactifs du GameplayToolkit" if demande.is_empty() else "Aperçu introuvable : " + demande
	titre.add_theme_font_size_override("font_size", 24)
	liste.add_child(titre)
	for page in catalogue:
		if page.begins_with("_") or not catalogue[page].has("apercu"):
			continue
		var bouton := Button.new()
		bouton.text = page
		bouton.pressed.connect(get_tree().change_scene_to_file.bind(catalogue[page]["apercu"]))
		liste.add_child(bouton)


func _demo_demandee() -> String:
	return _parametre("demo")


## Paramètre `nom` de l'adresse de la page (web) ou argument --nom=valeur (ailleurs) ; "" s'il est absent.
func _parametre(nom: String) -> String:
	if OS.has_feature("web"):
		var valeur = JavaScriptBridge.eval("new URLSearchParams(window.location.search).get('%s') || ''" % nom, true)
		return str(valeur) if valeur != null else ""
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--%s=" % nom):
			return argument.trim_prefix("--%s=" % nom)
	return ""


func _rapport(apercu: String) -> void:
	var scene: Node = load(apercu).instantiate()
	add_child(scene)
	await get_tree().create_timer(2.6).timeout
	if scene.has_method("_jouer"): # action principale de l'aperçu, pour voir son effet sur la capture
		scene.call("_jouer")
	await get_tree().create_timer(0.4).timeout
	var etat := "scene=%s extensions=%s rendu=%s" % [scene.name, ",".join(GDExtensionManager.get_loaded_extensions()),
		RenderingServer.get_video_adapter_name()]
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	var vues := scene.find_children("*", "SubViewport", true, false)
	if not vues.is_empty():
		etat += " sous-vue=%s" % (vues[0] as SubViewport).get_texture().get_image().get_pixel(10, 10)
	etat += " racine=%s" % image.get_pixel(10, 10)
	var corps := JSON.stringify({"etat": etat, "png": Marshalls.raw_to_base64(image.save_png_to_buffer())})
	if OS.has_feature("web"):
		JavaScriptBridge.eval("fetch('rapport', {method: 'POST', body: %s})" % JSON.stringify(corps))
	else:
		print(etat)
