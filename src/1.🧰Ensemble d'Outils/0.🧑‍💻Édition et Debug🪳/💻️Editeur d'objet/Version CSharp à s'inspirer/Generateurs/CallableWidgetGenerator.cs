using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// ▶️ <b>Générateur de <see cref="Callable"/></b> : un <see cref="Button"/> qui exécute la <c>Callable</c>
	/// stockée dans la propriété (sans argument) à chaque clic.
	/// </summary>
	[GlobalClass, Tool]
	public partial class CallableWidgetGenerator : PropertyWidgetGenerator
	{
		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			WidgetUtils.TypeOf(info) == Variant.Type.Callable ? 100 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var button = new Button
			{
				Text = "▶ " + memberName,
				TooltipText = "Exécute la Callable",
				Disabled = WidgetUtils.IsReadOnly(GetMemberInfo(target, memberName)),
			};
			button.Pressed += () =>
			{
				if (GodotObject.IsInstanceValid(target))
					ReadValue(target, memberName).AsCallable().Call();
			};
			return button;
		}
	}
}
