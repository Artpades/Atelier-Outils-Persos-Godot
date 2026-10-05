using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// Contrat d'un objet qui <b>porte</b> une configuration d'interface (les deux registres) et sait quelle cible inspecter.<br/>
	/// Les objets qui l'implémentent (<see cref="DynamicInspectorPanel"/>, <see cref="InspectorWindowEditor"/>) reçoivent
	/// automatiquement le bouton « Configurer l'interface » dans l'Inspecteur Godot.
	/// </summary>
	public interface IInspectorLayoutHost
	{
		/// <summary>Registre des données (créé par l'éditeur s'il est <c>null</c>).</summary>
		DataRegistry DataRegistry { get; set; }

		/// <summary>Registre des groupes (créé par l'éditeur s'il est <c>null</c>).</summary>
		GroupRegistry GroupRegistry { get; set; }

		/// <summary>Registre des effets (créé par l'éditeur s'il est <c>null</c>).</summary>
		EffectRegistry EffectRegistry { get; set; }

		/// <summary>Registre des gizmos (créé par l'éditeur s'il est <c>null</c>).</summary>
		GizmoRegistry GizmoRegistry { get; set; }

		/// <summary>Objet dont l'interface est générée, ou <c>null</c> si aucune cible n'est disponible.</summary>
		GodotObject ResolveInspectedTarget();
	}
}
