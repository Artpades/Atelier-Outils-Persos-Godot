using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📄 <b>Entrée du registre de données</b> : configuration d'affichage d'<b>un</b> membre (propriété, méthode ou signal).<br/>
	/// Sérialisable (Resource) : elle est enregistrée avec la scène ou le fichier qui contient le <see cref="DataRegistry"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class DataEntry : Resource
	{
		/// <summary>Nom de la propriété, de la méthode ou du signal.</summary>
		[Export] public string DataName { get; set; } = "";

		/// <summary>Catégorie du membre. Renseignée par la synchronisation.</summary>
		[Export] public MemberKind Member { get; set; } = MemberKind.Property;

		/// <summary>Famille de donnée. Renseignée par la synchronisation ; détermine les widgets autorisés (<see cref="WidgetTable"/>).</summary>
		[Export] public DataKind Kind { get; set; } = DataKind.Unsupported;

		/// <summary>Widget choisi. <c>Auto</c> = défaut du type de donnée.</summary>
		[Export] public WidgetChoice WidgetChoice { get; set; } = WidgetChoice.Auto;

		/// <summary>Générateur utilisé quand <see cref="WidgetChoice"/> vaut <see cref="WidgetChoice.Custom"/>.</summary>
		[Export] public PropertyWidgetGenerator CustomGenerator { get; set; }

		/// <summary>Paramètres supplémentaires du widget (clés reconnues : <see cref="WidgetTable.ExtraHelp"/>).</summary>
		[Export] public Godot.Collections.Dictionary ExtraData { get; set; } = new();

		/// <summary>
		/// Orientation du <c>BoxContainer</c> englobant le widget : <c>true</c> = libellé au-dessus, <c>false</c> = libellé à gauche.<br/>
		/// Pour <b>Objet / NodePath / Array / Dictionary</b>, c'est l'orientation du contenu déplié.
		/// </summary>
		[Export] public bool IsVertical { get; set; }

		/// <summary>Si <c>false</c>, la donnée n'est pas affichée.</summary>
		[Export] public bool Enabled { get; set; } = true;

		/// <summary>
		/// Appels persistants d'un <b>signal</b> (entrées de catégorie <see cref="MemberKind.Signal"/>), à la manière d'un
		/// <c>UnityEvent</c>. Enregistrés avec le registre ; édités par <see cref="SignalWidgetGenerator"/>.
		/// </summary>
		[Export] public Godot.Collections.Array<SignalConnection> Connections { get; set; } = new();

		/// <summary>Vrai si le membre n'existe plus sur la cible : l'entrée est ignorée à l'affichage.</summary>
		[Export] public bool IsMissing { get; set; }

		/// <summary>Nombre décimal de <see cref="ExtraData"/>, ou <paramref name="fallback"/>.</summary>
		public double ExtraDouble(string key, double fallback) =>
			ExtraData != null && ExtraData.TryGetValue(key, out var v) && v.VariantType is Variant.Type.Int or Variant.Type.Float
				? v.AsDouble() : fallback;

		/// <summary>Booléen de <see cref="ExtraData"/>, ou <paramref name="fallback"/>.</summary>
		public bool ExtraBool(string key, bool fallback) =>
			ExtraData != null && ExtraData.TryGetValue(key, out var v) && v.VariantType == Variant.Type.Bool
				? v.AsBool() : fallback;

		/// <summary>Texte de <see cref="ExtraData"/>, ou <paramref name="fallback"/>.</summary>
		public string ExtraString(string key, string fallback) =>
			ExtraData != null && ExtraData.TryGetValue(key, out var v) ? v.AsString() : fallback;

		/// <summary>Widget effectivement utilisé : résout <c>Auto</c> et remplace un choix devenu invalide pour <see cref="Kind"/>.</summary>
		public WidgetChoice ResolvedChoice() =>
			WidgetChoice == WidgetChoice.Auto || !WidgetTable.IsAllowed(Kind, WidgetChoice)
				? WidgetTable.DefaultFor(Kind) : WidgetChoice;
	}
}
