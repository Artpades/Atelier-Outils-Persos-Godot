#ifndef WIDGET_PROPERTY_MODIFIER_BUILDER_HPP
#define WIDGET_PROPERTY_MODIFIER_BUILDER_HPP

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/container.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/viewport.hpp>

namespace godot
{
class WidgetPropertyModifierBuilder : public RefCounted
{
    GDCLASS(WidgetPropertyModifierBuilder, RefCounted)
  private:
  protected:
    static void _bind_methods();

  public:
    WidgetPropertyModifierBuilder();
    ~WidgetPropertyModifierBuilder();

    static BoxContainer *create_visual_base();
    static Label *create_label_name_widget(Control *base_visual, const String &name);

    // Type de widget
    // Booléen : CheckBox, CheckButton
    // Décimaux : SpinBox, Slider V/H, ProgresBar, TextureProgressBar

    static CheckBox *create_bool_widget(Object *obj, const String &property_name, bool vertical = false, Control *control_for_adding_widget = nullptr);
};
} // namespace godot
#endif // WIDGET_PROPERTY_MODIFIER_BUILDER