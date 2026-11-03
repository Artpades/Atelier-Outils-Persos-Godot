shader_type canvas_item;

// 🌈 Exemple de shader de pinceau (DrawingBrush.material) : la teinte varie le long du dessin.
// TEXTURE contient le trait (couleur du pinceau, alpha = couverture) ; dans vertex(), VERTEX est la
// position en pixels du dessin, identique à l'écran et lors de l'application au calque.

uniform float frequence = 0.02;
uniform float saturation : hint_range(0.0, 1.0) = 0.9;

varying vec2 position_dessin;

void vertex() {
	position_dessin = VERTEX;
}

void fragment() {
	vec4 trait = texture(TEXTURE, UV);
	float teinte = (position_dessin.x + position_dessin.y) * frequence;
	vec3 arc_en_ciel = 0.5 + 0.5 * cos(6.2831 * (teinte + vec3(0.0, 0.33, 0.67)));
	COLOR = vec4(mix(trait.rgb, arc_en_ciel, saturation), trait.a);
}
