using System;
using System.Text.RegularExpressions;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🧩 <b>Générateur de widget</b><br/>
	/// Classe de base <b>abstraite</b> d'une <b>Resource</b> qui sait construire un <see cref="Control"/> lié à un membre
	/// (propriété, méthode ou signal) d'un <see cref="GodotObject"/>.<br/>
	/// <br/>
	/// <b>📍 Choix du générateur</b> : le <see cref="PropertyWidgetRegistry"/> interroge
	/// <see cref="GetMatchScore"/> sur chaque générateur et retient le meilleur score.<br/>
	/// • Si <see cref="TargetPropertyName"/> est renseigné, le nom du membre <b>doit</b> correspondre (bonus de score).<br/>
	/// • <see cref="ScoreMember"/> (à surcharger) décide si le <b>type / l'indice</b> du membre convient.<br/>
	/// <br/>
	/// <b>🧑‍🔧 Créer un générateur</b> : hériter, surcharger <see cref="ScoreMember"/> et <see cref="GenerateWidget(GodotObject, string)"/>.
	/// Dans ce dernier, appeler <see cref="Bind"/> puis <see cref="Commit"/> dans les signaux du widget
	/// pour obtenir la liaison bidirectionnelle, et retourner <see cref="Labeled"/>.
	/// </summary>
	[GlobalClass, Tool]
	public abstract partial class PropertyWidgetGenerator : Resource
	{
		/// <summary>Bonus ajouté au score quand <see cref="TargetPropertyName"/> correspond : il prime sur toute correspondance par type.</summary>
		public const int NamedMatchBonus = 1000;

		#region ⚙️ Configuration
		/// <summary>
		/// Nom (ou motif, selon <see cref="NameMatchMode"/>) du membre ciblé.<br/>
		/// Vide : le générateur s'applique à tous les membres dont le <b>type</b> convient.
		/// </summary>
		[Export] public string TargetPropertyName { get; set; } = "";

		/// <summary>Manière d'interpréter <see cref="TargetPropertyName"/>.</summary>
		[Export] public PropertyNameMatchMode NameMatchMode { get; set; } = PropertyNameMatchMode.Exact;

		/// <summary>Départage deux générateurs de score égal (et permet de forcer un choix). Plus grand = prioritaire.</summary>
		[Export] public int Priority { get; set; }

		/// <summary>Affiche le nom du membre à gauche du widget.</summary>
		[Export] public bool ShowLabel { get; set; } = true;

		/// <summary>Poids du libellé dans la ligne (le widget prend le reste).</summary>
		[Export(PropertyHint.Range, "0.1,0.9,0.05")] public float LabelRatio { get; set; } = 0.4f;

		/// <summary>
		/// Orientation du <c>BoxContainer</c> qui englobe libellé + widget : <c>false</c> = libellé à gauche,
		/// <c>true</c> = libellé au-dessus. Pour les générateurs de conteneurs, c'est l'orientation du contenu.
		/// </summary>
		[Export] public bool IsVertical { get; set; }
		#endregion

		#region 🎯 Résolution
		/// <summary>Catégorie de membre gérée par ce générateur (propriété par défaut).</summary>
		public virtual MemberKind Kind => MemberKind.Property;

		/// <summary>
		/// Score de compatibilité avec un membre. <b>0 = incompatible.</b><br/>
		/// Ne pas surcharger : surcharger <see cref="ScoreMember"/>.
		/// </summary>
		/// <param name="target">Objet inspecté.</param>
		/// <param name="info">Description du membre (format de <c>GetPropertyList()</c> / <c>GetMethodList()</c> / <c>GetSignalList()</c>).</param>
		/// <param name="kind">Catégorie du membre.</param>
		public int GetMatchScore(GodotObject target, GodotDict info, MemberKind kind = MemberKind.Property)
		{
			if (info == null || kind != Kind)
				return 0;

			bool named = !string.IsNullOrEmpty(TargetPropertyName);
			if (named && !MatchesName(WidgetUtils.NameOf(info)))
				return 0;

			int score = ScoreMember(target, info);
			return score <= 0 ? 0 : score + Priority + (named ? NamedMatchBonus : 0);
		}

		/// <summary>
		/// Compatibilité <b>de type / d'indice</b> avec le membre. Convention : 100 = type correspondant,
		/// 150-200 = indice spécialisé (Range, Enum...). <b>0 = incompatible.</b><br/>
		/// Par défaut : compatible (score 1) uniquement si <see cref="TargetPropertyName"/> est renseigné.
		/// </summary>
		protected virtual int ScoreMember(GodotObject target, GodotDict info) =>
			string.IsNullOrEmpty(TargetPropertyName) ? 0 : 1;

		/// <summary>Teste <paramref name="memberName"/> contre <see cref="TargetPropertyName"/>.</summary>
		public bool MatchesName(string memberName)
		{
			if (string.IsNullOrEmpty(TargetPropertyName))
				return true;

			switch (NameMatchMode)
			{
				case PropertyNameMatchMode.Contains:
					return memberName.Contains(TargetPropertyName, StringComparison.OrdinalIgnoreCase);
				case PropertyNameMatchMode.Regex:
					try { return Regex.IsMatch(memberName, TargetPropertyName); }
					catch (ArgumentException) { return false; }
				default:
					return string.Equals(memberName, TargetPropertyName, StringComparison.OrdinalIgnoreCase);
			}
		}
		#endregion

		#region 🏗️ Génération
		/// <summary>
		/// Construit le <see cref="Control"/> lié au membre <paramref name="memberName"/> de <paramref name="target"/>.<br/>
		/// Utiliser <see cref="GetMemberInfo"/> pour retrouver son type, ses indices et son mode lecture seule.
		/// </summary>
		public abstract Control GenerateWidget(GodotObject target, string memberName);

		/// <summary>
		/// Variante recevant le <paramref name="registry"/> courant. À surcharger par les générateurs
		/// <b>récursifs</b> (objets, tableaux...) pour résoudre les widgets enfants avec le même registre.<br/>
		/// Par défaut : appelle <see cref="GenerateWidget(GodotObject, string)"/>.
		/// </summary>
		public virtual Control GenerateWidget(GodotObject target, string memberName, PropertyWidgetRegistry registry) =>
			GenerateWidget(target, memberName);
		#endregion

		#region 🔬 Introspection et accès
		/// <summary>
		/// Retrouve la description d'un membre via <c>GetPropertyList()</c>, <c>GetMethodList()</c> ou <c>GetSignalList()</c>
		/// (ou via <see cref="IMemberAccessor"/>). Retourne un dictionnaire <b>vide</b> si introuvable.
		/// </summary>
		public static GodotDict GetMemberInfo(GodotObject target, string memberName, MemberKind kind = MemberKind.Property)
		{
			if (target is IMemberAccessor accessor)
				return accessor.TryGetMemberInfo(memberName, kind, out var found) ? found : new GodotDict();

			var list = kind switch
			{
				MemberKind.Method => target.GetMethodList(),
				MemberKind.Signal => target.GetSignalList(),
				_ => target.GetPropertyList(),
			};
			foreach (var info in list)
				if (WidgetUtils.NameOf(info) == memberName)
					return info;
			return new GodotDict();
		}

		/// <summary>Lit une propriété (ou la valeur d'un <see cref="IMemberAccessor"/>).</summary>
		public static Variant ReadValue(GodotObject target, string memberName) =>
			target is IMemberAccessor accessor ? accessor.ReadValue(memberName) : target.Get(memberName);

		/// <summary>Écrit une propriété (ou la valeur d'un <see cref="IMemberAccessor"/>).</summary>
		public static void WriteValue(GodotObject target, string memberName, Variant value)
		{
			if (target is IMemberAccessor accessor) accessor.WriteValue(memberName, value);
			else target.Set(memberName, value);
		}

		/// <summary>Nom lisible d'un membre (<c>my_property</c> / <c>MyProperty</c> → « My Property »).</summary>
		public static string DisplayName(GodotObject target, string memberName) =>
			target is IMemberAccessor accessor ? accessor.GetDisplayName(memberName) : memberName.Capitalize();
		#endregion

		#region 🔗 Aides de liaison
		/// <summary>
		/// Attache un <see cref="PropertyBinding"/> à <paramref name="widget"/> : <paramref name="pull"/> est appelé
		/// à l'initialisation puis à chaque changement <b>extérieur</b> de la valeur.<br/>
		/// <paramref name="pull"/> ne doit <b>pas</b> réécrire dans l'objet (utiliser <c>SetValueNoSignal</c>, etc.).
		/// </summary>
		protected static PropertyBinding Bind(Control widget, GodotObject target, string memberName,
			Action<Variant> pull, Func<Variant, object> key = null) =>
			PropertyBinding.Attach(widget, target, memberName, pull, key);

		/// <summary>
		/// Écrit <paramref name="value"/> dans l'objet depuis un gestionnaire de signal du widget.<br/>
		/// Ignoré pendant une mise à jour venant de l'objet (<see cref="PropertyBinding.IsPulling"/>).
		/// </summary>
		protected static void Commit(PropertyBinding binding, GodotObject target, string memberName, Variant value)
		{
			if (binding == null || binding.IsPulling || !GodotObject.IsInstanceValid(target))
				return;
			WriteValue(target, memberName, value);
			binding.NotifyWritten(value);
			NotifyEdited(target);

			if (LogWrites)
				GD.Print($"[DynamicInspector] {Describe(target)}.{memberName} ← {GD.VarToStr(value)}   (relu dans l'objet : {GD.VarToStr(ReadValue(target, memberName))})");
		}

		/// <summary>
		/// Si vrai, chaque écriture faite par un widget est journalisée dans la console (objet, membre, valeur écrite et valeur
		/// relue dans l'objet). Réglé par <see cref="DynamicInspectorPanel.LogWrites"/>. Sert à vérifier qu'une modification
		/// atteint bien l'objet visé, notamment un sous-objet.
		/// </summary>
		public static bool LogWrites { get; set; }

		/// <summary>
		/// Identité d'un objet pour le journal : classe, identifiant d'instance et, pour une <see cref="Resource"/>, son chemin
		/// (une sous-ressource embarquée : <c>res://scene.tscn::Resource_xxx</c>) et son indicateur « local to scene ».
		/// Deux objets qui semblent identiques mais ont un identifiant différent sont deux <b>instances distinctes</b>.
		/// </summary>
		public static string Describe(GodotObject obj)
		{
			if (!GodotObject.IsInstanceValid(obj))
				return "<null>";
			string script = obj.GetScript().As<Script>()?.ResourcePath is { Length: > 0 } path ? $" script={path.GetFile()}" : "";
			return obj is Resource r
				? $"{obj}{script} path='{r.ResourcePath}' localToScene={r.ResourceLocalToScene}"
				: $"{obj}{script}";
		}

		/// <summary>
		/// Écrit une valeur dans l'objet <b>sans passer par un <see cref="PropertyBinding"/></b>, puis signale la modification
		/// (voir <see cref="NotifyEdited"/>). Pour les widgets qui modifient une <b>collection</b> : il faut toujours réécrire la
		/// collection modifiée dans l'objet, car une propriété exposée en tableau C# (<c>T[]</c>, <c>List&lt;T&gt;</c>...) renvoie
		/// à chaque lecture une <b>copie</b> : modifier cette copie sans la réécrire ne change rien dans l'objet.
		/// </summary>
		protected static void WriteAndNotify(GodotObject target, string memberName, Variant value)
		{
			if (!GodotObject.IsInstanceValid(target))
				return;
			WriteValue(target, memberName, value);
			NotifyEdited(target);

			if (LogWrites)
				GD.Print($"[DynamicInspector] {Describe(target)}.{memberName} ← {GD.VarToStr(value)}   (relu dans l'objet : {GD.VarToStr(ReadValue(target, memberName))})");
		}

		/// <summary>
		/// Signale qu'un objet vient d'être modifié depuis un widget :<br/>
		/// • une <see cref="Resource"/> émet <c>changed</c> (l'Inspecteur Godot et les autres vues se rafraîchissent) ;<br/>
		/// • dans l'éditeur, la scène est marquée comme <b>modifiée</b> (sinon Godot ne l'enregistre pas et relance le jeu avec
		/// les anciennes valeurs, surtout pour une sous-ressource embarquée dans la scène).
		/// </summary>
		protected static void NotifyEdited(GodotObject target)
		{
			if (target is Resource resource)
				resource.EmitChanged();
#if TOOLS
			if (Engine.IsEditorHint())
				EditorInterface.Singleton.MarkSceneAsUnsaved();
#endif
		}

		/// <summary>
		/// Place <paramref name="editor"/> à droite (ou sous, si <see cref="IsVertical"/>) du libellé du membre
		/// (sauf si <see cref="ShowLabel"/> est faux).
		/// </summary>
		protected Control Labeled(GodotObject target, string memberName, Control editor)
		{
			editor.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;
			editor.SizeFlagsStretchRatio = 1f - LabelRatio;
			if (!ShowLabel || string.IsNullOrEmpty(DisplayName(target, memberName)))
				return editor;

			var row = new BoxContainer { Vertical = IsVertical, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			row.AddChild(new Label
			{
				Text = DisplayName(target, memberName),
				TooltipText = memberName,
				MouseFilter = Control.MouseFilterEnum.Stop,
				ClipText = true,
				SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
				SizeFlagsVertical = Control.SizeFlags.ShrinkBegin,
				SizeFlagsStretchRatio = LabelRatio,
			});
			row.AddChild(editor);
			return row;
		}
		#endregion
	}
}
