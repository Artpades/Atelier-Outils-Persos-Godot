extends "../suite_tests_base.gd"

## Suite de tests automatiques du module changeur de visuels : VisualsRegister, VisualsAsset, VisualsSwapper.
##
## Chaque type de nœud récepteur est testé (texture, mesh ou scène instanciée dans un conteneur), ainsi
## que la sauvegarde / relecture d'un VisualsAsset en .tres (dossier DOSSIER_TESTS, vidé ensuite).

const DOSSIER_TESTS := "user://tests_visuels/"

var _texture_a := _texture(Color.RED)
var _texture_b := _texture(Color.BLUE)
var _mesh_a := QuadMesh.new()
var _mesh_b := SphereMesh.new()


func _executer() -> void:
	_tester_valeurs_par_defaut()
	await _tester_noeuds_a_texture()
	await _tester_noeuds_a_mesh()
	await _tester_ressource_absente()
	await _tester_conteneur()
	await _tester_entrees_invalides()
	await _tester_apply()
	await _tester_swapper()
	_tester_sauvegarde()


# 📋 Ressources

func _tester_valeurs_par_defaut() -> void:
	var registre := VisualsRegister.new()
	_verifier("VisualsRegister : vide par défaut",
		registre.node_visual.is_empty() and registre.texture == null and registre.mesh == null and registre.modeling_import == null)
	var asset := VisualsAsset.new()
	_verifier("VisualsAsset : vide par défaut", asset.suit_type_name == "" and asset.visuals_registers.is_empty())
	var swapper := VisualsSwapper.new()
	_verifier("VisualsSwapper : catalogue vide par défaut", swapper.visuals_assets.is_empty())
	swapper.free()


# 🖼️ Types de nœuds

func _tester_noeuds_a_texture() -> void:
	for classe in ["Sprite2D", "Sprite3D", "Polygon2D", "TextureRect"]:
		var noeud: Node = _ajouter(ClassDB.instantiate(classe))
		noeud.name = "Cible"
		noeud.texture = _texture_a
		VisualsAsset.swap_visuals(_bac_a_sable, _registre("Cible", _texture_b))
		_verifier("swap_visuals : texture d'un %s remplacée" % classe, noeud.texture == _texture_b)
		noeud.free()
	await _frames()


func _tester_noeuds_a_mesh() -> void:
	for classe in ["MeshInstance3D", "MeshInstance2D"]:
		var noeud: Node = _ajouter(ClassDB.instantiate(classe))
		noeud.name = "Cible"
		noeud.mesh = _mesh_a
		VisualsAsset.swap_visuals(_bac_a_sable, _registre("Cible", null, _mesh_b))
		_verifier("swap_visuals : mesh d'un %s remplacé" % classe, noeud.mesh == _mesh_b)
		noeud.free()
	await _frames()


func _tester_ressource_absente() -> void:
	var sprite: Sprite2D = _ajouter(Sprite2D.new())
	sprite.name = "Cible"
	sprite.texture = _texture_a
	VisualsAsset.swap_visuals(_bac_a_sable, _registre("Cible", null, _mesh_b))
	_verifier("swap_visuals : sans texture dans le registre, la texture actuelle est conservée", sprite.texture == _texture_a)

	var scene := _scene_accessoire("X")
	VisualsAsset.swap_visuals(_bac_a_sable, _registre("Cible", null, null, scene))
	_verifier("swap_visuals : un Sprite2D ne reçoit pas de scène (pas d'enfant ajouté)", sprite.get_child_count() == 0 and sprite.texture == _texture_a)
	sprite.free()
	await _frames()


func _tester_conteneur() -> void:
	var conteneur: Node2D = _ajouter(Node2D.new())
	conteneur.name = "Chapeau"
	var ancien_1: Node = _ajouter(Node2D.new(), conteneur)
	var ancien_2: Node = _ajouter(Node2D.new(), conteneur)

	VisualsAsset.swap_visuals(_bac_a_sable, _registre("Chapeau", _texture_b, _mesh_b))
	_verifier("swap_visuals : conteneur sans scène dans le registre → contenu inchangé", conteneur.get_child_count() == 2)

	VisualsAsset.swap_visuals(_bac_a_sable, _registre("Chapeau", null, null, _scene_accessoire("feu")))
	_verifier("swap_visuals : anciens enfants du conteneur libérés", ancien_1.is_queued_for_deletion() and ancien_2.is_queued_for_deletion())
	await _frames()
	_verifier("swap_visuals : la scène est instanciée dans le conteneur",
		conteneur.get_child_count() == 1 and conteneur.get_child(0).get_meta("id", "") == "feu")

	VisualsAsset.swap_visuals(_bac_a_sable, _registre("Chapeau", null, null, _scene_accessoire("glace")))
	await _frames()
	_verifier("swap_visuals : un second swap remplace l'accessoire précédent",
		conteneur.get_child_count() == 1 and conteneur.get_child(0).get_meta("id", "") == "glace")
	conteneur.free()
	await _frames()


