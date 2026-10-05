#if TOOLS
using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📚🪟 <b>Lanceur de registres d'interface</b><br/>
	/// Un <see cref="WindowEditor"/> qui liste les <see cref="InspectorWindowEditor"/> (<c>.tres</c>) déjà enregistrés
	/// dans un <b>dossier</b> et permet d'en <b>créer un nouveau</b>.<br/>
	/// <br/>
	/// Chaque <see cref="InspectorWindowEditor"/> est un <b>registre autonome</b> : il porte ses propres
	/// <see cref="DataRegistry"/> / <see cref="GroupRegistry"/> / <see cref="EffectRegistry"/> / <see cref="GizmoRegistry"/>
	/// et sa propre cible (<c>TargetResource</c> / <c>TargetNodePath</c>), indépendamment de toute scène.<br/>
	/// Ouvrir ou créer une entrée ici charge le <c>.tres</c>, l'affiche dans l'Inspecteur (pour renseigner sa cible)
	/// puis ouvre directement l'<see cref="InterfaceConfigWindowEditor"/> correspondant (matrices + aperçu en direct).<br/>
	/// <br/>
	/// <b>📍 Point d'entrée</b> : menu <c>Projet ▸ Outils ▸ Gestionnaire de registres d'interface...</c> de l'éditeur Godot.
	/// </summary>
	[Tool]
	public partial class RegistryLauncherWindowEditor : WindowEditor
	{
		/// <summary>Dossier proposé par défaut (créé à la demande s'il n'existe pas encore).</summary>
		public const string DefaultFolder = "res://Registres d'interface/";

		private LineEdit _folderField;
		private LineEdit _nameField;
		private ItemList _list;
		private readonly List<string> _foundPaths = new();

		/// <summary>Constructeur : titre et taille par défaut adaptés à la liste.</summary>
		public RegistryLauncherWindowEditor()
		{
			WindowTitle = "Gestionnaire de registres d'interface";
			WindowSizeOnCreation = new Vector2I(560, 480);
		}

		/// <inheritdoc/>
		protected override void OnOpenWindow()
		{
			ContentWindow.Theme = EditorInterface.Singleton.GetEditorTheme();

			var root = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill, SizeFlagsVertical = Control.SizeFlags.ExpandFill };

			var folderRow = new HBoxContainer();
			folderRow.AddChild(new Label { Text = "Dossier :" });
			_folderField = new LineEdit { Text = DefaultFolder, PlaceholderText = DefaultFolder, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			_folderField.TextSubmitted += _ => RefreshList();
			folderRow.AddChild(_folderField);
			var browse = new Button { Text = "Parcourir..." };
			browse.Pressed += BrowseFolder;
			folderRow.AddChild(browse);
			var refresh = new Button { Text = "Actualiser" };
			refresh.Pressed += RefreshList;
			folderRow.AddChild(refresh);
			root.AddChild(folderRow);

			root.AddChild(new Label { Text = "Registres existants (double-clic pour ouvrir et configurer) :" });
			_list = new ItemList
			{
				SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
				SizeFlagsVertical = Control.SizeFlags.ExpandFill,
				CustomMinimumSize = new Vector2(0, 260),
			};
			_list.ItemActivated += index => OpenExisting((int)index);
			root.AddChild(_list);

			var createRow = new HBoxContainer();
			createRow.AddChild(new Label { Text = "Nouveau registre :" });
			_nameField = new LineEdit { PlaceholderText = "Nom du registre", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			_nameField.TextSubmitted += _ => CreateNew();
			createRow.AddChild(_nameField);
			var create = new Button { Text = "Créer et configurer" };
			create.Pressed += CreateNew;
			createRow.AddChild(create);
			root.AddChild(createRow);

			var margin = new MarginContainer();
			margin.SetAnchorsPreset(Control.LayoutPreset.FullRect);
			foreach (string side in new[] { "left", "top", "right", "bottom" })
				margin.AddThemeConstantOverride($"margin_{side}", 8);
			margin.AddChild(root);

			var background = new Panel();
			background.SetAnchorsPreset(Control.LayoutPreset.FullRect);

			ContentWindow.AddChild(background);
			ContentWindow.AddChild(margin);

			RefreshList();
		}

		/// <summary>Ouvre un dialogue pour choisir le dossier des registres (limité à <c>res://</c>).</summary>
		private void BrowseFolder()
		{
			string current = DirAccess.DirExistsAbsolute(_folderField.Text) ? _folderField.Text : DefaultFolder;
			var dialog = new EditorFileDialog
			{
				FileMode = EditorFileDialog.FileModeEnum.OpenDir,
				Access = EditorFileDialog.AccessEnum.Resources,
				CurrentDir = current,
			};
			dialog.DirSelected += path =>
			{
				_folderField.Text = path;
				RefreshList();
			};
			dialog.Canceled += dialog.QueueFree;
			dialog.DirSelected += _ => dialog.QueueFree();
			ContentWindow.AddChild(dialog);
			dialog.PopupCentered(new Vector2I(700, 500));
		}

		/// <summary>Recharge la liste depuis le dossier saisi : un item par <see cref="InspectorWindowEditor"/> trouvé.</summary>
		private void RefreshList()
		{
			_list.Clear();
			_foundPaths.Clear();

			string folder = CurrentFolder();
			if (!DirAccess.DirExistsAbsolute(folder))
				return;

			using var dir = DirAccess.Open(folder);
			if (dir == null)
				return;

			dir.ListDirBegin();
			for (string fileName = dir.GetNext(); fileName != ""; fileName = dir.GetNext())
			{
				if (dir.CurrentIsDir() || !fileName.EndsWith(".tres"))
					continue;

				string path = folder.PathJoin(fileName);
				if (ResourceLoader.Load(path) is InspectorWindowEditor registry)
				{
					_list.AddItem($"{fileName.GetBaseName()}  —  {registry.WindowTitle}");
					_foundPaths.Add(path);
				}
			}
			dir.ListDirEnd();
		}

		/// <summary>Ouvre l'entrée sélectionnée : Inspecteur (pour la cible) + fenêtre de configuration.</summary>
		private void OpenExisting(int index)
		{
			if (index < 0 || index >= _foundPaths.Count)
				return;
			OpenRegistry(_foundPaths[index]);
		}

		/// <summary>
		/// Crée un nouveau <see cref="InspectorWindowEditor"/> vide, l'enregistre dans le dossier courant puis l'ouvre.
		/// </summary>
		private void CreateNew()
		{
			string name = _nameField.Text.Trim();
			if (name.Length == 0)
			{
				GD.PushWarning("[RegistryLauncherWindowEditor] Indiquez un nom avant de créer un registre.");
				return;
			}

			string folder = CurrentFolder();
			if (!DirAccess.DirExistsAbsolute(folder))
				DirAccess.MakeDirRecursiveAbsolute(folder);

			string path = folder.PathJoin($"{SanitizeFileName(name)}.tres");
			var registry = new InspectorWindowEditor { WindowTitle = name };

			var error = ResourceSaver.Save(registry, path);
			if (error != Error.Ok)
			{
				GD.PushError($"[RegistryLauncherWindowEditor] Échec de l'enregistrement « {path} » : {error}.");
				return;
			}

			EditorInterface.Singleton.GetResourceFilesystem().Scan();
			_nameField.Text = "";
			RefreshList();

			OpenRegistry(path);
		}

		/// <summary>Charge le <c>.tres</c>, l'affiche dans l'Inspecteur puis ouvre sa fenêtre de configuration.</summary>
		private void OpenRegistry(string path)
		{
			var registry = ResourceLoader.Load<InspectorWindowEditor>(path, cacheMode: ResourceLoader.CacheMode.Reuse);
			if (registry == null)
			{
				GD.PushError($"[RegistryLauncherWindowEditor] Impossible de charger « {path} ».");
				return;
			}

			EditorInterface.Singleton.EditResource(registry);
			registry.OpenConfigWindow();
		}

		private string CurrentFolder() => string.IsNullOrWhiteSpace(_folderField.Text) ? DefaultFolder : _folderField.Text;

		private static string SanitizeFileName(string name)
		{
			foreach (char invalid in new[] { '<', '>', ':', '"', '/', '\\', '|', '?', '*' })
				name = name.Replace(invalid, '_');
			return name;
		}
	}
}
#endif
