using System;
using System.Collections.Generic;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🔗 <b>Appel persistant d'un signal</b>, à la manière d'un <c>UnityEvent</c> : à chaque émission du signal, la méthode
	/// <see cref="TargetMethod"/> est appelée sur l'objet désigné par <see cref="TargetPath"/>.<br/>
	/// <br/>
	/// <b>📍 Cible</b> : <see cref="TargetPath"/> est relatif à l'objet émetteur (s'il est un <see cref="Node"/>) ou à la racine
	/// de la scène ; vide, la cible est l'émetteur lui-même.<br/>
	/// <b>📍 Paramètres</b> : <see cref="Arguments"/> a une entrée par paramètre de la méthode. Un paramètre qui a le même
	/// index et le même type qu'un argument du signal reçoit <b>cet argument</b> ; les autres reçoivent leur valeur statique.<br/>
	/// Les connexions sont établies par <see cref="SignalConnector"/> ; cette Resource suit les connexions qu'elle a faites.
	/// </summary>
	[GlobalClass, Tool]
	public partial class SignalConnection : Resource
	{
		private readonly Dictionary<ulong, (string Signal, Callable Callable)> _live = new();
		private bool _invoking;

		/// <summary>Si faux, l'appel est ignoré.</summary>
		[Export] public bool Enabled { get; set; } = true;

		/// <summary>Chemin de l'objet à appeler (vide : l'émetteur lui-même).</summary>
		[Export] public NodePath TargetPath { get; set; } = new();

		/// <summary>Méthode appelée.</summary>
		[Export] public string TargetMethod { get; set; } = "";

		/// <summary>Un argument par paramètre de la méthode, dans l'ordre.</summary>
		[Export] public Godot.Collections.Array<SignalArgument> Arguments { get; set; } = new();

		/// <summary>Retrouve l'objet à appeler pour un émetteur donné, ou <c>null</c>.</summary>
		public GodotObject ResolveReceiver(GodotObject source)
		{
			if (TargetPath == null || TargetPath.IsEmpty)
				return source;
			return (source as Node ?? WidgetUtils.FindSceneRoot())?.GetNodeOrNull(TargetPath);
		}

		/// <summary>Connecte cet appel au signal <paramref name="signalName"/> de <paramref name="source"/> (remplace une connexion précédente).</summary>
		public void ConnectTo(GodotObject source, string signalName)
		{
			DisconnectFrom(source);
			if (!Enabled || string.IsNullOrEmpty(TargetMethod) || !GodotObject.IsInstanceValid(source))
				return;

			var info = PropertyWidgetGenerator.GetMemberInfo(source, signalName, MemberKind.Signal);
			int count = info.TryGetValue("args", out var args) ? args.AsGodotArray().Count : 0;
			if (!TryBuildCallable(count, values => Invoke(source, values), out var callable))
			{
				GD.PushWarning($"[SignalConnection] Signal « {signalName} » : {count} arguments, 6 au maximum sont pris en charge.");
				return;
			}

			source.Connect(signalName, callable);
			_live[source.GetInstanceId()] = (signalName, callable);
		}

		/// <summary>Retire la connexion faite pour <paramref name="source"/>, s'il y en a une.</summary>
		public void DisconnectFrom(GodotObject source)
		{
			if (!GodotObject.IsInstanceValid(source) || !_live.Remove(source.GetInstanceId(), out var live))
				return;
			if (source.IsConnected(live.Signal, live.Callable))
				source.Disconnect(live.Signal, live.Callable);
		}

		/// <summary>
		/// Exécute l'appel : assemble les paramètres (arguments du signal ou valeurs statiques) puis appelle la méthode.
		/// </summary>
		/// <param name="source">Émetteur du signal.</param>
		/// <param name="signalArgs">Arguments émis.</param>
		public void Invoke(GodotObject source, Variant[] signalArgs)
		{
			if (!Enabled || string.IsNullOrEmpty(TargetMethod))
				return;

			var receiver = ResolveReceiver(source);
			if (!GodotObject.IsInstanceValid(receiver))
			{
				GD.PushWarning($"[SignalConnection] Cible « {TargetPath} » introuvable pour {TargetMethod}().");
				return;
			}

			var values = new Variant[Arguments.Count];
			for (int i = 0; i < values.Length; i++)
			{
				var argument = Arguments[i];
				values[i] = argument == null ? default
					: argument.FromSignal ? (argument.SignalIndex < signalArgs.Length ? signalArgs[argument.SignalIndex] : default)
					: argument.ResolveStatic();
			}
			// Protection contre la ré-entrée : une méthode qui réémet le signal qui l'a appelée ferait une boucle infinie.
			if (_invoking)
			{
				GD.PushWarning($"[SignalConnection] Appel récursif de {TargetMethod}() ignoré (la méthode réémet le signal qui la déclenche).");
				return;
			}
			_invoking = true;
			try { receiver.Call(TargetMethod, values); }
			finally { _invoking = false; }
		}

		/// <summary>Construit un <see cref="Callable"/> acceptant exactement <paramref name="count"/> arguments (0 à 6).</summary>
		private static bool TryBuildCallable(int count, Action<Variant[]> handler, out Callable callable)
		{
			switch (count)
			{
				case 0: callable = Callable.From(() => handler(Array.Empty<Variant>())); return true;
				case 1: callable = Callable.From((Variant a) => handler(new[] { a })); return true;
				case 2: callable = Callable.From((Variant a, Variant b) => handler(new[] { a, b })); return true;
				case 3: callable = Callable.From((Variant a, Variant b, Variant c) => handler(new[] { a, b, c })); return true;
				case 4: callable = Callable.From((Variant a, Variant b, Variant c, Variant d) => handler(new[] { a, b, c, d })); return true;
				case 5: callable = Callable.From((Variant a, Variant b, Variant c, Variant d, Variant e) => handler(new[] { a, b, c, d, e })); return true;
				case 6: callable = Callable.From((Variant a, Variant b, Variant c, Variant d, Variant e, Variant f) => handler(new[] { a, b, c, d, e, f })); return true;
				default: callable = default; return false;
			}
		}
	}
}
