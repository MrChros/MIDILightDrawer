#pragma once

#include "Theme_Manager.h"
#include "Device_Manager.h"
#include "Control_GroupBox.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public delegate void On_Device_Settings_Changed_Handler();
	
	ref class Form_Device_Configuration : public System::Windows::Forms::Form
	{
	public:
		event On_Device_Settings_Changed_Handler^ On_Device_Settings_Changed;

	public:
		Form_Device_Configuration();

	private:
		System::Resources::ResourceManager^ _Resources;

		// Layout
		TableLayoutPanel^ _Main_Layout;
		Button^ _Button_OK;
		Button^ _Button_Cancel;
		Button^ _Button_Apply;
		ToolTip^ _Tool_Tip;

		// MIDI Output Section
		Control_GroupBox^ _Group_Box_MIDI_Output;
		TableLayoutPanel^ _MIDI_Layout;
		Label^ _Label_MIDI_Device;
		ComboBox^ _Combo_Box_MIDI_Device;
		Label^ _Label_MIDI_Channel;
		ComboBox^ _Combo_Box_MIDI_Channel;
		Button^ _Button_Refresh_MIDI;

		// Audio Output Section
		Control_GroupBox^ _Group_Box_Audio_Output;
		TableLayoutPanel^ _Audio_Layout;
		Label^ _Label_Audio_Device;
		ComboBox^ _Combo_Box_Audio_Device;
		Button^ _Button_Refresh_Audio;
		Label^ _Label_Buffer_Size;
		ComboBox^ _Combo_Box_Buffer_Size;

		// Device manager
		Device_Manager^ _Device_Manager;

		void Initialize_Component();
		void Load_Current_Settings();
		void Save_Settings();
		void Populate_MIDI_Devices();
		void Populate_Audio_Devices();
		void Populate_MIDI_Channels();
		void Populate_Buffer_Sizes();

		void Button_OK_Click(System::Object^ sender, System::EventArgs^ e);
		void Button_Apply_Click(System::Object^ sender, System::EventArgs^ e);
		void Button_Cancel_Click(System::Object^ sender, System::EventArgs^ e);
		void Button_Refresh_MIDI_Click(System::Object^ sender, System::EventArgs^ e);
		void Button_Refresh_Audio_Click(System::Object^ sender, System::EventArgs^ e);
	};
}
