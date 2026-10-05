#if TOOLS
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📊 <b>Matrice des entrées de données</b> : un tableau éditable où <b>chaque ligne est une entrée</b> du
	/// <see cref="DataRegistry"/> et chaque colonne un champ du dictionnaire.<br/>
	/// Colonnes : ordre · nom · membre/type · <c>WidgetChoice</c> · <c>CustomGenerator</c> · <c>IsVertical</c> ·
	/// affiché · <c>ExtraData</c> · suppression.<br/>
	/// <b>📍 Note :</b> <c>ExtraData</c> s'écrit <c>clé=valeur; clé=valeur</c> (valeurs : bool, entier, décimal ou texte).
	/// </summary>
	public sealed class DataMatrixEditor
	{
		private static readonly string[] Headers =
			{ "", "Donnée", "Type", "Widget", "Générateur (Custom)", "Vertical", "Affiché", "ExtraData", "" };

		private readonly DataRegistry _data;
		private readonly Action _changed;
		private readonly GridContainer _grid = new() { Columns = Headers.Length };

		/// <summary>Contrôle racine (titre + tableau défilant).</summary>
		public Control Root { get; }

		/// <param name="data">Registre édité.</param>
		/// <param name="changed">Appelé après chaque modification.</param>
		public DataMatrixEditor(DataRegistry data, Action changed)
		{
			_data = data;
			_changed = changed;

			var scroll = new ScrollContainer { SizeFlagsVertical = Control.SizeFlags.ExpandFill, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			scroll.AddChild(_grid);

			var root = new VBoxContainer { Name = "Données", SizeFlagsVertical = Control.SizeFlags.ExpandFill };
			root.AddChild(new Label { Text = "Registre des données", ThemeTypeVariation = "HeaderSmall" });
			root.AddChild(scroll);
			Root = root;

			Refresh();
		}

		/// <summary>Reconstruit toute la matrice.</summary>
		public void Refresh()
		{
			foreach (Node child in _grid.GetChildren())
			{
				_grid.RemoveChild(child);
				child.QueueFree();
			}

			foreach (string header in Headers)
				_grid.AddChild(new Label { Text = header, Modulate = new Color(1, 1, 1, 0.7f) });

			for (int i = 0; i < _data.Entries.Count; i++)
				if (_data.Entries[i] != null)
					AddRow(i, _data.Entries[i]);
		}

		private void AddRow(int index, DataEntry entry)
		{
			// Ordre
			var order = new HBoxContainer();
			var up = new Button { Text = "▲", Disabled = index == 0 };
			var down = new Button { Text = "▼", Disabled = index == _data.Entries.Count - 1 };
			up.Pressed += () => Swap(index, index - 1);
			down.Pressed += () => Swap(index, index + 1);
			order.AddChild(up);
			order.AddChild(down);
			_grid.AddChild(order);

			// Nom / type
			_grid.AddChild(new Label
			{
				Text = (entry.IsMissing ? "⚠ " : "") + entry.DataName,
				TooltipText = entry.IsMissing ? "Ce membre n'existe plus sur la cible." : entry.DataName,
				CustomMinimumSize = new Vector2(150, 0),
			});
			_grid.AddChild(new Label { Text = $"{entry.Member} · {entry.Kind}", Modulate = new Color(1, 1, 1, 0.7f) });

			// Widget
			var choices = new List<WidgetChoice> { WidgetChoice.Auto };
			choices.AddRange(WidgetTable.AllowedFor(entry.Kind));
			var option = new OptionButton { CustomMinimumSize = new Vector2(170, 0) };
			foreach (var choice in choices)
				option.AddItem(choice == WidgetChoice.Auto ? $"Auto ({WidgetTable.DefaultFor(entry.Kind)})" : choice.ToString());
			option.Select(Math.Max(0, choices.IndexOf(entry.WidgetChoice)));
			option.ItemSelected += selected =>
			{
				entry.WidgetChoice = choices[(int)selected];
				_changed();
				Refresh();
			};
			_grid.AddChild(option);

			// Générateur personnalisé (actif seulement en mode Custom)
			var picker = new EditorResourcePicker
			{
				BaseType = "PropertyWidgetGenerator",
				EditedResource = entry.CustomGenerator,
				Editable = entry.WidgetChoice == WidgetChoice.Custom,
				CustomMinimumSize = new Vector2(170, 0),
			};
			picker.ResourceChanged += resource =>
			{
				entry.CustomGenerator = resource as PropertyWidgetGenerator;
				_changed();
			};
			_grid.AddChild(picker);

			// Vertical / Affiché
			_grid.AddChild(Check(entry.IsVertical, on => entry.IsVertical = on));
			_grid.AddChild(Check(entry.Enabled, on => entry.Enabled = on));

			// ExtraData
			var extra = new LineEdit
			{
				Text = ExtraDataText.Format(entry.ExtraData),
				PlaceholderText = "clé=valeur; clé=valeur",
				TooltipText = "Clés reconnues : " + WidgetTable.ExtraHelp(entry.ResolvedChoice()),
				CustomMinimumSize = new Vector2(220, 0),
				SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
			};
			void CommitExtra()
			{
				if (ExtraDataText.Apply(extra.Text, entry.ExtraData))
					_changed();
			}
			extra.TextSubmitted += _ => CommitExtra();
			extra.FocusExited += CommitExtra;
			_grid.AddChild(extra);

			// Suppression (uniquement pour un membre disparu)
			var delete = new Button { Text = "✕", Disabled = !entry.IsMissing, TooltipText = "Supprimer (membre disparu)" };
			delete.Pressed += () =>
			{
				_data.Entries.Remove(entry);
				_changed();
				Refresh();
			};
			_grid.AddChild(delete);
		}

		private CheckBox Check(bool value, Action<bool> apply)
		{
			var box = new CheckBox { ButtonPressed = value };
			box.Toggled += on => { apply(on); _changed(); };
			return box;
		}

		private void Swap(int a, int b)
		{
			if (b < 0 || b >= _data.Entries.Count)
				return;
			(_data.Entries[a], _data.Entries[b]) = (_data.Entries[b], _data.Entries[a]);
			_changed();
			Refresh();
		}
	}

	/// <summary>Conversion texte ⇄ <see cref="DataEntry.ExtraData"/> (<c>clé=valeur; clé=valeur</c>).</summary>
	internal static class ExtraDataText
	{
		public static string Format(GodotDict extra)
		{
			var sb = new StringBuilder();
			foreach (var key in extra.Keys)
			{
				if (sb.Length > 0)
					sb.Append("; ");
				var value = extra[key];
				sb.Append(key.AsString()).Append('=').Append(value.VariantType == Variant.Type.String ? value.AsString() : GD.VarToStr(value));
			}
			return sb.ToString();
		}

		/// <summary>Remplace le contenu de <paramref name="extra"/> par le texte analysé. Retourne <c>true</c> si quelque chose a changé.</summary>
		public static bool Apply(string text, GodotDict extra)
		{
			var parsed = new GodotDict();
			foreach (string pair in text.Split(';'))
			{
				int eq = pair.IndexOf('=');
				string key = (eq < 0 ? pair : pair[..eq]).Trim();
				if (key.Length > 0)
					parsed[key] = eq < 0 ? true : Parse(pair[(eq + 1)..].Trim());
			}

			if (Format(parsed) == Format(extra))
				return false;

			extra.Clear();
			foreach (var key in parsed.Keys)
				extra[key] = parsed[key];
			return true;
		}

		private static Variant Parse(string text)
		{
			if (bool.TryParse(text, out bool b)) return b;
			if (long.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out long l)) return l;
			if (double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out double d)) return d;
			return text;
		}
	}
}
#endif
