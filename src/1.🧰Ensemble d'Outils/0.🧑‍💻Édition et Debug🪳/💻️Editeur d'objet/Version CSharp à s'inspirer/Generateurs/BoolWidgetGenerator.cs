using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Apparence du widget d'un booléen.</summary>
	public enum BoolWidgetStyle
	{
		/// <summary>Case à cocher.</summary>
		CheckBox,
		/// <summary>Interrupteur.</summary>
		CheckButton,
		/// <summary>Texte « ✔ Vrai / ✘ Faux » (lecture seule).</summary>
		Label
	}

	/// <summary>
	/// ☑️ <b>Générateur de booléens</b> : <see cref="CheckBox"/>, <see cref="CheckButton"/> ou <see cref="Label"/> lié à une propriété <c>bool</c>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class BoolWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Apparence du widget.</summary>
		[Export] public BoolWidgetStyle Style { get; set; } = BoolWidgetStyle.CheckBox;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			WidgetUtils.TypeOf(info) == Variant.Type.Bool ? 100 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName);

			if (Style == BoolWidgetStyle.Label)
			{
				var label = new Label();
				Bind(label, target, memberName, v => label.Text = v.AsBool() ? "✔ Vrai" : "✘ Faux");
				return Labeled(target, memberName, label);
			}

			BaseButton box = Style == BoolWidgetStyle.CheckButton ? new CheckButton() : new CheckBox();
			box.Disabled = WidgetUtils.IsReadOnly(info);

			var binding = Bind(box, target, memberName, v => box.SetPressedNoSignal(v.AsBool()));
			box.Toggled += pressed => Commit(binding, target, memberName, pressed);

			return Labeled(target, memberName, box);
		}
	}
}
