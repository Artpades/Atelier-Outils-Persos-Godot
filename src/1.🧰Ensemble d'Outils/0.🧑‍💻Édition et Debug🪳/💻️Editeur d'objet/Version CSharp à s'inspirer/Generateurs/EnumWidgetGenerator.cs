using System.Collections.Generic;
using Godot;
using GodotDict = Godot.Collections.Dictionary;

namespace GameplayToolkit.DebugEdition
{
	/// <summary>Apparence du widget d'une énumération.</summary>
	public enum EnumWidgetStyle
	{
		/// <summary>Liste déroulante.</summary>
		OptionButton,
		/// <summary>Groupe de cases à cocher exclusives (radio).</summary>
		CheckBoxes,
		/// <summary>Groupe de <see cref="CheckButton"/> exclusifs (interrupteurs).</summary>
		CheckButtons
	}

	/// <summary>
	/// 🔽 <b>Générateur d'énumérations</b> : propriétés <c>int</c> / <c>string</c> avec <c>PropertyHint.Enum</c>
	/// (dont les <c>enum</c> C# exportés) et <b>masques de bits</b> avec <c>PropertyHint.Flags</c>.<br/>
	/// • Enum → <see cref="OptionButton"/> ou groupe exclusif de <see cref="CheckBox"/> / <see cref="CheckButton"/> (<see cref="Style"/>).<br/>
	/// • Flags → une case par bit, toujours multiples (<see cref="CheckButton"/> si <see cref="Style"/> vaut <c>CheckButtons</c>, sinon <see cref="CheckBox"/>).
	/// </summary>
	[GlobalClass, Tool]
	public partial class EnumWidgetGenerator : PropertyWidgetGenerator
	{
		/// <summary>Apparence d'une énumération simple (les flags utilisent toujours des cases).</summary>
		[Export] public EnumWidgetStyle Style { get; set; } = EnumWidgetStyle.OptionButton;

		/// <inheritdoc/>
		protected override int ScoreMember(GodotObject target, GodotDict info)
		{
			var type = WidgetUtils.TypeOf(info);
			var hint = WidgetUtils.HintOf(info);
			bool isText = type == Variant.Type.String || type == Variant.Type.StringName;

			if (hint == PropertyHint.Enum && (type == Variant.Type.Int || isText))
				return 200;
			if (hint == PropertyHint.Flags && type == Variant.Type.Int)
				return 200;
			return 0;
		}

		/// <inheritdoc/>
		public override Control GenerateWidget(GodotObject target, string memberName)
		{
			var info = GetMemberInfo(target, memberName);
			var type = WidgetUtils.TypeOf(info);
			bool isText = type != Variant.Type.Int;
			bool flags = WidgetUtils.HintOf(info) == PropertyHint.Flags;
			bool readOnly = WidgetUtils.IsReadOnly(info);
			var items = WidgetUtils.ParseEnum(WidgetUtils.HintStringOf(info), flags, isText);

			Variant Wrap(Variant value) => type == Variant.Type.StringName ? new StringName(value.AsString()) : value;

			bool useSwitch = Style == EnumWidgetStyle.CheckButtons;
			Control editor = flags ? BuildFlags(target, memberName, items, readOnly, useSwitch)
				: Style != EnumWidgetStyle.OptionButton ? BuildRadios(target, memberName, items, readOnly, Wrap, useSwitch)
				: BuildDropdown(target, memberName, items, readOnly, Wrap);

			return Labeled(target, memberName, editor);
		}

		private static BaseButton MakeCheck(bool useSwitch, string text, bool readOnly) =>
			useSwitch ? new CheckButton { Text = text, Disabled = readOnly } : new CheckBox { Text = text, Disabled = readOnly };

		private static bool SameValue(Variant a, Variant b) =>
			a.VariantType == Variant.Type.Int || b.VariantType == Variant.Type.Int
				? a.AsInt64() == b.AsInt64()
				: a.AsString() == b.AsString();

		private static Control BuildDropdown(GodotObject target, string memberName, List<(string Label, Variant Value)> items,
			bool readOnly, System.Func<Variant, Variant> wrap)
		{
			var option = new OptionButton { Disabled = readOnly };
			for (int i = 0; i < items.Count; i++)
				option.AddItem(items[i].Label, i);

			PropertyBinding binding = null;
			binding = Bind(option, target, memberName, v =>
			{
				int index = items.FindIndex(item => SameValue(item.Value, v));
				option.Select(index);
			});
			option.ItemSelected += index => Commit(binding, target, memberName, wrap(items[(int)index].Value));
			return option;
		}

		private static Control BuildRadios(GodotObject target, string memberName, List<(string Label, Variant Value)> items,
			bool readOnly, System.Func<Variant, Variant> wrap, bool useSwitch)
		{
			var box = new VBoxContainer();
			var group = new ButtonGroup();
			var buttons = new List<BaseButton>();

			foreach (var item in items)
			{
				var check = MakeCheck(useSwitch, item.Label, readOnly);
				check.ButtonGroup = group;
				buttons.Add(check);
				box.AddChild(check);
			}

			var binding = Bind(box, target, memberName, v =>
			{
				for (int i = 0; i < items.Count; i++)
					buttons[i].SetPressedNoSignal(SameValue(items[i].Value, v));
			});

			for (int i = 0; i < buttons.Count; i++)
			{
				var value = items[i].Value;
				buttons[i].Toggled += pressed =>
				{
					if (pressed)
						Commit(binding, target, memberName, wrap(value));
				};
			}
			return box;
		}

		private static Control BuildFlags(GodotObject target, string memberName, List<(string Label, Variant Value)> items, bool readOnly, bool useSwitch)
		{
			var box = new VBoxContainer();
			var buttons = new List<BaseButton>();

			foreach (var item in items)
			{
				var check = MakeCheck(useSwitch, item.Label, readOnly);
				buttons.Add(check);
				box.AddChild(check);
			}

			var binding = Bind(box, target, memberName, v =>
			{
				long mask = v.AsInt64();
				for (int i = 0; i < items.Count; i++)
				{
					long bit = items[i].Value.AsInt64();
					buttons[i].SetPressedNoSignal(bit != 0 && (mask & bit) == bit);
				}
			});

			foreach (var check in buttons)
			{
				check.Toggled += _ =>
				{
					long mask = 0;
					for (int i = 0; i < items.Count; i++)
						if (buttons[i].ButtonPressed)
							mask |= items[i].Value.AsInt64();
					Commit(binding, target, memberName, mask);
				};
			}
			return box;
		}
	}
}
