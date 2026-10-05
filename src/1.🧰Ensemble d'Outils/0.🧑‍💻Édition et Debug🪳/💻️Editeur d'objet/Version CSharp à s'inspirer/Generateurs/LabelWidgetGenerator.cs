using System.Collections.Generic;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🏷️ <b>Générateur générique d'étiquette</b> : affiche la valeur d'une propriété en <b>lecture seule</b>, quel que soit
	/// son type, sous forme de texte lisible (mis à jour en direct comme les autres widgets).<br/>
	/// <br/>
	/// Formatage : booléen (✔/✘), enum et flags (libellés plutôt que valeurs numériques), couleur (aperçu + code),
	/// objet (nom de classe ou <c>&lt;null&gt;</c>), <c>NodePath</c>, <c>Array</c> / <c>Dictionary</c> / <c>Packed*Array</c>
	/// (nombre d'éléments), structures (<c>GD.VarToStr</c>). Les autres types utilisent <c>GD.VarToStr</c>.<br/>
	/// <br/>
	/// <b>📍 Disponibilité</b> : choix <see cref="WidgetChoice.Label"/> pour tous les types de donnée dans la fenêtre de
	/// configuration. Utilisable aussi en mode automatique via <see cref="PropertyWidgetGenerator.TargetPropertyName"/> ;
	/// c'est également le générateur de <b>repli</b> à faible priorité pour un type que rien d'autre ne prend en charge.
	/// </summary>
	[GlobalClass, Tool]
	public partial class LabelWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Score bas et constant : ne l'emporte en mode automatique que si aucun autre générateur ne convient, ou par nom.</summary>
		protected override int ScoreMember(GodotObject target, GodotDict info) => 10;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName);
			bool color = WidgetUtils.TypeOf(info) == Variant.Type.Color;

			var text = new Label { ClipText = true, TooltipText = memberName, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			ColorRect swatch = color ? new ColorRect { CustomMinimumSize = new Vector2(20, 16) } : null;

			var editor = text as Control;
			if (swatch != null)
			{
				var row = new HBoxContainer();
				row.AddChild(swatch);
				row.AddChild(text);
				editor = row;
			}

			Bind(editor, target, memberName, v =>
			{
				text.Text = Format(v, info);
				if (swatch != null)
					swatch.Color = v.AsColor();
			});

			return Labeled(target, memberName, editor);
		}

		/// <summary>Formate une valeur en texte lisible selon son type et l'indice de la propriété (enum, flags...).</summary>
		public static string Format(Variant value, GodotDict info)
		{
			var hint = WidgetUtils.HintOf(info);
			switch (value.VariantType)
			{
				case Variant.Type.Nil:
					return "<null>";
				case Variant.Type.Bool:
					return value.AsBool() ? "✔ Vrai" : "✘ Faux";
				case Variant.Type.Int when hint == PropertyHint.Enum:
				case Variant.Type.String when hint == PropertyHint.Enum:
				case Variant.Type.StringName when hint == PropertyHint.Enum:
					return EnumLabel(value, info);
				case Variant.Type.Int when hint == PropertyHint.Flags:
					return FlagsLabel(value, info);
				case Variant.Type.Float:
					return value.AsDouble().ToString("0.###");
				case Variant.Type.Object:
					{
						var obj = ObjectWidgetGenerator.ToObject(value);
						return obj == null ? "<null>" : obj.GetType().Name;
					}
				case Variant.Type.NodePath:
					{
						string path = value.AsNodePath().ToString();
						return path.Length == 0 ? "<vide>" : path;
					}
				case Variant.Type.Array:
					return $"Array · {value.AsGodotArray().Count} élément(s)";
				case Variant.Type.Dictionary:
					return $"Dictionary · {value.AsGodotDictionary().Count} entrée(s)";
				case Variant.Type.Rid:
					{
						var rid = value.AsRid();
						return rid.IsValid ? $"RID({rid.Id})" : "RID(invalide)";
					}
				case Variant.Type.Callable:
					return value.AsCallable().Method.ToString();
				default:
					return PackedArrays.IsPacked(value.VariantType)
						? $"{value.VariantType} · {PackedArrays.Count(value)} élément(s)"
						: GD.VarToStr(value);
			}
		}

		private static string EnumLabel(Variant value, GodotDict info)
		{
			bool text = WidgetUtils.TypeOf(info) != Variant.Type.Int;
			foreach (var (label, entryValue) in WidgetUtils.ParseEnum(WidgetUtils.HintStringOf(info), false, text))
				if (text ? entryValue.AsString() == value.AsString() : entryValue.AsInt64() == value.AsInt64())
					return label;
			return GD.VarToStr(value);
		}

		private static string FlagsLabel(Variant value, GodotDict info)
		{
			long mask = value.AsInt64();
			var labels = new List<string>();
			foreach (var (label, entryValue) in WidgetUtils.ParseEnum(WidgetUtils.HintStringOf(info), true, false))
			{
				long bit = entryValue.AsInt64();
				if (bit != 0 && (mask & bit) == bit)
					labels.Add(label);
			}
			return labels.Count > 0 ? string.Join(" | ", labels) : "(aucun)";
		}
	}
}
