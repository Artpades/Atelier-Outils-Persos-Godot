using System;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🔗 <b>Liaison de données bidirectionnelle</b><br/>
	/// Nœud invisible attaché à un widget. Il <b>surveille</b> (polling) la valeur du membre cible et appelle
	/// <c>pull</c> quand elle change de l'extérieur : le widget <b>réagit</b> aux modifications de l'objet.<br/>
	/// Le sens inverse (widget → objet) passe par <see cref="NotifyWritten"/> après chaque écriture.<br/>
	/// <br/>
	/// Godot n'offre pas de signal générique « propriété modifiée » : le polling est donc la seule
	/// méthode fiable pour tout type d'objet. Le coût est borné par <see cref="PollInterval"/>.
	/// </summary>
	[Tool]
	public partial class PropertyBinding : Node
	{
		/// <summary>Intervalle de vérification par défaut, en secondes.</summary>
		public const double DefaultPollInterval = 0.1;

		/// <summary>Intervalle entre deux vérifications, en secondes.</summary>
		public double PollInterval { get; set; } = DefaultPollInterval;

		/// <summary>
		/// Vrai pendant que le binding <b>pousse une valeur vers le widget</b>.<br/>
		/// Les gestionnaires de signaux du widget doivent l'ignorer pour éviter une boucle d'écho.
		/// </summary>
		public bool IsPulling { get; private set; }

		private GodotObject _target;
		private string _member;
		private Action<Variant> _pull;
		private Func<Variant, object> _key;
		private object _lastKey;
		private bool _hasLast;
		private double _elapsed;

		/// <summary>
		/// Crée un binding enfant de <paramref name="owner"/>. La première synchronisation a lieu à l'entrée dans l'arbre
		/// (<c>_Ready</c>), donc <b>après</b> que le générateur a connecté ses signaux.
		/// </summary>
		/// <param name="owner">Widget propriétaire (le binding disparaît avec lui).</param>
		/// <param name="target">Objet inspecté.</param>
		/// <param name="member">Nom du membre lu.</param>
		/// <param name="pull">Applique une valeur au widget <b>sans</b> réécrire dans l'objet.</param>
		/// <param name="key">Extrait une clé de comparaison (comparée avec <c>Equals</c>). Par défaut : type + hash du contenu.</param>
		public static PropertyBinding Attach(Control owner, GodotObject target, string member,
			Action<Variant> pull, Func<Variant, object> key = null)
		{
			var binding = new PropertyBinding
			{
				Name = "PropertyBinding",
				_target = target,
				_member = member,
				_pull = pull,
				_key = key ?? DefaultKey,
			};
			owner.AddChild(binding);
			return binding;
		}

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

		/// <summary>Relit la valeur et met le widget à jour si elle a changé (ou si <paramref name="force"/>).</summary>
		public void Refresh(bool force = false)
		{
			if (!GodotObject.IsInstanceValid(_target))
			{
				SetProcess(false);
				return;
			}

			Variant value;
			try { value = PropertyWidgetGenerator.ReadValue(_target, _member); }
			catch (Exception) { return; }

			object key = _key(value);
			if (!force && _hasLast && Equals(key, _lastKey))
				return;

			_lastKey = key;
			_hasLast = true;
			Pull(value);
		}

		/// <summary>
		/// À appeler juste <b>après</b> avoir écrit <paramref name="sent"/> dans l'objet.<br/>
		/// Mémorise la valeur pour ne pas la renvoyer au widget, et corrige l'affichage si l'objet
		/// a <b>transformé</b> la valeur (clamp, arrondi, setter personnalisé).
		/// </summary>
		public void NotifyWritten(Variant sent)
		{
			if (!GodotObject.IsInstanceValid(_target))
				return;
			Variant actual = PropertyWidgetGenerator.ReadValue(_target, _member);
			object before = _lastKey;
			_lastKey = _key(actual);

			if (!AreClose(sent, actual))
			{
				// Écriture sans effet : l'objet a gardé sa valeur (setter qui refuse, propriété non écrivable, nom inconnu...).
				// Le widget revient à la valeur réelle ; on le signale pour ne pas laisser croire que la modification a réussi.
				if (_hasLast && Equals(before, _lastKey))
					GD.PushWarning($"[DynamicInspector] « {_member} » sur {_target} : la valeur écrite ({GD.VarToStr(sent)}) n'a pas été prise en compte (valeur relue : {GD.VarToStr(actual)}).");
				Pull(actual);
			}
			_hasLast = true;
		}

		private void Pull(Variant value)
		{
			IsPulling = true;
			try { _pull(value); }
			finally { IsPulling = false; }
		}

		/// <summary>
		/// Force la relecture de tous les <see cref="PropertyBinding"/> sous <paramref name="root"/>.
		/// Utile après un changement dont on est notifié (ex : signal <c>changed</c> d'une Resource).
		/// </summary>
		public static void RefreshAll(Node root)
		{
			if (!GodotObject.IsInstanceValid(root))
				return;
			if (root is PropertyBinding binding)
				binding.Refresh(true);
			foreach (Node child in root.GetChildren())
				RefreshAll(child);
		}

		/// <summary>
		/// Clé de comparaison par défaut : le type et le <b>hash de contenu</b> du Variant (<see cref="GD.Hash"/>).<br/>
		/// <see cref="Variant"/> ne redéfinit pas <c>Equals</c> : sa comparaison native compare des pointeurs bruts,
		/// donc deux lectures d'une même chaîne paraissent différentes. Le hash compare le <b>contenu</b>
		/// (y compris celui des Array / Dictionary, et l'identité pour les objets).
		/// </summary>
		private static object DefaultKey(Variant value) => (value.VariantType, GD.Hash(value));

		private static bool AreClose(Variant a, Variant b)
		{
			if (a.VariantType is Variant.Type.Float or Variant.Type.Int
				&& b.VariantType is Variant.Type.Float or Variant.Type.Int)
				return Math.Abs(a.AsDouble() - b.AsDouble()) <= 1e-4 * Math.Max(1.0, Math.Abs(b.AsDouble()));
			return a.VariantType == b.VariantType && GD.Hash(a) == GD.Hash(b);
		}
	}
}
