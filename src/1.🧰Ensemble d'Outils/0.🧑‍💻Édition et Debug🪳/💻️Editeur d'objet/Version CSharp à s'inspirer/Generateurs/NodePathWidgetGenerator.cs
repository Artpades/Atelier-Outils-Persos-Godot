using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 🧭 <b>Générateur de <see cref="NodePath"/></b> : champ de saisie du chemin, indicateur de résolution
	/// (✔ classe du nœud / ✘ introuvable) et bouton 🔍 qui <b>déplie l'inspecteur du nœud pointé</b>.<br/>
	/// Le chemin est validé à la touche Entrée ou à la perte du focus.<br/>
	/// <b>📍 Note :</b> le chemin est résolu depuis la cible si c'est un <see cref="Node"/>, sinon depuis la scène courante.
	/// </summary>
	[GlobalClass, Tool]
	public partial class NodePathWidgetGenerator : ContainerWidgetGenerator
	{
		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			WidgetUtils.TypeOf(info) == Variant.Type.NodePath ? 100 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName) =>
			GenerateWidget(target, memberName, PropertyWidgetRegistry.CreateDefault());

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName, PropertyWidgetRegistry registry)
		{
			bool readOnly = WidgetUtils.IsReadOnly(GetMemberInfo(target, memberName));

			var edit = new LineEdit { Editable = !readOnly, PlaceholderText = "Chemin du nœud", SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			var status = new Label { CustomMinimumSize = new Vector2(96, 0), ClipText = true };
			var search = new Button { Text = "🔍", ToggleMode = true, TooltipText = "Explorer le nœud pointé" };
			search.SetPressedNoSignal(StartExpanded);

			var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			row.AddChild(edit);
			row.AddChild(status);
			row.AddChild(search);

			var content = WidgetUtils.MakeContentContainer(Layout, GridColumns, IsVertical);
			var body = new MarginContainer { Visible = false };
			body.AddThemeConstantOverride("margin_left", 14);
			body.AddChild(content);

			var root = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			root.AddChild(Labeled(target, memberName, row));
			root.AddChild(body);

			Node ResolvePath(string text)
			{
				if (string.IsNullOrEmpty(text))
					return null;
				return (target as Node ?? WidgetUtils.FindSceneRoot())?.GetNodeOrNull(new NodePath(text));
			}

			Node Resolve() => ResolvePath(edit.Text);

			void Refresh()
			{
				var node = Resolve();
				status.Text = string.IsNullOrEmpty(edit.Text) ? "" : node != null ? $"✔ {node.GetType().Name}" : "✘ introuvable";
				status.Modulate = node != null ? new Color(0.5f, 1f, 0.5f) : new Color(1f, 0.5f, 0.5f);
				body.Visible = search.ButtonPressed;

				if (search.ButtonPressed && node != null)
					InspectorBuilder.Populate(content, node, registry);
				else
					InspectorBuilder.Clear(content);
			}

			var binding = Bind(root, target, memberName, v =>
			{
				string text = v.AsNodePath().ToString();
				if (edit.Text != text)
					edit.Text = text;
				Refresh();
			},
			// Le chemin seul ne suffit pas : si le nœud pointé apparaît plus tard ou est remplacé par un autre
			// au même chemin, l'instance résolue change et le contenu affiché doit être reconstruit.
			v =>
			{
				string text = v.AsNodePath().ToString();
				return (text, ResolvePath(text)?.GetInstanceId());
			});

			void CommitText()
			{
				if (readOnly || edit.Text == ReadValue(target, memberName).AsNodePath().ToString())
					return;
				Commit(binding, target, memberName, new NodePath(edit.Text));
				Refresh();
			}

			edit.TextSubmitted += _ => CommitText();
			edit.FocusExited += CommitText;
			search.Toggled += _ => Refresh();

			return root;
		}
	}
}
