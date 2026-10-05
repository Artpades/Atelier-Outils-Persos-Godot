#include "object_editor_demo.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace
{
// Description d'une propriété de démonstration (tableau constant : aucune destruction statique).
struct DemoProperty
{
    const char *name;
    Variant::Type type;
    PropertyHint hint;
    const char *hint_string;
    uint32_t usage;
};

const uint32_t DEFAULT = PROPERTY_USAGE_DEFAULT;
const uint32_t READ_ONLY = PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY;

const DemoProperty DEMO_PROPERTIES[] = {
    {"booleen", Variant::BOOL, PROPERTY_HINT_NONE, "", DEFAULT},
    {"entier", Variant::INT, PROPERTY_HINT_NONE, "", DEFAULT},
    {"decimal_borne", Variant::FLOAT, PROPERTY_HINT_RANGE, "0,10,0.5,or_greater,suffix:m", DEFAULT},
    {"valeur_bornee", Variant::FLOAT, PROPERTY_HINT_NONE, "", DEFAULT},
    {"lecture_seule", Variant::INT, PROPERTY_HINT_NONE, "", READ_ONLY},
    {"compteur", Variant::INT, PROPERTY_HINT_NONE, "", DEFAULT},
    {"auto_increment", Variant::BOOL, PROPERTY_HINT_NONE, "", DEFAULT},
    {"enum_entier", Variant::INT, PROPERTY_HINT_ENUM, "Rouge,Vert:5,Bleu", DEFAULT},
    {"enum_texte", Variant::STRING, PROPERTY_HINT_ENUM, "Petit,Moyen,Grand", DEFAULT},
    {"drapeaux", Variant::INT, PROPERTY_HINT_FLAGS, "Feu,Eau,Terre,Air", DEFAULT},
    {"texte", Variant::STRING, PROPERTY_HINT_NONE, "", DEFAULT},
    {"texte_multiligne", Variant::STRING, PROPERTY_HINT_MULTILINE_TEXT, "", DEFAULT},
    {"nom", Variant::STRING_NAME, PROPERTY_HINT_NONE, "", DEFAULT},
    {"couleur", Variant::COLOR, PROPERTY_HINT_NONE, "", DEFAULT},
    {"couleur_sans_alpha", Variant::COLOR, PROPERTY_HINT_COLOR_NO_ALPHA, "", DEFAULT},
    {"vecteur2", Variant::VECTOR2, PROPERTY_HINT_NONE, "", DEFAULT},
    {"vecteur2i", Variant::VECTOR2I, PROPERTY_HINT_NONE, "", DEFAULT},
    {"vecteur3", Variant::VECTOR3, PROPERTY_HINT_NONE, "", DEFAULT},
    {"vecteur3i", Variant::VECTOR3I, PROPERTY_HINT_NONE, "", DEFAULT},
    {"vecteur4", Variant::VECTOR4, PROPERTY_HINT_NONE, "", DEFAULT},
    {"vecteur4i", Variant::VECTOR4I, PROPERTY_HINT_NONE, "", DEFAULT},
    {"rect2", Variant::RECT2, PROPERTY_HINT_NONE, "", DEFAULT},
    {"rect2i", Variant::RECT2I, PROPERTY_HINT_NONE, "", DEFAULT},
    {"aabb", Variant::AABB, PROPERTY_HINT_NONE, "", DEFAULT},
    {"plan", Variant::PLANE, PROPERTY_HINT_NONE, "", DEFAULT},
    {"quaternion", Variant::QUATERNION, PROPERTY_HINT_NONE, "", DEFAULT},
    {"base", Variant::BASIS, PROPERTY_HINT_NONE, "", DEFAULT},
    {"transform2d", Variant::TRANSFORM2D, PROPERTY_HINT_NONE, "", DEFAULT},
    {"transform3d", Variant::TRANSFORM3D, PROPERTY_HINT_NONE, "", DEFAULT},
    {"projection", Variant::PROJECTION, PROPERTY_HINT_NONE, "", DEFAULT},
    {"rid", Variant::RID, PROPERTY_HINT_NONE, "", READ_ONLY},
    {"dernier_appel", Variant::STRING, PROPERTY_HINT_NONE, "", DEFAULT},
    // Phase 2 : groupes de l'inspecteur (équivalents de @export_group / @export_subgroup).
    {"Collections", Variant::NIL, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP},
    {"tableau_entiers", Variant::ARRAY, PROPERTY_HINT_TYPE_STRING, "2:", DEFAULT},
    {"tableau_libre", Variant::ARRAY, PROPERTY_HINT_NONE, "", DEFAULT},
    {"dictionnaire", Variant::DICTIONARY, PROPERTY_HINT_TYPE_STRING, "4:;2:", DEFAULT},
    {"vecteurs", Variant::PACKED_VECTOR3_ARRAY, PROPERTY_HINT_NONE, "", DEFAULT},
    {"Objets", Variant::NIL, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP},
    {"ressource", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "ObjectEditorDemoResource", DEFAULT},
    {"noeud", Variant::OBJECT, PROPERTY_HINT_NODE_TYPE, "Node", DEFAULT},
    {"Chemins", Variant::NIL, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_SUBGROUP},
    {"chemin", Variant::NODE_PATH, PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node", DEFAULT},
    {"variant", Variant::NIL, PROPERTY_HINT_NONE, "", DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT},
};

const DemoProperty *find_demo_property(const StringName &p_name)
{
    for (const DemoProperty &property : DEMO_PROPERTIES)
    {
        if ((property.usage & (PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP)) == 0 && p_name == StringName(property.name))
        {
            return &property;
        }
    }
    return nullptr;
}
} // namespace

// --- ObjectEditorDemoResource ---

void ObjectEditorDemoResource::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_transform", "transform"), &ObjectEditorDemoResource::set_transform);
    ClassDB::bind_method(D_METHOD("get_transform"), &ObjectEditorDemoResource::get_transform);
    ClassDB::bind_method(D_METHOD("set_valeur", "valeur"), &ObjectEditorDemoResource::set_valeur);
    ClassDB::bind_method(D_METHOD("get_valeur"), &ObjectEditorDemoResource::get_valeur);
    ClassDB::bind_method(D_METHOD("set_autre", "autre"), &ObjectEditorDemoResource::set_autre);
    ClassDB::bind_method(D_METHOD("get_autre"), &ObjectEditorDemoResource::get_autre);

    ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "transform"), "set_transform", "get_transform");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "valeur"), "set_valeur", "get_valeur");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "autre", PROPERTY_HINT_RESOURCE_TYPE, "ObjectEditorDemoResource"), "set_autre", "get_autre");
}

