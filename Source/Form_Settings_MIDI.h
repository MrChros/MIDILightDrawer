#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

#include "Theme_Manager.h"
#include "Control_DataGrid.h"
#include "Control_GroupBox.h"

namespace MIDILightDrawer {

	public delegate void On_Settings_Accepted_Handler();

	public value struct Note_Entry {
		String^ Name;
		int Value;

		Note_Entry(String^ name, int value) : Name(name), Value(value) {}

		virtual String^ ToString() override {
			return Name;
		}
	};

	public ref class Form_Settings_MIDI : public System::Windows::Forms::Form
	{
	public:
		event On_Settings_Accepted_Handler^ On_Settings_Accepted;

	public:
		Form_Settings_MIDI();

	private:
		System::Resources::ResourceManager^ _Resources;

		TableLayoutPanel^ _Main_Layout;
		Button^ _Button_OK;
		Button^ _Button_Cancel;
		ToolTip^ _Tool_Tip;

		// Controls for note selection
		ComboBox^ _Combo_Box_Red;
		ComboBox^ _Combo_Box_Green;
		ComboBox^ _Combo_Box_Blue;

		// Additional Check for Anti-Flicker Option
		CheckBox^ _Checkbox_Anti_Flicker;

		// Status icons
		PictureBox^ _Icon_Red;
		PictureBox^ _Icon_Green;
		PictureBox^ _Icon_Blue;

		Control_GroupBox^	_Group_Box_Notes;
		Control_GroupBox^	_Group_Box_Octaves;
		TableLayoutPanel^	_Notes_Layout;
		TableLayoutPanel^	_Octaves_Layout;

		Button^ _Button_Add_Octave;
		Button^ _Button_Remove;
		Button^ _Button_Move_Up;
		Button^ _Button_Move_Down;

		Control_DataGrid^			_Grid_Octaves;
		DataGridViewTextBoxColumn^	_Column_Name;
		DataGridViewComboBoxColumn^ _Column_Octave;

		array<Note_Entry>^ _Note_Names;
		static array<int>^ VALID_OCTAVES = { -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

		void Initialize_Note_Names();
		void Initialize_Component();
		void Load_Current_Settings();
		void Save_Settings();
		void Update_Status_Icons();
		
		int Find_Note_Index_By_Value(int value);
		int Find_Note_Index_By_Name(String^ name);

		void ComboBox_Selected_Index_Changed(System::Object^ sender, System::EventArgs^ e);
		void Checkbox_Anti_Flicker_CheckStateChanged(System::Object^ sender, System::EventArgs^ e);
		void Button_OK_Click(System::Object^ sender, System::EventArgs^ e);
		
		void Initialize_Octaves_Section();
		void Add_Octave_Entry_Click(System::Object^ sender, System::EventArgs^ e);
		void Remove_Entry_Click(System::Object^ sender, System::EventArgs^ e);
		void Move_Up_Click(System::Object^ sender, System::EventArgs^ e);
		void Move_Down_Click(System::Object^ sender, System::EventArgs^ e);
		void Load_Octave_Entries();
		void Save_Octave_Entries();
		void Update_Button_States();
				
		void Grid_Octaves_Selection_Changed(System::Object^ sender, System::EventArgs^ e);
		void Grid_Octaves_Cell_Validating(System::Object^ sender, DataGridViewCellValidatingEventArgs^ e);
	};
}