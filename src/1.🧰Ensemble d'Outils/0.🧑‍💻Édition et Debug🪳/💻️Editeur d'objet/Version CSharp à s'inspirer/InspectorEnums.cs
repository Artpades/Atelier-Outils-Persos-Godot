namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// Catégorie de membre introspectable d'un <see cref="Godot.GodotObject"/>.<br/>
	/// Correspond à <c>GetPropertyList()</c>, <c>GetMethodList()</c> et <c>GetSignalList()</c>.
	/// </summary>
	public enum MemberKind
	{
		/// <summary>Une propriété (<c>GetPropertyList()</c>).</summary>
		Property,
		/// <summary>Une méthode (<c>GetMethodList()</c>).</summary>
		Method,
		/// <summary>Un signal (<c>GetSignalList()</c>).</summary>
		Signal
	}

	/// <summary>Manière de comparer <see cref="PropertyWidgetGenerator.TargetPropertyName"/> au nom d'un membre.</summary>
	public enum PropertyNameMatchMode
	{
		/// <summary>Le nom doit être <b>identique</b> (insensible à la casse).</summary>
		Exact,
		/// <summary>Le nom du membre doit <b>contenir</b> le motif (insensible à la casse).</summary>
		Contains,
		/// <summary>Le motif est une <b>expression régulière</b> .NET.</summary>
		Regex
	}

	/// <summary>Disposition du contenu déplié d'un objet, d'un tableau ou d'un dictionnaire.</summary>
	public enum ContainerLayout
	{
		/// <summary><c>BoxContainer</c> (orientation : <see cref="PropertyWidgetGenerator.IsVertical"/>).</summary>
		Box,
		/// <summary><c>GridContainer</c> (colonnes : <see cref="ContainerWidgetGenerator.GridColumns"/>).</summary>
		Grid
	}

	/// <summary>Périmètre des membres affichés par l'inspecteur.</summary>
	public enum InspectionScope
	{
		/// <summary>Membres du <b>script</b> si l'objet en possède un, sinon <b>tous</b> les membres.</summary>
		Auto,
		/// <summary>Uniquement les membres déclarés par le script attaché.</summary>
		ScriptOnly,
		/// <summary>Tous les membres, y compris ceux du moteur (très long pour un <c>Node</c>).</summary>
		All
	}
}
