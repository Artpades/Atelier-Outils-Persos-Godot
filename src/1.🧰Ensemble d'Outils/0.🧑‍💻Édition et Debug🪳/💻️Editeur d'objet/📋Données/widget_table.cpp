#include "widget_table.hpp"

#include <godot_cpp/core/property_info.hpp>

using namespace godot;

namespace
{
using WC = DataEntry::WidgetChoice;

// Premier élément = widget par défaut du type.
const WC BOOL_CHOICES[] = {DataEntry::WIDGET_CHECK_BOX, DataEntry::WIDGET_CHECK_BUTTON, DataEntry::WIDGET_LABEL, DataEntry::WIDGET_CUSTOM};
const WC NUMBER_CHOICES[] = {DataEntry::WIDGET_SPIN_BOX,    DataEntry::WIDGET_H_SLIDER,
                             DataEntry::WIDGET_V_SLIDER,    DataEntry::WIDGET_PROGRESS_BAR,
                             DataEntry::WIDGET_TEXTURE_PROGRESS_BAR, DataEntry::WIDGET_LABEL,
                             DataEntry::WIDGET_CUSTOM};
const WC TEXT_CHOICES[] = {DataEntry::WIDGET_LINE_EDIT, DataEntry::WIDGET_TEXT_EDIT, DataEntry::WIDGET_LABEL, DataEntry::WIDGET_CUSTOM};
const WC ENUM_CHOICES[] = {DataEntry::WIDGET_OPTION_BUTTON, DataEntry::WIDGET_CHECK_BUTTON_LIST, DataEntry::WIDGET_CHECK_BOX_LIST, DataEntry::WIDGET_LABEL,
                           DataEntry::WIDGET_CUSTOM};
const WC FLAGS_CHOICES[] = {DataEntry::WIDGET_CHECK_BOX_LIST, DataEntry::WIDGET_CHECK_BUTTON_LIST, DataEntry::WIDGET_LABEL, DataEntry::WIDGET_CUSTOM};
const WC COLOR_CHOICES[] = {DataEntry::WIDGET_COLOR_PICKER_BUTTON, DataEntry::WIDGET_COLOR_RECT_CHANNELS, DataEntry::WIDGET_LABEL, DataEntry::WIDGET_CUSTOM};
const WC BUTTON_CHOICES[] = {DataEntry::WIDGET_BUTTON, DataEntry::WIDGET_CUSTOM};
// Objet : dépliage du sous-objet par défaut ; sélecteur (nœud) en option.
const WC OBJECT_CHOICES[] = {DataEntry::WIDGET_BOX_CONTAINER, DataEntry::WIDGET_GRID_CONTAINER, DataEntry::WIDGET_FLOW_CONTAINER, DataEntry::WIDGET_TAB_CONTAINER,
                             DataEntry::WIDGET_PATH_BUTTON,   DataEntry::WIDGET_LABEL,          DataEntry::WIDGET_CUSTOM};
// NodePath : sélecteur par défaut ; dépliage du nœud ciblé en option.
const WC NODE_PATH_CHOICES[] = {DataEntry::WIDGET_PATH_BUTTON,    DataEntry::WIDGET_BOX_CONTAINER, DataEntry::WIDGET_GRID_CONTAINER, DataEntry::WIDGET_FLOW_CONTAINER,
                                DataEntry::WIDGET_TAB_CONTAINER, DataEntry::WIDGET_LABEL,         DataEntry::WIDGET_CUSTOM};
const WC COLLECTION_CHOICES[] = {DataEntry::WIDGET_BOX_CONTAINER, DataEntry::WIDGET_GRID_CONTAINER, DataEntry::WIDGET_FLOW_CONTAINER, DataEntry::WIDGET_TAB_CONTAINER,
                                 DataEntry::WIDGET_LABEL,         DataEntry::WIDGET_CUSTOM};
// Array : en plus, le tableau (éléments objets ou dictionnaires : une colonne par propriété / clé).
const WC ARRAY_CHOICES[] = {DataEntry::WIDGET_BOX_CONTAINER, DataEntry::WIDGET_GRID_CONTAINER, DataEntry::WIDGET_FLOW_CONTAINER, DataEntry::WIDGET_TAB_CONTAINER,
                            DataEntry::WIDGET_TABLE,         DataEntry::WIDGET_LABEL,          DataEntry::WIDGET_CUSTOM};
const WC PREVIEW_CHOICES[] = {DataEntry::WIDGET_PREVIEW};
const WC STRUCT_CHOICES[] = {DataEntry::WIDGET_STRUCT_SPIN_BOXES, DataEntry::WIDGET_LABEL, DataEntry::WIDGET_CUSTOM};
const WC VARIANT_CHOICES[] = {DataEntry::WIDGET_VARIANT_SELECTOR, DataEntry::WIDGET_LABEL, DataEntry::WIDGET_CUSTOM};
const WC LABEL_CHOICES[] = {DataEntry::WIDGET_LABEL, DataEntry::WIDGET_CUSTOM};

template <int N> WidgetTable::Choices make_choices(const WC (&p_array)[N]) { return WidgetTable::Choices{p_array, N}; }

// Noms affichés, dans l'ordre des valeurs de DataEntry::WidgetChoice (index = valeur).
const char32_t *const CHOICE_NAMES[] = {U"Auto",
                                    U"CheckBox",
                                    U"CheckButton",
                                    U"Label",
                                    U"SpinBox",
                                    U"HSlider",
                                    U"VSlider",
                                    U"ProgressBar",
                                    U"TextureProgressBar",
                                    U"LineEdit",
                                    U"TextEdit",
                                    U"OptionButton",
                                    U"CheckButtonList",
                                    U"ColorPickerButton",
                                    U"ColorRectChannels",
                                    U"Button",
                                    U"BoxContainer",
                                    U"GridContainer",
                                    U"PathButton",
                                    U"StructSpinBoxes",
                                    U"Custom",
                                    U"VariantSelector",
                                    U"CheckBoxList",
                                    U"FlowContainer",
                                    U"TabContainer",
                                    U"Tableau",
                                    U"Aperçu"};

// Noms affichés, dans l'ordre des valeurs de DataEntry::DataKind (index = valeur).
const char32_t *const KIND_NAMES[] = {U"Non pris en charge", U"Bool",      U"Nombre",     U"Texte",     U"Enum",   U"Flags",   U"Couleur", U"Méthode", U"Signal",
                                  U"Objet",              U"NodePath", U"Array",      U"Dictionary", U"Structure", U"Variant", U"RID",     U"PackedArray", U"Aperçu"};

String make_hint(const char32_t *const *p_names, int p_count)
{
    String hint;
    for (int i = 0; i < p_count; i++)
    {
        if (i > 0)
        {
            hint += ",";
        }
        hint += String(p_names[i]) + ":" + String::num_int64(i);
    }
    return hint;
}
} // namespace

