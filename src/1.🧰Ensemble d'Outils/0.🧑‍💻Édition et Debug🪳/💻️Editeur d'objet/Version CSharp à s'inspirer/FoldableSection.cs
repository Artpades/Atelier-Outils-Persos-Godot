using System;
using Godot;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📂 <b>Section repliable</b> : un bouton d'en-tête et un corps indenté.<br/>
	/// Utilisée pour la génération <b>récursive</b> (objets, tableaux, dictionnaires) :
	/// le contenu n'est construit qu'à la <b>première ouverture</b> (<see cref="OpenChanged"/>), ce qui évite
	/// le coût des grandes hiérarchies et les cycles infinis (objet qui se référence lui-même).
	/// </summary>
	public sealed class FoldableSection
	{
		private string _title = "";

		/// <summary>Conteneur racine à retourner par le générateur.</summary>
		public VBoxContainer Root { get; } = new();

		/// <summary>Bouton bascule d'en-tête.</summary>
		public Button Header { get; } = new() { ToggleMode = true, Flat = true, Alignment = HorizontalAlignment.Left };

		/// <summary>Conteneur où placer le contenu généré (<c>VBoxContainer</c> par défaut).</summary>
		public Container Content { get; }

		/// <summary>Vrai si la section est dépliée.</summary>
		public bool IsOpen => Header.ButtonPressed;

		/// <summary>Émis quand la section est dépliée (<c>true</c>) ou repliée (<c>false</c>).</summary>
		public event Action<bool> OpenChanged;

		/// <summary>Titre affiché sans la flèche.</summary>
		public string Title
		{
			get => _title;
			set { _title = value; UpdateHeader(); }
		}

		/// <summary>Construit la section repliée.</summary>
		/// <param name="content">Conteneur du contenu (<c>null</c> : <c>VBoxContainer</c>).</param>
		public FoldableSection(Container content = null)
		{
			Content = content ?? new VBoxContainer();
			var body = new MarginContainer { Visible = false };
			body.AddThemeConstantOverride("margin_left", 14);
			body.AddChild(Content);

			Root.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;
			Root.AddChild(Header);
			Root.AddChild(body);

			Header.Toggled += open =>
			{
				body.Visible = open;
				UpdateHeader();
				OpenChanged?.Invoke(open);
			};
		}

		/// <summary>
		/// Règle le comportement de repli.
		/// </summary>
		/// <param name="foldable">Si faux, l'en-tête est masqué et la section reste ouverte.</param>
		/// <param name="open">Ouvre la section (implicite si <paramref name="foldable"/> est faux).</param>
		public void SetFoldable(bool foldable, bool open)
		{
			Header.Visible = foldable;
			if (open || !foldable)
				Header.ButtonPressed = true;
		}

		/// <summary>Supprime le contenu généré.</summary>
		public void Clear()
		{
			foreach (Node child in Content.GetChildren())
			{
				Content.RemoveChild(child);
				child.QueueFree();
			}
		}

		private void UpdateHeader() => Header.Text = (IsOpen ? "▼ " : "▶ ") + _title;
	}
}
