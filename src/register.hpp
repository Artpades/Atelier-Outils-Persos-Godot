#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void creer_bouton_editeur();

void initialize(ModuleInitializationLevel p_level);
void uninitialize(ModuleInitializationLevel p_level);

#endif