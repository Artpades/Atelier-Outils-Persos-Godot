using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🔎 <b>Évaluation d'une condition booléenne</b> partagée par les effets qui en ont besoin
	/// (<see cref="VisibilityEffect"/>, <see cref="InteractivityEffect"/>).<br/>
	/// La condition désigne soit une <b>propriété <c>bool</c></b>, soit une <b>méthode</b> (appelée avec ses valeurs
	/// par défaut) dont le résultat doit être un <c>bool</c>.
	/// </summary>
	public static class ConditionEvaluator
	{
		/// <summary>Lit la condition. <c>null</c> = illisible (membre introuvable, mauvais type) : un avertissement est émis.</summary>
		/// <param name="tag">Préfixe du message d'avertissement (nom de l'effet appelant).</param>
		public static bool? Read(GodotObject target, string memberName, string tag)
		{
			if (string.IsNullOrEmpty(memberName) || !GodotObject.IsInstanceValid(target))
				return null;

			var property = PropertyWidgetGenerator.GetMemberInfo(target, memberName, MemberKind.Property);
			if (property.Count > 0)
			{
				if (WidgetUtils.TypeOf(property) != Variant.Type.Bool)
				{
					GD.PushWarning($"[{tag}] « {memberName} » n'est pas une propriété booléenne.");
					return null;
				}
				return PropertyWidgetGenerator.ReadValue(target, memberName).AsBool();
			}

			var method = PropertyWidgetGenerator.GetMemberInfo(target, memberName, MemberKind.Method);
			if (method.Count == 0)
			{
				GD.PushWarning($"[{tag}] « {memberName} » introuvable (ni propriété, ni méthode).");
				return null;
			}

			var result = target.Call(memberName, WidgetUtils.DefaultArguments(method));
			if (result.VariantType != Variant.Type.Bool)
			{
				GD.PushWarning($"[{tag}] « {memberName}() » ne retourne pas un booléen.");
				return null;
			}
			return result.AsBool();
		}

		/// <summary>
		/// Décrit la condition pour l'éditeur : « ✔ propriété bool », « ✔ méthode → bool », un message d'erreur, ou vide.<br/>
		/// <b>📍 Note :</b> le type de retour d'une méthode n'est vérifiable ici que si le dictionnaire renvoyé par
		/// <c>GetMethodList()</c> contient la clé <c>"return"</c> (comportement standard de Godot, non garanti par le SDK).
		/// L'évaluation réelle (<see cref="Read"/>) vérifie de toute façon le type du résultat après l'appel.
		/// </summary>
		public static string Describe(GodotObject target, string memberName)
		{
			if (string.IsNullOrEmpty(memberName) || !GodotObject.IsInstanceValid(target))
				return "";

			var property = PropertyWidgetGenerator.GetMemberInfo(target, memberName, MemberKind.Property);
			if (property.Count > 0)
				return WidgetUtils.TypeOf(property) == Variant.Type.Bool ? "✔ propriété bool" : "✘ propriété non booléenne";

			var method = PropertyWidgetGenerator.GetMemberInfo(target, memberName, MemberKind.Method);
			if (method.Count > 0)
			{
				if (method.TryGetValue("return", out var returnInfo) && returnInfo.VariantType == Variant.Type.Dictionary)
					return WidgetUtils.TypeOf(returnInfo.AsGodotDictionary()) == Variant.Type.Bool
						? "✔ méthode → bool" : "✘ méthode : ne retourne pas un bool";
				return "✔ méthode trouvée (type de retour vérifié à l'appel)";
			}
			return "✘ introuvable";
		}
	}
}
