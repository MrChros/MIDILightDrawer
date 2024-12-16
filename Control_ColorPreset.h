#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;

#include "Settings.h"

namespace MIDILightDrawer
{
	public ref class Control_ColorPreset : public UserControl {
	private:
		array<Button^>^ _Color_Preset_Buttons;
		array<Button^>^ _Set_Buttons;
		Color			_Selected_Color;

		void Initialize_Component();
		void Save_PresetColors();
		void Load_PresetColors();
		void Color_Preset_Button_Click(System::Object^ sender, System::EventArgs^ e);
		void Set_Button_Click(System::Object^ sender, System::EventArgs^ e);

	public:
		Control_ColorPreset(void);

		property Color SelectedColor{
			Color get() { return _Selected_Color; }
			void set(Color color) {
				_Selected_Color = color;
			}
		}

		property int SelectedIndex{
			void set(int index);
		}

		// Event that fires when a color preset is modified
		event EventHandler^ PresetColorsChanged;
		// Event that fires when the selected color is changed
		event EventHandler^ SelectedColorChanged;

	protected:
		void OnPresetColorsChanged() {
			PresetColorsChanged(this, EventArgs::Empty);
		}

		void OnSelectedColorChanged() {
			SelectedColorChanged(this, EventArgs::Empty);
		}
	};
}
