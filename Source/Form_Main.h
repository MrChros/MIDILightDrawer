#pragma once

#include <string>
#include <vcclr.h>

#include "gp_parser.h"
#include "Settings.h"
#include "MIDI_Exporter.h"
#include "MIDI_Event_Raster.h"
#include "Hotkey_Manager.h"
#include "Playback_Manager.h"
#include "Playback_Audio_File_Manager.h"

#include "Control_DropDown.h"
#include "Control_Trackbar_Zoom.h"

#include "Widget_Tab_Info.h"
#include "Widget_Tools_And_Control.h"
#include "Widget_Timeline.h"
#include "Widget_Timeline_Tools.h"
#include "Widget_Timeline_Common.h"
#include "Widget_Audio_Container.h"
#include "Widget_Transport_Controls.h"

#include "Form_BatchAction.h"
#include "Form_Light_Import.h"
#include "Form_Settings_MIDI.h"
#include "Form_Settings_Hotkeys.h"
#include "Form_Device_Configuration.h"


using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace MIDILightDrawer
{
	public ref struct MIDI_Writer_Event_Info
	{
		int Track_Index;
		String^ Color;
		uint32_t Start_Tick;
		uint32_t Length_Ticks;
		uint8_t Channel;
		uint8_t Note;
		uint8_t Velocity;
		int Original_Color_Note_Index;
	};

	// Comparison function for sorting - define as a separate function
	public ref class MIDI_Writer_Event_Comparer : System::Collections::Generic::IComparer<MIDI_Writer_Event_Info^>
	{
	public:
		virtual int Compare(MIDI_Writer_Event_Info^ a, MIDI_Writer_Event_Info^ b)
		{
			if (a->Start_Tick != b->Start_Tick)
				return a->Start_Tick.CompareTo(b->Start_Tick);
			// If same start tick, sort by end tick
			uint32_t a_end = a->Start_Tick + a->Length_Ticks;
			uint32_t b_end = b->Start_Tick + b->Length_Ticks;
			return a_end.CompareTo(b_end);
		}
	};
	
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
			ToolStripMenuItem^					_Menu_Edit_Undo;
			ToolStripMenuItem^					_Menu_Edit_Redo;
			ToolStripMenuItem^					_Menu_Edit_Copy;
			ToolStripMenuItem^					_Menu_Edit_Paste;
			ToolStripMenuItem^					_Menu_Edit_Delete;
			ToolStripMenuItem^					_Menu_Edit_UndoSteps;
			List<ToolStripMenuItem^>^			_Menu_Edit_UndoSteps_Items;
			ToolStripMenuItem^					_Menu_Edit_BatchAction;

			gp_parser::Parser*					_GP_Tab;
			Widget_Tab_Info^					_Tab_Info;
			Widget_Tools_And_Control^			_Tools_And_Control;
			Widget_Transport_Controls^			_Transport_Controls;

			Control_DropDown^					_DropDown_Track_Height;
			Control_DropDown^					_DropDown_Marker;
			Control_TrackBar_Zoom^				_TrackBar_Zoom;

			Widget_Timeline^					_Timeline;
			Widget_Toolbar^						_Toolbar;
			Widget_Pointer_Options^				_Pointer_Options;
			Widget_Draw_Options^				_Draw_Options;
			Widget_Length_Options^				_Length_Options;
			Widget_Color_Options^				_Color_Options;
			Widget_Fade_Options^				_Fade_Options;
			Widget_Strobe_Options^				_Strobe_Options;
			Widget_Bucket_Options^				_Bucket_Options;
			Widget_Audio_Container^				_Audio_Container;

			MIDI_Exporter^						_MIDI_Exporter;
			MIDI_Event_Raster^					_MIDI_Event_Raster;

			Playback_Manager^					_Playback_Manager;
			Playback_Audio_File_Manager^		_Playback_Audio_File_Manager;
			System::Windows::Forms::Timer^		_Playback_Update_Timer;

			Dictionary<String^, Keys>^			_Active_Hotkeys;

			// Internal Setup Methods
			void InitializeBottomControls(Panel^ container);
			void InitializeToolOptions();
			void InitializeMainMenu();
			void InitializeDebugButtons();
			void InitializePlaybackSystem();
			void ShutdownPlaybackSystem();

			// Menu Callbacks
			void Menu_File_Open_GP_Click(Object^ sender, System::EventArgs^ e);
			void Menu_File_Open_Light_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_File_Open_Light_Special_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_File_Save_Light_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Audio_Open_File_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Audio_Clear_File_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_File_Export_MIDI_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_File_Exit_Click(System::Object^ sender, System::EventArgs^ e);

			void Menu_Edit_Undo_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Edit_UndoHistory_Click(Object^ sender, EventArgs^ e);
			void Menu_Edit_Redo_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Edit_Copy_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Edit_Paste_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Edit_Delete_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Edit_BatchAction_Click(System::Object^ sender, System::EventArgs^ e);

			void Menu_Settings_Hotkeys_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Settings_Midi_Click(System::Object^ sender, System::EventArgs^ e);
			void Menu_Settings_Device_Click(System::Object^ sender, System::EventArgs^ e);

			// Form Callbacks
			void Form_Main_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e);

			// Undo/Redo Handler
			void UpdateUndoRedoState();
			void UpdateUndoHistoryMenu();
			void UpdateEditMenuState(System::Object^ sender, MIDILightDrawer::TimelineToolType e);
			void UpdateEditMenuState(System::Object^ sender, System::EventArgs^ e);
			void UpdateEditMenuState();

			// Control and Widget Callbacks
			void Toolbar_OnToolChanged(System::Object^ sender, TimelineToolType e);
			void SettingsMIDI_On_Settings_Accepted();
			void SettingsDevice_On_Settings_Changed();
			void DropDown_Track_Height_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
			void DropDown_View_Marker_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e);
			void TrackBar_Zoom_OnValue_Changed(System::Object^ sender, Track_Bar_Value_Changed_Event_Args^ e);

			// Hotkey Members
			void Initialize_Hotkeys();
			void Register_Hotkey_Handlers();
			bool Process_Hotkey(System::Windows::Forms::Keys key_code);
			void Form_KeyDown(Object^ sender, System::Windows::Forms::KeyEventArgs^ e);
			void Form_KeyUp(Object^ sender, System::Windows::Forms::KeyEventArgs^ e);

			// Playback Timer Callback
			void Playback_Update_Timer_Tick(Object ^ sender, EventArgs ^ e);

			void Pointer_Options_OnSnappingChanged(int value);
			void Pointer_Options_OnQuantizationChanged(int value);
			void Draw_Options_OnSnappingChanged(int value);
			void Draw_Options_OnLengthChanged(int value);
			void Draw_Options_OnColorChanged(System::Drawing::Color color);
			void Draw_Options_OnConsiderTabChanged(bool value);
			void Length_Options_OnLengthChanged(int value);
			void Color_Options_OnColorChanged(System::Drawing::Color color);
			void Fade_Options_OnLengthChanged(int value);
			void Fade_Options_OnFadeModeChanged(FadeType mode);
			void Fade_Options_OnColorStartChanged(System::Drawing::Color color);
			void Fade_Options_OnColorEndChanged(System::Drawing::Color color);
			void Fade_Options_OnColorCenterChanged(System::Drawing::Color color);
			void Fade_Options_OnEasingsChanged(FadeEasing easeIn, FadeEasing easeOut);
			void Strobe_Options_OnLengthChanged(int value);
			void Strobe_Options_OnColorChanged(System::Drawing::Color color);
			void Bucket_Options_OnColorChanged(System::Drawing::Color color);

			// Debug Members
			void Button_1_Click(System::Object^ sender, System::EventArgs^ e);
			void Button_2_Click(System::Object^ sender, System::EventArgs^ e);
			void Button_3_Click(System::Object^ sender, System::EventArgs^ e);
	};
}
