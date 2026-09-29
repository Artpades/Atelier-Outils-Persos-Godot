#include "custom_logger.hpp"

#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;
void L::log(const String &message, const Variant &object_caller, const String &icon_path) { print_line_rich("[color=orange]" + message + "[/color]"); }

void L::_bind_methods() { ClassDB::bind_static_method("L", D_METHOD("log", "message", "object_caller", "icon_path"), &L::log); }