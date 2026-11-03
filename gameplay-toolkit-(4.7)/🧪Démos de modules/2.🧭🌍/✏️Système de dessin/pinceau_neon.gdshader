shader_type canvas_item;

// ✨ Exemple de shader de pinceau : cœur blanc et halo de la couleur du pinceau (néon).

uniform float coeur : hint_range(0.0, 1.0) = 0.75;

void fragment() {
	vec4 trait = texture(TEXTURE, UV);
	float halo = smoothstep(0.0, 1.0, trait.a);
	vec3 couleur = mix(trait.rgb, vec3(1.0), smoothstep(coeur, 1.0, trait.a));
	COLOR = vec4(couleur, halo);
}
