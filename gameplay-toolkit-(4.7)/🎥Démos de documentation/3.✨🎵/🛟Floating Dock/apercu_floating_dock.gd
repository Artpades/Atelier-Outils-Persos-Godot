extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🛟 Aperçu interactif de FloatingDock : survoler le dock avec la souris, régler grossissement, rayon,
## écarts, ancrage, orientation et mode de redimensionnement.

const COULEURS := [Color("4ea1ff"), Color("f2a33a"), Color("c678dd"), Color("5ccf7a"), Color("ef5f5f"), Color("ffd166"), Color("56d6e0")]
const NOMS := ["Pinceau", "Gomme", "Seau", "Texte", "Pipette", "Loupe", "Calques"]

var _dock := FloatingDock.new()
var _nom: Label


func titre() -> String:
	return "Floating Dock"


func aide() -> String:
	return "Survoler la rangée d'icônes avec la souris. Les flèches du clavier déplacent le focus (follow_focus)."


func construire() -> void:
	var zone := Control.new()
	zone.size = ZONE.size
	zone.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(zone)
	for i in 7:
		var icone := TextureRect.new()
		icone.texture = Formes.dessin(Vector2i(64, 64), COULEURS[i], [[Rect2i(16, 16, 32, 32), COULEURS[i].lightened(0.4)]])
		icone.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		icone.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		icone.tooltip_text = NOMS[i]
		icone.focus_mode = Control.FOCUS_ALL
		_dock.add_child(icone)
	zone.add_child(_dock)
	_placer()
	_nom = etiquette(Vector2(CENTRE.x - 100, 300), "", 24, Formes.ACCENT)
	_nom.size.x = 200
	_nom.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_dock.hovered_item_changed.connect(func(element): _nom.text = element.tooltip_text if element else ""; journaliser("hovered_item_changed(%s)" % (element.tooltip_text if element else "null")))

	panneau.ajouter_curseur("magnification", 1.0, 3.0, 0.05, _dock.magnification, func(v: float): _regler("magnification", v))
	panneau.ajouter_curseur("influence_radius", 20.0, 500.0, 5.0, _dock.influence_radius, func(v: float): _regler("influence_radius", v))
	panneau.ajouter_curseur("hover_spacing", 0.0, 40.0, 1.0, _dock.hover_spacing, func(v: float): _regler("hover_spacing", v))
	panneau.ajouter_curseur("hover_lift", 0.0, 40.0, 1.0, _dock.hover_lift, func(v: float): _regler("hover_lift", v))
	panneau.ajouter_curseur("smoothing", 0.0, 30.0, 0.5, _dock.smoothing, func(v: float): _regler("smoothing", v))
	panneau.ajouter_curseur("item_size", 24.0, 96.0, 2.0, _dock.item_size.x, func(v: float): _regler("item_size", Vector2(v, v)))
	panneau.ajouter_curseur("separation", 0.0, 30.0, 1.0, _dock.separation, func(v: float): _regler("separation", v))
	panneau.ajouter_options("item_anchor", PackedStringArray(["ANCHOR_BEGIN", "ANCHOR_CENTER", "ANCHOR_END"]), _dock.item_anchor, func(i: int): _regler("item_anchor", i))
	panneau.ajouter_options("resize_mode", PackedStringArray(["RESIZE_SIZE", "RESIZE_SCALE"]), 0, func(i: int): _regler("resize_mode", i))
	panneau.ajouter_case("vertical", false, func(v: bool): _regler("vertical", v))
	panneau.ajouter_bouton("Donner le focus au 1er élément", func(): _dock.get_child(0).grab_focus())
	_maj_code()


func _regler(propriete: String, valeur) -> void:
	_dock.set(propriete, valeur)
	_placer.call_deferred()
	_maj_code()


func _placer() -> void:
	if _dock.vertical:
		_dock.set_anchors_and_offsets_preset(Control.PRESET_CENTER_LEFT, Control.PRESET_MODE_MINSIZE, 24)
	else:
		_dock.set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM, Control.PRESET_MODE_MINSIZE, 40)


func _maj_code() -> void:
	afficher_code("$Dock.magnification = %s\n$Dock.influence_radius = %s\n$Dock.hover_spacing = %s · hover_lift = %s\n$Dock.item_anchor = %s · vertical = %s" % [
		snappedf(_dock.magnification, 0.01), _dock.influence_radius, _dock.hover_spacing, _dock.hover_lift,
		["ANCHOR_BEGIN", "ANCHOR_CENTER", "ANCHOR_END"][_dock.item_anchor], _dock.vertical])
