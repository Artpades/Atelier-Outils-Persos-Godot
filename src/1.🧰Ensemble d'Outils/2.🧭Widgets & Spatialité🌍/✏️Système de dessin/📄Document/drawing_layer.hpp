#ifndef DRAWING_LAYER_HPP
#define DRAWING_LAYER_HPP

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace godot
{
/// 🗂️ Calque matriciel d'un DrawingDocument : pixels RGBA8 (alpha non prémultiplié) et réglages de fusion.
///
/// Les réglages (visibilité, opacité, mode de fusion, verrous, écrêtage) se modifient directement, ou par
/// DrawingDocument.set_layer_property pour qu'ils soient annulables. Les pixels se modifient par les outils du
/// document (pinceau, remplissage...) ; set_image remplace tout le contenu.
class DrawingLayer : public Resource
{
    GDCLASS(DrawingLayer, Resource)

  public:
    /// Modes de fusion avec les calques inférieurs (formules du W3C, comme dans Clip Studio Paint).
    enum LayerBlend
    {
        LAYER_BLEND_NORMAL,
        LAYER_BLEND_MULTIPLY,    // produit
        LAYER_BLEND_SCREEN,      // superposition claire (« Écran »)
        LAYER_BLEND_OVERLAY,     // incrustation
        LAYER_BLEND_DARKEN,      // obscurcir
        LAYER_BLEND_LIGHTEN,     // éclaircir
        LAYER_BLEND_COLOR_DODGE, // densité couleur -
        LAYER_BLEND_COLOR_BURN,  // densité couleur +
        LAYER_BLEND_ADD,         // ajout
        LAYER_BLEND_SUBTRACT,    // soustraction
        LAYER_BLEND_DIFFERENCE,  // différence
        LAYER_BLEND_SOFT_LIGHT,  // lumière tamisée
        LAYER_BLEND_HARD_LIGHT,  // lumière crue
        LAYER_BLEND_MAX,
    };

    /// Calque vide (transparent) de la taille donnée.
    static Ref<DrawingLayer> create(const Vector2i &p_size, const String &p_name = String());

    void set_layer_name(const String &p_name);
    String get_layer_name() const { return layer_name; }
    void set_visible(bool p_visible);
    bool is_visible() const { return visible; }
    void set_opacity(float p_opacity);
    float get_opacity() const { return opacity; }
    void set_blend_mode(LayerBlend p_mode);
    LayerBlend get_blend_mode() const { return blend_mode; }
    void set_alpha_locked(bool p_locked);
    bool is_alpha_locked() const { return alpha_locked; }
    void set_clipped(bool p_clipped);
    bool is_clipped() const { return clipped; }
    void set_locked(bool p_locked);
    bool is_locked() const { return locked; }

    /// Copie du contenu (RGBA8).
    Ref<Image> get_image() const;
    /// Remplace le contenu (converti en RGBA8). La taille du calque devient celle de l'image.
    void set_image(const Ref<Image> &p_image);
    Vector2i get_size() const { return Vector2i(width, height); }
    Color get_pixel(int p_x, int p_y) const;
    /// Vrai si tous les pixels sont transparents.
    bool is_empty() const;

    // Accès direct (C++) : 4 octets par pixel, ligne par ligne.
    const uint8_t *ptr() const { return data.ptr(); }
    uint8_t *ptrw() { return data.ptrw(); }
    const PackedByteArray &get_data() const { return data; }
    /// Remplace les données brutes (copie partagée, sans conversion) : taille attendue = width × height × 4.
    void set_data(int p_width, int p_height, const PackedByteArray &p_data);
    /// Copie d'un rectangle (4 octets par pixel).
    PackedByteArray get_region(const Rect2i &p_rect) const;
    void set_region(const Rect2i &p_rect, const PackedByteArray &p_bytes);
    /// Recadre ou agrandit (pixels transparents) le calque, ancré en haut à gauche.
    void resize(const Vector2i &p_size);

    /// Pixels encodés en PNG : forme sauvegardée du calque (fichiers compacts, .tres comme .res).
    PackedByteArray get_png_data() const;
    void set_png_data(const PackedByteArray &p_png);

  protected:
    static void _bind_methods();

  private:
    String layer_name;
    bool visible = true;
    float opacity = 1.0f;
    LayerBlend blend_mode = LAYER_BLEND_NORMAL;
    bool alpha_locked = false;
    bool clipped = false;
    bool locked = false;

    int width = 0;
    int height = 0;
    PackedByteArray data;

    // Dernier encodage PNG et les pixels encodés (copie partagée) : réencodé seulement si les pixels ont changé
    // (toute modification de data en fait une copie, donc change son adresse).
    mutable PackedByteArray png_cache;
    mutable PackedByteArray png_cache_source;
};
} // namespace godot

VARIANT_ENUM_CAST(DrawingLayer::LayerBlend);

#endif // DRAWING_LAYER_HPP
