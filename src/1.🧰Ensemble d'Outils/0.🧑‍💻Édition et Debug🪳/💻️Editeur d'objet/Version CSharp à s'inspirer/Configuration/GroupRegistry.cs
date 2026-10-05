using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🗂️ <b>Registre des groupes</b> : la hiérarchie de conteneurs de l'interface.<br/>
	/// Chaque <see cref="GroupEntry"/> regroupe des données par nom. Une donnée n'appartient qu'à <b>un seul</b> groupe
	/// (le premier qui la cite) ; les données sans groupe restent à la racine.<br/>
	/// Les groupes peuvent être créés à la main ou <b>automatiquement</b> depuis les <c>[ExportGroup]</c> de la cible
	/// par <see cref="DataRegistry.Sync"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class GroupRegistry : Resource
	{
		/// <summary>Groupes, dans l'ordre d'affichage (un groupe s'affiche à la place de sa première donnée).</summary>
		[Export] public Godot.Collections.Array<GroupEntry> Groups { get; set; } = new();

		/// <summary>Retourne le groupe portant ce nom, ou <c>null</c>.</summary>
		public GroupEntry Find(string groupName)
		{
			foreach (var group in Groups)
				if (group != null && group.GroupName == groupName)
					return group;
			return null;
		}

		/// <summary>Retourne le groupe portant ce nom, en le créant s'il n'existe pas.</summary>
		public GroupEntry Ensure(string groupName)
		{
			var group = Find(groupName);
			if (group == null)
			{
				group = new GroupEntry { GroupName = groupName };
				Groups.Add(group);
				EmitChanged();
			}
			return group;
		}

		/// <summary>Retire une donnée de tous les groupes.</summary>
		public void RemoveDataName(string dataName)
		{
			foreach (var group in Groups)
				while (group != null && group.TargetDataNames.Contains(dataName))
					group.TargetDataNames.Remove(dataName);
		}

		/// <summary>Place une donnée dans un groupe (et la retire des autres).</summary>
		public void Assign(string dataName, GroupEntry target)
		{
			RemoveDataName(dataName);
			target.TargetDataNames.Add(dataName);
		}
	}
}
