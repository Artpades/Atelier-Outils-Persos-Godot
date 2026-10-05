using System;
using System.Collections.Generic;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🖊️ <b>Pilote de gizmos</b> : nœud invisible qui exécute, <b>à chaque image</b>, les appels configurés dans un
	/// <see cref="GizmoRegistry"/> en résolvant leurs paramètres depuis l'objet inspecté.<br/>
	/// <br/>
	/// <b>📍 Un gizmo par image, pas par changement</b> : contrairement à <see cref="PropertyBinding"/> /
	/// <see cref="EffectWatcher"/> (sondage périodique, pensé pour de l'UI), les gizmos sont redessinés à
	/// <b>chaque <c>_Process</c></b> pour suivre une position qui bouge en continu, comme le fait <see cref="Gizmos"/>
	/// lui-même (les méthodes 3D <b>accumulent</b> sur un même <see cref="Rid"/> tant qu'il n'est pas vidé : ce pilote
	/// vide le sien une fois par image avant d'y rejouer tous les appels 3D actifs).<br/>
	/// <b>⚠️ Attention</b> : les méthodes 2D, elles, <b>effacent</b> leur <see cref="Rid"/> avant de dessiner — chaque
	/// appel 2D actif a donc son <b>propre</b> <see cref="Rid"/>, sans quoi les gizmos 2D s'effaceraient entre eux.<br/>
	/// <br/>
	/// <b>📍 Cible</b> : <see cref="ResolveTarget"/> est relu à chaque image (pas mis en cache), pour rester correct si
	/// l'objet inspecté change. <b>Viewport</b> : par défaut celui de la cible (<c>target.GetViewport()</c> si c'est un
	/// <see cref="Node"/>), sinon celui du pilote ; <see cref="ViewportPath"/> permet de forcer un autre viewport.
	/// </summary>
	[Tool]
	public partial class GizmoDriver : Node
	{
		private readonly Dictionary<GizmoEntry, Rid> _gizmos2D = new();
		private readonly HashSet<string> _warned = new();
		private Rid _gizmo3D;

		/// <summary>Registre exécuté. <c>null</c> ou vide : rien n'est dessiné.</summary>
		public GizmoRegistry Registry { get; set; }

		/// <summary>Fournit l'objet inspecté, relu à chaque image.</summary>
		public Func<GodotObject> ResolveTarget { get; set; }

		/// <summary>Viewport où dessiner. Vide : celui de la cible, ou celui du pilote à défaut.</summary>
		[Export] public NodePath ViewportPath { get; set; } = new();

		/// <inheritdoc/>
		public override void _Process(double delta)
		{
			var target = ResolveTarget?.Invoke();
			var entries = Registry?.Entries;

			if (!GodotObject.IsInstanceValid(target) || entries == null || entries.Count == 0)
			{
				FreeAll();
				return;
			}

			var viewport = ResolveViewport(target);
			if (viewport == null)
			{
				FreeAll();
				return;
			}

			Process3D(target, viewport, entries);
			Process2D(target, viewport, entries);
		}

		/// <inheritdoc/>
		public override void _ExitTree() => FreeAll();

		private Viewport ResolveViewport(GodotObject target)
		{
			if (!ViewportPath.IsEmpty && GetNodeOrNull(ViewportPath) is Viewport explicitViewport)
				return explicitViewport;
			if (target is Node targetNode && GodotObject.IsInstanceValid(targetNode) && targetNode.IsInsideTree())
				return targetNode.GetViewport();
			return GetViewport();
		}

		#region 🧊 3D : un Rid partagé, vidé puis rejoué chaque image
		private void Process3D(GodotObject target, Viewport viewport, Godot.Collections.Array<GizmoEntry> entries)
		{
			bool any3D = false;
			foreach (var entry in entries)
				if (entry != null && entry.Enabled && GizmoMethodCatalog.Find(entry.GizmoMethod)?.Is3D == true)
					{ any3D = true; break; }

			if (!any3D)
			{
				if (_gizmo3D.IsValid) { Gizmos.FreeGizmos3D(_gizmo3D); _gizmo3D = default; }
				return;
			}

			if (!_gizmo3D.IsValid)
				_gizmo3D = Gizmos.CreateGizmos3D(viewport);
			if (!_gizmo3D.IsValid)
				return;

			Gizmos.ClearGizmos3D(_gizmo3D);
			foreach (var entry in entries)
			{
				if (entry == null || !entry.Enabled)
					continue;
				var spec = GizmoMethodCatalog.Find(entry.GizmoMethod);
				if (spec is { Is3D: true })
					InvokeEntry(_gizmo3D, target, entry, spec);
			}
		}
		#endregion

		#region ⬛ 2D : un Rid par appel (chaque dessin 2D efface le sien)
		private void Process2D(GodotObject target, Viewport viewport, Godot.Collections.Array<GizmoEntry> entries)
		{
			var current = new HashSet<GizmoEntry>();

			foreach (var entry in entries)
			{
				if (entry == null)
					continue;
				var spec = GizmoMethodCatalog.Find(entry.GizmoMethod);
				if (spec is not { Is3D: false })
					continue;
				current.Add(entry);

				if (!entry.Enabled)
				{
					if (_gizmos2D.TryGetValue(entry, out var disabledRid))
						Gizmos.ClearGizmos2D(disabledRid);
					continue;
				}

				if (!_gizmos2D.TryGetValue(entry, out var rid) || !rid.IsValid)
				{
					rid = Gizmos.CreateGizmos2D(viewport);
					_gizmos2D[entry] = rid;
				}
				if (rid.IsValid)
					InvokeEntry(rid, target, entry, spec);
			}

			// Libère les gizmos des appels 2D supprimés du registre (ou passés en 3D par un changement de méthode).
			foreach (var entry in new List<GizmoEntry>(_gizmos2D.Keys))
			{
				if (current.Contains(entry))
					continue;
				Gizmos.FreeGizmos2D(_gizmos2D[entry]);
				_gizmos2D.Remove(entry);
			}
		}
		#endregion

		#region 🧮 Résolution des paramètres et appel
		private void InvokeEntry(Rid gizmoRid, GodotObject target, GizmoEntry entry, GizmoMethodSpec spec)
		{
			var args = new Variant[spec.Parameters.Length];
			for (int i = 0; i < spec.Parameters.Length; i++)
			{
				var paramSpec = spec.Parameters[i];
				var parameter = i < entry.Parameters.Count ? entry.Parameters[i] : null;

				if (!TryResolve(target, entry.GizmoMethod, paramSpec, parameter, out var value))
					return; // paramètre requis (texture) non résolu : l'appel entier est ignoré
				args[i] = value;
			}
			spec.Invoke(gizmoRid, args);
		}

		/// <summary>
		/// Résout la valeur d'un paramètre. Retourne <c>false</c> seulement pour un paramètre <see cref="GizmoParamSpec.Required"/>
		/// (texture) introuvable : l'appelant doit alors renoncer à tout l'appel plutôt que de dessiner sans texture.
		/// </summary>
		private bool TryResolve(GodotObject target, string methodName, GizmoParamSpec spec, GizmoParameter parameter, out Variant value)
		{
			if (parameter is { FromProperty: true } && !string.IsNullOrEmpty(parameter.TargetProperty))
			{
				var read = PropertyWidgetGenerator.ReadValue(target, parameter.TargetProperty);
				if (IsCompatible(read, spec))
				{
					value = read;
					return true;
				}
				Warn($"[Gizmos] {methodName} : « {parameter.TargetProperty} » introuvable ou incompatible avec « {spec.Name} » ({spec.Type}).");
			}
			else if (parameter != null && !parameter.FromProperty)
			{
				if (spec.Nullable && parameter.UseMethodDefault)
				{
					value = default; // Nil : le dispatcher du catalogue le traduit en `null` C#
					return true;
				}
				if (IsCompatible(parameter.Value, spec))
				{
					value = parameter.Value;
					return true;
				}
			}

			// Repli : valeur par défaut de la déclaration, ou échec pour un paramètre requis (texture).
			if (spec.Required)
			{
				Warn($"[Gizmos] {methodName} : paramètre requis « {spec.Name} » non renseigné, appel ignoré.");
				value = default;
				return false;
			}
			value = spec.Default;
			return true;
		}

		/// <summary>
		/// Compatibilité type + classe. Pour un paramètre objet (texture), vérifie en plus que l'objet résolu est bien
		/// une instance de <see cref="GizmoParamSpec.ClassName"/> (<see cref="ClassDB.IsParentClass"/>), pour ne jamais
		/// passer un objet du mauvais type à <see cref="Gizmos"/>.
		/// </summary>
		private static bool IsCompatible(Variant value, GizmoParamSpec spec)
		{
			if (value.VariantType != spec.Type)
				return value.VariantType is Variant.Type.Int or Variant.Type.Float && spec.Type is Variant.Type.Int or Variant.Type.Float;

			if (spec.Type != Variant.Type.Object || string.IsNullOrEmpty(spec.ClassName))
				return true;

			var obj = value.AsGodotObject();
			return GodotObject.IsInstanceValid(obj) && ClassDB.IsParentClass(obj.GetClass(), spec.ClassName);
		}

		/// <summary>Émet chaque message d'avertissement une seule fois (évite d'inonder la console à 60 images/seconde).</summary>
		private void Warn(string message)
		{
			if (_warned.Add(message))
				GD.PushWarning(message);
		}
		#endregion

		private void FreeAll()
		{
			if (_gizmo3D.IsValid) { Gizmos.FreeGizmos3D(_gizmo3D); _gizmo3D = default; }
			foreach (var rid in _gizmos2D.Values)
				if (rid.IsValid)
					Gizmos.FreeGizmos2D(rid);
			_gizmos2D.Clear();
		}
	}
}
