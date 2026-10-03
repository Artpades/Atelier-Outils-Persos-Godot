#include "custom_logger.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

// Les valeurs par défaut sont déclarées ici pour que GDScript puisse appeler L.log_it("message") seul.
void L::_bind_methods() { ClassDB::bind_static_method("L", D_METHOD("log_it", "message", "object_caller", "icon_path"), &L::log_it, DEFVAL(Variant()), DEFVAL("")); }
