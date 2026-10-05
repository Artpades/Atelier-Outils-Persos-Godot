using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🔘 <b>Générateur de méthodes</b> : un <see cref="Button"/> portant le nom de la méthode et l'exécutant au clic.<br/>
	/// Les arguments sont remplis avec leurs <b>valeurs par défaut</b> (celles déclarées, sinon la valeur « vide » du type).
	/// La signature complète est dans l'infobulle et la valeur retournée s'affiche à droite du bouton.<br/>
	/// <b>📍 Note :</b> ce générateur ne s'applique qu'aux membres de catégorie <see cref="MemberKind.Method"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class MethodWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Affiche la valeur retournée à droite du bouton.</summary>
		[Export] public bool ShowResult { get; set; } = true;

		/// <inheritdoc/>
		public override MemberKind Kind => MemberKind.Method;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) => 100;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName, MemberKind.Method);
			var args = WidgetUtils.DefaultArguments(info);

			var button = new Button { Text = memberName, TooltipText = WidgetUtils.Signature(info), Alignment = HorizontalAlignment.Left };
			var result = WidgetUtils.MakeHint("");

			button.Pressed += () =>
			{
				if (!GodotObject.IsInstanceValid(target))
					return;
				var returned = target.Call(memberName, args);
				if (ShowResult && returned.VariantType != Variant.Type.Nil)
					result.Text = "→ " + GD.VarToStr(returned);
			};

			var row = new BoxContainer { Vertical = IsVertical, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			row.AddChild(button);
			row.AddChild(result);
			return row;
		}
	}
}
