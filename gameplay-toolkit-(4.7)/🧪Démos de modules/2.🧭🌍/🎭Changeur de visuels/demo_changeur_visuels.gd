extends Node2D

## 🎭 Démo du module changeur de visuels : VisualsSwapper, VisualsAsset, VisualsRegister.
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_visuels.gd).
## - Partie interactive : un bouton par tenue du catalogue du VisualsSwapper (swap_by_name). Les tenues
##   sont définies dans la scène : texture (Sprite2D, Polygon2D, TextureRect), mesh (MeshInstance2D) et
##   scène instanciée dans un conteneur (Chapeau).

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_visuels.gd")

@onready var panneau: Panneau = $Interface/Panneau
@onready var swapper: VisualsSwapper = $Swapper
@onready var chapeau: Node2D = $Personnage/Chapeau

var _etat: Label


func _ready() -> void:
	panneau.ajouter_section("👕 Tenues du catalogue")
	for asset in swapper.visuals_assets:
		panneau.ajouter_bouton(asset.suit_type_name.capitalize(), _changer.bind(asset.suit_type_name))
	panneau.ajouter_bouton("Nom inconnu (aucun changement)", _changer.bind("inconnu"))
	_etat = panneau.ajouter_etiquette()

	panneau.ajouter_section("🧪 Tests")
	panneau.ajouter_bouton("Relancer les tests automatiques", _lancer_tests)

	_changer(swapper.visuals_assets[0].suit_type_name)
	_lancer_tests()


func _changer(nom: String) -> void:
	swapper.swap_by_name(nom)
	panneau.journaliser("🎭 swap_by_name(\"%s\")" % nom)
	_etat.text = "Dernière tenue demandée : %s" % nom
	# L'ancien accessoire est libéré en fin de frame (queue_free)
	await get_tree().process_frame
	_etat.text += "\nAccessoire : %s" % (chapeau.get_child(0).name if chapeau.get_child_count() > 0 else "aucun")


func _lancer_tests() -> void:
	panneau.lancer_tests("Tests du module changeur de visuels", SuiteTests, self)
