using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Apparence du widget d'un texte.</summary>
	public enum TextWidgetStyle
	{
		/// <summary><c>TextEdit</c> si l'indice est <c>MultilineText</c>, sinon <c>LineEdit</c>.</summary>
		Auto,
		/// <summary>Champ sur une ligne.</summary>
		LineEdit,
		/// <summary>Zone multiligne.</summary>
		TextEdit
	}

	/// <summary>
	/// 📝 <b>Générateur de texte</b> (<c>string</c> / <c>StringName</c>) : <see cref="LineEdit"/> ou <see cref="TextEdit"/>.<br/>
	/// Le <see cref="TextEdit"/> est utilisé pour <c>[Export(PropertyHint.MultilineText)]</c> (style Auto) ou si <see cref="Style"/> l'impose.
	/// </summary>
	[GlobalClass, Tool]
	public partial class StringWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Apparence du widget (Auto : selon l'indice de la propriété).</summary>
		[Export] public TextWidgetStyle Style { get; set; } = TextWidgetStyle.Auto;

		/// <summary>Hauteur minimale du <see cref="TextEdit"/>.</summary>
		[Export] public float MultilineHeight { get; set; } = 80;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info)
		{
			var type = WidgetUtils.TypeOf(info);
			return type == Variant.Type.String || type == Variant.Type.StringName ? 100 : 0;
		}

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName);
			bool readOnly = WidgetUtils.IsReadOnly(info);
			bool isStringName = WidgetUtils.TypeOf(info) == Variant.Type.StringName;
			bool multiline = Style == TextWidgetStyle.TextEdit
				|| (Style == TextWidgetStyle.Auto && WidgetUtils.HintOf(info) == PropertyHint.MultilineText);

			Variant ToVariant(string text) => isStringName ? new StringName(text) : text;

			if (multiline)
			{
				var textEdit = new TextEdit { Editable = !readOnly, CustomMinimumSize = new Vector2(0, MultilineHeight) };
				var binding = Bind(textEdit, target, memberName, v =>
				{
					string text = v.AsString();
					if (textEdit.Text == text)
						return;
					int line = textEdit.GetCaretLine(), column = textEdit.GetCaretColumn();
					textEdit.Text = text;
					textEdit.SetCaretLine(line);
					textEdit.SetCaretColumn(column);
				});
				textEdit.TextChanged += () => Commit(binding, target, memberName, ToVariant(textEdit.Text));
				return Labeled(target, memberName, textEdit);
			}

			var lineEdit = new LineEdit { Editable = !readOnly };
			var lineBinding = Bind(lineEdit, target, memberName, v =>
			{
				string text = v.AsString();
				if (lineEdit.Text == text)
					return;
				int caret = lineEdit.CaretColumn;
				lineEdit.Text = text;
				lineEdit.CaretColumn = caret;
			});
			lineEdit.TextChanged += text => Commit(lineBinding, target, memberName, ToVariant(text));
			return Labeled(target, memberName, lineEdit);
		}
	}
}
