using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🗄️ <b>Registre des gizmos</b> : la liste des appels de dessin configurés pour l'objet inspecté. Quatrième
	/// registre, aux côtés de <see cref="DataRegistry"/>, <see cref="GroupRegistry"/> et <see cref="EffectRegistry"/> —
	/// édité par l'onglet « Gizmos » de la fenêtre de configuration, exécuté par <see cref="GizmoDriver"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class GizmoRegistry : Resource
	{
		/// <summary>Appels configurés, dans l'ordre de dessin.</summary>
		[Export] public Godot.Collections.Array<GizmoEntry> Entries { get; set; } = new();
	}
}
