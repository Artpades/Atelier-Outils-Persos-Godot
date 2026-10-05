using System;
using System.Collections.Generic;
using System.Globalization;
using Godot;
using GodotArray = Godot.Collections.Array;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Contraintes extraites d'un indice <see cref="PropertyHint.Range"/>.</summary>
	/// <param name="HasBounds">Vrai si un minimum et un maximum ont été fournis.</param>
	public readonly record struct RangeHint(double Min, double Max, double Step, bool OrGreater, bool OrLess,
		string Suffix, bool HasBounds);

	/// <summary>Déclaration du type d'un élément de collection typée : type Variant, indice et chaîne d'indice (<c>"2/2:A,B"</c>).</summary>
	/// <param name="Type">Type Variant (<c>Nil</c> = non typé).</param>
	/// <param name="Hint">Indice de propriété de l'élément (ex : <c>Enum</c>).</param>
	/// <param name="HintString">Chaîne d'indice de l'élément.</param>
	public readonly record struct ElementDecl(Variant.Type Type, PropertyHint Hint, string HintString)
	{
		/// <summary>Élément non typé.</summary>
		public static readonly ElementDecl Untyped = new(Variant.Type.Nil, PropertyHint.None, "");

		/// <summary>Vrai si le type est déclaré.</summary>
		public bool IsTyped => Type != Variant.Type.Nil;
	}

	/// <summary>
	/// 🧰 <b>Utilitaires partagés</b> par les générateurs : lecture d'une description de membre,
	/// analyse des indices (<c>hint_string</c>), valeurs par défaut et décomposition des structures.
	/// </summary>
	public static class WidgetUtils
	{
		#region 📖 Lecture d'une description de membre
		/// <summary>Champ <c>name</c>.</summary>
		public static string NameOf(GodotDict info) =>
			info != null && info.TryGetValue("name", out var v) ? v.AsString() : "";

		/// <summary>Champ <c>type</c> (type Variant).</summary>
		public static Variant.Type TypeOf(GodotDict info) =>
			info != null && info.TryGetValue("type", out var v) ? (Variant.Type)v.AsInt32() : Variant.Type.Nil;

		/// <summary>Champ <c>hint</c>.</summary>
		public static PropertyHint HintOf(GodotDict info) =>
			info != null && info.TryGetValue("hint", out var v) ? (PropertyHint)v.AsInt32() : PropertyHint.None;

		/// <summary>Champ <c>hint_string</c>.</summary>
		public static string HintStringOf(GodotDict info) =>
			info != null && info.TryGetValue("hint_string", out var v) ? v.AsString() : "";

		/// <summary>Champ <c>usage</c>.</summary>
		public static PropertyUsageFlags UsageOf(GodotDict info) =>
			info != null && info.TryGetValue("usage", out var v) ? (PropertyUsageFlags)v.AsInt64() : PropertyUsageFlags.Default;

		/// <summary>Vrai si la propriété est marquée en lecture seule.</summary>
		public static bool IsReadOnly(GodotDict info) => (UsageOf(info) & PropertyUsageFlags.ReadOnly) != 0;
		#endregion

		#region 🔎 Analyse des indices
		/// <summary>
		/// Analyse un <c>hint_string</c> de type Range : <c>"min,max[,step][,or_greater][,or_less][,suffix:m]"</c>.
		/// </summary>
		public static RangeHint ParseRange(string hintString, bool isInt)
		{
			double min = 0, max = 0, step = 0;
			bool hasBounds = false, orGreater = false, orLess = false;
			string suffix = "";
			int numbers = 0;

			foreach (string raw in (hintString ?? "").Split(','))
			{
				string part = raw.Trim();
				if (part.Length == 0)
					continue;

				if (numbers < 3 && double.TryParse(part, NumberStyles.Float, CultureInfo.InvariantCulture, out double n))
				{
					if (numbers == 0) min = n;
					else if (numbers == 1) { max = n; hasBounds = true; }
					else step = n;
					numbers++;
				}
				else if (part == "or_greater") orGreater = true;
				else if (part == "or_less") orLess = true;
				else if (part.StartsWith("suffix:", StringComparison.Ordinal)) suffix = part[7..];
			}

			if (isInt && step <= 0) step = 1;
			return new RangeHint(min, max, step, orGreater, orLess, suffix, hasBounds);
		}

		/// <summary>
		/// Analyse un <c>hint_string</c> d'énumération (<c>"A,B:5,C"</c>) ou de flags.<br/>
		/// Valeurs implicites : précédente + 1 (enum) ou puissances de deux (flags).
		/// </summary>
		/// <param name="stringValues">Si vrai, la valeur d'une entrée est son libellé (enum de <c>string</c>).</param>
		public static List<(string Label, Variant Value)> ParseEnum(string hintString, bool flags, bool stringValues)
		{
			var result = new List<(string, Variant)>();
			long next = flags ? 1 : 0;

			foreach (string raw in (hintString ?? "").Split(','))
			{
				string part = raw.Trim();
				if (part.Length == 0)
					continue;

				if (stringValues)
				{
					result.Add((part, part));
					continue;
				}

				long value = next;
				string label = part;
				int colon = part.LastIndexOf(':');
				if (colon > 0 && long.TryParse(part[(colon + 1)..], NumberStyles.Integer, CultureInfo.InvariantCulture, out long parsed))
				{
					label = part[..colon];
					value = parsed;
				}
				result.Add((label, value));
				next = flags ? value << 1 : value + 1;
			}
			return result;
		}

		/// <summary>
		/// Analyse la déclaration d'un élément typé, au format Godot <c>"type/indice:chaîne_d_indice"</c>
		/// (ex : <c>"2/0:"</c> = int, <c>"2/2:Idle,Run"</c> = enum int, <c>"24/17:Node"</c> = objet).
		/// Retourne <see cref="ElementDecl.Untyped"/> si le format est absent.
		/// </summary>
		public static ElementDecl ParseElementDecl(string declaration)
		{
			declaration ??= "";
			int slash = declaration.IndexOf('/');
			if (slash <= 0 || !int.TryParse(declaration[..slash], out int type))
				return ElementDecl.Untyped;

			int colon = declaration.IndexOf(':', slash);
			string hintPart = colon < 0 ? declaration[(slash + 1)..] : declaration[(slash + 1)..colon];
			int.TryParse(hintPart, out int hint);
			return new ElementDecl((Variant.Type)type, (PropertyHint)hint, colon < 0 ? "" : declaration[(colon + 1)..]);
		}

		/// <summary>Déclaration des éléments d'une propriété <c>Array</c> (vide si le tableau n'est pas typé).</summary>
		public static ElementDecl ArrayElementDecl(GodotDict info) =>
			HintOf(info) == PropertyHint.TypeString ? ParseElementDecl(HintStringOf(info)) : ElementDecl.Untyped;

		/// <summary>
		/// Déclarations clé / valeur d'une propriété <c>Dictionary</c> typée (<c>"clé;valeur"</c>, chacune au format
		/// <see cref="ParseElementDecl"/>). Non typée : deux <see cref="ElementDecl.Untyped"/>.
		/// </summary>
		public static (ElementDecl Key, ElementDecl Value) DictionaryDecl(GodotDict info)
		{
			if (HintOf(info) != PropertyHint.DictionaryType)
				return (ElementDecl.Untyped, ElementDecl.Untyped);

			string hs = HintStringOf(info);
			int semicolon = hs.IndexOf(';');
			return semicolon < 0
				? (ParseElementDecl(hs), ElementDecl.Untyped)
				: (ParseElementDecl(hs[..semicolon]), ParseElementDecl(hs[(semicolon + 1)..]));
		}

		/// <summary>Vrai pour une propriété de type <c>Variant</c> (type Nil marqué <c>NilIsVariant</c>) : son type réel est libre.</summary>
		public static bool IsVariantMember(GodotDict info) =>
			TypeOf(info) == Variant.Type.Nil && (UsageOf(info) & PropertyUsageFlags.NilIsVariant) != 0;

		/// <summary>Valeur par défaut d'un nouvel élément : première valeur d'une énumération, sinon <see cref="DefaultFor"/>.</summary>
		public static Variant DefaultElement(ElementDecl decl)
		{
			if (decl.Hint == PropertyHint.Enum && decl.Type is Variant.Type.Int or Variant.Type.String or Variant.Type.StringName)
			{
				var items = ParseEnum(decl.HintString, false, decl.Type != Variant.Type.Int);
				if (items.Count > 0)
					return decl.Type == Variant.Type.StringName ? new StringName(items[0].Value.AsString()) : items[0].Value;
			}
			return DefaultFor(decl.Type);
		}
		#endregion

		#region 🧮 Valeurs par défaut et arguments
		/// <summary>Valeur « vide » raisonnable pour un type Variant (utilisée pour les arguments et l'ajout d'éléments).</summary>
		public static Variant DefaultFor(Variant.Type type) => type switch
		{
			Variant.Type.Bool => false,
			Variant.Type.Int => 0L,
			Variant.Type.Float => 0.0,
			Variant.Type.String => "",
			Variant.Type.StringName => new StringName(""),
			Variant.Type.NodePath => new NodePath(""),
			Variant.Type.Vector2 => Vector2.Zero,
			Variant.Type.Vector2I => Vector2I.Zero,
			Variant.Type.Vector3 => Vector3.Zero,
			Variant.Type.Vector3I => Vector3I.Zero,
			Variant.Type.Vector4 => Vector4.Zero,
			Variant.Type.Vector4I => Vector4I.Zero,
			Variant.Type.Color => Colors.White,
			Variant.Type.Rect2 => new Rect2(),
			Variant.Type.Rect2I => new Rect2I(),
			Variant.Type.Quaternion => Quaternion.Identity,
			Variant.Type.Plane => new Plane(Vector3.Up, 0),
			Variant.Type.Aabb => new Aabb(),
			Variant.Type.Basis => Basis.Identity,
			Variant.Type.Transform2D => Transform2D.Identity,
			Variant.Type.Transform3D => Transform3D.Identity,
			Variant.Type.Projection => Projection.Identity,
			Variant.Type.Rid => new Rid(),
			Variant.Type.Array => new GodotArray(),
			Variant.Type.Dictionary => new GodotDict(),
			_ => PackedArrays.IsPacked(type) ? PackedArrays.Empty(type) : default,
		};

		/// <summary>
		/// Construit les arguments d'appel d'une méthode ou d'un signal : valeurs par défaut déclarées
		/// (<c>default_args</c>) pour les derniers, valeur « vide » du type pour les autres.
		/// </summary>
		public static Variant[] DefaultArguments(GodotDict info)
		{
			if (info == null || !info.TryGetValue("args", out var argsVariant))
				return Array.Empty<Variant>();

			var args = argsVariant.AsGodotArray<GodotDict>();
			var defaults = info.TryGetValue("default_args", out var d) ? d.AsGodotArray() : new GodotArray();
			var result = new Variant[args.Count];
			int firstDefault = args.Count - defaults.Count;

			for (int i = 0; i < args.Count; i++)
				result[i] = i >= firstDefault ? defaults[i - firstDefault] : DefaultFor(TypeOf(args[i]));
			return result;
		}

		/// <summary>Signature lisible, ex : <c>Heal(amount: Int, source: Object)</c>.</summary>
		public static string Signature(GodotDict info)
		{
			var parts = new List<string>();
			if (info != null && info.TryGetValue("args", out var argsVariant))
				foreach (var arg in argsVariant.AsGodotArray<GodotDict>())
					parts.Add($"{NameOf(arg)}: {TypeOf(arg)}");
			return $"{NameOf(info)}({string.Join(", ", parts)})";
		}
		#endregion

		#region 📐 Structures (Vector2, Rect2, Aabb...)
		/// <summary>
		/// Décompose une structure Godot simple en composantes numériques.<br/>
		/// Types gérés : <c>Vector2/3/4</c> (et <c>I</c>), <c>Rect2/2I</c>, <c>Quaternion</c>, <c>Plane</c>, <c>Aabb</c>,
		/// <c>Basis</c>, <c>Transform2D/3D</c> et <c>Projection</c> (matrices lues ligne par ligne).
		/// </summary>
		public static bool TryDecompose(Variant value, out string[] labels, out double[] values, out bool isInt)
		{
			isInt = false;
			switch (value.VariantType)
			{
				case Variant.Type.Vector2:
					{ var v = value.AsVector2(); labels = new[] { "X", "Y" }; values = new double[] { v.X, v.Y }; return true; }
				case Variant.Type.Vector2I:
					{ var v = value.AsVector2I(); isInt = true; labels = new[] { "X", "Y" }; values = new double[] { v.X, v.Y }; return true; }
				case Variant.Type.Vector3:
					{ var v = value.AsVector3(); labels = new[] { "X", "Y", "Z" }; values = new double[] { v.X, v.Y, v.Z }; return true; }
				case Variant.Type.Vector3I:
					{ var v = value.AsVector3I(); isInt = true; labels = new[] { "X", "Y", "Z" }; values = new double[] { v.X, v.Y, v.Z }; return true; }
				case Variant.Type.Vector4:
					{ var v = value.AsVector4(); labels = new[] { "X", "Y", "Z", "W" }; values = new double[] { v.X, v.Y, v.Z, v.W }; return true; }
				case Variant.Type.Vector4I:
					{ var v = value.AsVector4I(); isInt = true; labels = new[] { "X", "Y", "Z", "W" }; values = new double[] { v.X, v.Y, v.Z, v.W }; return true; }
				case Variant.Type.Quaternion:
					{ var v = value.AsQuaternion(); labels = new[] { "X", "Y", "Z", "W" }; values = new double[] { v.X, v.Y, v.Z, v.W }; return true; }
				case Variant.Type.Rect2:
					{ var v = value.AsRect2(); labels = new[] { "X", "Y", "W", "H" }; values = new double[] { v.Position.X, v.Position.Y, v.Size.X, v.Size.Y }; return true; }
				case Variant.Type.Rect2I:
					{ var v = value.AsRect2I(); isInt = true; labels = new[] { "X", "Y", "W", "H" }; values = new double[] { v.Position.X, v.Position.Y, v.Size.X, v.Size.Y }; return true; }
				case Variant.Type.Plane:
					{ var v = value.AsPlane(); labels = new[] { "X", "Y", "Z", "D" }; values = new double[] { v.Normal.X, v.Normal.Y, v.Normal.Z, v.D }; return true; }
				case Variant.Type.Aabb:
					{
						var v = value.AsAabb();
						labels = new[] { "PX", "PY", "PZ", "SX", "SY", "SZ" };
						values = new double[] { v.Position.X, v.Position.Y, v.Position.Z, v.Size.X, v.Size.Y, v.Size.Z };
						return true;
					}
				case Variant.Type.Basis:
					{
						var b = value.AsBasis();
						labels = new[] { "00", "01", "02", "10", "11", "12", "20", "21", "22" };
						values = BasisValues(b);
						return true;
					}
				case Variant.Type.Transform2D:
					{
						var t = value.AsTransform2D();
						labels = new[] { "XX", "XY", "YX", "YY", "OX", "OY" };
						values = new double[] { t.X.X, t.X.Y, t.Y.X, t.Y.Y, t.Origin.X, t.Origin.Y };
						return true;
					}
				case Variant.Type.Transform3D:
					{
						var t = value.AsTransform3D();
						labels = new[] { "00", "01", "02", "10", "11", "12", "20", "21", "22", "OX", "OY", "OZ" };
						var m = BasisValues(t.Basis);
						values = new double[] { m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], t.Origin.X, t.Origin.Y, t.Origin.Z };
						return true;
					}
				case Variant.Type.Projection:
					{
						var p = value.AsProjection();
						labels = new[] { "XX", "XY", "XZ", "XW", "YX", "YY", "YZ", "YW", "ZX", "ZY", "ZZ", "ZW", "WX", "WY", "WZ", "WW" };
						values = new double[]
						{
							p.X.X, p.X.Y, p.X.Z, p.X.W, p.Y.X, p.Y.Y, p.Y.Z, p.Y.W,
							p.Z.X, p.Z.Y, p.Z.Z, p.Z.W, p.W.X, p.W.Y, p.W.Z, p.W.W,
						};
						return true;
					}
				default:
					labels = null; values = null; return false;
			}
		}

		/// <summary>Opération inverse de <see cref="TryDecompose"/> : reconstruit la structure de type <paramref name="type"/>.</summary>
		public static Variant Compose(Variant.Type type, double[] v) => type switch
		{
			Variant.Type.Vector2 => new Vector2((float)v[0], (float)v[1]),
			Variant.Type.Vector2I => new Vector2I((int)v[0], (int)v[1]),
			Variant.Type.Vector3 => new Vector3((float)v[0], (float)v[1], (float)v[2]),
			Variant.Type.Vector3I => new Vector3I((int)v[0], (int)v[1], (int)v[2]),
			Variant.Type.Vector4 => new Vector4((float)v[0], (float)v[1], (float)v[2], (float)v[3]),
			Variant.Type.Vector4I => new Vector4I((int)v[0], (int)v[1], (int)v[2], (int)v[3]),
			Variant.Type.Quaternion => new Quaternion((float)v[0], (float)v[1], (float)v[2], (float)v[3]),
			Variant.Type.Rect2 => new Rect2((float)v[0], (float)v[1], (float)v[2], (float)v[3]),
			Variant.Type.Rect2I => new Rect2I((int)v[0], (int)v[1], (int)v[2], (int)v[3]),
			Variant.Type.Plane => new Plane((float)v[0], (float)v[1], (float)v[2], (float)v[3]),
			Variant.Type.Aabb => new Aabb(new Vector3((float)v[0], (float)v[1], (float)v[2]),
				new Vector3((float)v[3], (float)v[4], (float)v[5])),
			Variant.Type.Basis => BasisFrom(v, 0),
			Variant.Type.Transform2D => new Transform2D(new Vector2((float)v[0], (float)v[1]), new Vector2((float)v[2], (float)v[3]),
				new Vector2((float)v[4], (float)v[5])),
			Variant.Type.Transform3D => new Transform3D(BasisFrom(v, 0), new Vector3((float)v[9], (float)v[10], (float)v[11])),
			Variant.Type.Projection => new Projection(
				new Vector4((float)v[0], (float)v[1], (float)v[2], (float)v[3]), new Vector4((float)v[4], (float)v[5], (float)v[6], (float)v[7]),
				new Vector4((float)v[8], (float)v[9], (float)v[10], (float)v[11]), new Vector4((float)v[12], (float)v[13], (float)v[14], (float)v[15])),
			_ => default,
		};

		private static double[] BasisValues(Basis b) => new double[]
		{
			b.Row0.X, b.Row0.Y, b.Row0.Z, b.Row1.X, b.Row1.Y, b.Row1.Z, b.Row2.X, b.Row2.Y, b.Row2.Z,
		};

		private static Basis BasisFrom(double[] v, int offset)
		{
			var b = Basis.Identity;
			b.Row0 = new Vector3((float)v[offset], (float)v[offset + 1], (float)v[offset + 2]);
			b.Row1 = new Vector3((float)v[offset + 3], (float)v[offset + 4], (float)v[offset + 5]);
			b.Row2 = new Vector3((float)v[offset + 6], (float)v[offset + 7], (float)v[offset + 8]);
			return b;
		}

		/// <summary>Vrai si le type est décomposable par <see cref="TryDecompose"/>.</summary>
		public static bool IsStructType(Variant.Type type) => type is
			Variant.Type.Vector2 or Variant.Type.Vector2I or Variant.Type.Vector3 or Variant.Type.Vector3I
			or Variant.Type.Vector4 or Variant.Type.Vector4I or Variant.Type.Quaternion or Variant.Type.Rect2
			or Variant.Type.Rect2I or Variant.Type.Plane or Variant.Type.Aabb or Variant.Type.Basis
			or Variant.Type.Transform2D or Variant.Type.Transform3D or Variant.Type.Projection;
		#endregion

		#region 🖼️ Petits widgets
		/// <summary>Crée un <see cref="SpinBox"/> compact avec un préfixe (nom du canal / de la composante).</summary>
		public static SpinBox MakeSpinBox(string prefix, double min, double max, double step, bool integer)
		{
			return new SpinBox
			{
				Prefix = prefix,
				MinValue = min,
				MaxValue = max,
				Step = step,
				Rounded = integer,
				AllowGreater = true,
				AllowLesser = true,
				SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
				CustomMinimumSize = new Vector2(64, 0),
			};
		}

		/// <summary>Crée le conteneur de contenu d'une section : <c>BoxContainer</c> ou <c>GridContainer</c>.</summary>
		public static Container MakeContentContainer(ContainerLayout layout, int columns, bool vertical) =>
			layout == ContainerLayout.Grid
				? new GridContainer { Columns = Math.Max(1, columns), SizeFlagsHorizontal = Control.SizeFlags.ExpandFill }
				: new BoxContainer { Vertical = vertical, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };

		/// <summary>
		/// Racine de scène utilisée pour résoudre / choisir des nœuds : scène éditée (éditeur), sinon scène courante,
		/// sinon racine de l'arbre.
		/// </summary>
		public static Node FindSceneRoot()
		{
#if TOOLS
			if (Engine.IsEditorHint() && EditorInterface.Singleton.GetEditedSceneRoot() is { } edited)
				return edited;
#endif
			return Engine.GetMainLoop() is SceneTree tree ? tree.CurrentScene ?? tree.Root : null;
		}

		/// <summary>Crée un <see cref="Label"/> atténué, utilisé pour les messages d'état.</summary>
		public static Label MakeHint(string text) =>
			new() { Text = text, Modulate = new Color(1, 1, 1, 0.6f), ClipText = true, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
		#endregion
	}
}
