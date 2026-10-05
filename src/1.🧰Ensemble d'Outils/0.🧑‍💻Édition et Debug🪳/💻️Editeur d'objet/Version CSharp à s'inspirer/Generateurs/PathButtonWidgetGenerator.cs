using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📍 <b>Générateur de bouton de sélection</b> pour les <see cref="NodePath"/> et les objets.<br/>
	/// Un bouton affiche la valeur courante ; un clic ouvre un sélecteur :<br/>
	/// • <b>NodePath</b> ou objet <b>Node</b> → arbre de la scène (racine : scène éditée ou courante) ;<br/>
	/// • objet <b>Resource</b> → boîte de dialogue de fichiers (<c>res://</c>).<br/>
	/// Le bouton ✕ vide la valeur. Ce générateur n'est pas dans les défauts : il s'active via
	/// <see cref="WidgetChoice.PathButton"/> ou <see cref="PropertyWidgetGenerator.TargetPropertyName"/>.<br/>
	/// <b>📍 Note :</b> un <see cref="NodePath"/> est écrit <b>relatif à la cible</b> quand c'est un <see cref="Node"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class PathButtonWidgetGenerator : PropertyWidgetGenerator
	{
		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info)
		{
			var type = WidgetUtils.TypeOf(info);
			return type == Variant.Type.NodePath || type == Variant.Type.Object ? 90 : 0;
		}

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName);
			bool isNodePath = WidgetUtils.TypeOf(info) == Variant.Type.NodePath;
			bool readOnly = WidgetUtils.IsReadOnly(info);
			string className = info.TryGetValue("class_name", out var cn) ? cn.AsString() : "";

			var button = new Button { ClipText = true, Alignment = HorizontalAlignment.Left, Disabled = readOnly, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			var clear = new Button { Text = "✕", TooltipText = "Vider", Disabled = readOnly };
			var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			row.AddChild(button);
			row.AddChild(clear);

			var binding = Bind(row, target, memberName, v =>
			{
				string text = isNodePath ? v.AsNodePath().ToString() : Describe(ObjectWidgetGenerator.ToObject(v));
				button.Text = string.IsNullOrEmpty(text) ? "‹ choisir… ›" : text;
				button.TooltipText = button.Text;
			}, v => isNodePath ? v.AsNodePath().ToString() : ObjectWidgetGenerator.ToObject(v)?.GetInstanceId());

			clear.Pressed += () => Commit(binding, target, memberName,
				isNodePath ? new NodePath("") : default(Variant));

			button.Pressed += () =>
			{
				bool wantsResource = !isNodePath && (className.Length > 0
					? ClassDB.IsParentClass(className, "Resource")
					: ObjectWidgetGenerator.ToObject(ReadValue(target, memberName)) is Resource);

				if (wantsResource)
					PickResource(button, v => Commit(binding, target, memberName, v));
				else
					PickNode(button, target, node =>
					{
						if (isNodePath)
							Commit(binding, target, memberName, new NodePath(target is Node self ? self.GetPathTo(node).ToString() : node.GetPath().ToString()));
						else
							Commit(binding, target, memberName, node);
					});
			};

			return Labeled(target, memberName, row);
		}

		private static string Describe(GodotObject obj) => obj switch
		{
			null => "",
			Resource { ResourcePath: { Length: > 0 } path } => path,
			Node node => $"{node.Name} ({node.GetType().Name})",
			_ => obj.GetType().Name,
		};

		/// <summary>Ouvre un arbre de la scène ; <paramref name="onPicked"/> reçoit le nœud validé.</summary>
		internal static void PickNode(Control owner, GodotObject context, System.Action<Node> onPicked)
		{
			var root = WidgetUtils.FindSceneRoot();
			if (root == null)
			{
				GD.PushWarning("[PathButton] Aucune scène disponible pour choisir un nœud.");
				return;
			}

			var tree = new Tree { HideRoot = false, SizeFlagsVertical = Control.SizeFlags.ExpandFill, CustomMinimumSize = new Vector2(360, 380) };
			AddNode(tree, null, root);

			var dialog = new ConfirmationDialog { Title = "Choisir un nœud", OkButtonText = "Choisir" };
			dialog.AddChild(tree);
			dialog.Confirmed += () =>
			{
				if (tree.GetSelected()?.GetMetadata(0).AsGodotObject() is Node picked)
					onPicked(picked);
			};
			ShowDialog(owner, dialog);
		}

		private static void AddNode(Tree tree, TreeItem parent, Node node)
		{
			var item = tree.CreateItem(parent);
			item.SetText(0, $"{node.Name}  ({node.GetType().Name})");
			item.SetMetadata(0, node);
			foreach (Node child in node.GetChildren())
				AddNode(tree, item, child);
		}

		/// <summary>Ouvre un sélecteur de fichier ; <paramref name="onPicked"/> reçoit la ressource chargée.</summary>
		private static void PickResource(Control owner, System.Action<Variant> onPicked)
		{
			var dialog = new FileDialog
			{
				FileMode = FileDialog.FileModeEnum.OpenFile,
				Access = FileDialog.AccessEnum.Resources,
				Title = "Choisir une ressource",
			};
			dialog.FileSelected += path =>
			{
				var resource = ResourceLoader.Load(path);
				if (resource != null)
					onPicked(resource);
			};
			ShowDialog(owner, dialog, new Vector2I(700, 460));
		}

		private static void ShowDialog(Control owner, Window dialog, Vector2I? size = null)
		{
			dialog.CloseRequested += dialog.QueueFree;
			dialog.FreeWhenClosed();
			owner.AddChild(dialog);
			dialog.PopupCentered(size ?? new Vector2I(400, 480));
		}
	}

	internal static class DialogExtensions
	{
		/// <summary>Libère automatiquement un <see cref="ConfirmationDialog"/> quand il est fermé ou annulé.</summary>
		public static void FreeWhenClosed(this Window dialog)
		{
			if (dialog is AcceptDialog accept)
			{
				accept.Confirmed += accept.QueueFree;
				if (dialog is ConfirmationDialog confirmation)
					confirmation.Canceled += confirmation.QueueFree;
			}
		}
	}
}
