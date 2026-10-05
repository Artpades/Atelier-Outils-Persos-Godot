using System;
using System.Collections.Generic;
using Godot;
using GodotArray = Godot.Collections.Array;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Une méthode <c>Gizmos.DrawXxx</c> utilisable : ses paramètres et comment l'appeler.</summary>
	public sealed class GizmoMethodSpec
	{
		/// <summary>Nom de la méthode (ex : <c>Draw3DSphere</c>).</summary>
		public string Name { get; init; }

		/// <summary>Vrai pour les méthodes 3D (<c>Draw3D...</c>), faux pour les méthodes 2D (<c>Draw2D...</c>).</summary>
		public bool Is3D { get; init; }

		/// <summary>Paramètres, dans l'ordre d'appel (hors le <see cref="Rid"/> du gizmo, fourni par le pilote).</summary>
		public GizmoParamSpec[] Parameters { get; init; }

		/// <summary>Appelle la méthode réelle avec les valeurs déjà résolues (une par entrée de <see cref="Parameters"/>).</summary>
		public Action<Rid, Variant[]> Invoke { get; init; }
	}

	/// <summary>
	/// 📚 <b>Catalogue des méthodes de dessin de <see cref="Gizmos"/></b> exposées à la configuration d'interface.<br/>
	/// <see cref="Gizmos"/> est une classe C# statique <b>sans introspection Godot</b> (ce n'est pas un
	/// <see cref="GodotObject"/>) : ses méthodes et leurs paramètres sont donc décrits <b>à la main</b> ici, un par un,
	/// transcrits depuis les signatures réelles de <c>Gizmos_2D.cs</c> / <c>Gizmos_3D.cs</c>.<br/>
	/// <br/>
	/// <b>📍 Deux catégories de paramètre</b>, gérées uniformément avec les widgets déjà utilisés dans le reste de
	/// l'inspecteur :<br/>
	/// • valeur normale (nombre, vecteur, couleur, texte, texture, tableau de points...) — widget habituel ;<br/>
	/// • paramètre <c>Nullable</c> côté C# (ex : <c>Color? color = null</c>) — une case « valeur par défaut de la
	/// méthode » s'ajoute, qui revient à ne pas fournir l'argument.
	/// </summary>
	public static class GizmoMethodCatalog
	{
		private static Dictionary<string, GizmoMethodSpec> _byName;

		/// <summary>Toutes les méthodes disponibles, dans un ordre stable (2D puis 3D, dans l'ordre du fichier source).</summary>
		public static IReadOnlyList<GizmoMethodSpec> All { get; } = Build();

		/// <summary>Retrouve une méthode par son nom, ou <c>null</c>.</summary>
		public static GizmoMethodSpec Find(string name)
		{
			if (string.IsNullOrEmpty(name))
				return null;
			_byName ??= BuildIndex();
			return _byName.TryGetValue(name, out var spec) ? spec : null;
		}

		private static Dictionary<string, GizmoMethodSpec> BuildIndex()
		{
			var map = new Dictionary<string, GizmoMethodSpec>();
			foreach (var spec in All)
				map[spec.Name] = spec;
			return map;
		}

		#region 🧮 Aides de déclaration
		private static GizmoParamSpec P(string name, Variant.Type type, Variant def,
			bool nullable = false, PropertyHint hint = PropertyHint.None, string hintString = "") => new()
		{
			Name = name,
			Type = type,
			Default = def,
			Nullable = nullable,
			Hint = hint,
			HintString = hintString,
		};

		/// <summary>Paramètre <c>Vector2[]</c> / <c>Vector3[]</c> : tableau typé (édité via <see cref="CollectionWidgetGenerator"/>).</summary>
		private static GizmoParamSpec PArray(string name, Variant.Type elementType) => new()
		{
			Name = name,
			Type = Variant.Type.Array,
			Default = new GodotArray(),
			Hint = PropertyHint.TypeString,
			HintString = $"{(int)elementType}/0:",
		};

		/// <summary>Paramètre objet requis (texture) : aucune valeur de repli sensée, l'appel est ignoré si non résolu.</summary>
		private static GizmoParamSpec PTexture(string name) => new()
		{
			Name = name,
			Type = Variant.Type.Object,
			ClassName = "Texture2D",
			Required = true,
		};

		/// <summary>Paramètre énuméré depuis un <c>enum</c> C# (filtre de texture...).</summary>
		private static GizmoParamSpec PEnum(string name, Type enumType, int defaultValue) => new()
		{
			Name = name,
			Type = Variant.Type.Int,
			Hint = PropertyHint.Enum,
			HintString = EnumHint(enumType),
			Default = defaultValue,
		};

		private static string EnumHint(Type enumType)
		{
			var names = Enum.GetNames(enumType);
			var values = Enum.GetValues(enumType);
			var parts = new string[names.Length];
			for (int i = 0; i < names.Length; i++)
				// values.GetValue(i) est boxé comme le type enum concret : un cast direct en int (unboxing) lève
				// une InvalidCastException. Convert.ToInt32 passe par IConvertible et fonctionne pour tout enum.
				parts[i] = $"{names[i]}:{Convert.ToInt32(values.GetValue(i))}";
			return string.Join(",", parts);
		}

		private static Color? Col(Variant v) => v.VariantType == Variant.Type.Nil ? null : v.AsColor();
		private static Vector2? Vec2N(Variant v) => v.VariantType == Variant.Type.Nil ? null : v.AsVector2();
		private static Vector3? Vec3N(Variant v) => v.VariantType == Variant.Type.Nil ? null : v.AsVector3();
		private static Transform2D? Tf2N(Variant v) => v.VariantType == Variant.Type.Nil ? null : v.AsTransform2D();
		private static Transform3D? Tf3N(Variant v) => v.VariantType == Variant.Type.Nil ? null : v.AsTransform3D();
		private static Texture2D Tex(Variant v) => v.VariantType == Variant.Type.Object ? v.AsGodotObject() as Texture2D : null;

		private static Vector2[] Vec2Arr(Variant v)
		{
			if (v.VariantType != Variant.Type.Array) return Array.Empty<Vector2>();
			var a = v.AsGodotArray();
			var result = new Vector2[a.Count];
			for (int i = 0; i < a.Count; i++) result[i] = a[i].AsVector2();
			return result;
		}

		private static Vector3[] Vec3Arr(Variant v)
		{
			if (v.VariantType != Variant.Type.Array) return Array.Empty<Vector3>();
			var a = v.AsGodotArray();
			var result = new Vector3[a.Count];
			for (int i = 0; i < a.Count; i++) result[i] = a[i].AsVector3();
			return result;
		}
		#endregion

		private static List<GizmoMethodSpec> Build()
		{
			var list = new List<GizmoMethodSpec>();

			// ============================== 2D ==============================
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DLine",
				Parameters = new[]
				{
					P("from", Variant.Type.Vector2, Vector2.Zero), P("to", Variant.Type.Vector2, Vector2.Zero),
					P("width", Variant.Type.Float, 2.0), P("color", Variant.Type.Color, Colors.White, nullable: true),
					P("antialiasing", Variant.Type.Bool, false),
				},
				Invoke = (rid, a) => Gizmos.Draw2DLine(rid, a[0].AsVector2(), a[1].AsVector2(), a[2].AsSingle(), Col(a[3]), a[4].AsBool()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DArrow",
				Parameters = new[]
				{
					P("from", Variant.Type.Vector2, Vector2.Zero), P("to", Variant.Type.Vector2, Vector2.Right * 32),
					P("arrowHeadSize", Variant.Type.Float, 12.0), P("width", Variant.Type.Float, 1.0),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("antialiasing", Variant.Type.Bool, false),
				},
				Invoke = (rid, a) => Gizmos.Draw2DArrow(rid, a[0].AsVector2(), a[1].AsVector2(), a[2].AsSingle(), a[3].AsSingle(), Col(a[4]), a[5].AsBool()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DCircle",
				Parameters = new[]
				{
					P("center", Variant.Type.Vector2, Vector2.Zero), P("radius", Variant.Type.Float, 16.0),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("filled", Variant.Type.Bool, false),
					P("thickness", Variant.Type.Float, 2.0), P("antialiasing", Variant.Type.Bool, false),
				},
				Invoke = (rid, a) => Gizmos.Draw2DCircle(rid, a[0].AsVector2(), a[1].AsSingle(), Col(a[2]), a[3].AsBool(), a[4].AsSingle(), a[5].AsBool()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DRectangle",
				Parameters = new[]
				{
					P("rect", Variant.Type.Rect2, new Rect2(0, 0, 32, 32)),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("filled", Variant.Type.Bool, false),
					P("thickness", Variant.Type.Float, 2.0), P("antialiasing", Variant.Type.Bool, false),
				},
				Invoke = (rid, a) => Gizmos.Draw2DRectangle(rid, a[0].AsRect2(), Col(a[1]), a[2].AsBool(), a[3].AsSingle(), a[4].AsBool()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DCapsule",
				Parameters = new[]
				{
					P("position", Variant.Type.Vector2, Vector2.Zero), P("size", Variant.Type.Vector2, new Vector2(16, 32)),
					P("color", Variant.Type.Color, Colors.White, nullable: true),
					P("thickness", Variant.Type.Float, 2.0), P("antialiasing", Variant.Type.Bool, false),
				},
				Invoke = (rid, a) => Gizmos.Draw2DCapsule(rid, a[0].AsVector2(), a[1].AsVector2(), Col(a[2]), a[3].AsSingle(), a[4].AsBool()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DPolygon",
				Parameters = new[]
				{
					PArray("points", Variant.Type.Vector2), P("color", Variant.Type.Color, Colors.White, nullable: true),
					P("filled", Variant.Type.Bool, false), P("width", Variant.Type.Float, 2.0), P("antialiasing", Variant.Type.Bool, false),
				},
				Invoke = (rid, a) => Gizmos.Draw2DPolygon(rid, Vec2Arr(a[0]), Col(a[1]), a[2].AsBool(), a[3].AsSingle(), a[4].AsBool()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DPath",
				Parameters = new[]
				{
					PArray("points", Variant.Type.Vector2), P("asArrows", Variant.Type.Bool, false),
					P("segmentsPerLine", Variant.Type.Int, 1L), P("spacing", Variant.Type.Float, 0.0),
					P("useBezier", Variant.Type.Bool, false), P("showPoints", Variant.Type.Bool, false),
					P("pointRadius", Variant.Type.Float, 4.0), P("width", Variant.Type.Float, 2.0),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("arrowHeadSize", Variant.Type.Float, 12.0),
					P("antialiasing", Variant.Type.Bool, false),
				},
				Invoke = (rid, a) => Gizmos.Draw2DPath(rid, Vec2Arr(a[0]), a[1].AsBool(), a[2].AsInt32(), a[3].AsSingle(),
					a[4].AsBool(), a[5].AsBool(), a[6].AsSingle(), a[7].AsSingle(), Col(a[8]), a[9].AsSingle(), a[10].AsBool()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DOrigin",
				Parameters = new[]
				{
					P("transform", Variant.Type.Transform2D, Transform2D.Identity, nullable: true),
					P("length", Variant.Type.Float, 32.0), P("arrowHeadSize", Variant.Type.Float, 8.0),
					P("crossSize", Variant.Type.Float, 8.0), P("width", Variant.Type.Float, 2.0),
					P("crossColor", Variant.Type.Color, Colors.White, nullable: true), P("antialiasing", Variant.Type.Bool, false),
				},
				Invoke = (rid, a) => Gizmos.Draw2DOrigin(rid, Tf2N(a[0]), a[1].AsSingle(), a[2].AsSingle(), a[3].AsSingle(), a[4].AsSingle(), Col(a[5]), a[6].AsBool()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DIcon",
				Parameters = new[]
				{
					P("position", Variant.Type.Vector2, Vector2.Zero), PTexture("icon"),
					P("size", Variant.Type.Vector2, Vector2.Zero),
					PEnum("filter", typeof(RenderingServer.CanvasItemTextureFilter), (int)RenderingServer.CanvasItemTextureFilter.Nearest),
				},
				Invoke = (rid, a) => Gizmos.Draw2DIcon(rid, a[0].AsVector2(), Tex(a[1]), a[2].AsVector2(), (RenderingServer.CanvasItemTextureFilter)a[3].AsInt32()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw2DText",
				Parameters = new[]
				{
					P("position", Variant.Type.Vector2, Vector2.Zero), P("text", Variant.Type.String, ""),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("fontSize", Variant.Type.Int, 12L),
				},
				Invoke = (rid, a) => Gizmos.Draw2DText(rid, a[0].AsVector2(), a[1].AsString(), Col(a[2]), a[3].AsInt32()),
			});

			// ============================== 3D ==============================
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DLine", Is3D = true,
				Parameters = new[]
				{
					P("from", Variant.Type.Vector3, Vector3.Zero), P("to", Variant.Type.Vector3, Vector3.Zero),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("width", Variant.Type.Float, 1.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DLine(rid, a[0].AsVector3(), a[1].AsVector3(), Col(a[2]), a[3].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DArrow", Is3D = true,
				Parameters = new[]
				{
					P("from", Variant.Type.Vector3, Vector3.Zero), P("to", Variant.Type.Vector3, Vector3.Up),
					P("color", Variant.Type.Color, Colors.White, nullable: true),
					P("headSize", Variant.Type.Float, 0.3), P("width", Variant.Type.Float, 1.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DArrow(rid, a[0].AsVector3(), a[1].AsVector3(), Col(a[2]), a[3].AsSingle(), a[4].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DCube", Is3D = true,
				Parameters = new[]
				{
					P("center", Variant.Type.Vector3, Vector3.Zero), P("size", Variant.Type.Vector3, Vector3.One),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("filled", Variant.Type.Bool, false),
					P("width", Variant.Type.Float, 1.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DCube(rid, a[0].AsVector3(), a[1].AsVector3(), Col(a[2]), a[3].AsBool(), a[4].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DSphere", Is3D = true,
				Parameters = new[]
				{
					P("center", Variant.Type.Vector3, Vector3.Zero), P("radius", Variant.Type.Float, 0.5),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("filled", Variant.Type.Bool, false),
					P("segments", Variant.Type.Int, 16L), P("width", Variant.Type.Float, 1.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DSphere(rid, a[0].AsVector3(), a[1].AsSingle(), Col(a[2]), a[3].AsBool(), a[4].AsInt32(), a[5].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DCircle", Is3D = true,
				Parameters = new[]
				{
					P("center", Variant.Type.Vector3, Vector3.Zero), P("radius", Variant.Type.Float, 0.5),
					P("normal", Variant.Type.Vector3, Vector3.Up, nullable: true),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("filled", Variant.Type.Bool, false),
					P("segments", Variant.Type.Int, 24L), P("width", Variant.Type.Float, 1.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DCircle(rid, a[0].AsVector3(), a[1].AsSingle(), Vec3N(a[2]), Col(a[3]), a[4].AsBool(), a[5].AsInt32(), a[6].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DRectangle", Is3D = true,
				Parameters = new[]
				{
					P("center", Variant.Type.Vector3, Vector3.Zero), P("size", Variant.Type.Vector2, Vector2.One),
					P("normal", Variant.Type.Vector3, Vector3.Up, nullable: true),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("filled", Variant.Type.Bool, false),
					P("width", Variant.Type.Float, 1.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DRectangle(rid, a[0].AsVector3(), a[1].AsVector2(), Vec3N(a[2]), Col(a[3]), a[4].AsBool(), a[5].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DCapsule", Is3D = true,
				Parameters = new[]
				{
					P("center", Variant.Type.Vector3, Vector3.Zero), P("radius", Variant.Type.Float, 0.5),
					P("height", Variant.Type.Float, 2.0), P("color", Variant.Type.Color, Colors.White, nullable: true),
					P("filled", Variant.Type.Bool, false), P("width", Variant.Type.Float, 1.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DCapsule(rid, a[0].AsVector3(), a[1].AsSingle(), a[2].AsSingle(), Col(a[3]), a[4].AsBool(), a[5].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DPolygon", Is3D = true,
				Parameters = new[]
				{
					PArray("points", Variant.Type.Vector3), P("color", Variant.Type.Color, Colors.White, nullable: true),
					P("filled", Variant.Type.Bool, false), P("width", Variant.Type.Float, 1.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DPolygon(rid, Vec3Arr(a[0]), Col(a[1]), a[2].AsBool(), a[3].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DPath", Is3D = true,
				Parameters = new[]
				{
					PArray("points", Variant.Type.Vector3), P("asArrows", Variant.Type.Bool, false),
					P("segmentsPerLine", Variant.Type.Int, 1L), P("spacing", Variant.Type.Float, 0.0),
					P("useBezier", Variant.Type.Bool, false), P("showPoints", Variant.Type.Bool, false),
					P("pointRadius", Variant.Type.Float, 0.1), P("width", Variant.Type.Float, 2.0),
					P("color", Variant.Type.Color, Colors.White, nullable: true), P("arrowHeadSize", Variant.Type.Float, 12.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DPath(rid, Vec3Arr(a[0]), a[1].AsBool(), a[2].AsInt32(), a[3].AsSingle(),
					a[4].AsBool(), a[5].AsBool(), a[6].AsSingle(), a[7].AsSingle(), Col(a[8]), a[9].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DOrigin", Is3D = true,
				Parameters = new[]
				{
					P("transform", Variant.Type.Transform3D, Transform3D.Identity, nullable: true),
					P("size", Variant.Type.Float, 1.0), P("headSize", Variant.Type.Float, 0.2), P("width", Variant.Type.Float, 2.0),
				},
				Invoke = (rid, a) => Gizmos.Draw3DOrigin(rid, Tf3N(a[0]), a[1].AsSingle(), a[2].AsSingle(), a[3].AsSingle()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DCameraFrustum", Is3D = true,
				Parameters = new[]
				{
					P("transform", Variant.Type.Transform3D, Transform3D.Identity),
					P("fov", Variant.Type.Float, 75.0), P("zNear", Variant.Type.Float, 0.05),
					P("zFar", Variant.Type.Float, 100.0), P("aspect", Variant.Type.Float, 1.777),
					P("color", Variant.Type.Color, Colors.White, nullable: true),
				},
				Invoke = (rid, a) => Gizmos.Draw3DCameraFrustum(rid, a[0].AsTransform3D(), a[1].AsSingle(), a[2].AsSingle(), a[3].AsSingle(), a[4].AsSingle(), Col(a[5])),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DIcon", Is3D = true,
				Parameters = new[]
				{
					P("position", Variant.Type.Vector3, Vector3.Zero), PTexture("texture"),
					P("size", Variant.Type.Vector2, Vector2.One, nullable: true),
					PEnum("filter", typeof(BaseMaterial3D.TextureFilterEnum), (int)BaseMaterial3D.TextureFilterEnum.Nearest),
				},
				Invoke = (rid, a) => Gizmos.Draw3DIcon(rid, a[0].AsVector3(), Tex(a[1]), Vec2N(a[2]), (BaseMaterial3D.TextureFilterEnum)a[3].AsInt32()),
			});
			list.Add(new GizmoMethodSpec
			{
				Name = "Draw3DText", Is3D = true,
				Parameters = new[]
				{
					P("position", Variant.Type.Vector3, Vector3.Zero), P("text", Variant.Type.String, ""),
					P("color", Variant.Type.Color, Colors.White, nullable: true),
					P("fontSize", Variant.Type.Int, 32L), P("pixelSize", Variant.Type.Float, 0.01),
				},
				Invoke = (rid, a) => Gizmos.Draw3DText(rid, a[0].AsVector3(), a[1].AsString(), Col(a[2]), a[3].AsInt32(), a[4].AsSingle()),
			});

			return list;
		}
	}
}
