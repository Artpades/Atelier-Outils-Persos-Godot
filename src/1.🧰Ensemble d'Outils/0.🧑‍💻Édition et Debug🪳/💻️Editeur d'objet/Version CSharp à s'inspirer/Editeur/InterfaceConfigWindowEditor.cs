#if TOOLS
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🛠️🪟 <b>Fenêtre de configuration de l'interface</b><br/>
	/// Un <see cref="WindowEditor"/> qui édite les deux registres d'un <see cref="IInspectorLayoutHost"/> sous forme de
	/// <b>matrices</b> :<br/>
	/// • <see cref="DataMatrixEditor"/> : une ligne par entrée du <see cref="DataRegistry"/> ;<br/>
	/// • <see cref="GroupMatrixEditor"/> : une ligne par groupe du <see cref="GroupRegistry"/> ;<br/>
	/// • <see cref="EffectMatrixEditor"/> : une ligne par effet du <see cref="EffectRegistry"/> (ex : visibilité conditionnelle) ;<br/>
	/// • <see cref="GizmoListEditor"/> : une carte par appel du <see cref="GizmoRegistry"/> (formes dessinées dans l'espace) ;<br/>
	/// • à droite, un <b>aperçu en direct</b> reconstruit après chaque modification.<br/>
	/// <br/>
	/// Ouverte par le bouton « Configurer l'interface » du <see cref="DynamicInspectorPanel"/> (ou de
	/// <see cref="InspectorWindowEditor"/>), qui renseigne <see cref="Host"/>.<br/>
	/// <b>⚠️ Attention</b> : l'aperçu est branché sur la <b>vraie</b> cible : modifier une valeur dedans la modifie réellement.
	/// Les registres sont des Resources : enregistrez la scène (ou le <c>.tres</c>) pour conserver la configuration.
	/// </summary>
	[Tool]
	public partial class InterfaceConfigWindowEditor : WindowEditor
	{
		private DataRegistry _data;
		private GroupRegistry _groups;
		private EffectRegistry _effects;
		private GizmoRegistry _gizmos;
		private DataMatrixEditor _dataMatrix;
		private GroupMatrixEditor _groupMatrix;
		private EffectMatrixEditor _effectMatrix;
		private GizmoListEditor _gizmoEditor;
		private DynamicInspectorPanel _preview;

		/// <summary>Objet dont on configure l'interface. À renseigner avant <c>OpenWindow()</c>.</summary>
		public IInspectorLayoutHost Host { get; set; }

		/// <summary>Constructeur : titre et taille par défaut adaptés aux matrices.</summary>
		public InterfaceConfigWindowEditor()
		{
			WindowTitle = "Configurer l'interface";
			WindowSizeOnCreation = new Vector2I(1400, 780);
		}

		/// <inheritdoc/>
		protected override void OnOpenWindow()
		{
			if (Host == null)
			{
				GD.PushWarning("[InterfaceConfigWindowEditor] Aucun hôte : rien à configurer.");
				return;
			}

			// Registres créés à la demande ; l'Inspecteur est rafraîchi pour les afficher.
			_data = Host.DataRegistry ??= new DataRegistry();
			_groups = Host.GroupRegistry ??= new GroupRegistry();
			_effects = Host.EffectRegistry ??= new EffectRegistry();
			_gizmos = Host.GizmoRegistry ??= new GizmoRegistry();
			(Host as GodotObject)?.NotifyPropertyListChanged();

			var target = Host.ResolveInspectedTarget();
			if (GodotObject.IsInstanceValid(target))
			{
				_data.SetTarget(target, _groups, true);
				_data.Sync(_groups);
			}

			_dataMatrix = new DataMatrixEditor(_data, NotifyChanged);
			_groupMatrix = new GroupMatrixEditor(_groups, _data, NotifyChanged);
			_effectMatrix = new EffectMatrixEditor(_effects, _data, target, NotifyChanged);
			_gizmoEditor = new GizmoListEditor(_gizmos, target, NotifyChanged);

			var matrices = new TabContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill, SizeFlagsVertical = Control.SizeFlags.ExpandFill };
			matrices.AddChild(_dataMatrix.Root);
			matrices.AddChild(_groupMatrix.Root);
			matrices.AddChild(_effectMatrix.Root);
			matrices.AddChild(_gizmoEditor.Root);

			var left = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			left.AddChild(BuildScanBar(target));
			left.AddChild(matrices);

			_preview = new DynamicInspectorPanel
			{
				DataRegistry = _data,
				GroupRegistry = _groups,
				EffectRegistry = _effects,
				GizmoRegistry = _gizmos,
				TargetObject = target,
				SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
				SizeFlagsVertical = Control.SizeFlags.ExpandFill,
				CustomMinimumSize = new Vector2(340, 0),
			};

			var right = new VBoxContainer();
			right.AddChild(new Label { Text = "Aperçu (branché sur la vraie cible)", Modulate = new Color(1, 1, 1, 0.7f) });
			right.AddChild(_preview);

			var split = new HSplitContainer();
			split.AddChild(left);
			split.AddChild(right);

			var margin = new MarginContainer();
			margin.SetAnchorsPreset(Control.LayoutPreset.FullRect);
			foreach (string side in new[] { "left", "top", "right", "bottom" })
				margin.AddThemeConstantOverride($"margin_{side}", 8);
			margin.AddChild(split);

			var background = new Panel();
			background.SetAnchorsPreset(Control.LayoutPreset.FullRect);

			ContentWindow.Theme = EditorInterface.Singleton.GetEditorTheme();
			ContentWindow.AddChild(background);
			ContentWindow.AddChild(margin);
		}

		/// <summary>Barre de synchronisation : membres scannés, périmètre et bouton « Actualiser ».</summary>
		private Control BuildScanBar(GodotObject target)
		{
			var bar = new HBoxContainer();

			CheckBox Scan(string text, bool value, System.Action<bool> apply)
			{
				var box = new CheckBox { Text = text, ButtonPressed = value };
				box.Toggled += on => { apply(on); Resync(); };
				return box;
			}

			bar.AddChild(Scan("Propriétés", _data.ScanProperties, on => _data.ScanProperties = on));
			bar.AddChild(Scan("Méthodes", _data.ScanMethods, on => _data.ScanMethods = on));
			bar.AddChild(Scan("Signaux", _data.ScanSignals, on => _data.ScanSignals = on));

			var scope = new OptionButton { TooltipText = "Périmètre des membres scannés" };
			foreach (var value in System.Enum.GetValues<InspectionScope>())
				scope.AddItem(value.ToString());
			scope.Select((int)_data.Scope);
			scope.ItemSelected += index => { _data.Scope = (InspectionScope)(int)index; Resync(); };
			bar.AddChild(scope);

			var refresh = new Button { Text = "Actualiser", TooltipText = "Relit les membres de la cible" };
			refresh.Pressed += Resync;
			bar.AddChild(refresh);

			if (!GodotObject.IsInstanceValid(target))
				bar.AddChild(new Label { Text = "⚠ Aucune cible : renseignez TargetResource ou TargetNodePath puis rouvrez.", Modulate = new Color(1, 0.7f, 0.4f) });
			return bar;
		}

		private void Resync()
		{
			var target = Host.ResolveInspectedTarget();
			if (GodotObject.IsInstanceValid(target))
			{
				_data.SetTarget(target, _groups);
				_data.Sync(_groups);
			}
			_dataMatrix.Refresh();
			_groupMatrix.Refresh();
			_effectMatrix.Refresh();
			_gizmoEditor.Refresh();
			NotifyChanged();
		}

		/// <summary>Répercute une modification : signale les Resources, marque la scène et reconstruit l'aperçu.</summary>
		private void NotifyChanged()
		{
			_data.EmitChanged();
			_groups.EmitChanged();
			_effects.EmitChanged();
			_gizmos.EmitChanged();
			EditorInterface.Singleton.MarkSceneAsUnsaved();
			if (GodotObject.IsInstanceValid(_preview))
				_preview.Rebuild();
		}
	}
}
#endif