void ObjectEditorDemoResource::set_transform(const Transform3D &p_transform)
{
    transform = p_transform;
    emit_changed();
}
Transform3D ObjectEditorDemoResource::get_transform() const { return transform; }
void ObjectEditorDemoResource::set_valeur(int64_t p_valeur)
{
    valeur = p_valeur;
    emit_changed();
}
int64_t ObjectEditorDemoResource::get_valeur() const { return valeur; }
void ObjectEditorDemoResource::set_autre(const Ref<Resource> &p_autre)
{
    autre = p_autre;
    emit_changed();
}
Ref<Resource> ObjectEditorDemoResource::get_autre() const { return autre; }

// --- ObjectEditorDemo ---

void ObjectEditorDemo::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("increment_counter"), &ObjectEditorDemo::increment_counter);
    ClassDB::bind_method(D_METHOD("combine", "count", "text"), &ObjectEditorDemo::combine, DEFVAL(3), DEFVAL("x"));
    ClassDB::bind_method(D_METHOD("emettre", "nouvelle"), &ObjectEditorDemo::emettre);
    ClassDB::bind_method(D_METHOD("recevoir", "a", "b", "c"), &ObjectEditorDemo::recevoir);

    ADD_SIGNAL(MethodInfo("valeur_changee", PropertyInfo(Variant::INT, "nouvelle"), PropertyInfo(Variant::INT, "ancienne")));
}

void ObjectEditorDemo::emettre(int64_t p_nouvelle)
{
    int64_t ancienne = values.get("entier", 0);
    values["entier"] = p_nouvelle;
    emit_signal("valeur_changee", p_nouvelle, ancienne);
}

void ObjectEditorDemo::recevoir(int64_t p_a, const String &p_b, double p_c)
{
    values["dernier_appel"] = String::num_int64(p_a) + "|" + p_b + "|" + String::num(p_c);
}

String ObjectEditorDemo::combine(int64_t p_count, const String &p_text) const { return p_text.repeat(p_count); }

