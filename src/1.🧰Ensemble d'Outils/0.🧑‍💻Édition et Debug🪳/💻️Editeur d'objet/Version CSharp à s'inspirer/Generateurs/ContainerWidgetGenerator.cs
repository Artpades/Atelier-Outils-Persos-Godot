using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📦 <b>Base des générateurs à contenu déplié</b> (objets, tableaux, dictionnaires, NodePath).<br/>
	/// Regroupe les réglages de disposition du contenu : type de conteneur, colonnes, repli, ouverture initiale.<br/>
	/// Ici <see cref="PropertyWidgetGenerator.IsVertical"/> vaut <c>true</c> par défaut (contenu empilé).
	/// </summary>
	[GlobalClass, Tool]
	public abstract partial class ContainerWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Disposition du contenu : <c>BoxContainer</c> ou <c>GridContainer</c>.</summary>
		[Export] public ContainerLayout Layout { get; set; } = ContainerLayout.Box;

		/// <summary>Nombre de colonnes en disposition <see cref="ContainerLayout.Grid"/>.</summary>
		[Export(PropertyHint.Range, "1,12,1")] public int GridColumns { get; set; } = 2;

		/// <summary>Si faux, pas d'en-tête repliable : le contenu est toujours visible.</summary>
		[Export] public bool Foldable { get; set; } = true;

		/// <summary>Ouvre la section dès l'affichage. <b>⚠️ Attention :</b> risque de récursion profonde sur des objets qui se référencent.</summary>
		[Export] public bool StartExpanded { get; set; }

		/// <summary>Constructeur : contenu vertical par défaut.</summary>
		protected ContainerWidgetGenerator() => IsVertical = true;

		/// <summary>Crée une section repliable dont le conteneur respecte les réglages de disposition.</summary>
		protected FoldableSection CreateSection()
		{
			var section = new FoldableSection(WidgetUtils.MakeContentContainer(Layout, GridColumns, IsVertical));
			section.SetFoldable(Foldable, StartExpanded);
			return section;
		}
	}
}
