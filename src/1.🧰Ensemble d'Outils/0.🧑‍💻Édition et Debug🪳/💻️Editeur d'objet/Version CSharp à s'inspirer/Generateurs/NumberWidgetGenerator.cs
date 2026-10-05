using System;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Apparence du widget d'un nombre.</summary>
	public enum NumberWidgetStyle
	{
		/// <summary>Champ numérique avec flèches.</summary>
		SpinBox,
		/// <summary>Curseur horizontal.</summary>
		HSlider,
		/// <summary>Curseur vertical.</summary>
		VSlider,
		/// <summary>Barre de progression (lecture seule).</summary>
		ProgressBar,
		/// <summary>Barre de progression texturée (lecture seule). Retombe sur <see cref="ProgressBar"/> sans <see cref="NumberWidgetGenerator.ProgressTexture"/>.</summary>
		TextureProgressBar
	}

	/// <summary>
	/// 🔢 <b>Générateur de nombres</b> (<c>int</c> / <c>float</c>) : <see cref="SpinBox"/>, sliders ou barres de progression.<br/>
	/// Respecte l'indice <c>[Export(PropertyHint.Range, "min,max,step,or_greater,or_less,suffix:x")]</c>.<br/>
	/// <b>📍 Note :</b> sliders et barres exigent des bornes ; sans indice Range, <see cref="DefaultMin"/> / <see cref="DefaultMax"/> s'appliquent.
	/// </summary>
	[GlobalClass, Tool]
	public partial class NumberWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Apparence du widget.</summary>
		[Export] public NumberWidgetStyle Style { get; set; } = NumberWidgetStyle.SpinBox;

		/// <summary>Minimum utilisé par les sliders/barres si la propriété n'a pas de Range.</summary>
		[Export] public double DefaultMin { get; set; } = 0;

		/// <summary>Maximum utilisé par les sliders/barres si la propriété n'a pas de Range.</summary>
		[Export] public double DefaultMax { get; set; } = 100;

		/// <summary>Pas des nombres décimaux si la propriété n'en précise pas.</summary>
		[Export] public double DefaultStep { get; set; } = 0.001;

		/// <summary>Texture de remplissage du <see cref="TextureProgressBar"/>.</summary>
		[Export] public Texture2D ProgressTexture { get; set; }

		/// <summary>Texture de fond du <see cref="TextureProgressBar"/>.</summary>
		[Export] public Texture2D UnderTexture { get; set; }

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info)
		{
			var type = WidgetUtils.TypeOf(info);
			if (type != Variant.Type.Int && type != Variant.Type.Float)
				return 0;
			return WidgetUtils.HintOf(info) == PropertyHint.Range ? 150 : 100;
		}

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName);
			bool isInt = WidgetUtils.TypeOf(info) == Variant.Type.Int;
			bool readOnly = WidgetUtils.IsReadOnly(info);
			var range = WidgetUtils.ParseRange(
				WidgetUtils.HintOf(info) == PropertyHint.Range ? WidgetUtils.HintStringOf(info) : "", isInt);

			bool bounded = Style != NumberWidgetStyle.SpinBox;
			double min = range.HasBounds ? range.Min : bounded ? DefaultMin : -1e12;
			double max = range.HasBounds ? range.Max : bounded ? DefaultMax : 1e12;
			double step = range.Step > 0 ? range.Step : isInt ? 1 : DefaultStep;

			Godot.Range slider;
			Control editor;
			Label valueLabel = null;

			switch (Style)
			{
				case NumberWidgetStyle.SpinBox:
					var spin = new SpinBox { Suffix = range.Suffix, Editable = !readOnly };
					slider = spin;
					editor = spin;
					break;

				case NumberWidgetStyle.HSlider:
				case NumberWidgetStyle.VSlider:
					var handle = Style == NumberWidgetStyle.HSlider ? (Slider)new HSlider() : new VSlider();
					handle.Editable = !readOnly;
					handle.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;
					if (handle is VSlider)
						handle.CustomMinimumSize = new Vector2(0, 100);
					valueLabel = new Label { CustomMinimumSize = new Vector2(48, 0), HorizontalAlignment = HorizontalAlignment.Right };
					var row = new HBoxContainer();
					row.AddChild(handle);
					row.AddChild(valueLabel);
					slider = handle;
					editor = row;
					break;

				default:
					slider = Style == NumberWidgetStyle.TextureProgressBar && ProgressTexture != null
						? new TextureProgressBar { TextureProgress = ProgressTexture, TextureUnder = UnderTexture }
						: new ProgressBar { ShowPercentage = false };
					slider.CustomMinimumSize = new Vector2(0, 20);
					editor = slider;
					break;
			}

			slider.MinValue = min;
			slider.MaxValue = max;
			slider.Step = step;
			slider.Rounded = isInt;
			slider.AllowGreater = range.OrGreater || (!range.HasBounds && !bounded);
			slider.AllowLesser = range.OrLess || (!range.HasBounds && !bounded);

			void UpdateLabel()
			{
				if (valueLabel != null)
					valueLabel.Text = isInt ? ((long)slider.Value).ToString() : slider.Value.ToString("0.###");
			}

			var binding = Bind(editor, target, memberName, v =>
			{
				slider.SetValueNoSignal(v.AsDouble());
				UpdateLabel();
			});

			slider.ValueChanged += value =>
			{
				UpdateLabel();
				Commit(binding, target, memberName, isInt ? Variant.From((long)Math.Round(value)) : Variant.From(value));
			};

			return Labeled(target, memberName, editor);
		}
	}
}
