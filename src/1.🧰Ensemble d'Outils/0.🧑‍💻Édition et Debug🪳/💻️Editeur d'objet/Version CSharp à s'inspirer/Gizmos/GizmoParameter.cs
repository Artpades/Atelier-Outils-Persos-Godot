using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🧷 <b>Valeur d'un paramètre</b> d'un appel de gizmo (<see cref="GizmoEntry"/>) : soit <b>référencée</b> depuis une
	/// propriété de l'objet inspecté (relue à chaque image), soit <b>saisie manuellement</b>. Même principe que
	/// <see cref="SignalArgument"/>, appliqué aux paramètres de <see cref="Gizmos"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class GizmoParameter : Resource
	{
		/// <summary>Si vrai, la valeur vient de <see cref="TargetProperty"/> (relue à chaque image). Sinon : <see cref="Value"/>.</summary>
		[Export] public bool FromProperty { get; set; }

		/// <summary>Nom de la propriété référencée quand <see cref="FromProperty"/> est vrai.</summary>
		[Export] public string TargetProperty { get; set; } = "";

		/// <summary>Valeur saisie manuellement (ignorée si <see cref="FromProperty"/> est vrai).</summary>
		[Export] public Variant Value { get; set; }

		/// <summary>
		/// Pour un paramètre <c>Nullable</c> (<see cref="GizmoParamSpec.Nullable"/>) non référencé : si vrai, aucune
		/// valeur n'est passée (la méthode utilise son propre défaut, ex : <c>color = null → Colors.White</c>).
		/// </summary>
		[Export] public bool UseMethodDefault { get; set; } = true;
	}
}
