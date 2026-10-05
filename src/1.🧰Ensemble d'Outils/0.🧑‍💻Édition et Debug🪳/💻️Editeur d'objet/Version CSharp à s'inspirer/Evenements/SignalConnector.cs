using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🔌 <b>Connecteur de signaux</b> : établit ou retire les <see cref="SignalConnection"/> d'un signal.<br/>
	/// Les opérations sont <b>idempotentes</b> : appeler <see cref="Apply"/> plusieurs fois ne crée pas de doublons.<br/>
	/// Utilisable sans aucune interface : <c>SignalConnector.Apply(monObjet, "MonSignal", connexions)</c>.
	/// </summary>
	public static class SignalConnector
	{
		/// <summary>Connecte toutes les connexions actives de <paramref name="connections"/> au signal, et retire les autres.</summary>
		public static void Apply(GodotObject source, string signalName, IEnumerable<SignalConnection> connections)
		{
			if (!GodotObject.IsInstanceValid(source) || connections == null)
				return;

			foreach (var connection in connections)
				connection?.ConnectTo(source, signalName);
		}

		/// <summary>Retire toutes les connexions de <paramref name="connections"/> faites pour <paramref name="source"/>.</summary>
		public static void Clear(GodotObject source, IEnumerable<SignalConnection> connections)
		{
			if (!GodotObject.IsInstanceValid(source) || connections == null)
				return;

			foreach (var connection in connections)
				connection?.DisconnectFrom(source);
		}
	}
}
