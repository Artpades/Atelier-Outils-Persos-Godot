using System;
using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🧱 <b>Constructeur d'interface configurée</b><br/>
	/// Génère l'interface d'un objet à partir d'un <see cref="DataRegistry"/> (quoi afficher, avec quel widget) et d'un
	/// <see cref="GroupRegistry"/> (dans quels conteneurs).<br/>
	/// <br/>
	/// <b>📍 Ordre</b> : les données suivent l'ordre du <see cref="DataRegistry"/>. Un groupe apparaît à la place de sa
	/// <b>première</b> donnée et rassemble toutes ses données ; les données sans groupe restent à la racine.<br/>
	/// Tous les widgets sont liés à la valeur en mémoire (voir <see cref="PropertyBinding"/>).
	/// </summary>
	public static class ConfiguredInterfaceBuilder
	{
		/// <summary>Vide <paramref name="container"/> puis y génère l'interface configurée de <paramref name="target"/>.</summary>
		/// <param name="container">Conteneur racine (dans le <see cref="DynamicInspectorPanel"/> : le <c>VBoxContainer</c> du scroll).</param>
		/// <param name="target">Objet inspecté.</param>
		/// <param name="data">Registre des données.</param>
		/// <param name="groups">Registre des groupes (optionnel).</param>
		/// <param name="registry">Registre utilisé pour le contenu <b>récursif</b> (objets, tableaux). <c>null</c> : défauts.</param>
		/// <param name="effects">Registre d'effets (optionnel) appliqué après la génération (ex : visibilité conditionnelle).</param>
		public static void Populate(Container container, GodotObject target, DataRegistry data, GroupRegistry groups,
			PropertyWidgetRegistry registry = null, EffectRegistry effects = null)
		{
			InspectorBuilder.Clear(container);
			if (!GodotObject.IsInstanceValid(target) || data == null)
				return;

			registry ??= PropertyWidgetRegistry.CreateDefault();

			var groupOf = MapGroups(groups);
			var built = new Dictionary<GroupEntry, (Container Inner, List<(string Name, Control Widget)> Members)>();
			var widgetsByName = new Dictionary<string, Control>();

			foreach (var entry in data.Entries)
			{
				if (entry == null || !entry.Enabled || entry.IsMissing)
					continue;

				var widget = BuildEntry(target, entry, registry);
				if (widget == null)
					continue;
				widgetsByName[entry.DataName] = widget;

				if (groupOf.TryGetValue(entry.DataName, out var group))
				{
					if (!built.TryGetValue(group, out var slot))
					{
						container.AddChild(CreateGroupContainer(group, out var inner));
						slot = (inner, new List<(string, Control)>());
						built[group] = slot;
					}
					slot.Inner.AddChild(widget);
					slot.Members.Add((entry.DataName, widget));
				}
				else
				{
					container.AddChild(widget);
				}
			}

			OrderGroupMembers(built);
			effects?.Apply(container, target, widgetsByName);
		}

		/// <summary>Associe chaque nom de donnée à son premier groupe.</summary>
		private static Dictionary<string, GroupEntry> MapGroups(GroupRegistry groups)
		{
			var map = new Dictionary<string, GroupEntry>();
			if (groups == null)
				return map;

			foreach (var group in groups.Groups)
				if (group != null)
					foreach (var name in group.TargetDataNames)
						map.TryAdd(name, group);
			return map;
		}

		/// <summary>Réordonne les widgets de chaque groupe selon <see cref="GroupEntry.TargetDataNames"/>.</summary>
		private static void OrderGroupMembers(Dictionary<GroupEntry, (Container Inner, List<(string Name, Control Widget)> Members)> built)
		{
			foreach (var (group, slot) in built)
			{
				slot.Members.Sort((a, b) => group.TargetDataNames.IndexOf(a.Name).CompareTo(group.TargetDataNames.IndexOf(b.Name)));
				for (int i = 0; i < slot.Members.Count; i++)
					slot.Inner.MoveChild(slot.Members[i].Widget, i);
			}
		}

		private static Control BuildEntry(GodotObject target, DataEntry entry, PropertyWidgetRegistry registry)
		{
			var info = PropertyWidgetGenerator.GetMemberInfo(target, entry.DataName, entry.Member);
			if (info.Count == 0)
				return null;

			try
			{
				var generator = EntryGeneratorFactory.Create(entry);
				if (generator == null)
				{
					return entry.ResolvedChoice() == WidgetChoice.Custom && entry.Kind != DataKind.Unsupported
						? WidgetUtils.MakeHint($"⚠ {entry.DataName} : aucun générateur personnalisé")
						: InspectorBuilder.MakeFallback(target, entry.DataName);
				}
				return generator.GenerateWidget(target, entry.DataName, registry);
			}
			catch (Exception e)
			{
				GD.PushWarning($"[DynamicInspector] « {entry.DataName} » impossible à générer : {e.Message}");
				return WidgetUtils.MakeHint($"⚠ {entry.DataName} : {e.Message}");
			}
		}

		/// <summary>Crée le conteneur d'un groupe ; <paramref name="inner"/> reçoit les widgets.</summary>
		private static Control CreateGroupContainer(GroupEntry group, out Container inner)
		{
			var root = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			if (!string.IsNullOrEmpty(group.GroupName))
				root.AddChild(new Label { Text = group.GroupName, Modulate = new Color(1, 1, 1, 0.75f) });

			Container box = group.ContainerType == GroupContainerType.GridContainer
				? new GridContainer { Columns = Math.Max(1, group.Columns) }
				: new BoxContainer { Vertical = group.IsVertical };
			box.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;

			if (group.ContainerType == GroupContainerType.PanelContainer)
			{
				var panel = new PanelContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				var margin = new MarginContainer();
				foreach (var side in new[] { "left", "top", "right", "bottom" })
					margin.AddThemeConstantOverride($"margin_{side}", 6);
				margin.AddChild(box);
				panel.AddChild(margin);
				root.AddChild(panel);
			}
			else
			{
				root.AddChild(box);
			}

			inner = box;
			return root;
		}
	}
}
