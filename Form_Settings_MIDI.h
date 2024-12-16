#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer {

	public delegate void On_Settings_Accepted_Handler();

	public ref class Form_Settings_MIDI : public System::Windows::Forms::Form
	{
	public:
		event On_Settings_Accepted_Handler^ On_Settings_Accepted;

	private:
		System::Resources::ResourceManager^ _Resources;

		System::Windows::Forms::TableLayoutPanel^ _Main_Layout;
		System::Windows::Forms::Button^ _Button_OK;
		System::Windows::Forms::Button^ _Button_Cancel;

		// Controls for note selection
		System::Windows::Forms::ComboBox^ _Combo_Box_Red;
		System::Windows::Forms::ComboBox^ _Combo_Box_Green;
		System::Windows::Forms::ComboBox^ _Combo_Box_Blue;

		// Status icons
		System::Windows::Forms::PictureBox^ _Icon_Red;
		System::Windows::Forms::PictureBox^ _Icon_Green;
		System::Windows::Forms::PictureBox^ _Icon_Blue;

		System::Windows::Forms::GroupBox^ _Group_Box_Notes;
		System::Windows::Forms::GroupBox^ _Group_Box_Octaves;
		System::Windows::Forms::TableLayoutPanel^ _Notes_Layout;
		System::Windows::Forms::TableLayoutPanel^ _Octaves_Layout;

		System::Windows::Forms::Button^ _Button_Add_Octave;
		System::Windows::Forms::Button^ _Button_Remove;
		System::Windows::Forms::Button^ _Button_Move_Up;
		System::Windows::Forms::Button^ _Button_Move_Down;

		System::Windows::Forms::DataGridView^ _Grid_Octaves;
		System::Windows::Forms::DataGridViewTextBoxColumn^ _Column_Name;
		System::Windows::Forms::DataGridViewComboBoxColumn^ _Column_Octave;

		array<String^>^ _Note_Names;
		static array<int>^ VALID_OCTAVES = { -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

		void Initialize_Component();
		void Load_Current_Settings();
		void Save_Settings();
		void Button_OK_Click(System::Object^ sender, System::EventArgs^ e);
		void Combo_Box_Selected_Index_Changed(System::Object^ sender, System::EventArgs^ e);
		void Initialize_Note_Names();
		void Update_Status_Icons();

		void Initialize_Octaves_Section();
		void Add_Octave_Entry_Click(System::Object^ sender, System::EventArgs^ e);
		void Remove_Entry_Click(System::Object^ sender, System::EventArgs^ e);
		void Move_Up_Click(System::Object^ sender, System::EventArgs^ e);
		void Move_Down_Click(System::Object^ sender, System::EventArgs^ e);
		void Load_Octave_Entries();
		void Save_Octave_Entries();
		void Update_Button_States();

		void Grid_Selection_Changed(System::Object^ sender, System::EventArgs^ e);
		void Grid_Cell_Validating(System::Object^ sender, DataGridViewCellValidatingEventArgs^ e);

	public:
		Form_Settings_MIDI();
	};
}