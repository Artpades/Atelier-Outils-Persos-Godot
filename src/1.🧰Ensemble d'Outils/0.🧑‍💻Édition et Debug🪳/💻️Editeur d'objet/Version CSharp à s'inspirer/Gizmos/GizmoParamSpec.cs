using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📐 <b>Déclaration d'un paramètre</b> d'une méthode <c>Gizmos.DrawXxx</c>, utilisée pour générer son widget
	/// (via <see cref="ValueProxy"/> + <see cref="PropertyWidgetRegistry"/>, exactement comme une propriété normale)
	/// et pour convertir la valeur résolue avant l'appel réel.
	/// </summary>
	public readonly struct GizmoParamSpec
	{
		/// <summary>Nom du paramètre (affiché tel quel).</summary>
		public string Name { get; init; }

		/// <summary>Type Variant présenté au widget.</summary>
		public Variant.Type Type { get; init; }

		/// <summary>Indice de propriété (enum, tableau typé...).</summary>
		public PropertyHint Hint { get; init; }

		/// <summary>Chaîne d'indice associée.</summary>
		public string HintString { get; init; }

		/// <summary>Classe déclarée pour un paramètre objet (ex : <c>Texture2D</c>).</summary>
		public string ClassName { get; init; }

		/// <summary>Valeur utilisée quand le paramètre n'est ni référencé ni saisi (ou en repli sur erreur).</summary>
		public Variant Default { get; init; }

		/// <summary>
		/// Si vrai, le paramètre est <c>Nullable</c> côté C# (ex : <c>Color? color = null</c>) : « Utiliser la valeur
		/// par défaut de la méthode » (qui passe <c>null</c>) est un choix valide, distinct de saisir une valeur.
		/// </summary>
		public bool Nullable { get; init; }

		/// <summary>
		/// Si vrai (uniquement les textures), l'appel entier est <b>ignoré</b> quand ce paramètre ne peut pas être
		/// résolu, plutôt que d'utiliser <see cref="Default"/> (une texture vide n'a pas de repli sensé).
		/// </summary>
		public bool Required { get; init; }
	}
}
