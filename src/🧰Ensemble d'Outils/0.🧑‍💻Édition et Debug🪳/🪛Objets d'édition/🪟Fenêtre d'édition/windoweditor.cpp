#include "windoweditor.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/tween.hpp>

using namespace godot;

WindowEditor::WindowEditor() {}

WindowEditor::~WindowEditor()
{
	// Garde-fou : si la Resource est libérée pendant qu'une fenêtre est ouverte,
	// on évite de laisser un Window orphelin dans l'arbre.
	if (_viewport_window != nullptr)
	{
		_viewport_window->disconnect("close_requested", Callable(this, "close_window"));

		close_window();
	}
}

// ---------------------------------------------------------------------
// Getters / Setters
// ---------------------------------------------------------------------

void WindowEditor::set_prefab_window(const Ref<PackedScene> &p_scene) { prefab_window = p_scene; }
Ref<PackedScene> WindowEditor::get_prefab_window() const { return prefab_window; }

void WindowEditor::set_window_title(const String &p_title) { window_title = p_title; }
String WindowEditor::get_window_title() const { return window_title; }

void WindowEditor::set_window_position_on_creation(const Vector2i &p_pos) { window_position_on_creation = p_pos; }
Vector2i WindowEditor::get_window_position_on_creation() const { return window_position_on_creation; }

void WindowEditor::set_window_size_on_creation(const Vector2i &p_size) { window_size_on_creation = p_size; }
Vector2i WindowEditor::get_window_size_on_creation() const { return window_size_on_creation; }

void WindowEditor::set_always_on_top(bool p_value) { always_on_top = p_value; }
bool WindowEditor::get_always_on_top() const { return always_on_top; }

void WindowEditor::set_transparent_bg(bool p_value) { transparent_bg = p_value; }
bool WindowEditor::get_transparent_bg() const { return transparent_bg; }

void WindowEditor::set_own_world_3d(bool p_value) { own_world_3d = p_value; }
bool WindowEditor::get_own_world_3d() const { return own_world_3d; }

bool WindowEditor::get_is_window_opened() const { return is_window_opened; }

// ---------------------------------------------------------------------
// Cycle de vie
// ---------------------------------------------------------------------

void WindowEditor::open_window()
{
	if (is_window_opened)
	{
		// Garde : une seule fenêtre à la fois par instance de WindowEditor.
		return;
	}

	_viewport_window = memnew(Window);
	_viewport_window->set_title(window_title);
	_viewport_window->set_position(window_position_on_creation);
	_viewport_window->set_size(window_size_on_creation);
	_viewport_window->set_flag(Window::FLAG_ALWAYS_ON_TOP, always_on_top);
	_viewport_window->set_use_own_world_3d(own_world_3d);
	_viewport_window->set_transparent_background(transparent_bg);

	if (prefab_window.is_valid())
	{
		// Instancie le contenu et l'injecte dans la Window avant de l'ajouter à l'arbre.
		Node *scene_instance = prefab_window->instantiate();
		_viewport_window->add_child(scene_instance);
	}

	// Rattache la Window au root pour qu'elle soit visible hors de la scène active.
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	if (tree != nullptr)
	{
		Node *root = tree->get_root();
		if (root != nullptr)
		{
			root->add_child(_viewport_window);
		}
	}

	// Rattache la Window au root pour qu'elle soit visible hors de la scène active.
	// SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	// if (tree != nullptr)
	//{
	//	tree->get_root()->add_child(_viewport_window);
	//		Ref<Tween> tween = tree->create_tween();
	//		if (tween.is_valid())
	//		{
	//			tween->set_parallel(true);
	//		}
	//}

	// Écoute la croix de fermeture native de l'OS pour synchroniser l'état interne.
	_viewport_window->connect("close_requested", Callable(this, "close_window"));

	_on_open_window(); // Point d'extension pour les sous-classes ou les surcharges.
	emit_signal("open_requested");
	is_window_opened = true;
}

void WindowEditor::close_window()
{
	if (_viewport_window != nullptr)
	{
		// queue_free : destruction sûre en fin de frame, évite les erreurs si appelée depuis un signal.
		memdelete(_viewport_window);
		_viewport_window = nullptr;
	}
	is_window_opened = false;
}