ObjectEditorDemo::ObjectEditorDemo()
{
    values["booleen"] = true;
    values["entier"] = 7;
    values["decimal_borne"] = 2.5;
    values["valeur_bornee"] = 5.0;
    values["lecture_seule"] = 42;
    values["compteur"] = 0;
    values["auto_increment"] = false;
    values["enum_entier"] = 5;
    values["enum_texte"] = "Moyen";
    values["drapeaux"] = 5; // Feu + Terre
    values["texte"] = "Bonjour";
    values["texte_multiligne"] = "Ligne 1\nLigne 2";
    values["nom"] = StringName("demo");
    values["couleur"] = Color(1.0, 0.5, 0.0, 0.8);
    values["couleur_sans_alpha"] = Color(0.2, 0.6, 1.0);
    values["vecteur2"] = Vector2(1.5, -2.0);
    values["vecteur2i"] = Vector2i(3, 4);
    values["vecteur3"] = Vector3(1, 2, 3);
    values["vecteur3i"] = Vector3i(-1, 0, 1);
    values["vecteur4"] = Vector4(1, 2, 3, 4);
    values["vecteur4i"] = Vector4i(5, 6, 7, 8);
    values["rect2"] = Rect2(0, 0, 64, 32);
    values["rect2i"] = Rect2i(1, 2, 3, 4);
    values["aabb"] = AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
    values["plan"] = Plane(Vector3(0, 1, 0), 0.5);
    values["quaternion"] = Quaternion();
    values["base"] = Basis();
    values["transform2d"] = Transform2D();
    values["transform3d"] = Transform3D();
    values["projection"] = Projection();
    values["rid"] = RID();
    values["dernier_appel"] = "";

    Array tableau_entiers(Array(), Variant::INT, StringName(), Variant()); // Array[int] typé
    tableau_entiers.push_back(1);
    tableau_entiers.push_back(2);
    tableau_entiers.push_back(3);
    values["tableau_entiers"] = tableau_entiers;
    Array tableau_libre;
    tableau_libre.push_back(1);
    tableau_libre.push_back("deux");
    tableau_libre.push_back(Vector2(3, 3));
    values["tableau_libre"] = tableau_libre;
    Dictionary dictionnaire(Dictionary(), Variant::STRING, StringName(), Variant(), Variant::INT, StringName(), Variant()); // Dictionary[String, int]
    dictionnaire["a"] = 1;
    dictionnaire["b"] = 2;
    values["dictionnaire"] = dictionnaire;
    PackedVector3Array vecteurs;
    vecteurs.push_back(Vector3(1, 2, 3));
    vecteurs.push_back(Vector3(4, 5, 6));
    values["vecteurs"] = vecteurs;
    Ref<ObjectEditorDemoResource> ressource;
    ressource.instantiate();
    values["ressource"] = ressource;
    values["noeud"] = Variant();
    values["chemin"] = NodePath();
    values["variant"] = 42;
}

bool ObjectEditorDemo::_set(const StringName &p_name, const Variant &p_value)
{
    const DemoProperty *property = find_demo_property(p_name);
    if (!property)
    {
        return false;
    }
    // Conversion au type déclaré (ex: un int écrit dans une propriété float), sauf Variant libre et objets.
    Variant value = (property->type == Variant::NIL || property->type == Variant::OBJECT) ? p_value : UtilityFunctions::type_convert(p_value, property->type);
    if (p_name == StringName("valeur_bornee"))
    {
        value = CLAMP(double(value), 0.0, 10.0); // setter qui transforme la valeur
    }
    values[String(p_name)] = value;
    return true;
}

bool ObjectEditorDemo::_get(const StringName &p_name, Variant &r_value) const
{
    if (!find_demo_property(p_name))
    {
        return false;
    }
    r_value = values.get(String(p_name), Variant());
    return true;
}

void ObjectEditorDemo::_get_property_list(List<PropertyInfo> *p_list) const
{
    for (const DemoProperty &property : DEMO_PROPERTIES)
    {
        p_list->push_back(PropertyInfo(property.type, property.name, property.hint, property.hint_string, property.usage));
    }
}

void ObjectEditorDemo::_process(double p_delta)
{
    if (!bool(values.get("auto_increment", false)))
    {
        return;
    }
    elapsed += p_delta;
    if (elapsed >= 0.5)
    {
        elapsed = 0.0;
        increment_counter();
    }
}

void ObjectEditorDemo::increment_counter() { values["compteur"] = int64_t(values.get("compteur", 0)) + 1; }
