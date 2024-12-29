#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;

#include "Settings.h"
#include "Control_DropDown.h"
#include "Control_GroupBox.h"
#include "Control_ColorPreset.h"
#include "Control_ColorPicker.h"
#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer
{
	public delegate void QuantizationChangedHandler(int value);
	public delegate void ConsiderTabChangedHandler(bool value);
	public delegate void ColorChangedHandler(Color color);

	public ref class Widget_Draw_Options : public UserControl
	{
	public:
		Widget_Draw_Options(Control_ColorPicker^ color_picker);

	private:
		System::ComponentModel::Container^ _Components;
		Control_GroupBox^		_GroupBox;
		Control_DropDown^		_DropDown_Draw_Snapping;
		Control_DropDown^		_DropDown_Draw_Length;
		CheckBox^				_CheckBox_Consider_Tab;
		Control_ColorPicker^	_Color_Picker;
		Control_ColorPreset^	_Color_Presets;

		void Initialize_Component(void);
		void DropDown_Draw_Snapping_OnItem_Selected(System::Object^ sender, MIDILightDrawer::Control_DropDown_Item_Selected_Event_Args^ e);
		void DropDown_Draw_Length_OnItem_Selected(System::Object^ sender, MIDILightDrawer::Control_DropDown_Item_Selected_Event_Args^ e);
		void CheckBox_Consider_Tab_OnCheckStateChanged(System::Object^ sender, System::EventArgs^ e);
		void PresetPanel_SelectedColorChanged(System::Object^ sender, System::EventArgs^ e);
		void Color_Picker_OnColorChanged(Object^ sender, EventArgs^ e);

	protected:
		~Widget_Draw_Options();

	public:
		void Select_Draw_Snapping_Next(void);
		void Select_Draw_Snapping_Previous(void);
		void Select_Draw_Length_Next(void);
		void Select_Draw_Length_Previous(void);
		void Toggle_LengthByTablature(void);
		
		property int DrawSnapping{
			int get();
			void set(int value);
		}

		property int DrawLength {
			int get();
			void set(int value);
		}

		property bool LengthByTablature {
			bool get();
			void set(bool value);
		}

		property Color SelectedColor {
			Color get();
			void set(Color color);
		}

		property int PresetColor {
			void set(int index);
		}

		event QuantizationChangedHandler^ SnappingChanged;
		event QuantizationChangedHandler^ LengthChanged;
		event ConsiderTabChangedHandler^ ConsiderTabChanged;
		event ColorChangedHandler^ ColorChanged;
	};
}