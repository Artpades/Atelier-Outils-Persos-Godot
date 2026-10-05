using System.Collections.Generic;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// Widget choisi pour afficher une donnée (<see cref="DataEntry.WidgetChoice"/>).<br/>
	/// Les choix valides dépendent du <see cref="DataKind"/> : voir <see cref="WidgetTable.AllowedFor"/>.
	/// </summary>
	public enum WidgetChoice
	{
		/// <summary>Le widget par défaut du type de donnée (premier de la table).</summary>
		Auto,
		/// <summary>Case à cocher.</summary>
		CheckBox,
		/// <summary>Interrupteur.</summary>
		CheckButton,
		/// <summary>Simple texte (lecture seule).</summary>
		Label,
		/// <summary>Champ numérique.</summary>
		SpinBox,
		/// <summary>Curseur horizontal.</summary>
		HSlider,
		/// <summary>Curseur vertical.</summary>
		VSlider,
		/// <summary>Barre de progression.</summary>
		ProgressBar,
		/// <summary>Barre de progression texturée.</summary>
		TextureProgressBar,
		/// <summary>Champ de texte sur une ligne.</summary>
		LineEdit,
		/// <summary>Zone de texte multiligne.</summary>
		TextEdit,
		/// <summary>Liste déroulante.</summary>
		OptionButton,
		/// <summary>Liste de <c>CheckButton</c> (exclusifs pour un enum, multiples pour des flags).</summary>
		CheckButtonList,
		/// <summary>Bouton sélecteur de couleur.</summary>
		ColorPickerButton,
		/// <summary><c>ColorRect</c> d'aperçu et un <c>SpinBox</c> par canal RGBA.</summary>
		ColorRectChannels,
		/// <summary>Bouton qui exécute la méthode / émet le signal / appelle la <c>Callable</c>.</summary>
		Button,
		/// <summary>Contenu dans un <c>BoxContainer</c> (vertical ou horizontal selon <see cref="DataEntry.IsVertical"/>).</summary>
		BoxContainer,
		/// <summary>Contenu dans un <c>GridContainer</c> (clé extra <c>columns</c>).</summary>
		GridContainer,
		/// <summary>Bouton de sélection d'un nœud (arbre de scène) ou d'une ressource (fichier).</summary>
		PathButton,
		/// <summary>Un <c>SpinBox</c> par champ de la structure.</summary>
		StructSpinBoxes,
		/// <summary>Utilise <see cref="DataEntry.CustomGenerator"/>.</summary>
		Custom,
		/// <summary><c>OptionButton</c> de choix du type + widget du type choisi (valeur <c>Variant</c>).</summary>
		VariantSelector
	}

	/// <summary>Famille de donnée, déduite de l'introspection. Détermine les widgets autorisés.</summary>
	public enum DataKind
	{
		/// <summary>Type sans widget natif : seul <see cref="WidgetChoice.Custom"/> est possible.</summary>
		Unsupported,
		/// <summary><c>bool</c>.</summary>
		Bool,
		/// <summary><c>int</c> / <c>float</c>.</summary>
		Number,
		/// <summary><c>string</c> / <c>StringName</c>.</summary>
		Text,
		/// <summary>Énumération (<c>PropertyHint.Enum</c>).</summary>
		Enum,
		/// <summary>Masque de bits (<c>PropertyHint.Flags</c>).</summary>
		Flags,
		/// <summary><see cref="Color"/>.</summary>
		Color,
		/// <summary>Méthode ou propriété <c>Callable</c>.</summary>
		Method,
		/// <summary>Signal.</summary>
		Signal,
		/// <summary>Objet (<c>Node</c>, <c>Resource</c>...).</summary>
		Object,
		/// <summary><see cref="NodePath"/>.</summary>
		NodePath,
		/// <summary><c>Array</c>.</summary>
		Array,
		/// <summary><c>Dictionary</c>.</summary>
		Dictionary,
		/// <summary>Structure Godot (<c>Vector2</c>, <c>Rect2</c>, <c>Basis</c>, <c>Transform2D/3D</c>, <c>Projection</c>...).</summary>
		Struct,
		/// <summary>Valeur <c>Variant</c> au type libre.</summary>
		Variant,
		/// <summary><see cref="Rid"/> (lecture seule).</summary>
		Rid,
		/// <summary><c>Packed*Array</c>.</summary>
		PackedArray
	}

	/// <summary>Type de conteneur d'un groupe (<see cref="GroupEntry.ContainerType"/>).</summary>
	public enum GroupContainerType
	{
		/// <summary>Conteneur vertical.</summary>
		VBoxContainer,
		/// <summary>Conteneur horizontal.</summary>
		HBoxContainer,
		/// <summary>Grille (nombre de colonnes : <see cref="GroupEntry.Columns"/>).</summary>
		GridContainer,
		/// <summary>Cadre <c>PanelContainer</c> entourant un conteneur (orienté par <see cref="GroupEntry.IsVertical"/>).</summary>
		PanelContainer
	}

	/// <summary>
	/// 📋 <b>Table des widgets autorisés par type de donnée</b> et classification des membres.<br/>
	/// C'est la source de vérité utilisée par l'éditeur de configuration et par
	/// <see cref="EntryGeneratorFactory"/>.
	/// </summary>
	public static class WidgetTable
	{
		private static readonly Dictionary<DataKind, WidgetChoice[]> Allowed = new()
		{
			[DataKind.Bool] = new[] { WidgetChoice.CheckBox, WidgetChoice.CheckButton, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Number] = new[] { WidgetChoice.SpinBox, WidgetChoice.HSlider, WidgetChoice.VSlider, WidgetChoice.ProgressBar, WidgetChoice.TextureProgressBar, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Text] = new[] { WidgetChoice.LineEdit, WidgetChoice.TextEdit, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Enum] = new[] { WidgetChoice.OptionButton, WidgetChoice.CheckButtonList, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Flags] = new[] { WidgetChoice.CheckButtonList, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Color] = new[] { WidgetChoice.ColorPickerButton, WidgetChoice.ColorRectChannels, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Method] = new[] { WidgetChoice.Button, WidgetChoice.Custom },
			[DataKind.Signal] = new[] { WidgetChoice.Button, WidgetChoice.Custom },
			[DataKind.Object] = new[] { WidgetChoice.BoxContainer, WidgetChoice.GridContainer, WidgetChoice.PathButton, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.NodePath] = new[] { WidgetChoice.BoxContainer, WidgetChoice.GridContainer, WidgetChoice.PathButton, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Array] = new[] { WidgetChoice.BoxContainer, WidgetChoice.GridContainer, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Dictionary] = new[] { WidgetChoice.BoxContainer, WidgetChoice.GridContainer, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Struct] = new[] { WidgetChoice.StructSpinBoxes, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Variant] = new[] { WidgetChoice.VariantSelector, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Rid] = new[] { WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.PackedArray] = new[] { WidgetChoice.BoxContainer, WidgetChoice.GridContainer, WidgetChoice.Label, WidgetChoice.Custom },
			[DataKind.Unsupported] = new[] { WidgetChoice.Label, WidgetChoice.Custom },
		};

		/// <summary>Widgets autorisés pour un type de donnée (hors <see cref="WidgetChoice.Auto"/>). Le premier est le défaut.</summary>
		public static IReadOnlyList<WidgetChoice> AllowedFor(DataKind kind) => Allowed[kind];

		/// <summary>Widget utilisé quand le choix est <see cref="WidgetChoice.Auto"/>.</summary>
		public static WidgetChoice DefaultFor(DataKind kind) => Allowed[kind][0];

		/// <summary>Vrai si <paramref name="choice"/> est valide pour <paramref name="kind"/> (<c>Auto</c> l'est toujours).</summary>
		public static bool IsAllowed(DataKind kind, WidgetChoice choice) =>
			choice == WidgetChoice.Auto || System.Array.IndexOf(Allowed[kind], choice) >= 0;

		/// <summary>Classe un membre à partir de sa description d'introspection.</summary>
		public static DataKind KindOf(GodotDict info, MemberKind member)
		{
			if (member == MemberKind.Method) return DataKind.Method;
			if (member == MemberKind.Signal) return DataKind.Signal;

			var hint = WidgetUtils.HintOf(info);
			var type = WidgetUtils.TypeOf(info);
			return type switch
			{
				Variant.Type.Bool => DataKind.Bool,
				Variant.Type.Int => hint == PropertyHint.Enum ? DataKind.Enum : hint == PropertyHint.Flags ? DataKind.Flags : DataKind.Number,
				Variant.Type.Float => DataKind.Number,
				Variant.Type.String or Variant.Type.StringName => hint == PropertyHint.Enum ? DataKind.Enum : DataKind.Text,
				Variant.Type.Color => DataKind.Color,
				Variant.Type.Callable => DataKind.Method,
				Variant.Type.NodePath => DataKind.NodePath,
				Variant.Type.Object => DataKind.Object,
				Variant.Type.Array => DataKind.Array,
				Variant.Type.Dictionary => DataKind.Dictionary,
				Variant.Type.Rid => DataKind.Rid,
				Variant.Type.Nil => WidgetUtils.IsVariantMember(info) ? DataKind.Variant : DataKind.Unsupported,
				_ => PackedArrays.IsPacked(type) ? DataKind.PackedArray
					: WidgetUtils.IsStructType(type) ? DataKind.Struct : DataKind.Unsupported,
			};
		}

		/// <summary>Vrai pour les types dont le contenu s'organise dans un conteneur (orientation = <see cref="DataEntry.IsVertical"/>).</summary>
		public static bool IsContainerKind(DataKind kind) =>
			kind is DataKind.Object or DataKind.NodePath or DataKind.Array or DataKind.Dictionary or DataKind.PackedArray;

		/// <summary>Aide affichée dans l'éditeur : clés <see cref="DataEntry.ExtraData"/> reconnues par un widget.</summary>
		public static string ExtraHelp(WidgetChoice choice)
		{
			const string common = "show_label (bool), label_ratio (0.1-0.9)";
			return choice switch
			{
				WidgetChoice.SpinBox => $"suffix (texte), step. {common}",
				WidgetChoice.HSlider or WidgetChoice.VSlider or WidgetChoice.ProgressBar =>
					$"min, max, step : valeurs par défaut si la propriété n'a pas d'indice Range. {common}",
				WidgetChoice.TextureProgressBar => $"min, max, step, texture_progress (chemin res://), texture_under (chemin res://). {common}",
				WidgetChoice.TextEdit => $"height (pixels). {common}",
				WidgetChoice.BoxContainer => $"foldable (bool, défaut true), expanded (bool, défaut false). {common}",
				WidgetChoice.GridContainer => $"columns (int), foldable, expanded, max_displayed (int, Packed*Array). {common}",
				WidgetChoice.StructSpinBoxes => $"step. {common}",
				WidgetChoice.VariantSelector => common,
				_ => common,
			};
		}
	}
}
