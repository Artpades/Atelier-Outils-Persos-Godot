using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 💬 <b>Effet d'infobulle</b> : remplace l'infobulle de survol (par défaut, le nom technique du membre) par un
	/// <b>texte personnalisé</b>, sur le widget entier de chaque donnée ciblée (y compris ses contrôles imbriqués, pour
	/// que l'infobulle apparaisse quel que soit l'endroit survolé).<br/>
	/// <b>📍 Note :</b> le texte est statique ; il est appliqué une fois, sans surveillance.
	/// </summary>
	[GlobalClass, Tool]
	public partial class TooltipEffect : InterfaceEffect
	{
		/// <summary>Texte affiché au survol.</summary>
		[Export(PropertyHint.MultilineText)] public string Text { get; set; } = "";

		/// <summary>Noms des données (<see cref="DataEntry.DataName"/>) qui reçoivent cette infobulle.</summary>
		[Export] public Godot.Collections.Array<string> TargetDataNames { get; set; } = new();

		/// <inheritdoc/>
		public override void Apply(Control owner, GodotObject target, System.Collections.Generic.IReadOnlyDictionary<string, Control> widgets)
		{
			if (!Enabled || TargetDataNames.Count == 0)
				return;

			foreach (var widget in CollectTargets(TargetDataNames, widgets))
				WalkControls(widget, control => control.TooltipText = Text);
		}
	}
}
