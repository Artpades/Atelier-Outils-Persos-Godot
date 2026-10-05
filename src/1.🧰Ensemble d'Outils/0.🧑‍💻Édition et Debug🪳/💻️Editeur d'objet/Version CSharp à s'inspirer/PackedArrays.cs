using System;
using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📦 <b>Aides pour les <c>Packed*Array</c></b> : type d'élément, taille, lecture / écriture d'un élément et
	/// conversion vers une liste de <see cref="Variant"/>.<br/>
	/// <b>⚠️ Attention</b> : les tableaux packés sont des <b>types valeur</b> ; chaque lecture / écriture <b>copie</b> le tableau
	/// entier. Prévu pour des tableaux de taille modeste (voir <see cref="PackedArrayWidgetGenerator.MaxDisplayed"/>).
	/// </summary>
	public static class PackedArrays
	{
		/// <summary>Vrai si <paramref name="type"/> est un <c>Packed*Array</c>.</summary>
		public static bool IsPacked(Variant.Type type) => type is
			Variant.Type.PackedByteArray or Variant.Type.PackedInt32Array or Variant.Type.PackedInt64Array
			or Variant.Type.PackedFloat32Array or Variant.Type.PackedFloat64Array or Variant.Type.PackedStringArray
			or Variant.Type.PackedVector2Array or Variant.Type.PackedVector3Array or Variant.Type.PackedColorArray
			or Variant.Type.PackedVector4Array;

		/// <summary>Type Variant d'un élément (<c>PackedByteArray</c> → <c>Int</c>, <c>PackedColorArray</c> → <c>Color</c>...).</summary>
		public static Variant.Type ElementType(Variant.Type packed) => packed switch
		{
			Variant.Type.PackedByteArray or Variant.Type.PackedInt32Array or Variant.Type.PackedInt64Array => Variant.Type.Int,
			Variant.Type.PackedFloat32Array or Variant.Type.PackedFloat64Array => Variant.Type.Float,
			Variant.Type.PackedStringArray => Variant.Type.String,
			Variant.Type.PackedVector2Array => Variant.Type.Vector2,
			Variant.Type.PackedVector3Array => Variant.Type.Vector3,
			Variant.Type.PackedColorArray => Variant.Type.Color,
			Variant.Type.PackedVector4Array => Variant.Type.Vector4,
			_ => Variant.Type.Nil,
		};

		/// <summary>Indice de l'élément : un octet est borné à 0-255.</summary>
		public static (PropertyHint Hint, string HintString) ElementHint(Variant.Type packed) =>
			packed == Variant.Type.PackedByteArray ? (PropertyHint.Range, "0,255,1") : (PropertyHint.None, "");

		/// <summary>Tableau vide du type demandé.</summary>
		public static Variant Empty(Variant.Type packed) => FromList(packed, new List<Variant>());

		/// <summary>Nombre d'éléments.</summary>
		public static int Count(Variant value) => value.VariantType switch
		{
			Variant.Type.PackedByteArray => value.AsByteArray().Length,
			Variant.Type.PackedInt32Array => value.AsInt32Array().Length,
			Variant.Type.PackedInt64Array => value.AsInt64Array().Length,
			Variant.Type.PackedFloat32Array => value.AsFloat32Array().Length,
			Variant.Type.PackedFloat64Array => value.AsFloat64Array().Length,
			Variant.Type.PackedStringArray => value.AsStringArray().Length,
			Variant.Type.PackedVector2Array => value.AsVector2Array().Length,
			Variant.Type.PackedVector3Array => value.AsVector3Array().Length,
			Variant.Type.PackedColorArray => value.AsColorArray().Length,
			Variant.Type.PackedVector4Array => value.AsVector4Array().Length,
			_ => 0,
		};

		/// <summary>Lit l'élément <paramref name="index"/> (Nil si hors limites).</summary>
		public static Variant GetAt(Variant value, int index)
		{
			static Variant At<T>(T[] array, int i, Func<T, Variant> convert) => i >= 0 && i < array.Length ? convert(array[i]) : default;

			return value.VariantType switch
			{
				Variant.Type.PackedByteArray => At(value.AsByteArray(), index, e => (long)e),
				Variant.Type.PackedInt32Array => At(value.AsInt32Array(), index, e => (long)e),
				Variant.Type.PackedInt64Array => At(value.AsInt64Array(), index, e => e),
				Variant.Type.PackedFloat32Array => At(value.AsFloat32Array(), index, e => (double)e),
				Variant.Type.PackedFloat64Array => At(value.AsFloat64Array(), index, e => e),
				Variant.Type.PackedStringArray => At(value.AsStringArray(), index, e => e),
				Variant.Type.PackedVector2Array => At(value.AsVector2Array(), index, e => e),
				Variant.Type.PackedVector3Array => At(value.AsVector3Array(), index, e => e),
				Variant.Type.PackedColorArray => At(value.AsColorArray(), index, e => e),
				Variant.Type.PackedVector4Array => At(value.AsVector4Array(), index, e => e),
				_ => default,
			};
		}

		/// <summary>Convertit un tableau packé en liste de <see cref="Variant"/>.</summary>
		public static List<Variant> ToList(Variant value)
		{
			var list = new List<Variant>();
			switch (value.VariantType)
			{
				case Variant.Type.PackedByteArray: foreach (var e in value.AsByteArray()) list.Add((long)e); break;
				case Variant.Type.PackedInt32Array: foreach (var e in value.AsInt32Array()) list.Add((long)e); break;
				case Variant.Type.PackedInt64Array: foreach (var e in value.AsInt64Array()) list.Add(e); break;
				case Variant.Type.PackedFloat32Array: foreach (var e in value.AsFloat32Array()) list.Add((double)e); break;
				case Variant.Type.PackedFloat64Array: foreach (var e in value.AsFloat64Array()) list.Add(e); break;
				case Variant.Type.PackedStringArray: foreach (var e in value.AsStringArray()) list.Add(e); break;
				case Variant.Type.PackedVector2Array: foreach (var e in value.AsVector2Array()) list.Add(e); break;
				case Variant.Type.PackedVector3Array: foreach (var e in value.AsVector3Array()) list.Add(e); break;
				case Variant.Type.PackedColorArray: foreach (var e in value.AsColorArray()) list.Add(e); break;
				case Variant.Type.PackedVector4Array: foreach (var e in value.AsVector4Array()) list.Add(e); break;
			}
			return list;
		}

		/// <summary>Construit un tableau packé de type <paramref name="packed"/> depuis une liste d'éléments.</summary>
		public static Variant FromList(Variant.Type packed, IReadOnlyList<Variant> items)
		{
			int n = items.Count;
			switch (packed)
			{
				case Variant.Type.PackedByteArray:
					{ var a = new byte[n]; for (int i = 0; i < n; i++) a[i] = (byte)Math.Clamp(items[i].AsInt64(), 0, 255); return a; }
				case Variant.Type.PackedInt32Array:
					{ var a = new int[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsInt32(); return a; }
				case Variant.Type.PackedInt64Array:
					{ var a = new long[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsInt64(); return a; }
				case Variant.Type.PackedFloat32Array:
					{ var a = new float[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsSingle(); return a; }
				case Variant.Type.PackedFloat64Array:
					{ var a = new double[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsDouble(); return a; }
				case Variant.Type.PackedStringArray:
					{ var a = new string[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsString(); return a; }
				case Variant.Type.PackedVector2Array:
					{ var a = new Vector2[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsVector2(); return a; }
				case Variant.Type.PackedVector3Array:
					{ var a = new Vector3[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsVector3(); return a; }
				case Variant.Type.PackedColorArray:
					{ var a = new Color[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsColor(); return a; }
				case Variant.Type.PackedVector4Array:
					{ var a = new Vector4[n]; for (int i = 0; i < n; i++) a[i] = items[i].AsVector4(); return a; }
				default:
					return default;
			}
		}
	}
}
