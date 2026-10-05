using System.Collections.Generic;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📡 <b>Générateur de signaux</b>, à la manière d'un <c>UnityEvent</c> : chaque signal affiche la liste de ses
	/// <b>appels persistants</b> (<see cref="SignalConnection"/>) et un bouton pour <b>émettre</b> le signal (arguments par défaut).<br/>
	/// <br/>
	/// <b>📍 Un appel</b> = une case d'activation, un <b>NodePath</b> vers l'objet à appeler (vide : l'objet qui émet, bouton 🔍
	/// pour choisir dans l'arbre), une <b>méthode</b> à choisir dans une liste, et les <b>paramètres</b> de cette méthode :<br/>
	/// • un paramètre qui a le <b>même index et le même type</b> qu'un argument du signal (ou qui est un <c>Variant</c>) est
	/// <b>correct</b> : il reçoit l'argument du signal et n'est <b>pas modifiable</b> ;<br/>
	/// • sinon, un <b>widget spécialisé</b> permet de saisir une valeur statique (mêmes widgets que le reste de l'inspecteur :
	/// nombres, texte, couleurs, tableaux, dictionnaires... ; sélecteur de nœud ou de ressource pour un objet).<br/>
	/// <br/>
	/// Les appels sont <b>connectés</b> au signal dès que le widget est construit. Dans l'éditeur, ils sont retirés à la
	/// destruction du widget (les scripts <c>[Tool]</c> ne doivent pas garder de connexions cachées).<br/>
	/// <b>📍 Persistance</b> : les appels sont stockés dans <see cref="Connections"/>. Avec un
	/// <see cref="DataRegistry"/>, c'est <see cref="DataEntry.Connections"/> qui sert (enregistré avec la scène) ; sans liste
	/// fournie, les appels sont temporaires.<br/>
	/// <b>📍 Note :</b> ce générateur ne s'applique qu'aux membres de catégorie <see cref="MemberKind.Signal"/>.
	/// </summary>
	[GlobalClass, Tool]
	public partial class SignalWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Appels persistants de ce signal. <c>null</c> : liste temporaire, perdue à la reconstruction.</summary>
		[Export] public Godot.Collections.Array<SignalConnection> Connections { get; set; }

		/// <inheritdoc/>
		public override MemberKind Kind => MemberKind.Signal;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) => 100;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName) =>
			GenerateWidget(target, memberName, PropertyWidgetRegistry.CreateDefault());

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName, PropertyWidgetRegistry registry)
		{
			var info = GetMemberInfo(target, memberName, MemberKind.Signal);
			var signalArgs = ArgsOf(info);
			var list = Connections ?? new Godot.Collections.Array<SignalConnection>();

			var root = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			var callsBox = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };

			// ---- en-tête : nom, émettre, ajouter
			var emit = new Button { Text = "▶ Émettre", TooltipText = "Émet le signal avec des arguments par défaut" };
			emit.Pressed += () =>
			{
				if (GodotObject.IsInstanceValid(target))
					target.EmitSignal(memberName, WidgetUtils.DefaultArguments(info));
			};
			var add = new Button { Text = "+ Appel", TooltipText = "Ajouter un appel de méthode" };
			add.Pressed += () =>
			{
				var connection = new SignalConnection();
				list.Add(connection);
				Touch(connection);
				Rebuild();
			};

			var header = new BoxContainer { Vertical = IsVertical, SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
			header.AddChild(new Label
			{
				Text = "📡 " + WidgetUtils.Signature(info),
				TooltipText = memberName,
				ClipText = true,
				SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
			});
			header.AddChild(emit);
			header.AddChild(add);

			root.AddChild(header);
			root.AddChild(callsBox);

			void Apply() => SignalConnector.Apply(target, memberName, list);

			void Touch(SignalConnection connection)
			{
				NotifyEdited(connection); // Resource : émet « changed » et marque la scène modifiée dans l'éditeur
				Apply();
			}

			void Rebuild()
			{
				InspectorBuilder.Clear(callsBox);
				foreach (var connection in new List<SignalConnection>(list))
					if (connection != null)
						callsBox.AddChild(BuildCall(connection));
			}

			Control BuildCall(SignalConnection connection)
			{
				var box = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				var receiver = connection.ResolveReceiver(target);

				// ---- ligne 1 : activation, chemin de la cible, sélecteur, suppression
				var enabled = new CheckBox { ButtonPressed = connection.Enabled, TooltipText = "Appel actif" };
				enabled.Toggled += on => { connection.Enabled = on; Touch(connection); };

				var path = new LineEdit
				{
					Text = connection.TargetPath?.ToString() ?? "",
					PlaceholderText = "(cet objet)",
					TooltipText = "NodePath de l'objet à appeler, relatif à l'émetteur. Vide : l'émetteur lui-même.",
					SizeFlagsHorizontal = Control.SizeFlags.ExpandFill,
				};
				void CommitPath()
				{
					if (path.Text == (connection.TargetPath?.ToString() ?? ""))
						return;
					connection.TargetPath = new NodePath(path.Text);
					Touch(connection);
					Rebuild();
				}
				path.TextSubmitted += _ => CommitPath();
				path.FocusExited += CommitPath;

				var status = WidgetUtils.MakeHint(receiver == null ? "✘ introuvable" : $"✔ {receiver.GetType().Name}");
				status.CustomMinimumSize = new Vector2(120, 0);
				status.Modulate = receiver == null ? new Color(1, 0.5f, 0.5f) : new Color(0.5f, 1, 0.5f);

				var pick = new Button { Text = "🔍", TooltipText = "Choisir l'objet dans l'arbre de la scène" };
				pick.Pressed += () => PathButtonWidgetGenerator.PickNode(pick, target, node =>
				{
					var from = target as Node ?? WidgetUtils.FindSceneRoot();
					if (from == null)
						return;
					connection.TargetPath = from.GetPathTo(node);
					Touch(connection);
					Rebuild();
				});

				var remove = new Button { Text = "✕", TooltipText = "Supprimer l'appel" };
				remove.Pressed += () =>
				{
					connection.DisconnectFrom(target);
					list.Remove(connection);
					Touch(connection);
					Rebuild();
				};

				var row1 = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				foreach (Control control in new Control[] { enabled, path, status, pick, remove })
					row1.AddChild(control);
				box.AddChild(row1);

				// ---- ligne 2 : méthode ; puis paramètres
				if (receiver == null)
					box.AddChild(WidgetUtils.MakeHint("Choisissez un objet valide pour lister ses méthodes."));
				else
					AddMethodPart(box, connection, receiver);

				var panel = new PanelContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				var margin = new MarginContainer();
				foreach (var side in new[] { "left", "top", "right", "bottom" })
					margin.AddThemeConstantOverride($"margin_{side}", 4);
				margin.AddChild(box);
				panel.AddChild(margin);
				return panel;
			}

			void AddMethodPart(VBoxContainer box, SignalConnection connection, GodotObject receiver)
			{
				var methods = ListMethods(receiver);
				var names = new List<string>(methods.Keys);
				bool known = methods.ContainsKey(connection.TargetMethod);

				var option = new OptionButton { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				option.AddItem("‹ choisir une méthode ›");
				if (!known && !string.IsNullOrEmpty(connection.TargetMethod))
					option.AddItem($"{connection.TargetMethod}  (introuvable)");
				foreach (var name in names)
					option.AddItem(WidgetUtils.Signature(methods[name]));

				int offset = option.ItemCount - names.Count;
				option.Select(known ? offset + names.IndexOf(connection.TargetMethod)
					: string.IsNullOrEmpty(connection.TargetMethod) ? 0 : 1);

				option.ItemSelected += index =>
				{
					int i = (int)index - offset;
					if (i < 0)
						return;
					connection.TargetMethod = names[i];
					AdaptArguments(connection, methods[names[i]], signalArgs);
					Touch(connection);
					Rebuild();
				};

				var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
				row.AddChild(new Label { Text = "Méthode" });
				row.AddChild(option);
				box.AddChild(row);

				if (known)
				{
					AdaptArguments(connection, methods[connection.TargetMethod], signalArgs);
					AddParameters(box, connection, methods[connection.TargetMethod]);
				}
			}

			void AddParameters(VBoxContainer box, SignalConnection connection, GodotDict methodInfo)
			{
				var parameters = ArgsOf(methodInfo);
				for (int j = 0; j < parameters.Count && j < connection.Arguments.Count; j++)
				{
					var parameter = parameters[j];
					var argument = connection.Arguments[j];
					string name = WidgetUtils.NameOf(parameter);
					var type = WidgetUtils.TypeOf(parameter);

					if (argument.FromSignal)
					{
						// Bon index et bon type : reçoit l'argument du signal, non modifiable.
						var row = new HBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
						row.AddChild(new Label { Text = $"{name} : {TypeText(parameter)}", CustomMinimumSize = new Vector2(140, 0) });
						row.AddChild(WidgetUtils.MakeHint($"◀ argument {argument.SignalIndex} du signal ({TypeText(signalArgs[argument.SignalIndex])}) — automatique"));
						box.AddChild(row);
						continue;
					}

					// Sinon : widget spécialisé pour saisir la valeur statique.
					var proxy = new ValueProxy(
						() => StoredValue(argument, type),
						value => { Store(argument, value); Touch(connection); },
						type, name,
						WidgetUtils.HintOf(parameter), WidgetUtils.HintStringOf(parameter),
						readOnly: false, isVariant: type == Variant.Type.Nil,
						className: parameter.TryGetValue("class_name", out var cn) ? cn.AsString() : "");

					proxy.TryGetMemberInfo(ValueProxy.ValueName, MemberKind.Property, out var proxyInfo);
					Control editor = type == Variant.Type.Object
						? new PathButtonWidgetGenerator().GenerateWidget(proxy, ValueProxy.ValueName)
						: registry.BuildWidget(proxy, proxyInfo) ?? WidgetUtils.MakeHint($"{name} : type non pris en charge");
					box.AddChild(editor);
				}
			}

			root.TreeExiting += () =>
			{
				if (Engine.IsEditorHint())
					SignalConnector.Clear(target, list);
			};

			Apply();
			Rebuild();
			return root;
		}

		#region 🧮 Adaptation des paramètres
		/// <summary>
		/// Aligne <see cref="SignalConnection.Arguments"/> sur les paramètres de la méthode : pour chaque paramètre, dit s'il
		/// est fourni par le signal (même index, même type) ou saisi à la main. Conserve les valeurs déjà saisies quand le type ne change pas.
		/// </summary>
		public static void AdaptArguments(SignalConnection connection, GodotDict methodInfo, List<GodotDict> signalArgs)
		{
			var parameters = ArgsOf(methodInfo);
			var adapted = new Godot.Collections.Array<SignalArgument>();

			for (int j = 0; j < parameters.Count; j++)
			{
				var argument = j < connection.Arguments.Count && connection.Arguments[j] != null ? connection.Arguments[j] : new SignalArgument();
				var type = WidgetUtils.TypeOf(parameters[j]);

				argument.FromSignal = IsProvidedBySignal(signalArgs, j, parameters[j]);
				argument.SignalIndex = j;
				if (!argument.FromSignal && type != Variant.Type.Nil && argument.Value.VariantType != type && type != Variant.Type.Object)
					argument.Value = WidgetUtils.DefaultFor(type);
				adapted.Add(argument);
			}

			// Ne réaffecter que si la forme change (évite de rendre la ressource « modifiée » à chaque ouverture).
			if (adapted.Count != connection.Arguments.Count)
				connection.Arguments = adapted;
			else
				for (int j = 0; j < adapted.Count; j++)
					if (!ReferenceEquals(adapted[j], connection.Arguments[j]))
					{
						connection.Arguments = adapted;
						break;
					}
		}

		/// <summary>
		/// Vrai si le paramètre <paramref name="index"/> de la méthode reçoit l'argument du signal de même index : celui-ci
		/// doit exister et avoir le même type (un paramètre <c>Variant</c> accepte n'importe quel type).
		/// </summary>
		public static bool IsProvidedBySignal(List<GodotDict> signalArgs, int index, GodotDict parameter)
		{
			if (index >= signalArgs.Count)
				return false;
			var parameterType = WidgetUtils.TypeOf(parameter);
			return parameterType == Variant.Type.Nil || WidgetUtils.TypeOf(signalArgs[index]) == parameterType;
		}

		private static List<GodotDict> ArgsOf(GodotDict info)
		{
			var result = new List<GodotDict>();
			if (info != null && info.TryGetValue("args", out var args))
				foreach (var arg in args.AsGodotArray<GodotDict>())
					result.Add(arg);
			return result;
		}

		private static string TypeText(GodotDict arg)
		{
			var type = WidgetUtils.TypeOf(arg);
			string className = arg.TryGetValue("class_name", out var cn) ? cn.AsString() : "";
			return type == Variant.Type.Nil ? "Variant" : type == Variant.Type.Object && className.Length > 0 ? className : type.ToString();
		}

		/// <summary>Valeur affichée pour un argument statique : valeur enregistrée, nœud résolu, ou valeur par défaut du type.</summary>
		private static Variant StoredValue(SignalArgument argument, Variant.Type type)
		{
			var value = argument.ResolveStatic();
			return value.VariantType == Variant.Type.Nil && type != Variant.Type.Nil && type != Variant.Type.Object
				? WidgetUtils.DefaultFor(type) : value;
		}

		/// <summary>Enregistre la valeur saisie ; un nœud est mémorisé par son chemin depuis la racine de la scène.</summary>
		private static void Store(SignalArgument argument, Variant value)
		{
			var node = value.VariantType == Variant.Type.Object ? ObjectWidgetGenerator.ToObject(value) as Node : null;
			var root = WidgetUtils.FindSceneRoot();
			if (node != null && root != null)
			{
				argument.IsNodeReference = true;
				argument.Value = root.GetPathTo(node);
				return;
			}
			argument.IsNodeReference = false;
			argument.Value = value;
		}

		/// <summary>Méthodes proposées pour un objet : celles de son script, ou toutes si aucun script (hors « _ »), triées par nom.</summary>
		private static SortedDictionary<string, GodotDict> ListMethods(GodotObject receiver)
		{
			var methods = new SortedDictionary<string, GodotDict>(System.StringComparer.OrdinalIgnoreCase);
			foreach (var info in InspectorBuilder.GetMembers(receiver, MemberKind.Method, InspectionScope.Auto))
			{
				string name = WidgetUtils.NameOf(info);
				if (name.Length > 0 && !name.StartsWith('_'))
					methods.TryAdd(name, info);
			}
			return methods;
		}
		#endregion
	}
}
