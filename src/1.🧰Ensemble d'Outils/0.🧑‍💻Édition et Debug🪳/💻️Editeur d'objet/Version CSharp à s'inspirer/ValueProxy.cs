using System;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🔌 <b>Proxy de valeur</b><br/>
	/// Objet léger exposant <b>une seule valeur</b> (nommée <see cref="ValueName"/>) via des lambdas
	/// <c>getter</c> / <c>setter</c>.<br/>
	/// Il permet de réutiliser les <see cref="PropertyWidgetGenerator"/> pour des données qui ne sont pas
	/// des propriétés Godot : <b>éléments d'un Array</b>, <b>valeurs d'un Dictionary</b>...
	/// </summary>
	public partial class ValueProxy : RefCounted, IMemberAccessor
	{
		/// <summary>Nom du membre unique exposé par le proxy.</summary>
		public const string ValueName = "value";

		private readonly Func<Variant> _getter;
		private readonly Action<Variant> _setter;
		private readonly GodotDict _info;
		private readonly string _displayName;

		/// <summary>Constructeur requis par Godot. Un proxy ainsi créé est vide.</summary>
		public ValueProxy() { }

		/// <param name="getter">Lit la valeur.</param>
		/// <param name="setter">Écrit la valeur.</param>
		/// <param name="type">Type Variant présenté aux générateurs.</param>
		/// <param name="displayName">Libellé affiché à gauche du widget.</param>
		/// <param name="hint">Indice de propriété (<see cref="PropertyHint"/>).</param>
		/// <param name="hintString">Chaîne d'indice associée.</param>
		/// <param name="readOnly">Si vrai, le widget est non éditable.</param>
		/// <param name="className">Classe déclarée d'un paramètre objet (<c>class_name</c>), ex : <c>Node2D</c>, <c>Texture2D</c>.</param>
		/// <param name="isVariant">Si vrai, la valeur est un <c>Variant</c> au type libre (type Nil + <c>NilIsVariant</c>) :
		/// c'est <see cref="VariantWidgetGenerator"/> qui la prend en charge.</param>
		public ValueProxy(Func<Variant> getter, Action<Variant> setter, Variant.Type type, string displayName,
			PropertyHint hint = PropertyHint.None, string hintString = "", bool readOnly = false, bool isVariant = false, string className = "")
		{
			_getter = getter;
			_setter = setter;
			_displayName = displayName;
			if (isVariant)
				type = Variant.Type.Nil;
			var usage = PropertyUsageFlags.Default
				| (readOnly ? PropertyUsageFlags.ReadOnly : 0)
				| (isVariant ? PropertyUsageFlags.NilIsVariant : 0);
			_info = new GodotDict
			{
				{ "name", ValueName },
				{ "class_name", className ?? "" },
				{ "type", (int)type },
				{ "hint", (int)hint },
				{ "hint_string", hintString },
				{ "usage", (int)usage },
			};
		}

		/// <inheritdoc/>
		public bool TryGetMemberInfo(string memberName, MemberKind kind, out GodotDict info)
		{
			info = _info;
			return _info != null && kind == MemberKind.Property && memberName == ValueName;
		}

		/// <inheritdoc/>
		public Variant ReadValue(string memberName) => _getter != null ? _getter() : default;

		/// <inheritdoc/>
		public void WriteValue(string memberName, Variant value) => _setter?.Invoke(value);

		/// <inheritdoc/>
		public string GetDisplayName(string memberName) => _displayName ?? memberName;
	}
}
