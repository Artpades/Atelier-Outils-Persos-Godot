using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🗄️ <b>Registre des données</b> : la liste des membres d'un objet et la façon de les afficher.<br/>
	/// Dès qu'une cible est renseignée (<see cref="SetTarget"/> ou <see cref="TargetResource"/>), le registre
	/// <b>se synchronise</b> avec <c>GetPropertyList()</c>, <c>GetMethodList()</c> et <c>GetSignalList()</c> :<br/>
	/// • un membre <b>nouveau</b> reçoit une entrée (widget <c>Auto</c>) ;<br/>
	/// • un membre <b>connu</b> garde sa configuration ;<br/>
	/// • un membre <b>disparu</b> est marqué <see cref="DataEntry.IsMissing"/> (jamais supprimé automatiquement).
	/// </summary>
	[GlobalClass, Tool]
	public partial class DataRegistry : Resource
	{
		private Resource _targetResource;

		/// <summary>Émis après chaque synchronisation avec la cible.</summary>
		[Signal] public delegate void SyncedEventHandler();

		#region ⚙️ Configuration
		/// <summary>Une entrée par membre, dans l'ordre d'affichage.</summary>
		[Export] public Godot.Collections.Array<DataEntry> Entries { get; set; } = new();

		/// <summary>Périmètre des membres scannés.</summary>
		[Export] public InspectionScope Scope { get; set; } = InspectionScope.Auto;

		/// <summary>Scanne les propriétés.</summary>
		[Export] public bool ScanProperties { get; set; } = true;

		/// <summary>Scanne les méthodes.</summary>
		[Export] public bool ScanMethods { get; set; }

		/// <summary>Scanne les signaux.</summary>
		[Export] public bool ScanSignals { get; set; }

		/// <summary>Synchronise automatiquement quand la cible change. Sinon, appeler <see cref="Sync"/>.</summary>
		[Export] public bool AutoSync { get; set; } = true;

		/// <summary>
		/// Cible de type Resource, assignable dans l'Inspecteur. La synchronisation est <b>différée</b> d'une frame
		/// pour ne pas s'exécuter au milieu du chargement du fichier.
		/// </summary>
		[Export]
		public Resource TargetResource
		{
			get => _targetResource;
			set
			{
				_targetResource = value;
				if (value != null)
					CallDeferred(MethodName.SetTarget, value, default(Variant), false);
			}
		}
		#endregion

		/// <summary>Objet actuellement synchronisé (non sérialisé).</summary>
		public GodotObject Target { get; private set; }

		/// <summary>
		/// Définit la cible et, si <see cref="AutoSync"/> est actif et que la cible a changé (ou <paramref name="force"/>), synchronise.
		/// </summary>
		/// <param name="target">Objet inspecté.</param>
		/// <param name="groups">Registre de groupes à compléter avec les <c>[ExportGroup]</c> (optionnel).</param>
		/// <param name="force">Synchronise même si la cible est inchangée.</param>
		public void SetTarget(GodotObject target, GroupRegistry groups = null, bool force = false)
		{
			bool changed = !ReferenceEquals(Target, target);
			Target = target;
			if (AutoSync && (changed || force))
				Sync(groups);
		}

		/// <summary>
		/// Connecte les <see cref="DataEntry.Connections"/> de tous les signaux de <paramref name="target"/> (idempotent).<br/>
		/// À appeler pour que les appels fonctionnent <b>sans</b> afficher d'interface. <see cref="DynamicInspectorPanel"/> le fait lui-même.
		/// </summary>
		public void ApplyConnections(GodotObject target)
		{
			foreach (var entry in Entries)
				if (entry != null && entry.Member == MemberKind.Signal && !entry.IsMissing && entry.Connections.Count > 0)
					SignalConnector.Apply(target, entry.DataName, entry.Connections);
		}

		/// <summary>Retire les connexions faites par <see cref="ApplyConnections"/> pour <paramref name="target"/>.</summary>
		public void ClearConnections(GodotObject target)
		{
			foreach (var entry in Entries)
				if (entry != null && entry.Member == MemberKind.Signal)
					SignalConnector.Clear(target, entry.Connections);
		}

		/// <summary>Retourne l'entrée d'un membre, ou <c>null</c>.</summary>
		public DataEntry Find(string dataName, MemberKind member = MemberKind.Property)
		{
			foreach (var entry in Entries)
				if (entry != null && entry.DataName == dataName && entry.Member == member)
					return entry;
			return null;
		}

		/// <summary>
		/// Fusionne la liste des membres de <see cref="Target"/> avec <see cref="Entries"/>.<br/>
		/// Les <c>[ExportGroup]</c> / <c>[ExportSubgroup]</c> des <b>nouveaux</b> membres alimentent <paramref name="groups"/>
		/// (le regroupement dynamique ; les membres déjà connus ne sont jamais déplacés).
		/// </summary>
		public void Sync(GroupRegistry groups = null)
		{
			if (!GodotObject.IsInstanceValid(Target))
				return;

			var seen = new HashSet<DataEntry>();
			bool changed = false;

			if (ScanProperties) changed |= SyncKind(MemberKind.Property, groups, seen);
			if (ScanMethods) changed |= SyncKind(MemberKind.Method, groups, seen);
			if (ScanSignals) changed |= SyncKind(MemberKind.Signal, groups, seen);

			foreach (var entry in Entries)
			{
				bool missing = entry != null && !seen.Contains(entry);
				if (entry != null && entry.IsMissing != missing)
				{
					entry.IsMissing = missing;
					changed = true;
				}
			}

			if (changed)
				EmitChanged();
			EmitSignal(SignalName.Synced);
		}

		private bool SyncKind(MemberKind member, GroupRegistry groups, HashSet<DataEntry> seen)
		{
			bool changed = false;
			string currentGroup = null, currentSub = null;
			var names = new HashSet<string>();

			foreach (var info in InspectorBuilder.GetMembers(Target, member, Scope))
			{
				string name = WidgetUtils.NameOf(info);
				if (string.IsNullOrEmpty(name))
					continue;

				if (member == MemberKind.Property)
				{
					var usage = WidgetUtils.UsageOf(info);
					if ((usage & PropertyUsageFlags.Category) != 0) { currentGroup = currentSub = null; continue; }
					if ((usage & PropertyUsageFlags.Group) != 0) { currentGroup = name; currentSub = null; continue; }
					if ((usage & PropertyUsageFlags.Subgroup) != 0) { currentSub = name; continue; }
					if ((usage & PropertyUsageFlags.Editor) == 0 || name == "script")
						continue;
				}
				else if (name.StartsWith('_'))
				{
					continue;
				}

				if (!names.Add(name))
					continue;

				var kind = WidgetTable.KindOf(info, member);
				var entry = Find(name, member);
				if (entry == null)
				{
					entry = new DataEntry { DataName = name, Member = member, Kind = kind, IsVertical = WidgetTable.IsContainerKind(kind) };
					Entries.Add(entry);
					changed = true;

					string groupName = currentGroup == null ? null : currentSub == null ? currentGroup : $"{currentGroup} / {currentSub}";
					if (groups != null && groupName != null)
						groups.Ensure(groupName).TargetDataNames.Add(name);
				}
				else if (entry.Kind != kind)
				{
					entry.Kind = kind;
					if (!WidgetTable.IsAllowed(kind, entry.WidgetChoice))
						entry.WidgetChoice = WidgetChoice.Auto;
					changed = true;
				}
				seen.Add(entry);
			}
			return changed;
		}
	}
}