WidgetTable::Choices WidgetTable::allowed_for(DataEntry::DataKind p_kind)
{
    switch (p_kind)
    {
    case DataEntry::KIND_BOOL:
        return make_choices(BOOL_CHOICES);
    case DataEntry::KIND_NUMBER:
        return make_choices(NUMBER_CHOICES);
    case DataEntry::KIND_TEXT:
        return make_choices(TEXT_CHOICES);
    case DataEntry::KIND_ENUM:
        return make_choices(ENUM_CHOICES);
    case DataEntry::KIND_FLAGS:
        return make_choices(FLAGS_CHOICES);
    case DataEntry::KIND_COLOR:
        return make_choices(COLOR_CHOICES);
    case DataEntry::KIND_METHOD:
    case DataEntry::KIND_SIGNAL:
        return make_choices(BUTTON_CHOICES);
    case DataEntry::KIND_OBJECT:
        return make_choices(OBJECT_CHOICES);
    case DataEntry::KIND_NODE_PATH:
        return make_choices(NODE_PATH_CHOICES);
    case DataEntry::KIND_ARRAY:
        return make_choices(ARRAY_CHOICES);
    case DataEntry::KIND_PREVIEW:
        return make_choices(PREVIEW_CHOICES);
    case DataEntry::KIND_DICTIONARY:
    case DataEntry::KIND_PACKED_ARRAY:
        return make_choices(COLLECTION_CHOICES);
    case DataEntry::KIND_STRUCT:
        return make_choices(STRUCT_CHOICES);
    case DataEntry::KIND_VARIANT:
        return make_choices(VARIANT_CHOICES);
    case DataEntry::KIND_RID:
    case DataEntry::KIND_UNSUPPORTED:
    default:
        return make_choices(LABEL_CHOICES);
    }
}

