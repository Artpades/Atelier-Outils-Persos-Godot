using System;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🧬 <b>Générateur d'objets</b> (<c>Node</c>, <c>Resource</c>, tout <see cref="GodotObject"/>).<br/>
	/// Crée une <see cref="FoldableSection"/> dont le contenu — l'inspecteur <b>récursif</b> de l'objet référencé —
	/// n'est construit qu'à l'ouverture. La référence elle-même est en lecture seule ; ses propriétés sont éditables.<br/>
	/// <br/>
	/// <b>📍 Le contenu reste synchronisé</b> avec l'objet référencé :<br/>
	/// • si la propriété pointe vers un <b>autre</b> objet, la section se reconstruit ;<br/>
	/// • si l'objet émet <c>property_list_changed</c> (propriétés ajoutées / retirées / rendues visibles), la section se reconstruit ;<br/>
	/// • si l'objet est une <c>Resource</c> qui émet <c>changed</c>, toutes les valeurs affichées sont relues immédiatement ;<br/>
	/// • sinon, chaque widget interroge sa propriété périodiquement (voir <see cref="PropertyBinding"/>).
	/// </summary>
	[GlobalClass, Tool]
	public partial class ObjectWidgetGenerator : ContainerWidgetGenerator
	{
		/// <summary>Périmètre des membres affichés pour l'objet référencé.</summary>
		[Export] public InspectionScope Scope { get; set; } = InspectionScope.Auto;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			WidgetUtils.TypeOf(info) == Variant.Type.Object ? 100 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName) =>
			GenerateWidget(target, memberName, PropertyWidgetRegistry.CreateDefault());

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName, PropertyWidgetRegistry registry)
		{
			var section = CreateSection();
			string label = DisplayName(target, memberName);
			if (string.IsNullOrEmpty(label))
				label = "Objet";
			GodotObject current = null;

			void Fill()
			{
				if (current != null && section.IsOpen)
					InspectorBuilder.Populate(section.Content, current, registry, Scope);
				else
					section.Clear();
			}

			// Signaux de l'objet référencé : connectés à l'objet courant, déconnectés dès qu'il change ou que le widget disparaît.
			var onListChanged = Callable.From(() => { if (section.IsOpen) Fill(); });
			var onChanged = Callable.From(() => PropertyBinding.RefreshAll(section.Content));

			void Detach()
			{
				if (!GodotObject.IsInstanceValid(current))
					return;
				if (current.IsConnected(GodotObject.SignalName.PropertyListChanged, onListChanged))
					current.Disconnect(GodotObject.SignalName.PropertyListChanged, onListChanged);
				if (current is Resource && current.IsConnected(Resource.SignalName.Changed, onChanged))
					current.Disconnect(Resource.SignalName.Changed, onChanged);
			}

			void Attach()
			{
				if (current == null)
					return;
				current.Connect(GodotObject.SignalName.PropertyListChanged, onListChanged);
				if (current is Resource)
					current.Connect(Resource.SignalName.Changed, onChanged);
			}

			section.OpenChanged += _ => Fill();
			section.Root.TreeExiting += Detach;

			Bind(section.Root, target, memberName, v =>
			{
				Detach();
				current = ToObject(v);
				Attach();
				if (LogWrites)
					GD.Print($"[DynamicInspector] {Describe(target)}.{memberName}  →  sous-objet affiché : {Describe(current)}");
				section.Title = current == null ? $"{label} : <null>" : $"{label}  ({current.GetType().Name})";
				section.Header.Disabled = current == null;
				Fill();
			}, v => ToObject(v)?.GetInstanceId());

			return section.Root;
		}

		/// <summary>Convertit un Variant en objet valide, ou <c>null</c> (vide, libéré, autre type).</summary>
		public static GodotObject ToObject(Variant value)
		{
			if (value.VariantType != Variant.Type.Object)
				return null;
			try
			{
				var obj = value.AsGodotObject();
				return GodotObject.IsInstanceValid(obj) ? obj : null;
			}
			catch (ObjectDisposedException)
			{
				return null;
			}
		}
	}
}
