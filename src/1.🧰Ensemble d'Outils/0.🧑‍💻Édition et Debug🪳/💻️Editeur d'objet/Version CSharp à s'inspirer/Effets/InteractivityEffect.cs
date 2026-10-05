using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Comportement d'un <see cref="InteractivityEffect"/>.</summary>
	public enum InteractivityMode
	{
		/// <summary>Toujours en lecture seule, sans condition (<see cref="InteractivityEffect.ConditionMember"/> ignoré).</summary>
		AlwaysReadOnly,
		/// <summary>En lecture seule quand la condition est vraie, éditable sinon.</summary>
		ReadOnlyIf,
		/// <summary>Éditable quand la condition est vraie, en lecture seule sinon.</summary>
		EditableIf
	}

	/// <summary>
	/// 🔒 <b>Effet d'interactivité</b> : rend des données <b>ininteragissables</b> — cases, champs, curseurs, boutons
	/// (y compris ceux imbriqués : lignes de tableau, canaux de couleur...) désactivés, plus un léger assombrissement.<br/>
	/// <br/>
	/// Trois comportements (<see cref="Mode"/>) :<br/>
	/// • <see cref="InteractivityMode.AlwaysReadOnly"/> : toujours verrouillé, aucune condition nécessaire ;<br/>
	/// • <see cref="InteractivityMode.ReadOnlyIf"/> : verrouillé quand <see cref="ConditionMember"/> (propriété ou
	/// méthode <c>bool</c>) est vrai ;<br/>
	/// • <see cref="InteractivityMode.EditableIf"/> : éditable seulement quand <see cref="ConditionMember"/> est vrai.<br/>
	/// <br/>
	/// <b>📍 Note :</b> le verrouillage porte sur <b>tout</b> le widget de la donnée, y compris les boutons de repli /
	/// déploiement et d'ajout/suppression d'éléments d'un tableau ou dictionnaire.<br/>
	/// <b>📍 Condition illisible</b> : par prudence, la donnée reste <b>éditable</b> plutôt que d'être verrouillée
	/// silencieusement par une erreur de configuration.
	/// </summary>
	[GlobalClass, Tool]
	public partial class InteractivityEffect : InterfaceEffect
	{
		/// <summary>Comportement de l'effet.</summary>
		[Export] public InteractivityMode Mode { get; set; } = InteractivityMode.AlwaysReadOnly;

		/// <summary>Propriété <c>bool</c> ou méthode retournant un <c>bool</c> qui pilote la condition. Ignoré en mode <see cref="InteractivityMode.AlwaysReadOnly"/>.</summary>
		[Export] public string ConditionMember { get; set; } = "";

		/// <summary>Noms des données (<see cref="DataEntry.DataName"/>) verrouillées ensemble.</summary>
		[Export] public Godot.Collections.Array<string> TargetDataNames { get; set; } = new();

		/// <summary>Opacité du widget quand il est verrouillé (1 = pas d'assombrissement).</summary>
		[Export(PropertyHint.Range, "0.1,1,0.05")] public float LockedOpacity { get; set; } = 0.6f;

		/// <inheritdoc/>
		public override void Apply(Control owner, GodotObject target, IReadOnlyDictionary<string, Control> widgets)
		{
			if (!Enabled || TargetDataNames.Count == 0)
				return;

			var targets = CollectTargets(TargetDataNames, widgets);
			if (targets.Count == 0)
				return;

			if (Mode == InteractivityMode.AlwaysReadOnly)
			{
				foreach (var widget in targets)
					SetInteractive(widget, false);
				return; // état figé : aucune condition à surveiller
			}

			if (string.IsNullOrEmpty(ConditionMember))
				return;

			Watch(owner, () => Evaluate(target), interactive =>
			{
				foreach (var widget in targets)
					if (GodotObject.IsInstanceValid(widget))
						SetInteractive(widget, interactive);
			});
		}

		/// <summary>Calcule si le widget doit rester interactif. Condition illisible : reste éditable (échec permissif).</summary>
		private bool Evaluate(GodotObject target)
		{
			bool? condition = ConditionEvaluator.Read(target, ConditionMember, "InteractivityEffect");
			if (condition == null)
				return true;
			return Mode == InteractivityMode.EditableIf ? condition.Value : !condition.Value;
		}

		/// <summary>Active ou verrouille un widget entier : chaque contrôle interactif connu de la hiérarchie, plus l'assombrissement.</summary>
		private void SetInteractive(Control widget, bool interactive)
		{
			WalkControls(widget, control =>
			{
				control.Modulate = new Color(1, 1, 1, interactive ? 1f : LockedOpacity);
				switch (control)
				{
					case BaseButton button: button.Disabled = !interactive; break;
					case SpinBox spin: spin.Editable = interactive; break;
					case Slider slider: slider.Editable = interactive; break; // HSlider, VSlider
					case LineEdit lineEdit: lineEdit.Editable = interactive; break;
					case TextEdit textEdit: textEdit.Editable = interactive; break;
				}
			});
		}
	}
}
