using System;
using System.Collections.Generic;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🏗️ <b>Constructeur d'inspecteur</b><br/>
	/// Énumère les membres d'un objet (<c>GetPropertyList()</c>, <c>GetMethodList()</c>, <c>GetSignalList()</c>),
	/// résout un générateur pour chacun via le <see cref="PropertyWidgetRegistry"/> et ajoute les widgets à un conteneur.<br/>
	/// Partagé par <see cref="DynamicInspectorPanel"/> et les générateurs récursifs.
	/// </summary>
	public static class InspectorBuilder
	{
		/// <summary>
		/// Vide <paramref name="container"/> puis le remplit avec les widgets de <paramref name="target"/>.
		/// </summary>
		/// <param name="container">Conteneur destination.</param>
		/// <param name="target">Objet inspecté.</param>
		/// <param name="registry">Registre de générateurs (<c>null</c> = générateurs par défaut).</param>
		/// <param name="scope">Périmètre des membres.</param>
		/// <param name="properties">Affiche les propriétés.</param>
		/// <param name="methods">Affiche les méthodes sous forme de boutons.</param>
		/// <param name="signals">Affiche les signaux sous forme de boutons d'émission.</param>
		public static void Populate(Container container, GodotObject target, PropertyWidgetRegistry registry,
			InspectionScope scope = InspectionScope.Auto, bool properties = true, bool methods = false, bool signals = false)
		{
			Clear(container);
			if (!GodotObject.IsInstanceValid(target))
				return;

			registry ??= PropertyWidgetRegistry.CreateDefault();

			if (properties)
				AddSection(container, target, registry, MemberKind.Property, scope);
			if (methods)
				AddSection(container, target, registry, MemberKind.Method, scope);
			if (signals)
				AddSection(container, target, registry, MemberKind.Signal, scope);
		}

		/// <summary>Supprime tous les enfants de <paramref name="container"/>.</summary>
		public static void Clear(Node container)
		{
			foreach (Node child in container.GetChildren())
			{
				container.RemoveChild(child);
				child.QueueFree();
			}
		}

		/// <summary>
		/// Liste les descriptions de membres d'un objet selon le <paramref name="scope"/>.
		/// </summary>
		public static IEnumerable<GodotDict> GetMembers(GodotObject target, MemberKind kind, InspectionScope scope)
		{
			var script = target.GetScript().As<Script>();
			bool scriptOnly = scope == InspectionScope.ScriptOnly || (scope == InspectionScope.Auto && script != null);

			if (scriptOnly && script != null)
			{
				var list = kind switch
				{
					MemberKind.Method => script.GetScriptMethodList(),
					MemberKind.Signal => script.GetScriptSignalList(),
					_ => script.GetScriptPropertyList(),
				};
				if (list.Count > 0 || scope == InspectionScope.ScriptOnly)
					return list;
			}

			return kind switch
			{
				MemberKind.Method => target.GetMethodList(),
				MemberKind.Signal => target.GetSignalList(),
				_ => target.GetPropertyList(),
			};
		}

		private static void AddSection(Container container, GodotObject target, PropertyWidgetRegistry registry,
			MemberKind kind, InspectionScope scope)
		{
			var seen = new HashSet<string>();

			foreach (var info in GetMembers(target, kind, scope))
			{
				string name = WidgetUtils.NameOf(info);
				if (string.IsNullOrEmpty(name))
					continue;

				if (kind == MemberKind.Property)
				{
					var usage = WidgetUtils.UsageOf(info);
					if ((usage & (PropertyUsageFlags.Category | PropertyUsageFlags.Group | PropertyUsageFlags.Subgroup)) != 0)
					{
						container.AddChild(MakeHeader(name, (usage & PropertyUsageFlags.Category) != 0));
						continue;
					}
					if ((usage & PropertyUsageFlags.Editor) == 0 || name == "script")
						continue;
				}
				else if (name.StartsWith('_') || !seen.Add(name))
				{
					continue;
				}

				container.AddChild(BuildMember(target, registry, info, kind));
			}
		}

		private static Control BuildMember(GodotObject target, PropertyWidgetRegistry registry, GodotDict info, MemberKind kind)
		{
			string name = WidgetUtils.NameOf(info);
			try
			{
				return registry.BuildWidget(target, info, kind) ?? MakeFallback(target, name);
			}
			catch (Exception e)
			{
				GD.PushWarning($"[DynamicInspector] Widget « {name} » impossible à générer : {e.Message}");
				return WidgetUtils.MakeHint($"⚠ {name} : {e.Message}");
			}
		}

		/// <summary>Widget lecture seule pour les membres sans générateur (packed arrays, Rid, Transform...).</summary>
		public static Control MakeFallback(GodotObject target, string name)
		{
			var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			row.AddChild(new Label { Text = PropertyWidgetGenerator.DisplayName(target, name), SizeFlagsHorizontal = Control.SizeFlags.ExpandFill, SizeFlagsStretchRatio = 0.4f });
			var value = WidgetUtils.MakeHint(GD.VarToStr(PropertyWidgetGenerator.ReadValue(target, name)));
			value.SizeFlagsStretchRatio = 0.6f;
			row.AddChild(value);
			return row;
		}

		private static Control MakeHeader(string text, bool category)
		{
			var box = new VBoxContainer();
			var label = new Label { Text = category ? text.ToUpperInvariant() : text };
			if (category)
			{
				box.AddChild(new HSeparator());
				label.HorizontalAlignment = HorizontalAlignment.Center;
			}
			else
			{
				label.Modulate = new Color(1, 1, 1, 0.7f);
			}
			box.AddChild(label);
			return box;
		}
	}
}
