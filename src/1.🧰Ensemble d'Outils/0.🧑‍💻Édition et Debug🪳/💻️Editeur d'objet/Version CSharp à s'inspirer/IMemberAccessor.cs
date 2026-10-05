using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// Contrat optionnel pour un <see cref="GodotObject"/> qui <b>simule</b> des membres
	/// (ex : <see cref="ValueProxy"/> pour un élément de tableau).<br/>
	/// Quand la cible d'un générateur l'implémente, <see cref="PropertyWidgetGenerator"/> l'utilise
	/// à la place de <c>Get()</c> / <c>Set()</c> / <c>GetPropertyList()</c>.
	/// </summary>
	public interface IMemberAccessor
	{
		/// <summary>Retourne la description du membre (format identique à <c>GetPropertyList()</c>).</summary>
		bool TryGetMemberInfo(string memberName, MemberKind kind, out GodotDict info);

		/// <summary>Lit la valeur courante du membre.</summary>
		Variant ReadValue(string memberName);

		/// <summary>Écrit la valeur du membre.</summary>
		void WriteValue(string memberName, Variant value);

		/// <summary>Nom lisible affiché à gauche du widget.</summary>
		string GetDisplayName(string memberName);
	}
}
