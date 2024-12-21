#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;

#include "Settings.h"
#include "Control_DropDown.h"
#include "Control_ColorPreset.h"
#include "Control_ColorPicker.h"
#include "Widget_Draw_Options.h"

namespace MIDILightDrawer
{
	public ref class Widget_Strobe_Options : public UserControl
	{
	public:
		Widget_Strobe_Options(Control_ColorPicker^ color_picker);

	private:
		System::ComponentModel::Container^ _Components;
		GroupBox^				_GroupBox;
		Control_DropDown^		_DropDown_Strobe_Quantization;
		Control_ColorPicker^	_Color_Picker;
		Control_ColorPreset^	_Color_Presets;

		int						_Strobe_Quantization_Ticks;

		void Initialize_Component(void);
		void DropDown_Strobe_Quantization_OnItem_Selected(System::Object^ sender, MIDILightDrawer::Control_DropDown_Item_Selected_Event_Args^ e);
		void PresetPanel_SelectedColorChanged(System::Object^ sender, System::EventArgs^ e);
		void Color_Picker_OnColorChanged(Object^ sender, EventArgs^ e);
		void GroupBox_Paint(Object^ sender, PaintEventArgs^ e);

	protected:
		~Widget_Strobe_Options();

	public:
		void Select_Next_Strobe_Value(void);
		void Select_Previous_Strobe_Value(void);

		property int TickLength {
			int get();
			void set(int value);
		}

		property Color SelectedColor {
			Color get();
			void set(Color color);
		}

		property int PresetColor {
			void set(int index);
		}

		event QuantizationChangedHandler^ QuantizationChanged;
		event ColorChangedHandler^ ColorChanged;
	};
}