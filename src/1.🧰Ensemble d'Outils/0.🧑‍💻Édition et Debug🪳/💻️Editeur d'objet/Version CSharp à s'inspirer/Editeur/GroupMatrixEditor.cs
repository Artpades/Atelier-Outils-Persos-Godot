#if TOOLS
using System;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📊 <b>Matrice des groupes</b> : un tableau éditable où <b>chaque ligne est un groupe</b> du
	/// <see cref="GroupRegistry"/> et chaque colonne un champ du dictionnaire.<br/>
	/// Colonnes : ordre · <c>GroupName</c> · <c>ContainerType</c> · <c>IsVertical</c> · colonnes de grille ·
	/// <c>TargetDataNames</c> (menu de cases à cocher) · suppression.<br/>
	/// <b>📍 Note :</b> cocher une donnée dans un groupe la retire de son ancien groupe.
	/// </summary>
	public sealed class GroupMatrixEditor
	{
		private static readonly string[] Headers = { "", "Groupe", "Conteneur", "Vertical", "Colonnes", "Données", "" };

		private readonly GroupRegistry _groups;
		private readonly DataRegistry _data;
		private readonly Action _changed;
		private readonly GridContainer _grid = new() { Columns = Headers.Length };

		/// <summary>Contrôle racine (titre + tableau défilant + bouton d'ajout).</summary>
		public Control Root { get; }

		/// <param name="groups">Registre édité.</param>
		/// <param name="data">Registre des données (liste des données assignables).</param>
		/// <param name="changed">Appelé après chaque modification.</param>
		public GroupMatrixEditor(GroupRegistry groups, DataRegistry data, Action changed)
		{
			_groups = groups;
			_data = data;
			_changed = changed;

			var scroll = new ScrollContainer { SizeFlagsVertical = Control.SizeFlags.ExpandFill, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			scroll.AddChild(_grid);

			var add = new Button { Text = "+ Ajouter un groupe" };
			add.Pressed += () =>
			{
				_groups.Groups.Add(new GroupEntry { GroupName = $"Groupe {_groups.Groups.Count + 1}" });
				_changed();
				Refresh();
			};

			var root = new VBoxContainer { Name = "Groupes", SizeFlagsVertical = Control.SizeFlags.ExpandFill };
			root.AddChild(new Label { Text = "Registre des groupes", ThemeTypeVariation = "HeaderSmall" });
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

			for (int i = 0; i < _groups.Groups.Count; i++)
				if (_groups.Groups[i] != null)
					AddRow(i, _groups.Groups[i]);
		}

		private void AddRow(int index, GroupEntry group)
		{
			// Ordre
			var order = new HBoxContainer();
			var up = new Button { Text = "▲", Disabled = index == 0 };
			var down = new Button { Text = "▼", Disabled = index == _groups.Groups.Count - 1 };
			up.Pressed += () => Swap(index, index - 1);
			down.Pressed += () => Swap(index, index + 1);
			order.AddChild(up);
			order.AddChild(down);
			_grid.AddChild(order);

			// Nom (refusé s'il est déjà pris par un autre groupe)
			var name = new LineEdit { Text = group.GroupName, CustomMinimumSize = new Vector2(160, 0) };
			void CommitName()
			{
				var clash = _groups.Find(name.Text);
				if (name.Text == group.GroupName || (clash != null && clash != group))
				{
					name.Text = group.GroupName;
					return;
				}
				group.GroupName = name.Text;
				_changed();
			}
			name.TextSubmitted += _ => CommitName();
			name.FocusExited += CommitName;
			_grid.AddChild(name);

			// Type de conteneur
			var type = new OptionButton { CustomMinimumSize = new Vector2(150, 0) };
			foreach (var value in Enum.GetValues<GroupContainerType>())
				type.AddItem(value.ToString());
			type.Select((int)group.ContainerType);
			type.ItemSelected += selected =>
			{
				group.ContainerType = (GroupContainerType)(int)selected;
				_changed();
				Refresh();
			};
			_grid.AddChild(type);

			bool grid = group.ContainerType == GroupContainerType.GridContainer;

			// Vertical (sans effet pour une grille)
			var vertical = new CheckBox { ButtonPressed = group.IsVertical, Disabled = grid };
			vertical.Toggled += on => { group.IsVertical = on; _changed(); };
			_grid.AddChild(vertical);

			// Colonnes (grille uniquement)
			var columns = new SpinBox { MinValue = 1, MaxValue = 12, Step = 1, Value = group.Columns, Editable = grid };
			columns.ValueChanged += value => { group.Columns = (int)value; _changed(); };
			_grid.AddChild(columns);

			_grid.AddChild(BuildDataMenu(group));

			// Suppression
			var delete = new Button { Text = "✕", TooltipText = "Supprimer le groupe" };
			delete.Pressed += () =>
			{
				_groups.Groups.Remove(group);
				_changed();
				Refresh();
			};
			_grid.AddChild(delete);
		}

		/// <summary>Menu de cases à cocher listant les données ; l'ordre de <c>TargetDataNames</c> est l'ordre de cochage.</summary>
		private Control BuildDataMenu(GroupEntry group)
		{
			var menu = new MenuButton { Text = MenuTitle(group), Flat = false, CustomMinimumSize = new Vector2(150, 0) };
			var popup = menu.GetPopup();
			popup.HideOnCheckableItemSelection = false;

			foreach (var entry in _data.Entries)
			{
				if (entry == null || entry.IsMissing)
					continue;
				popup.AddCheckItem(entry.DataName);
				popup.SetItemChecked(popup.ItemCount - 1, group.TargetDataNames.Contains(entry.DataName));
			}

			popup.IndexPressed += index =>
			{
				int i = (int)index;
				string dataName = popup.GetItemText(i);
				bool now = !popup.IsItemChecked(i);
				popup.SetItemChecked(i, now);

				if (now) _groups.Assign(dataName, group);
				else group.TargetDataNames.Remove(dataName);

				menu.Text = MenuTitle(group);
				_changed();
				Callable.From(Refresh).CallDeferred(); // les autres lignes ont pu perdre la donnée
			};
			return menu;
		}

		private static string MenuTitle(GroupEntry group) => $"{group.TargetDataNames.Count} donnée(s) ▾";

		private void Swap(int a, int b)
		{
			if (b < 0 || b >= _groups.Groups.Count)
				return;
			(_groups.Groups[a], _groups.Groups[b]) = (_groups.Groups[b], _groups.Groups[a]);
			_changed();
			Refresh();
		}
	}
}
#endif
