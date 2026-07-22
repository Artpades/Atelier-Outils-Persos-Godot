#include "test_example.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/timer.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "godot_cpp/variant/vector2.hpp"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

#pragma region Constructeur/Destructeur
TestExample::TestExample() {}
TestExample::~TestExample() {}
#pragma endregion

#pragma region Getter/Setter
Vector2 TestExample::get_test_property() const {
  // print_line_rich("[color=red]Obtention de la valeur[/color]");
  return test_property;
}
void TestExample::set_test_property(const Vector2 &value) {
  print_line_rich("[color=green]Définition de la valeur[/color]");
  test_property = value;
}

TestEnum TestExample::get_test_enum() const { return test_enum; }
void TestExample::set_test_enum(const TestEnum value) { test_enum = value; }
#pragma endregion

#pragma region Méthodes principales
void TestExample::test_method() {}
int TestExample::test2_method(int d) { return d; }
Node *TestExample::test_print_node(Node *n) {
  if (n != nullptr) {
    // Test de cast pour savoir quel est son type
    if (Timer *timer = Object::cast_to<Timer>(n)) {
      print_line_rich(n->get_name(), " : Je suis un Minuteur");
    }
    print_line(n->get_name());
    emit_signal("emitted", n);
  }
  return n;
}
#pragma endregion

#pragma region Getter/Setter
void TestExample::_bind_methods() {

  ClassDB::bind_method(D_METHOD("test_method"), &TestExample::test_method);
  ClassDB::bind_method(D_METHOD("test2_method", "d"),
                       &TestExample::test2_method);
  ClassDB::bind_method(D_METHOD("test_print_node", "n"),
                       &TestExample::test_print_node);

  ClassDB::bind_method(D_METHOD("set_test_property", "value"),
                       &TestExample::set_test_property);
  ClassDB::bind_method(D_METHOD("get_test_property"),
                       &TestExample::get_test_property);

  ClassDB::bind_method(D_METHOD("get_test_enum"), &TestExample::get_test_enum);
  ClassDB::bind_method(D_METHOD("set_test_enum", "value"),
                       &TestExample::set_test_enum);

  // Pour assigner une propriété, il faut que les getter/setters soit
  // assignés avant aussi pour que ça fonctionne

  // Cette macro comporte une structure de définition de propriété
  ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "test_property"),
               "set_test_property", "get_test_property");

  ADD_PROPERTY(PropertyInfo(Variant::INT, "test_enum", PROPERTY_HINT_ENUM,
                            "enum_1, ENUM_2, enum_3_Yaoui_Waoui"),
               "set_test_enum", "get_test_enum");

  BIND_ENUM_CONSTANT(TestEnum::ENUM_1);
  BIND_ENUM_CONSTANT(TestEnum::ENUM_2);
  BIND_ENUM_CONSTANT(TestEnum::ENUM_3);

  ADD_SIGNAL(
      MethodInfo("emitted", PropertyInfo(Variant::OBJECT, "node_parameter")));
}
#pragma endregion
