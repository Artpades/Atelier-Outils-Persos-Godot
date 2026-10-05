using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🧷 <b>Argument d'un appel de signal</b> : d'où vient la valeur d'un paramètre de la méthode appelée.<br/>
	/// • <see cref="FromSignal"/> : le paramètre <b>reçoit automatiquement</b> l'argument émis par le signal à
	/// l'index <see cref="SignalIndex"/> (même index, même type : il n'est pas modifiable) ;<br/>
	/// • sinon, c'est la <see cref="Value"/> <b>statique</b> saisie dans l'éditeur.
	/// </summary>
	[GlobalClass, Tool]
	public partial class SignalArgument : Resource
	{
		/// <summary>Vrai si la valeur est fournie par le signal lors de l'émission.</summary>
		[Export] public bool FromSignal { get; set; }

		/// <summary>Index de l'argument du signal utilisé quand <see cref="FromSignal"/> est vrai.</summary>
		[Export] public int SignalIndex { get; set; }

		/// <summary>
		/// Valeur statique. Pour une référence à un <c>Node</c>, c'est un <see cref="NodePath"/> relatif à la racine de
		/// la scène (voir <see cref="IsNodeReference"/>) : un nœud ne peut pas être enregistré directement.
		/// </summary>
		[Export] public Variant Value { get; set; }

		/// <summary>Vrai si <see cref="Value"/> est le chemin d'un nœud à résoudre à l'appel (le paramètre reçoit le nœud).</summary>
		[Export] public bool IsNodeReference { get; set; }

		/// <summary>Valeur à passer à la méthode pour un argument statique (résout la référence de nœud éventuelle).</summary>
		public Variant ResolveStatic()
		{
			if (!IsNodeReference)
				return Value;
			return WidgetUtils.FindSceneRoot()?.GetNodeOrNull(Value.AsNodePath()) is { } node ? node : default;
		}
	}
}
