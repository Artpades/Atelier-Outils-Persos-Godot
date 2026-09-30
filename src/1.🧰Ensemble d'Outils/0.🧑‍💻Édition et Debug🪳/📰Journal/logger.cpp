#include "logger.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void CustomLogger::_bind_methods() { ClassDB::bind_static_method("@GlobalScope", D_METHOD("log_in", "message"), &CustomLogger::log_in); }

void CustomLogger::log_in(const String &p_message) { UtilityFunctions::print_rich("[color=cornflower_blue][Journal][/color] ", p_message); }
