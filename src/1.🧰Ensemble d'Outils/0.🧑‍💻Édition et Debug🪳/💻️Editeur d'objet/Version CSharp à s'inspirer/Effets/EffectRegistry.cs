using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🗄️ <b>Registre des effets</b> : la liste des <see cref="InterfaceEffect"/> appliqués à l'interface générée,
	/// dans l'ordre. Troisième registre, aux côtés de <see cref="DataRegistry"/> et <see cref="GroupRegistry"/> —
	/// édité par l'onglet « Effets » de la fenêtre de configuration.
	/// </summary>
	[GlobalClass, Tool]
	public partial class EffectRegistry : Resource
	{
		/// <summary>Effets appliqués, dans l'ordre.</summary>
		[Export] public Godot.Collections.Array<InterfaceEffect> Effects { get; set; } = new();

		/// <summary>
		/// Applique tous les effets actifs sous <paramref name="owner"/>. À appeler <b>après</b> avoir généré les widgets,
		/// car chaque effet a besoin de <paramref name="widgets"/> pour retrouver le <see cref="Control"/> d'une donnée.
		/// </summary>
		public void Apply(Control owner, GodotObject target, System.Collections.Generic.IReadOnlyDictionary<string, Control> widgets)
		{
			if (!GodotObject.IsInstanceValid(owner) || !GodotObject.IsInstanceValid(target))
				return;
			foreach (var effect in Effects)
				effect?.Apply(owner, target, widgets);
		}
	}
}
