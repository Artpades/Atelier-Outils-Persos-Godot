using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📚 <b>Registre de générateurs</b><br/>
	/// Une <b>Resource</b> qui liste les <see cref="PropertyWidgetGenerator"/> disponibles et choisit le plus adapté
	/// à chaque membre via <see cref="GetGeneratorFor(GodotObject, GodotDict, MemberKind)"/>.<br/>
	/// <br/>
	/// <b>📍 Ordre de priorité</b> : les générateurs de <see cref="Generators"/> (personnalisés) sont évalués <b>avant</b> les
	/// générateurs par défaut ; à score égal, le premier rencontré l'emporte.<br/>
	/// Pour remplacer un défaut, donner à son générateur un <see cref="PropertyWidgetGenerator.Priority"/> plus élevé
	/// ou un <see cref="PropertyWidgetGenerator.TargetPropertyName"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class PropertyWidgetRegistry : Resource
	{
		private PropertyWidgetGenerator[] _defaults;

		/// <summary>Générateurs personnalisés, évalués en premier.</summary>
		[Export] public Godot.Collections.Array<PropertyWidgetGenerator> Generators { get; set; } = new();

		/// <summary>Ajoute (après <see cref="Generators"/>) les générateurs fournis par <see cref="CreateDefaultGenerators"/>.</summary>
		[Export] public bool IncludeDefaultGenerators { get; set; } = true;

		/// <summary>
		/// Trouve le générateur le plus adapté à une <b>propriété</b>.
		/// </summary>
		/// <param name="target">Objet inspecté.</param>
		/// <param name="propertyInfo">Entrée de <c>GetPropertyList()</c> (clés <c>name</c>, <c>type</c>, <c>hint</c>, <c>hint_string</c>, <c>usage</c>).</param>
		/// <returns>Le générateur, ou <c>null</c> si aucun ne convient.</returns>
		public PropertyWidgetGenerator GetGeneratorFor(GodotObject target, GodotDict propertyInfo) =>
			GetGeneratorFor(target, propertyInfo, MemberKind.Property);

		/// <summary>Trouve le générateur le plus adapté à un membre de catégorie <paramref name="kind"/>.</summary>
		public PropertyWidgetGenerator GetGeneratorFor(GodotObject target, GodotDict memberInfo, MemberKind kind)
		{
			PropertyWidgetGenerator best = null;
			int bestScore = 0;

			foreach (var generator in Generators)
				Consider(generator);

			if (IncludeDefaultGenerators)
			{
				_defaults ??= CreateDefaultGenerators();
				foreach (var generator in _defaults)
					Consider(generator);
			}
			return best;

			void Consider(PropertyWidgetGenerator generator)
			{
				if (generator == null)
					return;
				int score = generator.GetMatchScore(target, memberInfo, kind);
				if (score > bestScore)
				{
					best = generator;
					bestScore = score;
				}
			}
		}

		/// <summary>Résout le générateur puis construit le widget. Retourne <c>null</c> si aucun générateur ne convient.</summary>
		public Control BuildWidget(GodotObject target, GodotDict memberInfo, MemberKind kind = MemberKind.Property)
		{
			var generator = GetGeneratorFor(target, memberInfo, kind);
			return generator?.GenerateWidget(target, WidgetUtils.NameOf(memberInfo), this);
		}

		/// <summary>Crée un registre ne contenant que les générateurs par défaut.</summary>
		public static PropertyWidgetRegistry CreateDefault() => new();

		/// <summary>Instancie un exemplaire de chaque générateur fourni d'origine.</summary>
		public static PropertyWidgetGenerator[] CreateDefaultGenerators() => new PropertyWidgetGenerator[]
		{
			new BoolWidgetGenerator(),
			new NumberWidgetGenerator(),
			new StringWidgetGenerator(),
			new EnumWidgetGenerator(),
			new ColorWidgetGenerator(),
			new StructWidgetGenerator(),
			new NodePathWidgetGenerator(),
			new ObjectWidgetGenerator(),
			new CollectionWidgetGenerator(),
			new PackedArrayWidgetGenerator(),
			new VariantWidgetGenerator(),
			new RidWidgetGenerator(),
			new CallableWidgetGenerator(),
			new MethodWidgetGenerator(),
			new SignalWidgetGenerator(),
			new LabelWidgetGenerator(), // priorité basse : ne gagne que si rien d'autre ne convient, ou par TargetPropertyName
		};
	}
}
