#pragma once

#include <string>
#include <vcclr.h>

#include "gp_parser.h"
#include "Settings.h"
#include "MIDI_Writer.h"
#include "Hotkey_Manager.h"

#include "Control_DropDown.h"
#include "Control_Trackbar_Zoom.h"

#include "Widget_Tab_Info.h"
#include "Widget_Tools_And_Control.h"
#include "Widget_Timeline.h"
#include "Widget_Timeline_Tools.h"
#include "Widget_Timeline_Common.h"

#include "Form_Settings_Hotkeys.h"
#include "Form_Settings_MIDI.h"


using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

namespace MIDILightDrawer
{
	public ref class Form_Main : public System::Windows::Forms::Form
	{
		public:
			Form_Main(void);

			// Hotkey Members
			void Update_Hotkeys();

		protected:
			~Form_Main();

		private:
			System::Resources::ResourceManager^ _Resources;
			MenuStrip^							_Menu_Strip;

			gp_parser::Parser*					_GP_Tab;
			Widget_Tab_Info^					_Tab_Info;
			Widget_Tools_And_Control^			_Tools_And_Control;

			Control_DropDown^					_DropDown_Track_Height;
			Control_DropDown^					_DropDown_Marker;
			Control_TrackBar_Zoom^				_TrackBar_Zoom;

			Widget_Timeline^					_Timeline;
			Widget_Toolbar^						_Toolbar;
			Widget_Draw_Options^				_Draw_Options;
			Widget_Length_Options^				_Length_Options;
			Widget_Color_Options^				_Color_Options;
			Widget_Fade_Options^				_Fade_Options;
			Widget_Strobe_Options^				_Strobe_Options;
			Widget_Bucket_Options^				_Bucket_Options;

			Dictionary<String^, Keys>^			_Active_Hotkeys;

			// Internal Setup Methods
			void InitializeBottomControls(Panel^ container);
			void InitializeToolOptions();
			void InitializeMainMenu();
			void InitializeDebugButtons();

			// Menu Callbacks
			void Menu_File_Open_GP_Click(Object^ sender, System::EventArgs^ e);
			void Menu_File_Open_Light_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_File_Save_Light_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_File_Export_MIDI_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_File_Exit_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Settings_Hotkeys_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Settings_Midi_Click(System::Object^ sender, System::EventArgs^ e);

			// Form Callbacks
			void Form_Main_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e);

			// Control and Widget Callbacks
			void Toolbar_OnToolChanged(System::Object^ sender, TimelineToolType e);
			void OnMidiSettingsAccepted();
			void DropDown_Track_Height_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
			void DropDown_View_Marker_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
			void TrackBar_Zoom_OnValue_Changed(System::Object^ sender, Track_Bar_Value_Changed_Event_Args^ e);

			// Hotkey Members
			void Initialize_Hotkeys();
			bool Process_Hotkey(System::Windows::Forms::Keys key_code);
			void Register_Hotkey_Handlers();
			void Form_KeyDown(Object^ sender, System::Windows::Forms::KeyEventArgs^ e);

			void Draw_Options_OnQuantizationChanged(int value);
			void Draw_Options_OnColorChanged(System::Drawing::Color color);
			void Length_Options_OnQuantizationChanged(int value);
			void Color_Options_OnColorChanged(System::Drawing::Color color);
			void Fade_Options_OnQuantizationChanged(int value);
			void Fade_Options_OnColorStartChanged(System::Drawing::Color color);
			void Fade_Options_OnColorEndChanged(System::Drawing::Color color);
			void Strobe_Options_OnQuantizationChanged(int value);
			void Strobe_Options_OnColorChanged(System::Drawing::Color color);
			void Bucket_Options_OnColorChanged(System::Drawing::Color color);

			std::string ConvertToStdString(System::String^ input_string);

			// Debug Members
			void Button_1_Click(System::Object^ sender, System::EventArgs^ e);
			void Button_2_Click(System::Object^ sender, System::EventArgs^ e);
	};
}
