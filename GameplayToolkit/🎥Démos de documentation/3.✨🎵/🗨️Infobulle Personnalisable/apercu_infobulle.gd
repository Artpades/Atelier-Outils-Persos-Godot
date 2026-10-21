extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🗨️ Aperçu interactif de CustomTooltip : survoler les cases d'inventaire, éditer le contenu (titre, texte
## BBCode, icône, largeur) et l'affichage (délai, fondu, suivi de la souris, décalage, style).

const COULEURS := [Color("ef5f5f"), Color("4ea1ff"), Color("5ccf7a"), Color("c678dd")]

var _infobulles: Array[CustomTooltip] = []
var _contenu := TooltipContent.new()
var _style := StyleBoxFlat.new()


func titre() -> String:
	return "Infobulle personnalisable"


func aide() -> String:
	return "Survoler les cases avec la souris. La première utilise le contenu édité ci-dessous."


func construire() -> void:
	_style.bg_color = Color(0.08, 0.06, 0.12, 0.95)
	_style.border_color = Color(0.8, 0.65, 0.3)
	_style.set_border_width_all(2)
	_style.set_corner_radius_all(6)
	_style.set_content_margin_all(10)
	_contenu.title = "Épée de feu"
	_contenu.text = "[color=orange]+12[/color] dégâts de feu"
	_contenu.icon = Formes.dessin(Vector2i(64, 64), COULEURS[0], [[Rect2i(14, 14, 36, 36), COULEURS[0].lightened(0.45)]])
	var autres := [["Bouclier", "Bloque [b]40 %[/b] des dégâts"], ["Potion", "Rend [color=green]50 PV[/color]"], ["Anneau", "[i]Une lueur étrange...[/i]"]]
	for i in 4:
		var case_inventaire := Button.new()
		case_inventaire.size = Vector2(90, 90)
		case_inventaire.position = Vector2(90 + i * 140, 260) if i < 3 else Vector2(600, 480)
		var fond := StyleBoxFlat.new()
		fond.bg_color = COULEURS[i]
		fond.set_corner_radius_all(8)
		case_inventaire.add_theme_stylebox_override("normal", fond)
		case_inventaire.add_theme_stylebox_override("hover", fond)
		add_child(case_inventaire)
		var infobulle := CustomTooltip.new()
		if i == 0:
			infobulle.content = _contenu
		else:
			var contenu := TooltipContent.new()
			contenu.title = autres[i - 1][0]
			contenu.text = autres[i - 1][1]
			infobulle.content = contenu
		case_inventaire.add_child(infobulle)
		infobulle.tooltip_shown.connect(func(): journaliser("tooltip_shown (case %d)" % (i + 1)))
		infobulle.tooltip_hidden.connect(func(): journaliser("tooltip_hidden (case %d)" % (i + 1)))
		_infobulles.append(infobulle)

	panneau.ajouter_section("TooltipContent (case 1)")
	var titre_saisi := LineEdit.new()
	titre_saisi.text = _contenu.title
	titre_saisi.text_changed.connect(func(t: String): _contenu.title = t; _maj_code())
	panneau.ajouter(titre_saisi)
	var texte_saisi := LineEdit.new()
	texte_saisi.text = _contenu.text
	texte_saisi.text_changed.connect(func(t: String): _contenu.text = t; _maj_code())
	panneau.ajouter(texte_saisi)
	panneau.ajouter_case("icon", true, func(v: bool):
		_contenu.icon = Formes.dessin(Vector2i(64, 64), COULEURS[0], [[Rect2i(14, 14, 36, 36), COULEURS[0].lightened(0.45)]]) if v else null)
	panneau.ajouter_curseur("max_width", 80.0, 500.0, 10.0, _contenu.max_width, func(v: float): _contenu.max_width = v; _maj_code())
	panneau.ajouter_section("CustomTooltip (toutes)")
	panneau.ajouter_curseur("delay (s)", 0.0, 2.0, 0.05, 0.5, func(v: float): _regler("delay", v))
	panneau.ajouter_curseur("fade_duration (s)", 0.0, 1.0, 0.05, 0.15, func(v: float): _regler("fade_duration", v))
	panneau.ajouter_case("follow_mouse", true, func(v: bool): _regler("follow_mouse", v))
	panneau.ajouter_curseur("offset x / y", -80.0, 80.0, 2.0, 16.0, func(v: float): _regler("offset", Vector2(v, v)))
	panneau.ajouter_case("panel_style doré", false, func(v: bool): _regler("panel_style", _style if v else null))
	panneau.ajouter_bouton("show_tooltip() sur la case 1", func(): _infobulles[0].show_tooltip())
	_maj_code()


func _regler(propriete: String, valeur) -> void:
	for infobulle in _infobulles:
		infobulle.set(propriete, valeur)
	_maj_code()


func _jouer() -> void:
	_infobulles[0].show_tooltip()


func _maj_code() -> void:
	var t := _infobulles[0] if not _infobulles.is_empty() else null
	afficher_code("infos.title = \"%s\"\ninfos.text = \"%s\"\ninfos.max_width = %s\n$Case/CustomTooltip.content = infos\n$Case/CustomTooltip.delay = %s · fade_duration = %s · follow_mouse = %s" % [
		_contenu.title, _contenu.text, _contenu.max_width, snappedf(t.delay, 0.01) if t else 0.5, snappedf(t.fade_duration, 0.01) if t else 0.15, t.follow_mouse if t else true])
