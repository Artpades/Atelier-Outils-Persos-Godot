extends "res://🎥Démos de documentation/socle/apercu.gd"

## 🌈 Aperçu interactif : FXFade sur un objet (couleur ou texture) et ScreenFader sur un écran de jeu
## (masques, douceur, couleur, fade_out / fade_in / fade_to).

const MASQUES := ["aucun", "volet", "iris", "losanges", "dissolution"]
const COURBES := ["LINEAR", "SINE", "QUINT", "QUART", "QUAD", "EXPO", "ELASTIC", "CUBIC", "CIRC", "BOUNCE", "BACK", "SPRING"]

var _fondu := FXFade.new()
var _joueur := PlayerFX.new()
var _objet: Sprite2D
var _fader := ScreenFader.new()
var _texture_a: Texture2D
var _texture_b: Texture2D
var _masque := 0
var _etat: Label


func titre() -> String:
	return "Fondu couleur (FXFade, ScreenFader)"


func aide() -> String:
	return "En haut : FXFade sur un objet. En bas : ScreenFader sur un écran de jeu."


func construire() -> void:
	_texture_a = Formes.dessin(Vector2i(120, 120), Color("e8ecf4"), [[Rect2i(30, 30, 16, 16), Color("1b1f2a")], [Rect2i(74, 30, 16, 16), Color("1b1f2a")]])
	_texture_b = Formes.dessin(Vector2i(120, 120), Color("c678dd"), [[Rect2i(45, 0, 30, 120), Color("ffd166")], [Rect2i(0, 45, 120, 30), Color("ffd166")]])
	_objet = Sprite2D.new()
	_objet.texture = _texture_a
	_objet.position = Vector2(CENTRE.x, 210)
	add_child(_objet)
	_fondu.color = Color.RED
	_fondu.duration = 0.4
	_joueur.special_effects = [_fondu]
	_objet.add_child(_joueur)

	var vue := vue_2d(Rect2(66, 330, 580, 290))
	vue.get_meta("camera").zoom = Vector2.ONE * 0.75
	for i in 6:
		var bloc := Polygon2D.new()
		bloc.polygon = Formes.rectangle(Vector2(90, 60 + i * 25))
		bloc.position = Vector2(-300 + i * 120, 100 - i * 12)
		bloc.color = Formes.VARIANTES[0].darkened(i * 0.08)
		vue.add_child(bloc)
	var b := Button.new()
	b.text = "Bouton du jeu"
	b.position = Vector2(-80, -120)
	b.pressed.connect(func(): journaliser("Clic reçu par le bouton du jeu"))
	vue.add_child(b)
	_fader.duration = 0.6
	vue.add_child(_fader)
	_fader.faded_out.connect(func(): journaliser("faded_out"))
	_fader.faded_in.connect(func(): journaliser("faded_in"))
	_fader.fade_finished.connect(func(p: float): journaliser("fade_finished(%.2f)" % p))
	_etat = etiquette(Vector2(66, 302), "", 14, Formes.ACCENT)

	panneau.ajouter_section("FXFade (objet)")
	panneau.ajouter_options("mode", PackedStringArray(["FADE_COLOR", "FADE_TEXTURE"]), 0, func(i: int): _fondu.mode = i; _reinitialiser())
	var couleur := ColorPickerButton.new()
	couleur.color = _fondu.color
	couleur.custom_minimum_size = Vector2(0, 28)
	couleur.color_changed.connect(func(c: Color): _fondu.color = c; _maj_code())
	panneau.ajouter(couleur)
	panneau.ajouter_curseur("duration (s)", 0.0, 2.0, 0.05, _fondu.duration, func(v: float): _fondu.duration = v; _maj_code())
	panneau.ajouter_options("transition", PackedStringArray(COURBES), Tween.TRANS_SINE, func(i: int): _fondu.transition = i; _fader.transition = i; _maj_code())
	panneau.ajouter_case("revert_on_stop", true, func(v: bool): _fondu.revert_on_stop = v; _maj_code())
	var rangee := panneau.ajouter_rangee()
	panneau.ajouter_bouton("play()", _jouer, rangee)
	panneau.ajouter_bouton("stop()", func(): _joueur.stop(), rangee)
	panneau.ajouter_bouton("Réinit.", _reinitialiser, rangee)

	panneau.ajouter_section("ScreenFader (écran)")
	panneau.ajouter_options("mask", PackedStringArray(MASQUES), 0, func(i: int): _masque = i; _fader.mask = Formes.masque(MASQUES[i]) if i > 0 else null; _maj_code())
	panneau.ajouter_curseur("mask_smoothness", 0.0, 0.6, 0.01, _fader.mask_smoothness, func(v: float): _fader.mask_smoothness = v; _maj_code())
	panneau.ajouter_curseur("duration (s)", 0.0, 2.0, 0.05, _fader.duration, func(v: float): _fader.duration = v; _maj_code())
	panneau.ajouter_case("block_input", true, func(v: bool): _fader.block_input = v; _maj_code())
	rangee = panneau.ajouter_rangee()
	panneau.ajouter_bouton("fade_out()", func(): _fader.fade_out(), rangee)
	panneau.ajouter_bouton("fade_in()", func(): _fader.fade_in(), rangee)
	panneau.ajouter_bouton("fade_to(0.6)", func(): _fader.fade_to(0.6), rangee)
	_maj_code()


func _process(_delta: float) -> void:
	_etat.text = "progress = %.2f   is_fading() = %s   is_covered() = %s" % [_fader.progress, _fader.is_fading(), _fader.is_covered()]


func _jouer() -> void:
	_fondu.texture = _texture_b
	_joueur.play()


func _reinitialiser() -> void:
	_joueur.stop()
	_objet.modulate = Color.WHITE
	_objet.texture = _texture_a
	_maj_code()


func _maj_code() -> void:
	var lignes := PackedStringArray()
	if _fondu.mode == FXFade.FADE_COLOR:
		lignes.append("fondu.color = Color(%.2f, %.2f, %.2f, %.2f)" % [_fondu.color.r, _fondu.color.g, _fondu.color.b, _fondu.color.a])
	else:
		lignes.append("fondu.mode = FXFade.FADE_TEXTURE\nfondu.texture = autre_texture")
	lignes.append("fondu.duration = %s · revert_on_stop = %s" % [snappedf(_fondu.duration, 0.01), _fondu.revert_on_stop])
	lignes.append("$Objet/PlayerFX.play()")
	lignes.append("")
	lignes.append("Fondu.mask = %s · mask_smoothness = %s" % ["null" if _masque == 0 else "masque_" + MASQUES[_masque], snappedf(_fader.mask_smoothness, 0.01)])
	lignes.append("Fondu.fade_out()  # puis fade_in()")
	afficher_code("\n".join(lignes))
