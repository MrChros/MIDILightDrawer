#pragma once

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Collections;
using namespace System::Collections::Generic;

#include "Form_Main.h"
#include "Theme_Manager.h"
#include "Control_DataGrid.h"

namespace MIDILightDrawer
{
	public ref class Form_Settings_Hotkeys : public Form
	{
	private:
		System::Resources::ResourceManager^ _Resources;
		
		TextBox^				_Filter_Action;
		TextBox^				_Filter_Hotkey;
		ComboBox^				_Filter_Category;
		List<DataGridViewRow^>^ _All_Rows;
		
		TableLayoutPanel^		_Main_Layout;
		Button^					_Button_OK;
		Button^					_Button_Cancel;
		Control_DataGrid^		_Grid_Hotkeys;


		int _Current_Edit_Row;

		void Initialize_Component();
		void Initialize_Filter_Controls();
		void Load_Hotkeys();
		void Save_Hotkeys();
		void PopulateCategories();

		bool Check_Hotkey_Uniqueness(int current_row);
		void Update_Row_Status_Icons();

		void Button_OK_Click(Object^ sender, EventArgs^ e);
		void Grid_CellClick(Object^ sender, DataGridViewCellEventArgs^ e);
		void Grid_Hotkeys_RowPostPaint(Object^ sender, DataGridViewRowPostPaintEventArgs^ e);
		void Form_KeyDown(Object^ sender, KeyEventArgs^ e);
		void Filter_Changed(Object^ sender, EventArgs^ e);

		bool IsModifierKey(Keys key);
		String^ GetDisplayKeyName(Keys key);

		static int CompareHotkeyDefinitions(Hotkey_Definition^ a, Hotkey_Definition^ b);

		void GroupBox_Paint(Object^ sender, PaintEventArgs^ e);

	public:
		Form_Settings_Hotkeys();
	};
}