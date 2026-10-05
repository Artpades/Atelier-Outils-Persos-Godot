using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Sens de la condition d'un <see cref="VisibilityEffect"/>.</summary>
	public enum VisibilityMode
	{
		/// <summary>Les données sont affichées quand la condition est vraie, cachées sinon.</summary>
		ShowWhenTrue,
		/// <summary>Les données sont cachées quand la condition est vraie, affichées sinon.</summary>
		HideWhenTrue
	}

	/// <summary>
	/// 👁️‍🗨️ <b>Effet de visibilité</b> : cache ou montre des données selon une <b>condition booléenne</b>, portée par
	/// <see cref="ConditionMember"/> — soit une <b>propriété <c>bool</c></b>, soit une <b>méthode sans argument
	/// significatif retournant un <c>bool</c></b> (appelée à chaque vérification, avec ses valeurs par défaut).<br/>
	/// <br/>
	/// <b>📍 Note :</b> plusieurs données de <see cref="TargetDataNames"/> ; contrairement aux groupes, une donnée
	/// peut être visée par <b>plusieurs</b> effets de visibilité à la fois.
	/// </summary>
	[GlobalClass, Tool]
	public partial class VisibilityEffect : InterfaceEffect
	{
		/// <summary>Nom de la propriété booléenne ou de la méthode retournant un booléen qui pilote la condition.</summary>
		[Export] public string ConditionMember { get; set; } = "";

		/// <summary>Sens de la condition.</summary>
		[Export] public VisibilityMode Mode { get; set; } = VisibilityMode.ShowWhenTrue;

		/// <summary>Noms des données (<see cref="DataEntry.DataName"/>) montrées ou cachées ensemble.</summary>
		[Export] public Godot.Collections.Array<string> TargetDataNames { get; set; } = new();

		/// <inheritdoc/>
		public override void Apply(Control owner, GodotObject target, IReadOnlyDictionary<string, Control> widgets)
		{
			if (!Enabled || string.IsNullOrEmpty(ConditionMember) || TargetDataNames.Count == 0)
				return;

			var targets = CollectTargets(TargetDataNames, widgets);
			if (targets.Count == 0)
				return;

			Watch(owner, () => Evaluate(target), visible =>
			{
				foreach (var widget in targets)
					if (GodotObject.IsInstanceValid(widget))
						widget.Visible = visible;
			});
		}

		/// <summary>Calcule l'état affiché/caché à partir de <see cref="ConditionEvaluator"/>.</summary>
		private bool Evaluate(GodotObject target)
		{
			bool? condition = ConditionEvaluator.Read(target, ConditionMember, "VisibilityEffect");
			if (condition == null)
				return true; // condition illisible : ne cache rien plutôt que de masquer silencieusement une donnée
			return Mode == VisibilityMode.ShowWhenTrue ? condition.Value : !condition.Value;
		}
	}
}
