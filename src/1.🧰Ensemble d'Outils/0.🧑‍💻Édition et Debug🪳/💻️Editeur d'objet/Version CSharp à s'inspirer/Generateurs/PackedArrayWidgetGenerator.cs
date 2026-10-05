using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🧱 <b>Générateur de <c>Packed*Array</c></b> (<c>Byte</c>, <c>Int32/64</c>, <c>Float32/64</c>, <c>String</c>,
	/// <c>Vector2/3/4</c>, <c>Color</c>).<br/>
	/// Une section repliable liste les éléments (un widget chacun, généré par le registre) avec suppression (✕) et ajout.<br/>
	/// Les tableaux packés étant des types valeur, chaque modification <b>réécrit le tableau entier</b> dans l'objet.<br/>
	/// <b>⚠️ Attention</b> : seuls les <see cref="MaxDisplayed"/> premiers éléments sont affichés (chaque lecture copie le tableau).
	/// </summary>
	[GlobalClass, Tool]
	public partial class PackedArrayWidgetGenerator : ContainerWidgetGenerator
	{
		/// <summary>Nombre maximal d'éléments affichés.</summary>
		[Export(PropertyHint.Range, "1,1000,1,or_greater")] public int MaxDisplayed { get; set; } = 100;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			PackedArrays.IsPacked(WidgetUtils.TypeOf(info)) ? 100 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName) =>
			GenerateWidget(target, memberName, PropertyWidgetRegistry.CreateDefault());

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName, PropertyWidgetRegistry registry)
		{
			var info = GetMemberInfo(target, memberName);
			var type = WidgetUtils.TypeOf(info);
			bool readOnly = WidgetUtils.IsReadOnly(info);
			var elementType = PackedArrays.ElementType(type);
			var (hint, hintString) = PackedArrays.ElementHint(type);
			string label = DisplayName(target, memberName);
			string prefix = string.IsNullOrEmpty(label) ? "" : label + "  ";

			var section = CreateSection();
			PropertyBinding binding = null;

			void Rewrite(System.Action<System.Collections.Generic.List<Variant>> edit)
			{
				var items = PackedArrays.ToList(ReadValue(target, memberName));
				edit(items);
				WriteAndNotify(target, memberName, PackedArrays.FromList(type, items));
				binding?.Refresh(true);
			}

			void Fill(Variant value)
			{
				InspectorBuilder.Clear(section.Content);
				int count = PackedArrays.Count(value);
				int shown = System.Math.Min(count, MaxDisplayed);

				for (int i = 0; i < shown; i++)
				{
					int index = i;
					var proxy = new ValueProxy(
						() => PackedArrays.GetAt(ReadValue(target, memberName), index),
						element => Rewrite(items => { if (index < items.Count) items[index] = element; }),
						elementType, $"[{index}]", hint, hintString, readOnly);

					proxy.TryGetMemberInfo(ValueProxy.ValueName, MemberKind.Property, out var elementInfo);
					var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
					row.AddChild(registry.BuildWidget(proxy, elementInfo) ?? WidgetUtils.MakeHint("?"));
					if (!readOnly)
					{
						var remove = new Button { Text = "✕", TooltipText = "Supprimer l'élément" };
						remove.Pressed += () => Rewrite(items => { if (index < items.Count) items.RemoveAt(index); });
						row.AddChild(remove);
					}
					section.Content.AddChild(row);
				}

				if (count > shown)
					section.Content.AddChild(WidgetUtils.MakeHint($"… {count - shown} élément(s) non affiché(s)"));

				if (!readOnly)
				{
					var add = new Button { Text = "+ Ajouter un élément" };
					add.Pressed += () => Rewrite(items => items.Add(WidgetUtils.DefaultFor(elementType)));
					section.Content.AddChild(add);
				}
			}

			section.OpenChanged += open =>
			{
				if (open) Fill(ReadValue(target, memberName));
				else section.Clear();
			};

			binding = Bind(section.Root, target, memberName, v =>
			{
				section.Title = $"{prefix}({type.ToString().Replace("Packed", "").Replace("Array", "")} · {PackedArrays.Count(v)})";
				if (section.IsOpen)
					Fill(v);
			}, v => section.IsOpen ? PackedArrays.Count(v) : -1);

			return section.Root;
		}
	}
}
