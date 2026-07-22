#pragma once

#include "godot_cpp/variant/vector2.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
enum TestEnum {
  ENUM_1,
  ENUM_2,
  ENUM_3,

};
// Test d'ajout de classe
class TestExample : public Node {
  GDCLASS(TestExample, Node)
private:
  Vector2 test_property;

  TestEnum test_enum;

protected:
  static void _bind_methods();

public:
  TestExample();
  ~TestExample();

  Vector2 get_test_property() const;
  void set_test_property(const Vector2 &value);

  TestEnum get_test_enum() const;
  void set_test_enum(const TestEnum value);

  void test_method();
  int test2_method(int d);
  Node *test_print_node(Node *n);
};
} // namespace godot

// IMPORTANT : Cette macro doit être appelée une seule fois, en dehors de la
VARIANT_ENUM_CAST(TestEnum);