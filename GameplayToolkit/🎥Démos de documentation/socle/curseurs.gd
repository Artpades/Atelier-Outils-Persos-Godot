extends RefCounted

## 👆 Images de curseurs en pixel art (16 × 16, agrandies × 2) pour les démos du Changeur de curseur.
## const Curseurs := preload("res://🎥Démos de documentation/socle/curseurs.gd") ; Curseurs.epee()...

const Formes := preload("res://🎥Démos de documentation/socle/formes.gd")
const PALETTE := {"n": Color("1b1f2a"), "b": Color("e8ecf4"), "g": Color("9aa4b8"), "o": Color("f2a33a"), "r": Color("ef5f5f"), "j": Color("ffd166"), "m": Color("8a5a3c")}


static func _image(lignes: Array) -> Texture2D:
	return Formes.pixel_art(PackedStringArray(lignes), PALETTE, 2)


## Flèche système (hotspot (0, 0)).
static func fleche() -> Texture2D:
	return _image(["n...........", "nn..........", "nbn.........", "nbbn........", "nbbbn.......", "nbbbbn......", "nbbbbbn.....",
		"nbbbbbbn....", "nbbbbbbbn...", "nbbbbbnnnn..", "nbbnbbn.....", "nbn.nbbn....", "nn..nbbn....", "n....nbbn...", ".....nbbn...", "......nn...."])


## Épée, pointe en haut à gauche (hotspot (2, 2)).
static func epee() -> Texture2D:
	return _image(["nn..............", "nbn.............", ".nbn............", "..nbn...........", "...nbn..........", "....nbn.........",
		".....nbn........", "......nbn.......", ".......nbn......", "........nbn.nn..", ".........nbnon..", "..........nonn..",
		"..........nonmn.", ".........nnn.nmn", "..............nn", "................"])


## Main ouverte (hotspot (6, 1)).
static func main_ouverte() -> Texture2D:
	return _image(["......nn........", ".....nbbn.nn....", ".....nbbnnbbn...", ".....nbbnbbbn.nn", ".....nbbnbbnnbbn", "..nn.nbbnbbnbbbn",
		".nbbnnbbbbbbbbn.", ".nbbbnbbbbbbbbn.", "..nbbbbbbbbbbbn.", "..nbbbbbbbbbbn..", "...nbbbbbbbbbn..", "...nbbbbbbbbn...",
		"....nbbbbbbbn...", "....nbbbbbbn....", ".....nnnnnn.....", "................"])


## Main fermée (hotspot (6, 4)).
static func main_fermee() -> Texture2D:
	return _image(["................", "................", "................", ".....nnnnnnn....", "....nbbnbbnbnn..", "....nbbnbbnbbbn.",
		"..nnnbbbbbbbbbn.", ".nbbnbbbbbbbbbn.", "..nbbbbbbbbbbbn.", "..nbbbbbbbbbbn..", "...nbbbbbbbbbn..", "...nbbbbbbbbn...",
		"....nbbbbbbbn...", "....nbbbbbbn....", ".....nnnnnn.....", "................"])


## Viseur (hotspot (8, 8)).
static func viseur(serre: bool = false) -> Texture2D:
	if serre:
		return _image(["................", "................", "................", "................", ".......rr.......", ".......rr.......",
			"................", "....rr.rr.rr....", "....rr.rr.rr....", "................", ".......rr.......", ".......rr.......",
			"................", "................", "................", "................"])
	return _image([".......rr.......", ".......rr.......", ".......rr.......", "................", "................", "................",
		"................", "rrr....rr....rrr", "rrr....rr....rrr", "................", "................", "................",
		"................", ".......rr.......", ".......rr.......", ".......rr......."])


## Sablier animé : `etape` 0 à 3 (le sable s'écoule).
static func sablier(etape: int) -> Texture2D:
	var haut: Array = [["njjjjjjjjn", ".njjjjjjn.", "..njjjjn..", "...njjn..."], ["nbbjjjjbbn", ".njjjjjjn.", "..njjjjn..", "...njjn..."],
		["nbbbbbbbbn", ".nbbjjbbn.", "..njjjjn..", "...njjn..."], ["nbbbbbbbbn", ".nbbbbbbn.", "..nbbbbn..", "...njjn..."]][etape]
	var bas: Array = [["...nbbn...", "..nbbbbn..", ".nbbbbbbn.", "nbbbbbbbbn"], ["...njbn...", "..nbbbbn..", ".nbbjjbbn.", "njjjjjjjjn"],
		["...njbn...", "..nbjjbn..", ".njjjjjjn.", "njjjjjjjjn"], ["...njjn...", "..njjjjn..", ".njjjjjjn.", "njjjjjjjjn"]][etape]
	return _image(["nnnnnnnnnn", "nmmmmmmmmn"] + haut + ["....nn....", "....nn...."] + bas + ["nmmmmmmmmn", "nnnnnnnnnn"])
