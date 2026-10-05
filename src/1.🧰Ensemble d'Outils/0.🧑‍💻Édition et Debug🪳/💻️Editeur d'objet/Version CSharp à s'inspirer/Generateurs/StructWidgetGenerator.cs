using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>
	/// 📐 <b>Générateur de structures simples</b> : un <see cref="SpinBox"/> par champ.<br/>
	/// Types gérés : <c>Vector2/3/4</c> (et variantes <c>I</c>), <c>Rect2/Rect2I</c>, <c>Quaternion</c>, <c>Plane</c>, <c>Aabb</c>,
	/// <c>Basis</c>, <c>Transform2D</c>, <c>Transform3D</c> et <c>Projection</c> (matrices affichées ligne par ligne).<br/>
	/// <b>⚠️ Attention</b> : seules les structures <b>Godot</b> sont reconnues. Un <c>struct</c> C# personnalisé n'est pas
	/// visible via l'introspection Godot ; l'exposer via des propriétés exportées séparées.
	/// </summary>
	[GlobalClass, Tool]
	public partial class StructWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Pas des champs décimaux.</summary>
		[Export] public double Step { get; set; } = 0.001;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info) =>
			WidgetUtils.IsStructType(WidgetUtils.TypeOf(info)) ? 100 : 0;

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName);
			var type = WidgetUtils.TypeOf(info);
			bool readOnly = WidgetUtils.IsReadOnly(info);

			if (!WidgetUtils.TryDecompose(ReadValue(target, memberName), out var labels, out _, out bool isInt))
				return Labeled(target, memberName, WidgetUtils.MakeHint("Structure illisible"));

			var grid = new GridContainer { Columns = labels.Length switch { 2 => 2, 3 => 3, 6 => 3, 9 => 3, 12 => 3, 16 => 4, _ => 4 } };
			var boxes = new SpinBox[labels.Length];
			for (int i = 0; i < boxes.Length; i++)
			{
				boxes[i] = WidgetUtils.MakeSpinBox(labels[i], -1e12, 1e12, isInt ? 1 : Step, isInt);
				boxes[i].Editable = !readOnly;
				grid.AddChild(boxes[i]);
			}

			var binding = Bind(grid, target, memberName, v =>
			{
				if (!WidgetUtils.TryDecompose(v, out _, out var values, out _))
					return;
				for (int i = 0; i < boxes.Length; i++)
					boxes[i].SetValueNoSignal(values[i]);
			});

			foreach (var box in boxes)
			{
				box.ValueChanged += _ =>
				{
					var values = new double[boxes.Length];
					for (int i = 0; i < values.Length; i++)
						values[i] = boxes[i].Value;
					Commit(binding, target, memberName, WidgetUtils.Compose(type, values));
				};
			}

			return Labeled(target, memberName, grid);
		}
	}
}
