using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🏭 <b>Fabrique de générateurs</b> : traduit un <see cref="DataEntry"/> (widget choisi + <see cref="DataEntry.ExtraData"/>
	/// + orientation) en un <see cref="PropertyWidgetGenerator"/> configuré.<br/>
	/// Les générateurs créés sont éphémères ; seul <see cref="WidgetChoice.Custom"/> réutilise (sans la modifier)
	/// la ressource référencée par l'entrée.
	/// </summary>
	public static class EntryGeneratorFactory
	{
		/// <summary>
		/// Crée le générateur d'une entrée, ou retourne <c>null</c> (mode <c>Custom</c> sans générateur, ou type non pris en charge).
		/// </summary>
		public static PropertyWidgetGenerator Create(DataEntry entry)
		{
			var choice = entry.ResolvedChoice();
			if (choice == WidgetChoice.Custom)
				return entry.CustomGenerator;

			PropertyWidgetGenerator generator = choice switch
			{
				WidgetChoice.CheckBox => new BoolWidgetGenerator { Style = BoolWidgetStyle.CheckBox },
				WidgetChoice.CheckButton => new BoolWidgetGenerator { Style = BoolWidgetStyle.CheckButton },
				WidgetChoice.Label => entry.Kind switch
				{
					DataKind.Bool => new BoolWidgetGenerator { Style = BoolWidgetStyle.Label },
					DataKind.Rid => new RidWidgetGenerator(),
					_ => new LabelWidgetGenerator(),
				},
				WidgetChoice.VariantSelector => new VariantWidgetGenerator(),

				WidgetChoice.SpinBox => NumberFor(entry, NumberWidgetStyle.SpinBox),
				WidgetChoice.HSlider => NumberFor(entry, NumberWidgetStyle.HSlider),
				WidgetChoice.VSlider => NumberFor(entry, NumberWidgetStyle.VSlider),
				WidgetChoice.ProgressBar => NumberFor(entry, NumberWidgetStyle.ProgressBar),
				WidgetChoice.TextureProgressBar => NumberFor(entry, NumberWidgetStyle.TextureProgressBar),

				WidgetChoice.LineEdit => new StringWidgetGenerator { Style = TextWidgetStyle.LineEdit },
				WidgetChoice.TextEdit => new StringWidgetGenerator
				{
					Style = TextWidgetStyle.TextEdit,
					MultilineHeight = (float)entry.ExtraDouble("height", 80),
				},

				WidgetChoice.OptionButton => new EnumWidgetGenerator { Style = EnumWidgetStyle.OptionButton },
				WidgetChoice.CheckButtonList => new EnumWidgetGenerator { Style = EnumWidgetStyle.CheckButtons },

				WidgetChoice.ColorPickerButton => new ColorWidgetGenerator { Style = ColorWidgetStyle.ColorPickerButton },
				WidgetChoice.ColorRectChannels => new ColorWidgetGenerator { Style = ColorWidgetStyle.ColorRectChannels },

				WidgetChoice.Button => entry.Member switch
				{
					MemberKind.Method => new MethodWidgetGenerator(),
					MemberKind.Signal => new SignalWidgetGenerator { Connections = entry.Connections },
					_ => new CallableWidgetGenerator(),
				},

				WidgetChoice.BoxContainer => ContainerFor(entry, ContainerLayout.Box),
				WidgetChoice.GridContainer => ContainerFor(entry, ContainerLayout.Grid),
				WidgetChoice.PathButton => new PathButtonWidgetGenerator(),
				WidgetChoice.StructSpinBoxes => new StructWidgetGenerator { Step = entry.ExtraDouble("step", 0.001) },
				_ => null,
			};

			if (generator != null)
			{
				generator.IsVertical = entry.IsVertical;
				generator.ShowLabel = entry.ExtraBool("show_label", true);
				generator.LabelRatio = (float)entry.ExtraDouble("label_ratio", 0.4);
			}
			return generator;
		}

		private static NumberWidgetGenerator NumberFor(DataEntry entry, NumberWidgetStyle style) => new()
		{
			Style = style,
			DefaultMin = entry.ExtraDouble("min", 0),
			DefaultMax = entry.ExtraDouble("max", 100),
			DefaultStep = entry.ExtraDouble("step", 0.001),
			ProgressTexture = LoadTexture(entry.ExtraString("texture_progress", "")),
			UnderTexture = LoadTexture(entry.ExtraString("texture_under", "")),
		};

		private static ContainerWidgetGenerator ContainerFor(DataEntry entry, ContainerLayout layout)
		{
			ContainerWidgetGenerator generator = entry.Kind switch
			{
				DataKind.NodePath => new NodePathWidgetGenerator(),
				DataKind.Array or DataKind.Dictionary => new CollectionWidgetGenerator(),
				DataKind.PackedArray => new PackedArrayWidgetGenerator { MaxDisplayed = (int)entry.ExtraDouble("max_displayed", 100) },
				_ => new ObjectWidgetGenerator(),
			};
			generator.Layout = layout;
			generator.GridColumns = (int)entry.ExtraDouble("columns", 2);
			generator.Foldable = entry.ExtraBool("foldable", true);
			generator.StartExpanded = entry.ExtraBool("expanded", false);
			return generator;
		}

		private static Texture2D LoadTexture(string path) =>
			!string.IsNullOrEmpty(path) && ResourceLoader.Exists(path) ? ResourceLoader.Load<Texture2D>(path) : null;
	}
}
