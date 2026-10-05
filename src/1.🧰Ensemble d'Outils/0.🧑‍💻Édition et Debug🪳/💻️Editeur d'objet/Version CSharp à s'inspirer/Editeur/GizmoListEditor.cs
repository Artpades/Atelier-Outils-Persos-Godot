#if TOOLS
using System;
using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📊 <b>Éditeur des gizmos</b> : une carte par <see cref="GizmoEntry"/> (méthode <see cref="Gizmos"/> choisie
	/// dans <see cref="GizmoMethodCatalog"/>, activation, suppression) et, pour chaque paramètre de la méthode, une
	/// ligne qui bascule entre <b>référencer une propriété</b> (champ de nom + statut ✔/✘) et <b>saisir une valeur</b>
	/// (widget habituel de l'inspecteur — les paramètres <c>Nullable</c> côté C# ont en plus une case « valeur par
	/// défaut de la méthode »).
	/// </summary>
	public sealed class GizmoListEditor
	{
		private readonly GizmoRegistry _gizmos;
		private readonly GodotObject _target;
		private readonly Action _changed;
		private readonly PropertyWidgetRegistry _registry = PropertyWidgetRegistry.CreateDefault();
		private readonly VBoxContainer _list = new() { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };

		/// <summary>Contrôle racine (titre + liste défilante + bouton d'ajout).</summary>
		public Control Root { get; }

		/// <param name="gizmos">Registre édité.</param>
		/// <param name="target">Cible inspectée, pour vérifier les propriétés référencées.</param>
		/// <param name="changed">Appelé après chaque modification.</param>
		public GizmoListEditor(GizmoRegistry gizmos, GodotObject target, Action changed)
		{
			_gizmos = gizmos;
			_target = target;
			_changed = changed;

			var scroll = new ScrollContainer { SizeFlagsVertical = Control.SizeFlags.ExpandFill, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			scroll.AddChild(_list);

			var add = new Button { Text = "+ Ajouter un gizmo" };
			add.Pressed += () =>
			{
				_gizmos.Entries.Add(new GizmoEntry());
				_changed();
				Refresh();
			};

			var root = new VBoxContainer { Name = "Gizmos", SizeFlagsVertical = Control.SizeFlags.ExpandFill };
			root.AddChild(new Label { Text = "Registre des gizmos", ThemeTypeVariation = "HeaderSmall" });
			root.AddChild(WidgetUtils.MakeHint("Dessine des formes dans l'espace 2D/3D à partir des données de l'objet, à chaque image (voir GizmoDriver)."));
			if (!GodotObject.IsInstanceValid(_target))
				root.AddChild(new Label { Text = "⚠ Aucune cible : les propriétés référencées ne peuvent pas être vérifiées.", Modulate = new Color(1, 0.7f, 0.4f) });
			root.AddChild(scroll);
			root.AddChild(add);
			Root = root;

			Refresh();
		}

		/// <summary>Reconstruit toute la liste.</summary>
		public void Refresh()
		{
			InspectorBuilder.Clear(_list);
			for (int i = 0; i < _gizmos.Entries.Count; i++)
				if (_gizmos.Entries[i] != null)
					_list.AddChild(BuildCard(i, _gizmos.Entries[i]));
		}

		private Control BuildCard(int index, GizmoEntry entry)
		{
			var spec = GizmoMethodCatalog.Find(entry.GizmoMethod);
			var body = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };

			// ---- en-tête : actif, méthode, suppression
			var enabled = new CheckBox { ButtonPressed = entry.Enabled, TooltipText = "Actif" };
			enabled.Toggled += on => { entry.Enabled = on; _changed(); };

			var methods = GizmoMethodCatalog.All;
			var method = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			method.AddItem("‹ choisir une méthode ›");
			foreach (var candidate in methods)
				method.AddItem((candidate.Is3D ? "3D · " : "2D · ") + candidate.Name);

			int currentIndex = spec == null ? -1 : IndexOfMethod(methods, spec.Name);
			method.Select(currentIndex + 1);
			method.ItemSelected += selected =>
			{
				int i = (int)selected - 1;
				var chosen = i < 0 ? null : methods[i];
				entry.GizmoMethod = chosen?.Name ?? "";
				ResetParameters(entry, chosen);
				_changed();
				Refresh();
			};

			var delete = new Button { Text = "✕", TooltipText = "Supprimer le gizmo" };
			delete.Pressed += () =>
			{
				_gizmos.Entries.RemoveAt(index);
				_changed();
				Refresh();
			};

			var header = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			header.AddChild(enabled);
			header.AddChild(method);
			header.AddChild(delete);
			body.AddChild(header);

			// ---- paramètres
			if (spec == null)
			{
				body.AddChild(WidgetUtils.MakeHint("Choisissez une méthode pour configurer ses paramètres."));
			}
			else
			{
				EnsureParameterCount(entry, spec);
				for (int i = 0; i < spec.Parameters.Length; i++)
					body.AddChild(BuildParameterRow(entry, spec, i));
			}

			var panel = new PanelContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			var margin = new MarginContainer();
			foreach (var side in new[] { "left", "top", "right", "bottom" })
				margin.AddThemeConstantOverride($"margin_{side}", 6);
			margin.AddChild(body);
			panel.AddChild(margin);
			return panel;
		}

		/// <summary>Une ligne par paramètre : nom + bascule « Propriété », puis le champ de référence ou le widget de saisie.</summary>
		private Control BuildParameterRow(GizmoEntry entry, GizmoMethodSpec spec, int index)
		{
			var paramSpec = spec.Parameters[index];
			var parameter = entry.Parameters[index];

			var row = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			var top = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			top.AddChild(new Label
			{
				Text = paramSpec.Name,
				TooltipText = paramSpec.Type + (paramSpec.Required ? " (requis)" : ""),
				CustomMinimumSize = new Vector2(110, 0),
			});

			var fromProperty = new CheckBox { Text = "Propriété", ButtonPressed = parameter.FromProperty, TooltipText = "Référencer une propriété plutôt que saisir une valeur" };
			top.AddChild(fromProperty);
			row.AddChild(top);

			var valueSlot = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			row.AddChild(valueSlot);

			void BuildValueSlot()
			{
				InspectorBuilder.Clear(valueSlot);

				if (parameter.FromProperty)
				{
					var edit = new LineEdit { Text = parameter.TargetProperty, PlaceholderText = "nom de la propriété", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
					var status = WidgetUtils.MakeHint(DescribeReference(_target, parameter.TargetProperty, paramSpec));
					status.CustomMinimumSize = new Vector2(150, 0);

					void Commit()
					{
						if (edit.Text == parameter.TargetProperty)
							return;
						parameter.TargetProperty = edit.Text;
						status.Text = DescribeReference(_target, parameter.TargetProperty, paramSpec);
						_changed();
					}
					edit.TextSubmitted += _ => Commit();
					edit.FocusExited += Commit;

					var line = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
					line.AddChild(edit);
					line.AddChild(status);
					valueSlot.AddChild(line);
					return;
				}

				if (paramSpec.Nullable)
				{
					var useDefault = new CheckBox { Text = "Valeur par défaut de la méthode", ButtonPressed = parameter.UseMethodDefault };
					useDefault.Toggled += on =>
					{
						parameter.UseMethodDefault = on;
						_changed();
						BuildValueSlot();
					};
					valueSlot.AddChild(useDefault);
					if (parameter.UseMethodDefault)
						return;
				}

				valueSlot.AddChild(BuildValueWidget(paramSpec, parameter));
			}

			fromProperty.Toggled += on =>
			{
				parameter.FromProperty = on;
				_changed();
				BuildValueSlot();
			};

			BuildValueSlot();
			return row;
		}

		/// <summary>Widget de saisie manuelle, via le même système que le reste de l'inspecteur (<see cref="ValueProxy"/>).</summary>
		private Control BuildValueWidget(GizmoParamSpec paramSpec, GizmoParameter parameter)
		{
			var proxy = new ValueProxy(
				() => parameter.Value.VariantType == paramSpec.Type ? parameter.Value : paramSpec.Default,
				v => { parameter.Value = v; _changed(); },
				paramSpec.Type, "", paramSpec.Hint, paramSpec.HintString, readOnly: false, className: paramSpec.ClassName);

			proxy.TryGetMemberInfo(ValueProxy.ValueName, MemberKind.Property, out var info);

			// Un paramètre objet (texture) utilise le sélecteur de ressource plutôt que le registre par défaut,
			// qui n'a pas de générateur pour Object hors DataRegistry.
			var widget = paramSpec.Type == Variant.Type.Object
				? new PathButtonWidgetGenerator().GenerateWidget(proxy, ValueProxy.ValueName)
				: _registry.BuildWidget(proxy, info);
			return widget ?? WidgetUtils.MakeHint("(type non pris en charge)");
		}

		/// <summary>Statut ✔/✘ d'une propriété référencée pour un paramètre donné.</summary>
		private static string DescribeReference(GodotObject target, string propertyName, GizmoParamSpec spec)
		{
			if (string.IsNullOrEmpty(propertyName) || !GodotObject.IsInstanceValid(target))
				return "";

			var info = PropertyWidgetGenerator.GetMemberInfo(target, propertyName, MemberKind.Property);
			if (info.Count == 0)
				return "✘ introuvable";

			var type = WidgetUtils.TypeOf(info);
			bool numericMatch = type is Variant.Type.Int or Variant.Type.Float && spec.Type is Variant.Type.Int or Variant.Type.Float;
			return type == spec.Type || numericMatch ? "✔ compatible" : $"✘ {type}, attendu {spec.Type}";
		}

		private static void ResetParameters(GizmoEntry entry, GizmoMethodSpec spec)
		{
			entry.Parameters.Clear();
			if (spec == null)
				return;
			foreach (var paramSpec in spec.Parameters)
				entry.Parameters.Add(new GizmoParameter { Value = paramSpec.Default, UseMethodDefault = true });
		}

		/// <summary>Filet de sécurité si le nombre de paramètres stocké ne correspond plus à la méthode (fichier ancien...).</summary>
		private static void EnsureParameterCount(GizmoEntry entry, GizmoMethodSpec spec)
		{
			while (entry.Parameters.Count < spec.Parameters.Length)
				entry.Parameters.Add(new GizmoParameter { Value = spec.Parameters[entry.Parameters.Count].Default, UseMethodDefault = true });
			while (entry.Parameters.Count > spec.Parameters.Length)
				entry.Parameters.RemoveAt(entry.Parameters.Count - 1);
		}

		private static int IndexOfMethod(IReadOnlyList<GizmoMethodSpec> methods, string name)
		{
			for (int i = 0; i < methods.Count; i++)
				if (methods[i].Name == name)
					return i;
			return -1;
		}
	}
}
#endif