func _tester_entrees_invalides() -> void:
	VisualsAsset.swap_visuals(_bac_a_sable, _registre("Inexistant", _texture_b))
	VisualsAsset.swap_visuals(_bac_a_sable, _registre("", _texture_b))
	VisualsAsset.swap_visuals(_bac_a_sable, null)
	VisualsAsset.swap_visuals(null, _registre("Cible", _texture_b))
	_verifier("swap_visuals : chemin introuvable / vide, registre ou contexte nul → ignorés sans plantage", true)
	await _frames()


func _tester_apply() -> void:
	var sprite: Sprite2D = _ajouter(Sprite2D.new())
	sprite.name = "Corps"
	var maillage: MeshInstance2D = _ajouter(MeshInstance2D.new())
	maillage.name = "Aura"
	var asset := _asset("feu", [_registre("Corps", _texture_b), _registre("Aura", null, _mesh_b), _registre("Inexistant", _texture_a)])

	asset.apply(_bac_a_sable)
	_verifier("VisualsAsset.apply : tous les registres appliqués (un chemin invalide n'arrête pas les autres)",
		sprite.texture == _texture_b and maillage.mesh == _mesh_b)
	sprite.free()
	maillage.free()
	await _frames()


func _tester_swapper() -> void:
	# Les chemins sont résolus depuis le VisualsSwapper : ici, ses enfants.
	var swapper: VisualsSwapper = _ajouter(VisualsSwapper.new())
	var sprite: Sprite2D = _ajouter(Sprite2D.new(), swapper)
	sprite.name = "Corps"
	sprite.texture = _texture_a

	var feu := _asset("feu", [_registre("Corps", _texture_b)])
	var glace := _asset("glace", [_registre("Corps", _texture(Color.CYAN))])
	var feu_bis := _asset("feu", [_registre("Corps", _texture(Color.GREEN))])
	swapper.visuals_assets = [glace, feu, feu_bis]

	swapper.swap(feu)
	_verifier("VisualsSwapper.swap : asset appliqué, chemins relatifs au swapper", sprite.texture == _texture_b)

	swapper.swap_by_name("glace")
	_verifier("VisualsSwapper.swap_by_name : asset trouvé par suit_type_name", sprite.texture == glace.visuals_registers[0].texture)

	swapper.swap_by_name("feu")
	_verifier("VisualsSwapper.swap_by_name : le premier asset de ce nom est appliqué", sprite.texture == _texture_b)

	swapper.swap_by_name("inconnu")
	swapper.swap(null)
	_verifier("VisualsSwapper : nom inconnu ou asset nul → aucun changement", sprite.texture == _texture_b)

	swapper.visuals_assets = [null, feu]
	swapper.swap_by_name("feu")
	_verifier("VisualsSwapper : un asset nul dans le catalogue est ignoré", sprite.texture == _texture_b)
	swapper.free()
	await _frames()


# 💾 Sérialisation

func _tester_sauvegarde() -> void:
	DirAccess.make_dir_recursive_absolute(DOSSIER_TESTS)
	var chemin := DOSSIER_TESTS + "asset.tres"
	var asset := _asset("feu", [_registre("Corps", _texture_b, _mesh_b, _scene_accessoire("feu"))])

	var erreur := ResourceSaver.save(asset, chemin)
	_verifier("VisualsAsset : sauvegarde en .tres", erreur == OK, error_string(erreur))

	var relu: VisualsAsset = ResourceLoader.load(chemin, "", ResourceLoader.CACHE_MODE_IGNORE)
	var registre: VisualsRegister = relu.visuals_registers[0] if relu and relu.visuals_registers.size() == 1 else null
	_verifier("VisualsAsset : relecture (nom et registres)", relu != null and relu.suit_type_name == "feu" and registre != null)
	if registre:
		_verifier("VisualsRegister : relecture (chemin, texture, mesh, scène)",
			registre.node_visual == NodePath("Corps") and registre.texture is GradientTexture2D and registre.mesh is SphereMesh and registre.modeling_import is PackedScene)

	DirAccess.remove_absolute(chemin)
	DirAccess.remove_absolute(DOSSIER_TESTS)


# 🛠️ Utilitaires

func _registre(chemin: String, texture: Texture2D = null, mesh: Mesh = null, scene: PackedScene = null) -> VisualsRegister:
	var registre := VisualsRegister.new()
	registre.node_visual = NodePath(chemin)
	registre.texture = texture
	registre.mesh = mesh
	registre.modeling_import = scene
	return registre


func _asset(nom: String, registres: Array) -> VisualsAsset:
	var asset := VisualsAsset.new()
	asset.suit_type_name = nom
	asset.visuals_registers = registres
	return asset


func _scene_accessoire(id: String) -> PackedScene:
	var racine := Node2D.new()
	racine.set_meta("id", id)
	return _empaqueter(racine)


static func _texture(couleur: Color) -> GradientTexture2D:
	var degrade := Gradient.new()
	degrade.colors = PackedColorArray([couleur, couleur.darkened(0.5)])
	var texture := GradientTexture2D.new()
	texture.gradient = degrade
	return texture
