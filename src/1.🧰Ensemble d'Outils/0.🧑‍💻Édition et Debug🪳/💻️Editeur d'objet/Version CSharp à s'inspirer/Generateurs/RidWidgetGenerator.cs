using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🆔 <b>Générateur de <see cref="Rid"/></b> : un <see cref="Label"/> <b>lecture seule</b> affichant l'identifiant
	/// (mis à jour en direct).<br/>
	/// <b>📍 Note :</b> un <c>Rid</c> est un handle opaque vers une ressource du serveur (physique, rendu...) :
	/// l'éditer n'aurait aucun sens, il est donc volontairement non modifiable.
	/// </summary>
	[GlobalClass, Tool]
	public partial class RidWidgetGenerator : PropertyWidgetGenerator
	{
		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			WidgetUtils.TypeOf(info) == Variant.Type.Rid ? 100 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var label = new Label { Modulate = new Color(1, 1, 1, 0.8f) };
			Bind(label, target, memberName, v =>
			{
				var rid = v.AsRid();
				label.Text = rid.IsValid ? $"RID({rid.Id})" : "RID(invalide)";
			});
			return Labeled(target, memberName, label);
		}
	}
}
