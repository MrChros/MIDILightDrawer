#pragma once

#include "Control_ColorPicker.h"
#include "Control_ColorPreset.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public ref class Dialog_ColorPicker : public Form
	{
	public:
		Dialog_ColorPicker(void);

		Dialog_ColorPicker(Color initialColor);

	protected:
		~Dialog_ColorPicker();

	private:
		// Components
		Control_ColorPicker^ _ColorPicker;
		Control_ColorPreset^ _ColorPreset;
		Button^ _ButtonOK;
		Button^ _ButtonCancel;
		Panel^ _PreviewPanel;
		Label^ _PreviewLabel;
		TableLayoutPanel^ _MainLayout;
		System::ComponentModel::Container^ _Components;

		// Initialize form and controls
		void InitializeComponent(void);

		// Event handlers
		void OnColorPickerChanged(Object^ sender, EventArgs^ e);
		void OnPresetColorChanged(Object^ sender, EventArgs^ e);

	public:
		// Properties
		property Color SelectedColor
		{
			Color get();
			void set(Color value);
		}

		// Static method to show the dialog
		static Color ShowDialog(IWin32Window^ owner, Color initialColor);
	};
}