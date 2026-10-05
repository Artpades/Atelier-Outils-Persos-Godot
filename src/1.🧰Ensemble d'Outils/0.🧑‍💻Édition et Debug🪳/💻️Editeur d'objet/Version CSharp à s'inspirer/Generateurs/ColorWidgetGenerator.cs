using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Apparence du widget d'une couleur.</summary>
	public enum ColorWidgetStyle
	{
		/// <summary>Bouton ouvrant un sélecteur de couleur.</summary>
		ColorPickerButton,
		/// <summary>Un <see cref="SpinBox"/> par canal (R, G, B, A).</summary>
		ChannelSpinBoxes,
		/// <summary>Un <see cref="ColorRect"/> d'aperçu suivi d'un <see cref="SpinBox"/> par canal.</summary>
		ColorRectChannels
	}

	/// <summary>
	/// 🎨 <b>Générateur de couleurs</b> (<see cref="Color"/>) : <see cref="ColorPickerButton"/> ou quatre
	/// <see cref="SpinBox"/> (R, G, B, A). Respecte <c>PropertyHint.ColorNoAlpha</c>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class ColorWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Apparence du widget.</summary>
		[Export] public ColorWidgetStyle Style { get; set; } = ColorWidgetStyle.ColorPickerButton;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			WidgetUtils.TypeOf(info) == Variant.Type.Color ? 100 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName);
			bool readOnly = WidgetUtils.IsReadOnly(info);
			bool alpha = WidgetUtils.HintOf(info) != PropertyHint.ColorNoAlpha;

			return Labeled(target, memberName, Style == ColorWidgetStyle.ColorPickerButton
				? BuildPicker(target, memberName, readOnly, alpha)
				: BuildChannels(target, memberName, readOnly, alpha, Style == ColorWidgetStyle.ColorRectChannels));
		}

		private static Control BuildPicker(GodotObject target, string memberName, bool readOnly, bool alpha)
		{
			var picker = new ColorPickerButton { EditAlpha = alpha, Disabled = readOnly, CustomMinimumSize = new Vector2(0, 28) };
			var binding = Bind(picker, target, memberName, v => picker.Color = v.AsColor());
			picker.ColorChanged += color => Commit(binding, target, memberName, color);
			return picker;
		}

		private static Control BuildChannels(GodotObject target, string memberName, bool readOnly, bool alpha, bool withSwatch)
		{
			string[] names = { "R", "G", "B", "A" };
			var boxes = new SpinBox[alpha ? 4 : 3];
			var row = new HBoxContainer();
			var swatch = withSwatch ? new ColorRect { CustomMinimumSize = new Vector2(32, 24) } : null;
			if (swatch != null)
				row.AddChild(swatch);

			for (int i = 0; i < boxes.Length; i++)
			{
				boxes[i] = WidgetUtils.MakeSpinBox(names[i], 0, 1, 0.01, false);
				boxes[i].Editable = !readOnly;
				row.AddChild(boxes[i]);
			}

			var binding = Bind(row, target, memberName, v =>
			{
				var color = v.AsColor();
				if (swatch != null)
					swatch.Color = color;
				boxes[0].SetValueNoSignal(color.R);
				boxes[1].SetValueNoSignal(color.G);
				boxes[2].SetValueNoSignal(color.B);
				if (alpha)
					boxes[3].SetValueNoSignal(color.A);
			});

			foreach (var box in boxes)
			{
				box.ValueChanged += _ =>
				{
					var color = new Color((float)boxes[0].Value, (float)boxes[1].Value, (float)boxes[2].Value,
						alpha ? (float)boxes[3].Value : 1f);
					if (swatch != null)
						swatch.Color = color;
					Commit(binding, target, memberName, color);
				};
			}
			return row;
		}
	}
}
