#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;

#include "Widget_Draw_Options.h"

#include "Control_ColorPreset.h"
#include "Control_ColorPicker.h"


namespace MIDILightDrawer
{
	public ref class Widget_Color_Options : public System::Windows::Forms::UserControl
	{
	public:
		Widget_Color_Options(Control_ColorPicker^ color_picker);

	private:
		System::ComponentModel::Container^ _Components;
		GroupBox^ _GroupBox;
		Control_ColorPicker^ _Color_Picker;
		Control_ColorPreset^ _Color_Presets;

		void Initialize_Component(void);
		void PresetPanel_SelectedColorChanged(System::Object^ sender, System::EventArgs^ e);
		void Color_Picker_OnColorChanged(Object^ sender, EventArgs^ e);
		void GroupBox_Paint(Object^ sender, PaintEventArgs^ e);

	public:
		property Color SelectedColor {
			Color get();
			void set(Color color);
		}

		property int PresetColor {
			void set(int index);
		}

		event ColorChangedHandler^ ColorChanged;
	};
}