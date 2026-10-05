using System.Collections.Generic;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🎭 <b>Générateur de <c>Variant</c></b> : pour une valeur dont le <b>type est libre</b> (propriété <c>Variant</c>,
	/// élément d'un <c>Array</c> ou d'un <c>Dictionary</c> non typé), un <see cref="OptionButton"/> choisit le type et le widget
	/// correspondant est généré <b>récursivement</b> par le registre pour éditer la valeur.<br/>
	/// Changer de type <b>convertit</b> la valeur quand c'est possible (nombre ⇄ texte, <c>Vector2</c> ⇄ <c>Vector2I</c>,
	/// <c>Array</c> ⇄ <c>Packed*Array</c>...), sinon elle est remplacée par la valeur par défaut du type.<br/>
	/// Si la valeur change de type de l'extérieur, le widget suit.
	/// </summary>
	[GlobalClass, Tool]
	public partial class VariantWidgetGenerator : PropertyWidgetGenerator
	{
		private static readonly Variant.Type[] Selectable =
		{
			Variant.Type.Nil, Variant.Type.Bool, Variant.Type.Int, Variant.Type.Float, Variant.Type.String,
			Variant.Type.StringName, Variant.Type.NodePath, Variant.Type.Vector2, Variant.Type.Vector2I,
			Variant.Type.Vector3, Variant.Type.Vector3I, Variant.Type.Vector4, Variant.Type.Vector4I,
			Variant.Type.Rect2, Variant.Type.Rect2I, Variant.Type.Quaternion, Variant.Type.Plane, Variant.Type.Aabb,
			Variant.Type.Basis, Variant.Type.Transform2D, Variant.Type.Transform3D, Variant.Type.Projection,
			Variant.Type.Color, Variant.Type.Array, Variant.Type.Dictionary,
			Variant.Type.PackedByteArray, Variant.Type.PackedInt32Array, Variant.Type.PackedInt64Array,
			Variant.Type.PackedFloat32Array, Variant.Type.PackedFloat64Array, Variant.Type.PackedStringArray,
			Variant.Type.PackedVector2Array, Variant.Type.PackedVector3Array, Variant.Type.PackedColorArray,
			Variant.Type.PackedVector4Array,
		};

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			WidgetUtils.IsVariantMember(info) ? 150 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName) =>
			GenerateWidget(target, memberName, PropertyWidgetRegistry.CreateDefault());

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName, PropertyWidgetRegistry registry)
		{
			bool readOnly = WidgetUtils.IsReadOnly(GetMemberInfo(target, memberName));
			var types = new List<Variant.Type>(Selectable);
			var shown = (Variant.Type)(-1);

			var option = new OptionButton { Disabled = readOnly, TooltipText = "Type de la valeur" };
			var holder = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			row.AddChild(option);
			row.AddChild(holder);

			PropertyBinding binding = null;

			void Show(Variant.Type type)
			{
				shown = type;
				if (!types.Contains(type))
					types.Add(type); // type non sélectionnable (Object, Callable...) : affiché tel quel

				option.Clear();
				foreach (var t in types)
					option.AddItem(t == Variant.Type.Nil ? "null" : t.ToString());
				option.Select(types.IndexOf(type));

				InspectorBuilder.Clear(holder);
				if (type == Variant.Type.Nil)
				{
					holder.AddChild(WidgetUtils.MakeHint("null"));
					return;
				}

				var proxy = new ValueProxy(
					() => ReadValue(target, memberName), value => WriteValue(target, memberName, value),
					type, "", readOnly: readOnly);
				proxy.TryGetMemberInfo(ValueProxy.ValueName, MemberKind.Property, out var info);
				holder.AddChild(registry.BuildWidget(proxy, info)
					?? WidgetUtils.MakeHint(GD.VarToStr(ReadValue(target, memberName))));
			}

			option.ItemSelected += index =>
			{
				var newType = types[(int)index];
				Variant next = default;
				if (newType != Variant.Type.Nil)
				{
					var current = ReadValue(target, memberName);
					next = Convertible(current.VariantType, newType) ? GD.Convert(current, newType) : default;
					if (next.VariantType != newType)
						next = WidgetUtils.DefaultFor(newType);
				}
				Commit(binding, target, memberName, next);
				Show(newType);
			};

			binding = Bind(row, target, memberName, v =>
			{
				if (v.VariantType != shown)
					Show(v.VariantType);
			}, v => (int)v.VariantType);

			return Labeled(target, memberName, row);
		}

		/// <summary>Vrai si Godot sait convertir <paramref name="from"/> en <paramref name="to"/> sans erreur.</summary>
		private static bool Convertible(Variant.Type from, Variant.Type to)
		{
			static bool Scalar(Variant.Type t) => t is Variant.Type.Bool or Variant.Type.Int or Variant.Type.Float
				or Variant.Type.String or Variant.Type.StringName or Variant.Type.NodePath;
			static bool Pair(Variant.Type a, Variant.Type b, Variant.Type x, Variant.Type y) => (a == x && b == y) || (a == y && b == x);

			return (Scalar(from) && Scalar(to))
				|| Pair(from, to, Variant.Type.Vector2, Variant.Type.Vector2I)
				|| Pair(from, to, Variant.Type.Vector3, Variant.Type.Vector3I)
				|| Pair(from, to, Variant.Type.Vector4, Variant.Type.Vector4I)
				|| Pair(from, to, Variant.Type.Rect2, Variant.Type.Rect2I)
				|| Pair(from, to, Variant.Type.Transform3D, Variant.Type.Basis)
				|| (from == Variant.Type.Array && PackedArrays.IsPacked(to))
				|| (PackedArrays.IsPacked(from) && to == Variant.Type.Array);
		}
	}
}
