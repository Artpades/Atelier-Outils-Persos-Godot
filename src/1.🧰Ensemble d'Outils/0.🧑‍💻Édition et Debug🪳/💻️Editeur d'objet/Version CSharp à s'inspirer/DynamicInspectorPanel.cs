using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🔍 <b>Panneau d'inspection dynamique</b><br/>
	/// Génère automatiquement une interface à partir des <b>propriétés, méthodes et signaux</b> d'un objet en mémoire
	/// (équivalent des <i>Property Drawers</i> d'Unity).<br/>
	/// <br/>
	/// Hiérarchie créée par code : <c>PanelContainer → ScrollContainer → VBoxContainer</c>.<br/>
	/// <br/>
	/// <b>📍 Utilisation</b> : renseigner <see cref="TargetObject"/> (par code) ou <see cref="TargetNodePath"/> (dans l'éditeur),
	/// et optionnellement un <see cref="Registry"/> pour personnaliser les widgets. Sans registre, les générateurs par défaut sont utilisés.<br/>
	/// <br/>
	/// <b>📍 Deux modes</b> :<br/>
	/// • <b>Automatique</b> (sans <see cref="DataRegistry"/>) : un widget par membre, choisi par le <see cref="Registry"/>.<br/>
	/// • <b>Configuré</b> (avec <see cref="DataRegistry"/>) : widgets et conteneurs décrits par les deux registres, modifiables
	/// via le bouton « Configurer l'interface » de l'Inspecteur. Le registre se synchronise alors avec la cible ;
	/// <see cref="Scope"/> / <see cref="ShowProperties"/> / <see cref="ShowMethods"/> / <see cref="ShowSignals"/> sont ignorés
	/// (utiliser ceux du registre).<br/>
	/// <b>📍 Configuration</b> : le bouton d'outil « Configurer l'interface » (Inspecteur) ouvre une fenêtre à deux matrices,
	/// une ligne par donnée et une ligne par groupe.<br/>
	/// <b>⚠️ Attention</b> : changer <see cref="Scope"/>, <see cref="ShowProperties"/>, <see cref="ShowMethods"/> ou
	/// <see cref="ShowSignals"/> reconstruit l'interface.
	/// </summary>
	[GlobalClass, Tool]
	public partial class DynamicInspectorPanel : Control, IInspectorLayoutHost
	{
		private GodotObject _targetObject;
		private PropertyWidgetRegistry _registry;
		private InspectionScope _scope = InspectionScope.Auto;
		private bool _showProperties = true, _showMethods, _showSignals;
		private VBoxContainer _content;
		private DataRegistry _dataRegistry;
		private GroupRegistry _groupRegistry;
		private EffectRegistry _effectRegistry;
		private GizmoRegistry _gizmoRegistry;
		private GizmoDriver _gizmoDriver;

		/// <summary>Racine du contenu généré (<c>VBoxContainer</c> du <c>ScrollContainer</c>). <c>null</c> avant <c>_Ready</c>.</summary>
		public VBoxContainer Content => _content;

		/// <summary>Objet inspecté. L'assigner reconstruit l'interface.</summary>
		public GodotObject TargetObject
		{
			get => _targetObject;
			set { _targetObject = value; Rebuild(); }
		}

		/// <summary>
		/// Journalise dans la console chaque écriture faite par les widgets (objet, membre, valeur écrite, valeur relue).
		/// Pratique pour vérifier qu'une modification atteint bien un sous-objet. <b>Réglage global</b> : il s'applique à tous les panneaux.
		/// </summary>
		[Export]
		public bool LogWrites
		{
			get => PropertyWidgetGenerator.LogWrites;
			set => PropertyWidgetGenerator.LogWrites = value;
		}

		/// <summary>Resource à inspecter (assignable dans l'Inspecteur). Utilisée si <see cref="TargetObject"/> est vide.</summary>
		[Export] public Resource TargetResource { get; set; }

		/// <summary>Cible désignée dans l'éditeur, résolue depuis ce nœud à <c>_Ready</c> si <see cref="TargetObject"/> est vide.</summary>
		[Export] public NodePath TargetNodePath { get; set; } = new();

		/// <summary>Registre des données : active le mode <b>configuré</b> (voir la description de la classe).</summary>
		[Export]
		public DataRegistry DataRegistry
		{
			get => _dataRegistry;
			set { _dataRegistry = value; Rebuild(); }
		}

		/// <summary>Registre des groupes (conteneurs) du mode configuré.</summary>
		[Export]
		public GroupRegistry GroupRegistry
		{
			get => _groupRegistry;
			set { _groupRegistry = value; Rebuild(); }
		}

		/// <summary>Registre des effets (visibilité conditionnelle...) du mode configuré.</summary>
		[Export]
		public EffectRegistry EffectRegistry
		{
			get => _effectRegistry;
			set { _effectRegistry = value; Rebuild(); }
		}

		/// <summary>
		/// Registre des gizmos : dessine des formes dans l'espace 2D/3D à partir des données de l'objet, à chaque
		/// image (voir <see cref="GizmoDriver"/>). Fonctionne indépendamment de l'affichage de ce panneau.
		/// </summary>
		[Export]
		public GizmoRegistry GizmoRegistry
		{
			get => _gizmoRegistry;
			set { _gizmoRegistry = value; if (_gizmoDriver != null) _gizmoDriver.Registry = value; }
		}

		/// <summary>Registre de générateurs. <c>null</c> : générateurs par défaut.</summary>
		[Export]
		public PropertyWidgetRegistry Registry
		{
			get => _registry;
			set { _registry = value; Rebuild(); }
		}

		/// <summary>Périmètre des membres affichés.</summary>
		[Export]
		public InspectionScope Scope
		{
			get => _scope;
			set { _scope = value; Rebuild(); }
		}

		/// <summary>Affiche les propriétés.</summary>
		[Export]
		public bool ShowProperties
		{
			get => _showProperties;
			set { _showProperties = value; Rebuild(); }
		}

		/// <summary>Affiche les méthodes (un bouton chacune, appelée avec des arguments par défaut).</summary>
		[Export]
		public bool ShowMethods
		{
			get => _showMethods;
			set { _showMethods = value; Rebuild(); }
		}

		/// <summary>Affiche les signaux (un bouton d'émission chacun).</summary>
		[Export]
		public bool ShowSignals
		{
			get => _showSignals;
			set { _showSignals = value; Rebuild(); }
		}

		/// <inheritdoc/>
		public override void _Ready()
		{
			var panel = new PanelContainer { Name = "Panel" };
			panel.SetAnchorsPreset(LayoutPreset.FullRect);

			var scroll = new ScrollContainer
			{
				Name = "Scroll",
				HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
				SizeFlagsHorizontal = SizeFlags.ExpandFill,
				SizeFlagsVertical = SizeFlags.ExpandFill,
			};

			_content = new VBoxContainer { Name = "Content", SizeFlagsHorizontal = SizeFlags.ExpandFill };

			scroll.AddChild(_content);
			panel.AddChild(scroll);
			AddChild(panel);

			_gizmoDriver = new GizmoDriver { Name = "GizmoDriver", Registry = _gizmoRegistry, ResolveTarget = ResolveInspectedTarget };
			AddChild(_gizmoDriver);

			_targetObject ??= ResolveInspectedTarget();

			Rebuild();
		}

#if TOOLS
		#region 🔧 Bouton Outil Éditeur
		private InterfaceConfigWindowEditor _configWindow;

		/// <summary>
		/// Raccourci éditorial : ouvre la fenêtre de configuration (matrices des données et des groupes) depuis l'Inspecteur.
		/// </summary>
		[ExportToolButton("Configurer l'interface", Icon = "Window")] public Callable ConfigureInterfaceAction => Callable.From(OpenConfigWindow);

		/// <summary>
		/// Ouvre la fenêtre de configuration des registres <see cref="DataRegistry"/> et <see cref="GroupRegistry"/>
		/// (créés s'ils sont vides). Aucun effet si elle est déjà ouverte.
		/// </summary>
		public void OpenConfigWindow()
		{
			_configWindow ??= new InterfaceConfigWindowEditor { Host = this };
			_configWindow.OpenWindow();
		}
		#endregion
#endif

		/// <inheritdoc/>
		public override void _ExitTree()
		{
			// Dans l'éditeur, les scripts [Tool] ne doivent pas laisser de connexions cachées une fois le panneau fermé.
			if (Engine.IsEditorHint() && _dataRegistry != null && GodotObject.IsInstanceValid(_targetObject))
				_dataRegistry.ClearConnections(_targetObject);
		}

		/// <summary>Régénère tous les widgets (à appeler si la <b>structure</b> de l'objet change, ex : propriétés ajoutées).</summary>
		public void Rebuild()
		{
			if (_content == null)
				return;

			if (_dataRegistry != null)
			{
				if (GodotObject.IsInstanceValid(_targetObject))
				{
					_dataRegistry.SetTarget(_targetObject, _groupRegistry);
					_dataRegistry.ApplyConnections(_targetObject);
				}
				ConfiguredInterfaceBuilder.Populate(_content, _targetObject, _dataRegistry, _groupRegistry, _registry, _effectRegistry);
			}
			else
			{
				InspectorBuilder.Populate(_content, _targetObject, _registry, _scope, _showProperties, _showMethods, _showSignals);
			}
		}

		/// <summary>Cible effective : <see cref="TargetObject"/>, sinon <see cref="TargetResource"/>, sinon <see cref="TargetNodePath"/>.</summary>
		public GodotObject ResolveInspectedTarget()
		{
			if (GodotObject.IsInstanceValid(_targetObject))
				return _targetObject;
			if (TargetResource != null)
				return TargetResource;
			return TargetNodePath.IsEmpty ? null : GetNodeOrNull(TargetNodePath);
		}

		/// <summary>Force la relecture immédiate de toutes les valeurs, sans attendre le prochain cycle de surveillance.</summary>
		public void Refresh()
		{
			if (_content != null)
				RefreshRecursive(_content);
		}

		private static void RefreshRecursive(Node node)
		{
			if (node is PropertyBinding binding)
				binding.Refresh(true);
			foreach (Node child in node.GetChildren())
				RefreshRecursive(child);
		}
	}
}