DataEntry::WidgetChoice WidgetTable::default_for(DataEntry::DataKind p_kind) { return allowed_for(p_kind).data[0]; }

bool WidgetTable::is_allowed(DataEntry::DataKind p_kind, DataEntry::WidgetChoice p_choice)
{
    if (p_choice == DataEntry::WIDGET_AUTO)
    {
        return true;
    }
    Choices choices = allowed_for(p_kind);
    for (int i = 0; i < choices.size; i++)
    {
        if (choices.data[i] == p_choice)
        {
            return true;
        }
    }
    return false;
}

DataEntry::DataKind WidgetTable::kind_of(const Dictionary &p_info, DataEntry::MemberKind p_member)
{
    if (p_member == DataEntry::MEMBER_METHOD)
    {
        return DataEntry::KIND_METHOD;
    }
    if (p_member == DataEntry::MEMBER_PREVIEW)
    {
        return DataEntry::KIND_PREVIEW;
    }
    if (p_member == DataEntry::MEMBER_SIGNAL)
    {
        return DataEntry::KIND_SIGNAL;
    }

    Variant::Type type = (Variant::Type)(int)p_info.get("type", 0);
    int hint = p_info.get("hint", 0);
    int usage = p_info.get("usage", 0);

    switch (type)
    {
    case Variant::BOOL:
        return DataEntry::KIND_BOOL;
    case Variant::INT:
        return hint == PROPERTY_HINT_ENUM ? DataEntry::KIND_ENUM : hint == PROPERTY_HINT_FLAGS ? DataEntry::KIND_FLAGS : DataEntry::KIND_NUMBER;
    case Variant::FLOAT:
        return DataEntry::KIND_NUMBER;
    case Variant::STRING:
    case Variant::STRING_NAME:
        return hint == PROPERTY_HINT_ENUM ? DataEntry::KIND_ENUM : DataEntry::KIND_TEXT;
    case Variant::COLOR:
        return DataEntry::KIND_COLOR;
    case Variant::CALLABLE:
        return DataEntry::KIND_METHOD;
    case Variant::NODE_PATH:
        return DataEntry::KIND_NODE_PATH;
    case Variant::OBJECT:
        return DataEntry::KIND_OBJECT;
    case Variant::ARRAY:
        return DataEntry::KIND_ARRAY;
    case Variant::DICTIONARY:
        return DataEntry::KIND_DICTIONARY;
    case Variant::RID:
        return DataEntry::KIND_RID;
    case Variant::NIL:
        return (usage & PROPERTY_USAGE_NIL_IS_VARIANT) ? DataEntry::KIND_VARIANT : DataEntry::KIND_UNSUPPORTED;
    default:
        if (is_packed_type(type))
        {
            return DataEntry::KIND_PACKED_ARRAY;
        }
        return is_struct_type(type) ? DataEntry::KIND_STRUCT : DataEntry::KIND_UNSUPPORTED;
    }
}

bool WidgetTable::is_container_kind(DataEntry::DataKind p_kind)
{
    return p_kind == DataEntry::KIND_OBJECT || p_kind == DataEntry::KIND_NODE_PATH || p_kind == DataEntry::KIND_ARRAY || p_kind == DataEntry::KIND_DICTIONARY ||
           p_kind == DataEntry::KIND_PACKED_ARRAY;
}

