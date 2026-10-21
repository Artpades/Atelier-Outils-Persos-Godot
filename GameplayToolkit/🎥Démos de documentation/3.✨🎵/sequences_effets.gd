extends "res://🎥Démos de documentation/socle/sequences.gd"

## ✨ Outils communs aux plans filmés des pages « Effets spéciaux et sensations ».

const Traceur := preload("res://🎥Démos de documentation/socle/traceur.gd")


## Ajoute à `cible` un PlayerFX qui joue `effets` (un Effect ou un tableau d'Effect).
func joueur_fx(cible: Node, effets) -> PlayerFX:
	var joueur := PlayerFX.new()
	joueur.special_effects = effets if effets is Array else [effets]
	cible.add_child(joueur)
	return joueur


## Instance de `classe` (FXShake, FXFade...) réglée par `reglages` ({propriété: valeur}).
func effet(classe: String, reglages: Dictionary = {}) -> Effect:
	var resultat: Effect = ClassDB.instantiate(classe)
	for propriete in reglages:
		resultat.set(propriete, reglages[propriete])
	return resultat


## Traceur de `mesure` (Callable -> float) centré sous `position`.
func traceur(position: Vector2, mesure: Callable, couleur: Color, echelle: float = 1.5, largeur: float = 220.0) -> Traceur:
	var trace := Traceur.new()
	trace.mesure = mesure
	trace.couleur = couleur
	trace.largeur = largeur
	trace.echelle = echelle
	trace.position = position - Vector2(largeur / 2.0, 0)
	scene.add_child(trace)
	return trace


## Colonnes comparant les `valeurs` de `propriete` d'un effet `classe` (autres réglages : `communs`) sur des
## carrés (ou les nœuds rendus par `fabrique(position, couleur) -> Node`). `mesure(cible) -> float` ajoute un
## traceur sous chaque colonne. Renvoie les PlayerFX, à jouer ensemble avec jouer_ensemble().
func colonnes(classe: String, propriete: String, valeurs: Array, textes: Array, communs: Dictionary = {},
		mesure: Callable = Callable(), echelle: float = 1.5, fabrique: Callable = Callable()) -> Array[PlayerFX]:
	var joueurs: Array[PlayerFX] = []
	var pas := TAILLE.x / (valeurs.size() + 1)
	for i in valeurs.size():
		var x := pas * (i + 1)
		var cible: Node = fabrique.call(Vector2(x, 250), VARIANTES[i % VARIANTES.size()]) if fabrique.is_valid() \
				else carre(Vector2(x, 250), VARIANTES[i % VARIANTES.size()], 80)
		etiquette(Vector2(x, 335), textes[i], 18, VARIANTES[i % VARIANTES.size()])
		if mesure.is_valid():
			traceur(Vector2(x, 430), mesure.bind(cible), VARIANTES[i % VARIANTES.size()], echelle, minf(220.0, pas - 30.0))
		var reglages := communs.duplicate()
		reglages[propriete] = valeurs[i]
		joueurs.append(joueur_fx(cible, effet(classe, reglages)))
	return joueurs


## Joue tous les `joueurs` ensemble `repetitions` fois ; `arret` > 0 : stop() après `arret` secondes.
func jouer_ensemble(joueurs: Array, repetitions: int, pause: float, arret: float = 0.0) -> void:
	for i in repetitions:
		await secondes(0.5)
		for joueur in joueurs:
			joueur.play()
		if arret > 0.0:
			await secondes(arret)
			for joueur in joueurs:
				joueur.stop()
			await secondes(maxf(pause - arret, 0.0))
		else:
			await secondes(pause)


## Écran de jeu 2D avec un petit niveau (sol, plateformes, personnage bleu) ; renvoie la vue.
func niveau(cadre: Rect2) -> SubViewport:
	var vue := vue_2d(cadre)
	var sol := Polygon2D.new()
	sol.polygon = PackedVector2Array([Vector2(-1200, 120), Vector2(1200, 120), Vector2(1200, 800), Vector2(-1200, 800)])
	sol.color = Formes.SOL
	vue.add_child(sol)
	for bloc in [Rect2(-260, 40, 120, 24), Rect2(80, -20, 160, 24), Rect2(-60, 70, 70, 50)]:
		var forme := Polygon2D.new()
		forme.polygon = PackedVector2Array([bloc.position, Vector2(bloc.end.x, bloc.position.y), bloc.end, Vector2(bloc.position.x, bloc.end.y)])
		forme.color = Formes.DECOR
		vue.add_child(forme)
	var heros := Polygon2D.new()
	heros.name = "Heros"
	heros.polygon = PackedVector2Array([Vector2(-14, -40), Vector2(14, -40), Vector2(14, 0), Vector2(-14, 0)])
	heros.color = VARIANTES[0]
	heros.position = Vector2(-180, 120)
	vue.add_child(heros)
	return vue
