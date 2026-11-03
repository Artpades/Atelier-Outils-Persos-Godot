extends Node

## 🎬 Démo du module gestionnaire de scènes : SceneManager.
##
## - Au lancement, exécute la suite de tests automatiques du module (suite_tests_gestionnaire_scenes.gd),
##   sauf le cycle complet qui changerait la scène courante.
## - Partie interactive : charger la scène de destination de façon asynchrone et suivre les signaux ;
##   la scène de destination confirme la réception de load_finished et permet de revenir ici.
##   Le SceneManager est un singleton (sous la racine) : il survit au changement de scène.

const Panneau := preload("../panneau_demo.gd")
const SuiteTests := preload("suite_tests_gestionnaire_scenes.gd")
const DESTINATION := SuiteTests.DESTINATION

@onready var panneau: Panneau = $Interface/Panneau
@onready var etat: Label = %Etat
@onready var barre: ProgressBar = %Barre

var _gestionnaire: SceneManager


func _ready() -> void:
	_gestionnaire = SceneManager.get_instance()
	# Méthodes plutôt que lambdas : connexions retirées automatiquement quand cette scène est libérée
	_gestionnaire.loading_started.connect(_on_debut)
	_gestionnaire.loading_progress.connect(_on_progression)
	_gestionnaire.load_finished.connect(_on_fin)

	panneau.ajouter_section("🚀 Chargement")
	panneau.ajouter_bouton("Charger la scène de destination", func(): _gestionnaire.load_scene_async(DESTINATION))
	panneau.ajouter_bouton("Double requête (la seconde est ignorée)", _double_requete)
	panneau.ajouter_bouton("Chemin inexistant", func(): _gestionnaire.load_scene_async("res://inexistant/aucune_scene.tscn"))
	panneau.ajouter_bouton("Ressource qui n'est pas une scène", func(): _gestionnaire.load_scene_async("res://icon.svg"))
	panneau.ajouter_case("Journal du SceneManager (log_activated)", _gestionnaire.log_activated,
		func(actif: bool): _gestionnaire.log_activated = actif)

	panneau.ajouter_section("🧪 Tests")
	panneau.ajouter_bouton("Relancer les tests automatiques", _lancer_tests)
	_lancer_tests()


func _process(_delta: float) -> void:
	etat.text = "Chargement en cours : %.0f %%" % _gestionnaire.get_progress_percent() if _gestionnaire.is_loading() else "Aucun chargement en cours"


func _double_requete() -> void:
	_gestionnaire.load_scene_async(DESTINATION)
	_gestionnaire.load_scene_async(DESTINATION)


func _on_debut() -> void:
	panneau.journaliser("📡 loading_started")


func _on_fin() -> void:
	panneau.journaliser("📡 load_finished")


func _on_progression(pourcentage: float) -> void:
	barre.value = pourcentage
	panneau.journaliser("📡 loading_progress(%.1f)" % pourcentage)


func _lancer_tests() -> void:
	panneau.lancer_tests("Tests du module gestionnaire de scènes", SuiteTests, self)