void WindowEditor::_on_open_window()
{
	// Corps intentionnellement vide — surcharge en C++ (virtual override)
	// ou détourne via une classe fille GDScript qui hérite de WindowEditor.
}

Callable WindowEditor::get_open_window_callable() const
{
	// PROPERTY_HINT_TOOL_BUTTON exige un getter qui renvoie un Callable,
	// pas la méthode void elle-même. C'est ce Callable que l'Inspecteur invoque au clic.
	return Callable(const_cast<WindowEditor *>(this), "open_window");
}

// ---------------------------------------------------------------------
// Bind
// ---------------------------------------------------------------------

void WindowEditor::_bind_methods()
{
	// --- Méthodes publiques ---
	ClassDB::bind_method(D_METHOD("open_window"), &WindowEditor::open_window);
	ClassDB::bind_method(D_METHOD("close_window"), &WindowEditor::close_window);

	// --- Getters/Setters ---
	ClassDB::bind_method(D_METHOD("set_prefab_window", "scene"), &WindowEditor::set_prefab_window);
	ClassDB::bind_method(D_METHOD("get_prefab_window"), &WindowEditor::get_prefab_window);

	ClassDB::bind_method(D_METHOD("set_window_title", "title"), &WindowEditor::set_window_title);
	ClassDB::bind_method(D_METHOD("get_window_title"), &WindowEditor::get_window_title);

	ClassDB::bind_method(D_METHOD("set_window_position_on_creation", "pos"), &WindowEditor::set_window_position_on_creation);
	ClassDB::bind_method(D_METHOD("get_window_position_on_creation"), &WindowEditor::get_window_position_on_creation);

	ClassDB::bind_method(D_METHOD("set_window_size_on_creation", "size"), &WindowEditor::set_window_size_on_creation);
	ClassDB::bind_method(D_METHOD("get_window_size_on_creation"), &WindowEditor::get_window_size_on_creation);

	ClassDB::bind_method(D_METHOD("set_always_on_top", "value"), &WindowEditor::set_always_on_top);
	ClassDB::bind_method(D_METHOD("get_always_on_top"), &WindowEditor::get_always_on_top);

	ClassDB::bind_method(D_METHOD("set_transparent_bg", "value"), &WindowEditor::set_transparent_bg);
	ClassDB::bind_method(D_METHOD("get_transparent_bg"), &WindowEditor::get_transparent_bg);

	ClassDB::bind_method(D_METHOD("set_own_world_3d", "value"), &WindowEditor::set_own_world_3d);
	ClassDB::bind_method(D_METHOD("get_own_world_3d"), &WindowEditor::get_own_world_3d);

	ClassDB::bind_method(D_METHOD("get_is_window_opened"), &WindowEditor::get_is_window_opened);

	// --- Propriétés (@export équivalent) ---
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "prefab_window", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"),
				 "set_prefab_window", "get_prefab_window");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "window_title"), "set_window_title", "get_window_title");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "window_position_on_creation"),
				 "set_window_position_on_creation", "get_window_position_on_creation");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "window_size_on_creation"),
				 "set_window_size_on_creation", "get_window_size_on_creation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "always_on_top"), "set_always_on_top", "get_always_on_top");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transparent_bg"), "set_transparent_bg", "get_transparent_bg");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "own_world_3d"), "set_own_world_3d", "get_own_world_3d");

	// --- Bouton outil éditeur : équivalent de @export_tool_button("Ouvrir la fenêtre", "Window") ---
	// Nécessite Godot 4.4+. Le getter DOIT retourner un Callable (pas la méthode void open_window elle-même) :
	// PROPERTY_HINT_TOOL_BUTTON invoque le Callable renvoyé par le getter quand on clique le bouton.
	ClassDB::bind_method(D_METHOD("get_open_window_callable"), &WindowEditor::get_open_window_callable);
	ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "open_window_action", PROPERTY_HINT_TOOL_BUTTON,
							  "Ouvrir la fenêtre,Window"),
				 "", "get_open_window_callable");

	// --- Signal ---
	ADD_SIGNAL(MethodInfo("open_requested"));
}