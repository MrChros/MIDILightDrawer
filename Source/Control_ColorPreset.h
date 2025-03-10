#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::ComponentModel;
using namespace System::Collections::Generic;

#include "Settings.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer
{
	public ref class Control_ColorPreset : public UserControl
	{
	public:
		Control_ColorPreset();
		Control_ColorPreset(bool showSetButtons);

	private:
		array<Button^>^ _ColorPresetButtons;
		array<Button^>^ _SetButtons;
		array<Color>^	_Colors;

		int				_SelectedIndex;
		Color			_SelectedColor;
		Color			_HighlightColor;
		bool			_ShowSetButtons;

		static const int COUNT_PRESET_COLORS = 10;
		static const int COLOR_CIRCLE_DIAMETER = 27;

		void Initialize_Component();
		void Save_PresetColors();
		void Load_PresetColors();
		void UpdateButtonIcon(int index, Color color);
		void Color_Preset_Button_Click(System::Object^ sender, System::EventArgs^ e);
		void Set_Button_Click(System::Object^ sender, System::EventArgs^ e);

		void Color_Button_Paint(Object^ sender, PaintEventArgs^ e);
		void Button_MouseEnter(Object^ sender, EventArgs^ e);
		void Button_MouseLeave(Object^ sender, EventArgs^ e);

	protected:
		void OnPresetColorsChanged();
		void OnSelectedColorChanged();

	public:
		static Drawing::Icon^ CreateColorIcon(Color color, int diameter);
		static Drawing::Bitmap^ CreateColorBitmap(Color color, int diameter);

		property Color SelectedColor {
			Color get();
			void set(Color color);
		}

		property int SelectedIndex {
			void set(int index);
		}

		// Event that fires when a color preset is modified
		event EventHandler^ PresetColorsChanged;

		// Event that fires when the selected color is changed
		event EventHandler^ SelectedColorChanged;
	};
}
