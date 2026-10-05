#if TOOLS
using System;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📊 <b>Matrice des effets</b> : un tableau éditable où <b>chaque ligne est un effet</b> de
	/// l'<see cref="EffectRegistry"/> : <see cref="VisibilityEffect"/> (montrer/cacher), <see cref="InteractivityEffect"/>
	/// (verrouiller/déverrouiller) ou <see cref="TooltipEffect"/> (infobulle personnalisée).<br/>
	/// <br/>
	/// Colonnes communes : actif, type, <b>Configuration</b> et <b>Détail</b> (leur contenu dépend du type d'effet de la
	/// ligne), données ciblées, suppression.
	/// </summary>
	public sealed class EffectMatrixEditor
	{
		private static readonly string[] Headers = { "", "Actif", "Type", "Configuration", "Détail", "Données", "" };

		private readonly EffectRegistry _effects;
		private readonly DataRegistry _data;
		private readonly GodotObject _target;
		private readonly Action _changed;
		private readonly GridContainer _grid = new() { Columns = Headers.Length };

		/// <summary>Contrôle racine (titre + tableau défilant + bouton d'ajout).</summary>
		public Control Root { get; }

		/// <param name="effects">Registre édité.</param>
		/// <param name="data">Registre des données (liste des données ciblables).</param>
		/// <param name="target">Cible inspectée, pour vérifier les conditions (propriété / méthode).</param>
		/// <param name="changed">Appelé après chaque modification.</param>
		public EffectMatrixEditor(EffectRegistry effects, DataRegistry data, GodotObject target, Action changed)
		{
			_effects = effects;
			_data = data;
			_target = target;
			_changed = changed;

			var scroll = new ScrollContainer { SizeFlagsVertical = Control.SizeFlags.ExpandFill, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			scroll.AddChild(_grid);

			var add = new MenuButton { Text = "+ Ajouter un effet" };
			var addPopup = add.GetPopup();
			addPopup.AddItem("👁 Visibilité (montrer / cacher)");
			addPopup.AddItem("🔒 Interactivité (lecture seule)");
			addPopup.AddItem("💬 Infobulle personnalisée");
			addPopup.IndexPressed += index =>
			{
				InterfaceEffect effect = (int)index switch
				{
					0 => new VisibilityEffect(),
					1 => new InteractivityEffect(),
					_ => new TooltipEffect(),
				};
				_effects.Effects.Add(effect);
				_changed();
				Refresh();
			};

			var root = new VBoxContainer { Name = "Effets", SizeFlagsVertical = Control.SizeFlags.ExpandFill };
			root.AddChild(new Label { Text = "Registre des effets", ThemeTypeVariation = "HeaderSmall" });
			root.AddChild(WidgetUtils.MakeHint("Modifie l'interface générée selon l'état de l'objet (visibilité, verrouillage, infobulles)."));
			root.AddChild(scroll);
			root.AddChild(add);
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

			for (int i = 0; i < _effects.Effects.Count; i++)
				if (_effects.Effects[i] != null)
					AddRow(i, _effects.Effects[i]);
		}

		private void AddRow(int index, InterfaceEffect effect)
		{
			// Ordre
			var order = new HBoxContainer();
			var up = new Button { Text = "▲", Disabled = index == 0 };
			var down = new Button { Text = "▼", Disabled = index == _effects.Effects.Count - 1 };
			up.Pressed += () => Swap(index, index - 1);
			down.Pressed += () => Swap(index, index + 1);
			order.AddChild(up);
			order.AddChild(down);
			_grid.AddChild(order);

			// Actif
			var enabled = new CheckBox { ButtonPressed = effect.Enabled };
			enabled.Toggled += on => { effect.Enabled = on; _changed(); };
			_grid.AddChild(enabled);

			// Type
			string typeLabel = effect switch
			{
				VisibilityEffect => "Visibilité",
				InteractivityEffect => "Interactivité",
				TooltipEffect => "Infobulle",
				_ => effect.GetType().Name,
			};
			_grid.AddChild(new Label { Text = typeLabel, Modulate = new Color(1, 1, 1, 0.7f) });

			switch (effect)
			{
				case VisibilityEffect visibility: AddVisibilityColumns(visibility); break;
				case InteractivityEffect interactivity: AddInteractivityColumns(interactivity); break;
				case TooltipEffect tooltip: AddTooltipColumns(tooltip); break;
				default:
					for (int i = 0; i < 3; i++)
						_grid.AddChild(WidgetUtils.MakeHint("(type d'effet sans éditeur dédié)"));
					break;
			}

			// Suppression
			var delete = new Button { Text = "✕", TooltipText = "Supprimer l'effet" };
			delete.Pressed += () =>
			{
				_effects.Effects.Remove(effect);
				_changed();
				Refresh();
			};
			_grid.AddChild(delete);
		}

		#region 👁 Visibilité
		private void AddVisibilityColumns(VisibilityEffect effect)
		{
			_grid.AddChild(BuildConditionField(effect.ConditionMember, value => { effect.ConditionMember = value; _changed(); }));

			var mode = new OptionButton { CustomMinimumSize = new Vector2(150, 0) };
			mode.AddItem("Afficher si vrai");
			mode.AddItem("Cacher si vrai");
			mode.Select((int)effect.Mode);
			mode.ItemSelected += selected => { effect.Mode = (VisibilityMode)(int)selected; _changed(); };
			_grid.AddChild(mode);

			_grid.AddChild(BuildDataMenu(effect.TargetDataNames));
		}
		#endregion

		#region 🔒 Interactivité
		private void AddInteractivityColumns(InteractivityEffect effect)
		{
			bool needsCondition = effect.Mode != InteractivityMode.AlwaysReadOnly;
			if (needsCondition)
			{
				_grid.AddChild(BuildConditionField(effect.ConditionMember, value => { effect.ConditionMember = value; _changed(); }));
			}
			else
			{
				var hint = WidgetUtils.MakeHint("(toujours verrouillé, aucune condition)");
				hint.CustomMinimumSize = new Vector2(150, 0);
				_grid.AddChild(hint);
			}

			var mode = new OptionButton { CustomMinimumSize = new Vector2(160, 0) };
			mode.AddItem("Toujours lecture seule");
			mode.AddItem("Lecture seule si vrai");
			mode.AddItem("Éditable si vrai");
			mode.Select((int)effect.Mode);
			mode.ItemSelected += selected =>
			{
				effect.Mode = (InteractivityMode)(int)selected;
				_changed();
				Refresh(); // la colonne Configuration apparaît/disparaît selon le mode
			};
			_grid.AddChild(mode);

			_grid.AddChild(BuildDataMenu(effect.TargetDataNames));
		}
		#endregion

		#region 💬 Infobulle
		private void AddTooltipColumns(TooltipEffect effect)
		{
			var text = new LineEdit { Text = effect.Text, PlaceholderText = "Texte affiché au survol", CustomMinimumSize = new Vector2(150, 0) };
			void CommitText()
			{
				if (text.Text == effect.Text)
					return;
				effect.Text = text.Text;
				_changed();
			}
			text.TextSubmitted += _ => CommitText();
			text.FocusExited += CommitText;
			_grid.AddChild(text);

			_grid.AddChild(WidgetUtils.MakeHint(""));
			_grid.AddChild(BuildDataMenu(effect.TargetDataNames));
		}
		#endregion

		/// <summary>Champ de saisie d'un nom de membre condition (propriété/méthode bool) avec statut de validité.</summary>
		private Control BuildConditionField(string current, Action<string> onCommit)
		{
			var edit = new LineEdit { Text = current, PlaceholderText = "propriété bool ou méthode → bool", CustomMinimumSize = new Vector2(150, 0) };
			var status = WidgetUtils.MakeHint(ConditionEvaluator.Describe(_target, current));
			status.CustomMinimumSize = new Vector2(150, 0);

			void Commit()
			{
				if (edit.Text == current)
					return;
				current = edit.Text;
				status.Text = ConditionEvaluator.Describe(_target, current);
				onCommit(current);
			}
			edit.TextSubmitted += _ => Commit();
			edit.FocusExited += Commit;

			var column = new VBoxContainer();
			column.AddChild(edit);
			column.AddChild(status);
			return column;
		}

		/// <summary>Menu de cases à cocher listant les données. Une donnée peut appartenir à plusieurs effets à la fois.</summary>
		private Control BuildDataMenu(Godot.Collections.Array<string> targetDataNames)
		{
			var menu = new MenuButton { Text = MenuTitle(targetDataNames), Flat = false, CustomMinimumSize = new Vector2(150, 0) };
			var popup = menu.GetPopup();
			popup.HideOnCheckableItemSelection = false;

			foreach (var entry in _data.Entries)
			{
				if (entry == null || entry.IsMissing)
					continue;
				popup.AddCheckItem(entry.DataName);
				popup.SetItemChecked(popup.ItemCount - 1, targetDataNames.Contains(entry.DataName));
			}

			popup.IndexPressed += index =>
			{
				int i = (int)index;
				string dataName = popup.GetItemText(i);
				bool now = !popup.IsItemChecked(i);
				popup.SetItemChecked(i, now);

				if (now) targetDataNames.Add(dataName);
				else targetDataNames.Remove(dataName);

				menu.Text = MenuTitle(targetDataNames);
				_changed();
			};
			return menu;
		}

		private static string MenuTitle(Godot.Collections.Array<string> targetDataNames) => $"{targetDataNames.Count} donnée(s) ▾";

		private void Swap(int a, int b)
		{
			if (b < 0 || b >= _effects.Effects.Count)
				return;
			(_effects.Effects[a], _effects.Effects[b]) = (_effects.Effects[b], _effects.Effects[a]);
			_changed();
			Refresh();
		}
	}
}
#endif