bool WidgetTable::is_struct_type(Variant::Type p_type)
{
    switch (p_type)
    {
    case Variant::VECTOR2:
    case Variant::VECTOR2I:
    case Variant::VECTOR3:
    case Variant::VECTOR3I:
    case Variant::VECTOR4:
    case Variant::VECTOR4I:
    case Variant::RECT2:
    case Variant::RECT2I:
    case Variant::AABB:
    case Variant::PLANE:
    case Variant::QUATERNION:
    case Variant::BASIS:
    case Variant::TRANSFORM2D:
    case Variant::TRANSFORM3D:
    case Variant::PROJECTION:
        return true;
    default:
        return false;
    }
}

bool WidgetTable::is_packed_type(Variant::Type p_type) { return p_type >= Variant::PACKED_BYTE_ARRAY && p_type <= Variant::PACKED_VECTOR4_ARRAY; }

String WidgetTable::choice_name(DataEntry::WidgetChoice p_choice)
{
    int count = sizeof(CHOICE_NAMES) / sizeof(CHOICE_NAMES[0]);
    return (p_choice >= 0 && p_choice < count) ? String(CHOICE_NAMES[p_choice]) : String::num_int64(p_choice);
}

String WidgetTable::kind_name(DataEntry::DataKind p_kind)
{
    int count = sizeof(KIND_NAMES) / sizeof(KIND_NAMES[0]);
    return (p_kind >= 0 && p_kind < count) ? String(KIND_NAMES[p_kind]) : String::num_int64(p_kind);
}

String WidgetTable::member_name(DataEntry::MemberKind p_member)
{
    switch (p_member)
    {
    case DataEntry::MEMBER_METHOD:
        return U"Méthode";
    case DataEntry::MEMBER_SIGNAL:
        return "Signal";
    case DataEntry::MEMBER_PREVIEW:
        return U"Aperçu";
    default:
        return U"Propriété";
    }
}

String WidgetTable::choice_hint_string() { return make_hint(CHOICE_NAMES, sizeof(CHOICE_NAMES) / sizeof(CHOICE_NAMES[0])); }

String WidgetTable::kind_hint_string() { return make_hint(KIND_NAMES, sizeof(KIND_NAMES) / sizeof(KIND_NAMES[0])); }

String WidgetTable::choice_hint_string_for(DataEntry::DataKind p_kind)
{
    String hint = "Auto (" + choice_name(default_for(p_kind)) + "):" + String::num_int64(DataEntry::WIDGET_AUTO);
    Choices choices = allowed_for(p_kind);
    for (int i = 0; i < choices.size; i++)
    {
        hint += "," + choice_name(choices.data[i]) + ":" + String::num_int64(choices.data[i]);
    }
    return hint;
}

StringName WidgetTable::settings_class_for(DataEntry::WidgetChoice p_choice)
{
    switch (p_choice)
    {
    case DataEntry::WIDGET_SPIN_BOX:
    case DataEntry::WIDGET_H_SLIDER:
    case DataEntry::WIDGET_V_SLIDER:
    case DataEntry::WIDGET_PROGRESS_BAR:
        return "RangeWidgetSettings";
    case DataEntry::WIDGET_TEXTURE_PROGRESS_BAR:
        return "TextureProgressWidgetSettings";
    case DataEntry::WIDGET_TEXT_EDIT:
        return "TextEditWidgetSettings";
    case DataEntry::WIDGET_STRUCT_SPIN_BOXES:
        return "StructWidgetSettings";
    case DataEntry::WIDGET_BOX_CONTAINER:
    case DataEntry::WIDGET_GRID_CONTAINER:
    case DataEntry::WIDGET_FLOW_CONTAINER:
    case DataEntry::WIDGET_TAB_CONTAINER:
        return "ContainerWidgetSettings";
    case DataEntry::WIDGET_TABLE:
        return "TableWidgetSettings";
    case DataEntry::WIDGET_PREVIEW:
        return "PreviewSettings";
    default:
        return "WidgetSettings";
    }
}
