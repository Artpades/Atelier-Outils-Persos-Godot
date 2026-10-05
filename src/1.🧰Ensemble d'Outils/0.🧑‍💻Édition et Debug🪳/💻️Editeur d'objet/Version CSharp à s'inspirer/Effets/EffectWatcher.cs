using System;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 👁️ <b>Surveillance d'effet</b> : nœud invisible qui réévalue périodiquement <see cref="Evaluate"/> et appelle
	/// <see cref="ApplyResult"/> quand le résultat change (jamais à chaque frame). Même principe que
	/// <see cref="PropertyBinding"/>, généralisé à une condition quelconque (pas seulement la valeur d'une propriété).
	/// </summary>
	[Tool]
	public partial class EffectWatcher : Node
	{
		/// <summary>Intervalle entre deux vérifications, en secondes.</summary>
		public double PollInterval { get; set; } = PropertyBinding.DefaultPollInterval;

		/// <summary>Calcule l'état courant. Ne doit pas lancer d'exception (attraper les cas invalides et retourner une valeur par défaut).</summary>
		public Func<bool> Evaluate { get; set; }

		/// <summary>Applique l'état à l'interface.</summary>
		public Action<bool> ApplyResult { get; set; }

		private bool _hasLast, _last;
		private double _elapsed;

		/// <inheritdoc/>
		public override void _Ready() => Refresh(true);

		/// <inheritdoc/>
		public override void _Process(double delta)
		{
			_elapsed += delta;
			if (_elapsed < PollInterval)
				return;
			_elapsed = 0;
			Refresh(false);
		}

		/// <summary>Réévalue la condition et applique le résultat s'il a changé (ou si <paramref name="force"/>).</summary>
		public void Refresh(bool force = false)
		{
			if (Evaluate == null)
				return;

			bool value;
			try { value = Evaluate(); }
			catch (Exception e) { GD.PushWarning($"[EffectWatcher] Évaluation impossible : {e.Message}"); return; }

			if (!force && _hasLast && value == _last)
				return;
			_hasLast = true;
			_last = value;
			ApplyResult?.Invoke(value);
		}
	}
}
