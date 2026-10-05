using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using Godot;
using GodotArray = Godot.Collections.Array;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🗂️ <b>Générateur de collections</b> (<see cref="GodotArray"/> et <see cref="GodotDict"/>).<br/>
	/// Une <see cref="FoldableSection"/> contient un widget par élément, généré <b>récursivement</b> par le registre
	/// grâce à un <see cref="ValueProxy"/> (l'élément est édité <b>en place</b> dans la collection).<br/>
	/// <br/>
	/// <b>📍 Respect de la déclaration</b> :<br/>
	/// • <b>Array typé</b> (<c>Array&lt;T&gt;</c>, <c>Array&lt;MyEnum&gt;</c>) : chaque élément prend le widget de son type ;
	/// un tableau d'<b>enum</b> affiche un <see cref="OptionButton"/> par élément. <b>Non typé</b> : chaque élément a un
	/// sélecteur de type (<see cref="VariantWidgetGenerator"/>).<br/>
	/// • <b>Dictionary typé</b> (clé et valeur déclarées) : l'ajout propose une saisie adaptée au type de clé
	/// (liste des valeurs libres pour un enum, nombre, texte...) et crée une valeur par défaut du type déclaré.
	/// <b>Non typé</b> : la clé est saisie avec un choix de type.<br/>
	/// • Ajout et suppression d'éléments pour les deux collections.<br/>
	/// <b>⚠️ Attention</b> : les <c>Packed*Array</c> sont gérés par <see cref="PackedArrayWidgetGenerator"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class CollectionWidgetGenerator : ContainerWidgetGenerator
	{
		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info)
		{
			var type = WidgetUtils.TypeOf(info);
			return type == Variant.Type.Array || type == Variant.Type.Dictionary ? 100 : 0;
		}

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName) =>
			GenerateWidget(target, memberName, PropertyWidgetRegistry.CreateDefault());

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName, PropertyWidgetRegistry registry)
		{
			var info = GetMemberInfo(target, memberName);
			bool readOnly = WidgetUtils.IsReadOnly(info);
			var arrayDecl = WidgetUtils.ArrayElementDecl(info);
			var (keyDecl, valueDecl) = WidgetUtils.DictionaryDecl(info);
			string label = DisplayName(target, memberName);
			string prefix = string.IsNullOrEmpty(label) ? "" : label + "  ";

			var section = CreateSection();
			PropertyBinding binding = null;

			void Fill(Variant value)
			{
				InspectorBuilder.Clear(section.Content);
				void Refresh() => binding?.Refresh(true);

				// Lecture fraîche et écriture systématique dans l'objet : voir WriteAndNotify (copies des tableaux C#).
				Variant Read() => ReadValue(target, memberName);
				void Write(Variant collection) => WriteAndNotify(target, memberName, collection);

				if (value.VariantType == Variant.Type.Array)
					FillArray(section.Content, Read, Write, value.AsGodotArray().Count, arrayDecl, readOnly, registry, Refresh);
				else if (value.VariantType == Variant.Type.Dictionary)
					FillDictionary(section.Content, Read, Write, value.AsGodotDictionary(), keyDecl, valueDecl, readOnly, registry, Refresh);
				else if (!readOnly && WidgetUtils.TypeOf(info) is Variant.Type.Array or Variant.Type.Dictionary)
					section.Content.AddChild(BuildCreateButton(WidgetUtils.TypeOf(info), Write, Refresh));
			}

			section.OpenChanged += open =>
			{
				if (open)
					Fill(ReadValue(target, memberName));
				else
					section.Clear();
			};

			binding = Bind(section.Root, target, memberName, v =>
			{
				section.Title = $"{prefix}({Describe(v)})";
				if (section.IsOpen)
					Fill(v);
			}, v => section.IsOpen ? StructureKey(v) : Describe(v));

			return section.Root;
		}

		private static string Describe(Variant value) => value.VariantType switch
		{
			Variant.Type.Array => $"Array · {value.AsGodotArray().Count}",
			Variant.Type.Dictionary => $"Dictionary · {value.AsGodotDictionary().Count}",
			_ => value.VariantType.ToString(),
		};

		/// <summary>Clé de comparaison <b>structurelle</b> : taille, clés et types (les valeurs sont suivies par leurs propres widgets).</summary>
		private static string StructureKey(Variant value)
		{
			var sb = new StringBuilder();
			if (value.VariantType == Variant.Type.Array)
			{
				var array = value.AsGodotArray();
				sb.Append(array.Count);
				foreach (var item in array)
					sb.Append(',').Append((int)item.VariantType);
			}
			else if (value.VariantType == Variant.Type.Dictionary)
			{
				var dict = value.AsGodotDictionary();
				foreach (var key in dict.Keys)
					sb.Append(GD.VarToStr(key)).Append(':').Append((int)dict[key].VariantType).Append(';');
			}
			return sb.ToString();
		}

		/// <summary>Bouton qui crée une collection vide quand la propriété vaut <c>null</c> (jamais initialisée).</summary>
		private static Control BuildCreateButton(Variant.Type type, Action<Variant> write, Action refresh)
		{
			var button = new Button { Text = type == Variant.Type.Array ? "Créer le tableau" : "Créer le dictionnaire" };
			button.Pressed += () =>
			{
				write(type == Variant.Type.Array ? new GodotArray() : new GodotDict());
				refresh();
			};
			return button;
		}

		#region 📚 Array
		/// <summary>
		/// Chaque widget relit le tableau <b>frais</b> depuis l'objet (<paramref name="read"/>), le modifie, puis le <b>réécrit</b>
		/// (<paramref name="write"/>) : indispensable pour un tableau C# (<c>T[]</c>) dont chaque lecture est une copie.
		/// </summary>
		private static void FillArray(Container content, Func<Variant> read, Action<Variant> write, int count, ElementDecl decl,
			bool readOnly, PropertyWidgetRegistry registry, Action refresh)
		{
			GodotArray Current()
			{
				var value = read();
				return value.VariantType == Variant.Type.Array ? value.AsGodotArray() : new GodotArray();
			}

			var snapshot = Current();
			for (int i = 0; i < count && i < snapshot.Count; i++)
			{
				int index = i;
				var proxy = new ValueProxy(
					() => { var a = Current(); return index < a.Count ? a[index] : default; },
					value =>
					{
						var a = Current();
						if (index >= a.Count)
							return;
						a[index] = value;
						write(a);
					},
					decl.IsTyped ? decl.Type : snapshot[index].VariantType, $"[{index}]",
					decl.Hint, decl.HintString, readOnly, isVariant: !decl.IsTyped);

				var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				row.AddChild(BuildElement(proxy, registry));
				if (!readOnly)
				{
					var remove = new Button { Text = "✕", TooltipText = "Supprimer l'élément" };
					remove.Pressed += () =>
					{
						var a = Current();
						if (index < a.Count)
						{
							a.RemoveAt(index);
							write(a);
						}
						refresh();
					};
					row.AddChild(remove);
				}
				content.AddChild(row);
			}

			if (readOnly)
				return;

			var add = new Button { Text = "+ Ajouter un élément" };
			add.Pressed += () =>
			{
				var a = Current();
				a.Add(WidgetUtils.DefaultElement(decl));
				write(a);
				refresh();
			};
			content.AddChild(add);
		}
		#endregion

		#region 📖 Dictionary
		/// <summary>Même principe que <see cref="FillArray"/> : lecture fraîche, modification, réécriture dans l'objet.</summary>
		private static void FillDictionary(Container content, Func<Variant> read, Action<Variant> write, GodotDict snapshot,
			ElementDecl keyDecl, ElementDecl valueDecl, bool readOnly, PropertyWidgetRegistry registry, Action refresh)
		{
			GodotDict Current()
			{
				var value = read();
				return value.VariantType == Variant.Type.Dictionary ? value.AsGodotDictionary() : new GodotDict();
			}

			var keyLabels = EnumLabels(keyDecl);

			foreach (var key in new List<Variant>(snapshot.Keys))
			{
				var k = key;
				var proxy = new ValueProxy(
					() => Current().TryGetValue(k, out var v) ? v : default,
					value =>
					{
						var d = Current();
						d[k] = value;
						write(d);
					},
					valueDecl.IsTyped ? valueDecl.Type : snapshot[k].VariantType, KeyText(k, keyLabels),
					valueDecl.Hint, valueDecl.HintString, readOnly, isVariant: !valueDecl.IsTyped);

				var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				row.AddChild(BuildElement(proxy, registry));
				if (!readOnly)
				{
					var remove = new Button { Text = "✕", TooltipText = "Supprimer l'entrée" };
					remove.Pressed += () =>
					{
						var d = Current();
						d.Remove(k);
						write(d);
						refresh();
					};
					row.AddChild(remove);
				}
				content.AddChild(row);
			}

			if (!readOnly)
				content.AddChild(BuildAddRow(Current, write, keyDecl, valueDecl, refresh));
		}

		/// <summary>Ligne d'ajout : saisie de la clé selon la déclaration + bouton. La valeur créée respecte le type déclaré.</summary>
		private static Control BuildAddRow(Func<GodotDict> current, Action<Variant> write, ElementDecl keyDecl, ElementDecl valueDecl, Action refresh)
		{
			var input = BuildKeyInput(keyDecl, current());
			var add = new Button { Text = "+ Ajouter" };
			add.Pressed += () =>
			{
				var key = input.Read();
				if (key == null)
					return;

				var d = current();
				if (d.ContainsKey(key.Value))
				{
					GD.PushWarning($"[DynamicInspector] La clé {GD.VarToStr(key.Value)} existe déjà.");
					return;
				}
				d[key.Value] = WidgetUtils.DefaultElement(valueDecl);
				write(d);
				refresh();
			};

			var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			row.AddChild(new Label { Text = "Clé" });
			row.AddChild(input.Control);
			row.AddChild(add);
			return row;
		}

		private sealed record KeyInput(Control Control, Func<Variant?> Read);

		/// <summary>Construit la saisie de clé adaptée à la déclaration (enum, nombre, texte, booléen, type libre...).</summary>
		private static KeyInput BuildKeyInput(ElementDecl decl, GodotDict dict)
		{
			bool text = decl.Type is Variant.Type.String or Variant.Type.StringName;

			// Énumération : seules les valeurs encore libres sont proposées.
			if (decl.Hint == PropertyHint.Enum && (decl.Type == Variant.Type.Int || text))
			{
				var free = WidgetUtils.ParseEnum(decl.HintString, false, text).FindAll(item => !dict.ContainsKey(Wrap(item.Value, decl.Type)));
				var option = new OptionButton { Disabled = free.Count == 0, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				foreach (var item in free)
					option.AddItem(item.Label);
				return new KeyInput(option, () => option.Selected >= 0 && option.Selected < free.Count ? Wrap(free[option.Selected].Value, decl.Type) : (Variant?)null);
			}

			switch (decl.Type)
			{
				case Variant.Type.Int:
				case Variant.Type.Float:
					{
						bool isInt = decl.Type == Variant.Type.Int;
						var spin = WidgetUtils.MakeSpinBox("", -1e12, 1e12, isInt ? 1 : 0.001, isInt);
						return new KeyInput(spin, () => isInt ? (long)Math.Round(spin.Value) : spin.Value);
					}
				case Variant.Type.Bool:
					{
						var option = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
						option.AddItem("false");
						option.AddItem("true");
						return new KeyInput(option, () => option.Selected == 1);
					}
				case Variant.Type.String:
				case Variant.Type.StringName:
					{
						var edit = new LineEdit { PlaceholderText = "clé", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
						return new KeyInput(edit, () => edit.Text.Length == 0 ? (Variant?)null : Wrap(edit.Text, decl.Type));
					}
				case Variant.Type.Nil:
					return BuildFreeKeyInput();
				default:
					{
						// Autre type déclaré : saisie au format Godot (ex : Vector2(1, 2)).
						var edit = new LineEdit { PlaceholderText = decl.Type.ToString() + "(…)", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
						return new KeyInput(edit, () =>
						{
							var parsed = GD.StrToVar(edit.Text);
							return parsed.VariantType == decl.Type ? parsed : (Variant?)null;
						});
					}
			}
		}

		/// <summary>Clé d'un dictionnaire non typé : un sélecteur de type et un champ de saisie.</summary>
		private static KeyInput BuildFreeKeyInput()
		{
			var types = new[]
			{
				Variant.Type.String, Variant.Type.Int, Variant.Type.Float, Variant.Type.Bool,
				Variant.Type.StringName, Variant.Type.Vector2, Variant.Type.Vector2I,
			};
			var option = new OptionButton();
			foreach (var t in types)
				option.AddItem(t.ToString());
			var edit = new LineEdit { PlaceholderText = "clé", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };

			var box = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			box.AddChild(option);
			box.AddChild(edit);

			return new KeyInput(box, () =>
			{
				string t = edit.Text.Trim();
				switch (types[option.Selected])
				{
					case Variant.Type.String: return t.Length == 0 ? (Variant?)null : t;
					case Variant.Type.StringName: return t.Length == 0 ? (Variant?)null : new StringName(t);
					case Variant.Type.Int: return long.TryParse(t, NumberStyles.Integer, CultureInfo.InvariantCulture, out long l) ? l : (Variant?)null;
					case Variant.Type.Float: return double.TryParse(t, NumberStyles.Float, CultureInfo.InvariantCulture, out double d) ? d : (Variant?)null;
					case Variant.Type.Bool: return bool.TryParse(t, out bool b) ? b : (Variant?)null;
					case Variant.Type.Vector2: { var v = GD.StrToVar($"Vector2({t})"); return v.VariantType == Variant.Type.Vector2 ? v : (Variant?)null; }
					default: { var v = GD.StrToVar($"Vector2i({t})"); return v.VariantType == Variant.Type.Vector2I ? v : (Variant?)null; }
				}
			});
		}

		private static Variant Wrap(Variant value, Variant.Type type) =>
			type == Variant.Type.StringName ? new StringName(value.AsString()) : value;

		/// <summary>Libellés des valeurs d'une énumération (pour afficher <c>Idle</c> plutôt que <c>0</c>).</summary>
		private static Dictionary<string, string> EnumLabels(ElementDecl decl)
		{
			var map = new Dictionary<string, string>();
			if (decl.Hint == PropertyHint.Enum)
				foreach (var (label, value) in WidgetUtils.ParseEnum(decl.HintString, false, decl.Type != Variant.Type.Int))
					map[value.AsString()] = label;
			return map;
		}

		private static string KeyText(Variant key, Dictionary<string, string> enumLabels)
		{
			string raw = key.VariantType == Variant.Type.String ? key.AsString() : GD.VarToStr(key);
			return enumLabels.TryGetValue(key.AsString(), out var label) ? label : raw;
		}
		#endregion

		private static Control BuildElement(ValueProxy proxy, PropertyWidgetRegistry registry)
		{
			proxy.TryGetMemberInfo(ValueProxy.ValueName, MemberKind.Property, out var info);
			return registry.BuildWidget(proxy, info)
				?? WidgetUtils.MakeHint($"{DisplayName(proxy, ValueProxy.ValueName)} : {GD.VarToStr(proxy.ReadValue(ValueProxy.ValueName))}");
		}
	}
}
