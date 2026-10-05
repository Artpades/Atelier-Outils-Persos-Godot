using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🎯 <b>Un appel de gizmo configuré</b> : quelle méthode de <see cref="Gizmos"/> appeler et d'où vient chacun de
	/// ses paramètres. Dessiné à chaque image par <see cref="GizmoDriver"/> tant que <see cref="Enabled"/> est vrai.
	/// </summary>
	[GlobalClass, Tool]
	public partial class GizmoEntry : Resource
	{
		/// <summary>Si faux, l'appel est ignoré (rien n'est dessiné).</summary>
		[Export] public bool Enabled { get; set; } = true;

		/// <summary>Nom de la méthode <see cref="Gizmos"/> appelée (voir <see cref="GizmoMethodCatalog"/>).</summary>
		[Export] public string GizmoMethod { get; set; } = "";

		/// <summary>Un paramètre par argument de la méthode, dans l'ordre.</summary>
		[Export] public Godot.Collections.Array<GizmoParameter> Parameters { get; set; } = new();
	}
}
