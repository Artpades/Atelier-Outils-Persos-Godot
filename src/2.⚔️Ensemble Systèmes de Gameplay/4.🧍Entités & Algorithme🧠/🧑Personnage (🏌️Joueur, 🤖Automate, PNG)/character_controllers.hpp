#ifndef CHARACTER_CONTROLLERS_HPP
#define CHARACTER_CONTROLLERS_HPP

#include "character_core.hpp"
#include "interactable.hpp"

#include "../💬Système de Dialogue/dialogue_resource.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot
{
/// 🏌️ Contrôleur du joueur (enfant d'un Character2D / Character3D) : lit les actions de l'InputMap.
///
/// Déplacement (action_left/right/up/down), saut, course, interaction avec l'Interactable le plus proche,
/// et capacités : ability_actions associe une action à l'identifiant d'une capacité de l'AbilitySystem du
/// personnage (cible : l'ennemi le plus proche dans auto_target_range, si > 0). En 3D, le déplacement est
/// relatif à la caméra courante (camera_relative). Les actions absentes de l'InputMap sont créées au
/// lancement avec des touches par défaut (ZQSD/WASD + flèches, Espace, Maj, E ; manette) si
/// create_missing_actions. Les commandes sont coupées pendant un dialogue (block_during_dialogue).
class PlayerController : public Node
{
    GDCLASS(PlayerController, Node)

  public:
    PlayerController();

    void set_enabled(bool p_enabled) { enabled = p_enabled; }
    bool is_enabled() const { return enabled; }
    void set_action_left(const StringName &p_action) { action_left = p_action; }
    StringName get_action_left() const { return action_left; }
    void set_action_right(const StringName &p_action) { action_right = p_action; }
    StringName get_action_right() const { return action_right; }
    void set_action_up(const StringName &p_action) { action_up = p_action; }
    StringName get_action_up() const { return action_up; }
    void set_action_down(const StringName &p_action) { action_down = p_action; }
    StringName get_action_down() const { return action_down; }
    void set_action_jump(const StringName &p_action) { action_jump = p_action; }
    StringName get_action_jump() const { return action_jump; }
    void set_action_run(const StringName &p_action) { action_run = p_action; }
    StringName get_action_run() const { return action_run; }
    void set_action_interact(const StringName &p_action) { action_interact = p_action; }
    StringName get_action_interact() const { return action_interact; }
    void set_ability_actions(const Dictionary &p_actions) { ability_actions = p_actions; }
    Dictionary get_ability_actions() const { return ability_actions; }
    void set_auto_target_range(double p_range) { auto_target_range = MAX(0.0, p_range); }
    double get_auto_target_range() const { return auto_target_range; }
    void set_camera_relative(bool p_relative) { camera_relative = p_relative; }
    bool get_camera_relative() const { return camera_relative; }
    void set_block_during_dialogue(bool p_block) { block_during_dialogue = p_block; }
    bool get_block_during_dialogue() const { return block_during_dialogue; }
    void set_create_missing_actions(bool p_create) { create_missing_actions = p_create; }
    bool get_create_missing_actions() const { return create_missing_actions; }

    Node *get_character() const;
    Interactable *get_focused_interactable() const;
    /// Interagit avec l'Interactable le plus proche (comme l'action d'interaction).
    bool interact();
    /// Active une capacité du personnage (cible automatique selon auto_target_range).
    bool use_ability(const StringName &p_id);
    /// Ajoute à l'InputMap les actions de ce contrôleur qui manquent, avec les touches par défaut.
    void create_default_actions();

  protected:
    static void _bind_methods();
    void _notification(int p_what);

  private:
    bool enabled = true;
    StringName action_left = "move_left";
    StringName action_right = "move_right";
    StringName action_up = "move_up";
    StringName action_down = "move_down";
    StringName action_jump = "jump";
    StringName action_run = "run";
    StringName action_interact = "interact";
    Dictionary ability_actions;
    double auto_target_range = 0.0;
    bool camera_relative = true;
    bool block_during_dialogue = true;
    bool create_missing_actions = true;
    /// Interactable le plus proche à la dernière image (pour n'émettre focus_changed qu'au changement).
    ObjectID focused_id;

    /// Lit les entrées et pilote le personnage (appelé à chaque image physique).
    void _physics_update();
    /// Vrai si l'action est non vide et déclarée dans l'InputMap.
    bool _action_exists(const StringName &p_action) const;
};

/// 🤖 Contrôleur d'automate (IA) : déplace son personnage vers un point, suit un nœud, patrouille ou erre.
/// Utilise le NavigationAgent2D / NavigationAgent3D enfant du personnage s'il y en a un (use_navigation),
/// sinon va en ligne droite. Pensé pour être piloté par une StateMachine ou un BehaviorTree (BTMoveTo).
class AutomateController : public Node
{
    GDCLASS(AutomateController, Node)

  public:
    /// Mode de déplacement en cours (get_mode).
    enum Mode
    {
        MODE_IDLE,
        MODE_MOVE_TO,
        MODE_FOLLOW,
        MODE_PATROL,
        MODE_WANDER,
    };

    AutomateController();

    void set_enabled(bool p_enabled) { enabled = p_enabled; }
    bool is_enabled() const { return enabled; }
    void set_start_mode(Mode p_mode) { start_mode = p_mode; }
    Mode get_start_mode() const { return start_mode; }
    void set_arrive_distance(double p_distance) { arrive_distance = p_distance; }
    double get_arrive_distance() const { return arrive_distance; }
    /// arrive_distance, ou 8 (2D) / 0,3 (3D) si elle est négative.
    double get_effective_arrive_distance() const;
    void set_patrol_points(const Array &p_points) { patrol_points = p_points; }
    Array get_patrol_points() const { return patrol_points; }
    void set_patrol_loop(bool p_loop) { patrol_loop = p_loop; }
    bool get_patrol_loop() const { return patrol_loop; }
    void set_wait_time(double p_time) { wait_time = MAX(0.0, p_time); }
    double get_wait_time() const { return wait_time; }
    void set_wander_radius(double p_radius) { wander_radius = MAX(0.0, p_radius); }
    double get_wander_radius() const { return wander_radius; }
    void set_follow_distance(double p_distance) { follow_distance = MAX(0.0, p_distance); }
    double get_follow_distance() const { return follow_distance; }
    void set_run_distance(double p_distance) { run_distance = MAX(0.0, p_distance); }
    double get_run_distance() const { return run_distance; }
    void set_use_navigation(bool p_use) { use_navigation = p_use; }
    bool get_use_navigation() const { return use_navigation; }

    /// p_point : Vector2, Vector3 ou nœud (sa position actuelle).
    void move_to(const Variant &p_point);
    void follow(Node *p_target, double p_distance = -1.0);
    /// Sans argument : patrouille sur patrol_points.
    void patrol(const Array &p_points = Array());
    /// Erre autour de sa position (home), dans p_radius (< 0 : wander_radius).
    void wander(double p_radius = -1.0);
    void stop();

    Mode get_mode() const { return mode; }
    /// Vrai si l'entrée de déplacement du personnage n'est pas nulle.
    bool is_moving() const;
    Vector3 get_destination() const { return destination; }
    Node *get_follow_target() const;
    Node *get_character() const;
    void set_home(const Vector3 &p_home) { home = p_home; }
    Vector3 get_home() const { return home; }

  protected:
    static void _bind_methods();
    void _notification(int p_what);
    /// Vrai si le déplacement est suspendu (NPCController pendant un dialogue).
    virtual bool _is_paused() const { return false; }

    bool enabled = true;

  private:
    Mode start_mode = MODE_IDLE;
    double arrive_distance = -1.0;
    Array patrol_points;
    bool patrol_loop = true;
    double wait_time = 0.5;
    double wander_radius = 200.0;
    double follow_distance = 0.0;
    double run_distance = 0.0;
    bool use_navigation = true;

    /// Mode courant (start_mode n'est lu qu'au _ready).
    Mode mode = MODE_IDLE;
    /// But courant ; en suivi, recopié de la position de la cible à chaque image.
    Vector3 destination;
    /// Centre de l'errance : position du personnage à la première image physique, sauf set_home.
    Vector3 home;
    ObjectID follow_id;
    int patrol_index = 0;
    /// Sens de parcours de la patrouille aller-retour (+1 / -1).
    int patrol_direction = 1;
    /// Pause restante à un point de patrouille ou d'errance.
    double wait_remaining = 0.0;
    bool home_set = false;

    /// Vector2, Vector2i, Vector3 ou nœud (position actuelle) → Vector3 ; autre type → origine.
    static Vector3 _to_point(const Variant &p_point);
    /// Tire une destination uniforme dans le disque de rayon wander_radius autour de home.
    void _pick_wander_point();
    void _physics_update(double p_delta);
    /// Prochain point du chemin du NavigationAgent enfant (si use_navigation), sinon le but lui-même.
    Vector3 _next_path_point(CharacterAPI *p_character, const Vector3 &p_goal);
};

/// 🧑‍🌾 Contrôleur de personnage non joueur (PNG) : un automate qui parle. Quand le joueur interagit avec lui
/// (Interactable du personnage, créé s'il manque), il lance son dialogue sur le DialogueSystem
/// (dialogue_system_path, sinon DialogueSystem.get_main()), se tourne vers son interlocuteur et s'arrête
/// pendant la conversation.
class NPCController : public AutomateController
{
    GDCLASS(NPCController, AutomateController)

  public:
    void set_dialogue(const Ref<DialogueResource> &p_dialogue) { dialogue = p_dialogue; }
    Ref<DialogueResource> get_dialogue() const { return dialogue; }
    void set_dialogue_start_id(const StringName &p_id) { dialogue_start_id = p_id; }
    StringName get_dialogue_start_id() const { return dialogue_start_id; }
    void set_dialogue_system_path(const NodePath &p_path) { dialogue_system_path = p_path; }
    NodePath get_dialogue_system_path() const { return dialogue_system_path; }
    void set_interaction_prompt(const String &p_prompt) { interaction_prompt = p_prompt; }
    String get_interaction_prompt() const { return interaction_prompt; }
    void set_face_interlocutor(bool p_face) { face_interlocutor = p_face; }
    bool get_face_interlocutor() const { return face_interlocutor; }
    void set_stop_while_talking(bool p_stop) { stop_while_talking = p_stop; }
    bool get_stop_while_talking() const { return stop_while_talking; }

    /// Lance le dialogue (false sans système, sans dialogue ou si une conversation est en cours).
    bool talk(Node *p_interlocutor);
    bool is_talking() const { return talking; }
    Interactable *get_interactable() const;

  protected:
    static void _bind_methods();
    void _notification(int p_what);
    bool _is_paused() const override { return talking && stop_while_talking; }

  private:
    Ref<DialogueResource> dialogue;
    StringName dialogue_start_id;
    NodePath dialogue_system_path;
    String interaction_prompt = U"Parler";
    bool face_interlocutor = true;
    bool stop_while_talking = true;
    /// Vrai entre talk() et la fin du dialogue (dialogue_ended du système).
    bool talking = false;
    /// DialogueSystem de la conversation en cours, pour se déconnecter à la fin.
    ObjectID system_id;

    void _on_interacted(Node *p_by);
    void _on_dialogue_ended(const Ref<DialogueResource> &p_dialogue);
};
} // namespace godot

VARIANT_ENUM_CAST(AutomateController::Mode);

#endif // CHARACTER_CONTROLLERS_HPP
