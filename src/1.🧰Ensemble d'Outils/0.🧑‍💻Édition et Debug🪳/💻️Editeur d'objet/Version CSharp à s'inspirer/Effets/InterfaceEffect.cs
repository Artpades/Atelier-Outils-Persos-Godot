using System;
using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🎛️ <b>Effet d'interface</b><br/>
	/// Classe de base <b>abstraite</b> d'une Resource qui modifie l'interface générée en fonction de l'état de l'objet
	/// inspecté (ex : cacher une donnée selon un booléen). Un effet est <b>surveillé en continu</b> tant que l'interface
	/// existe : il réagit aux changements sans reconstruire toute l'interface.<br/>
	/// <br/>
	/// <b>🧑‍🔧 Créer un effet</b> : hériter et surcharger <see cref="Apply"/>. Utiliser <see cref="Watch"/> pour obtenir un
	/// <see cref="EffectWatcher"/> qui réévalue périodiquement une condition et applique son résultat.
	/// </summary>
	[GlobalClass, Tool]
	public abstract partial class InterfaceEffect : Resource
	{
		/// <summary>Si faux, l'effet est ignoré (l'interface reste dans son état par défaut).</summary>
		[Export] public bool Enabled { get; set; } = true;

		/// <summary>Intervalle de surveillance, en secondes.</summary>
		[Export] public double PollInterval { get; set; } = PropertyBinding.DefaultPollInterval;

		/// <summary>
		/// Applique l'effet sous <paramref name="owner"/> (le conteneur racine de l'interface générée).
		/// </summary>
		/// <param name="owner">Conteneur auquel accrocher la surveillance (elle disparaît avec lui).</param>
		/// <param name="target">Objet inspecté.</param>
		/// <param name="widgets">Widget produit pour chaque donnée affichée, par <see cref="DataEntry.DataName"/>.</param>
		public abstract void Apply(Control owner, GodotObject target, IReadOnlyDictionary<string, Control> widgets);

		/// <summary>Crée et attache un <see cref="EffectWatcher"/> qui réévalue <paramref name="evaluate"/> et applique son résultat.</summary>
		protected EffectWatcher Watch(Control owner, Func<bool> evaluate, Action<bool> apply)
		{
			var watcher = new EffectWatcher { PollInterval = PollInterval, Evaluate = evaluate, ApplyResult = apply };
			owner.AddChild(watcher);
			return watcher;
		}

		/// <summary>Résout les noms de <paramref name="names"/> en widgets réellement présents dans <paramref name="widgets"/>.</summary>
		protected static List<Control> CollectTargets(IEnumerable<string> names, IReadOnlyDictionary<string, Control> widgets)
		{
			var result = new List<Control>();
			foreach (var name in names)
				if (widgets.TryGetValue(name, out var widget))
					result.Add(widget);
			return result;
		}

		/// <summary>Visite <paramref name="root"/> et tous ses descendants qui sont des <see cref="Control"/>.</summary>
		protected static void WalkControls(Node root, Action<Control> visit)
		{
			if (root is Control control)
				visit(control);
			foreach (Node child in root.GetChildren())
				WalkControls(child, visit);
		}
	}
}
