using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🗃️ <b>Entrée du registre de groupes</b> : un conteneur qui regroupe plusieurs données dans l'interface.
	/// </summary>
	[GlobalClass, Tool]
	public partial class GroupEntry : Resource
	{
		private GroupContainerType _containerType = GroupContainerType.VBoxContainer;

		/// <summary>Titre affiché au-dessus du groupe (vide = pas de titre).</summary>
		[Export] public string GroupName { get; set; } = "";

		/// <summary>
		/// Type de conteneur. Choisir <c>VBoxContainer</c> ou <c>HBoxContainer</c> règle aussi <see cref="IsVertical"/>
		/// (que l'on peut ensuite modifier séparément).
		/// </summary>
		[Export]
		public GroupContainerType ContainerType
		{
			get => _containerType;
			set
			{
				_containerType = value;
				if (value == GroupContainerType.VBoxContainer) IsVertical = true;
				else if (value == GroupContainerType.HBoxContainer) IsVertical = false;
			}
		}

		/// <summary>Noms (<see cref="DataEntry.DataName"/>) des données regroupées, dans l'ordre d'affichage.</summary>
		[Export] public Godot.Collections.Array<string> TargetDataNames { get; set; } = new();

		/// <summary>Orientation verticale du conteneur (ignorée par <see cref="GroupContainerType.GridContainer"/>).</summary>
		[Export] public bool IsVertical { get; set; } = true;

		/// <summary>Nombre de colonnes pour <see cref="GroupContainerType.GridContainer"/>.</summary>
		[Export(PropertyHint.Range, "1,12,1")] public int Columns { get; set; } = 2;
	}
}
