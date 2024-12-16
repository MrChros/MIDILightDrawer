#include "pch.h"
#include "Form_Settings_Hotkeys.h"
#include "Settings.h"

namespace MIDILightDrawer
{
	Form_Settings_Hotkeys::Form_Settings_Hotkeys() {
		_Current_Edit_Row = -1;

		Initialize_Component();
		Load_Hotkeys();
		Update_Row_Status_Icons();
	}

	void Form_Settings_Hotkeys::Initialize_Component()
	{
		this->Text = "Hotkey Settings";
		this->Size = System::Drawing::Size(619, 600);
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->StartPosition = FormStartPosition::CenterParent;
		this->KeyPreview = true;
		this->KeyDown += gcnew KeyEventHandler(this, &Form_Settings_Hotkeys::Form_KeyDown);

		_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());

		_Main_Layout = gcnew TableLayoutPanel();
		_Main_Layout->Dock = DockStyle::Fill;
		_Main_Layout->RowCount = 3;
		_Main_Layout->ColumnCount = 3;
		_Main_Layout->Padding = System::Windows::Forms::Padding(20);

		_Main_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent,  2));
		_Main_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 96));
		_Main_Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent,  2));

		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 80));  // Height for filter panel
		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 90));   // Grid
		_Main_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 10));   // Buttons

		Initialize_Filter_Controls();

		_Grid_Hotkeys = gcnew DataGridView();
		_Grid_Hotkeys->Dock = DockStyle::Fill;
		_Grid_Hotkeys->AutoGenerateColumns = false;
		_Grid_Hotkeys->AllowUserToAddRows = false;
		_Grid_Hotkeys->AllowUserToDeleteRows = false;
		_Grid_Hotkeys->MultiSelect = false;
		_Grid_Hotkeys->SelectionMode = DataGridViewSelectionMode::FullRowSelect;
		_Grid_Hotkeys->AllowUserToResizeRows = false;
		_Grid_Hotkeys->AllowUserToResizeColumns = false;
		_Grid_Hotkeys->AllowUserToOrderColumns = false;
		_Grid_Hotkeys->ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode::DisableResizing;
		_Grid_Hotkeys->RowHeadersVisible = true;
		_Grid_Hotkeys->RowHeadersWidthSizeMode = DataGridViewRowHeadersWidthSizeMode::DisableResizing;
		_Grid_Hotkeys->EnableHeadersVisualStyles = false;
		_Grid_Hotkeys->ScrollBars = ScrollBars::Vertical;
		_Grid_Hotkeys->ColumnHeadersDefaultCellStyle->SelectionBackColor = _Grid_Hotkeys->ColumnHeadersDefaultCellStyle->BackColor;
		_Grid_Hotkeys->AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode::Fill;

		// Grid columns
		DataGridViewTextBoxColumn^ col_action = gcnew DataGridViewTextBoxColumn();
		col_action->Name = "Action";
		col_action->HeaderText = "Action";
		col_action->ReadOnly = true;
		col_action->FillWeight = 46;  // Slightly increased to distribute the extra space
		_Grid_Hotkeys->Columns->Add(col_action);

		DataGridViewTextBoxColumn^ col_category = gcnew DataGridViewTextBoxColumn();
		col_category->Name = "Category";
		col_category->HeaderText = "Category";
		col_category->ReadOnly = true;
		col_category->FillWeight = 22;  // Slightly increased to distribute the extra space
		_Grid_Hotkeys->Columns->Add(col_category);

		DataGridViewTextBoxColumn^ col_hotkey = gcnew DataGridViewTextBoxColumn();
		col_hotkey->Name = "Hotkey";
		col_hotkey->HeaderText = "Hotkey";
		col_hotkey->ReadOnly = true;
		col_hotkey->FillWeight = 23;  // Slightly increased to distribute the extra space
		_Grid_Hotkeys->Columns->Add(col_hotkey);

		DataGridViewButtonColumn^ col_set = gcnew DataGridViewButtonColumn();
		col_set->Name = "Set";
		col_set->HeaderText = "";
		col_set->Text = "Set";
		col_set->UseColumnTextForButtonValue = true;
		col_set->FillWeight = 10;
		_Grid_Hotkeys->Columns->Add(col_set);

		DataGridViewButtonColumn^ col_del = gcnew DataGridViewButtonColumn();
		col_del->Name = "Del";
		col_del->HeaderText = "";
		col_del->Text = "Del";
		col_del->UseColumnTextForButtonValue = true;
		col_del->FillWeight = 10;
		_Grid_Hotkeys->Columns->Add(col_del);

		// Update each column to disable sorting
		for each (DataGridViewColumn ^ column in _Grid_Hotkeys->Columns) {
			column->SortMode = DataGridViewColumnSortMode::NotSortable;
		}

		_Grid_Hotkeys->CellClick += gcnew DataGridViewCellEventHandler(this, &Form_Settings_Hotkeys::Grid_CellClick);
		_Grid_Hotkeys->RowPostPaint += gcnew DataGridViewRowPostPaintEventHandler(this, &Form_Settings_Hotkeys::Grid_Hotkeys_RowPostPaint);
		
		_Main_Layout->Controls->Add(_Grid_Hotkeys, 1, 1);

		FlowLayoutPanel^ button_panel = gcnew FlowLayoutPanel();
		button_panel->Dock = DockStyle::Fill;
		button_panel->FlowDirection = FlowDirection::RightToLeft;
		button_panel->Padding = System::Windows::Forms::Padding(0, 10, 0, 0);

		_Button_Cancel = gcnew Button();
		_Button_Cancel->Text = "Cancel";
		_Button_Cancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
		_Button_Cancel->Width = 80;
		button_panel->Controls->Add(_Button_Cancel);

		_Button_OK = gcnew Button();
		_Button_OK->Text = "OK";
		_Button_OK->DialogResult = System::Windows::Forms::DialogResult::OK;
		_Button_OK->Width = 80;
		_Button_OK->Margin = System::Windows::Forms::Padding(10, 3, 3, 3);
		_Button_OK->Click += gcnew EventHandler(this, &Form_Settings_Hotkeys::Button_OK_Click);
		button_panel->Controls->Add(_Button_OK);

		_Main_Layout->Controls->Add(button_panel, 1, 2);

		this->Controls->Add(_Main_Layout);
		this->AcceptButton = _Button_OK;
		this->CancelButton = _Button_Cancel;
	}

	void Form_Settings_Hotkeys::Initialize_Filter_Controls() {
		GroupBox^ Filter_Group = gcnew GroupBox();
		Filter_Group->Text = "Filters";
		Filter_Group->Dock = DockStyle::Fill;
		Filter_Group->Padding = System::Windows::Forms::Padding(10, 5, 10, 5);
		Filter_Group->FlatStyle = FlatStyle::Standard;

		TableLayoutPanel^ Filter_Panel = gcnew TableLayoutPanel();
		Filter_Panel->Dock = DockStyle::Fill;
		Filter_Panel->AutoSize = true;
		Filter_Panel->ColumnCount = 3;
		Filter_Panel->RowCount = 2;
		Filter_Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));
		Filter_Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));
		Filter_Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::AutoSize));
		Filter_Panel->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize));
		Filter_Panel->RowStyles->Add(gcnew RowStyle(SizeType::AutoSize));

		// Action filter
		Label^ Label_Action = gcnew Label();
		Label_Action->Text = "Action:";
		Label_Action->AutoSize = true;
		Label_Action->Margin = System::Windows::Forms::Padding(0, 0, 20, 0);
		Filter_Panel->Controls->Add(Label_Action, 0, 0);

		_Filter_Action = gcnew TextBox();
		_Filter_Action->Width = 100;
		_Filter_Action->Margin = System::Windows::Forms::Padding(0, 0, 20, 0);
		_Filter_Action->TextChanged += gcnew EventHandler(this, &Form_Settings_Hotkeys::Filter_Changed);
		Filter_Panel->Controls->Add(_Filter_Action, 0, 1);

		// Category filter
		Label^ Label_Category = gcnew Label();
		Label_Category->Text = "Category:";
		Label_Category->AutoSize = true;
		Label_Category->Margin = System::Windows::Forms::Padding(0, 0, 20, 0);
		Filter_Panel->Controls->Add(Label_Category, 1, 0);

		_Filter_Category = gcnew ComboBox();
		_Filter_Category->Width = 100;
		_Filter_Category->Margin = System::Windows::Forms::Padding(0, 0, 20, 0);
		_Filter_Category->DropDownStyle = ComboBoxStyle::DropDownList;
		_Filter_Category->SelectedIndexChanged += gcnew EventHandler(this, &Form_Settings_Hotkeys::Filter_Changed);
		Filter_Panel->Controls->Add(_Filter_Category, 1, 1);

		// Hotkey filter
		Label^ Label_Hotkey = gcnew Label();
		Label_Hotkey->Text = "Hotkey:";
		Label_Hotkey->AutoSize = true;
		Filter_Panel->Controls->Add(Label_Hotkey, 2, 0);

		_Filter_Hotkey = gcnew TextBox();
		_Filter_Hotkey->Width = 100;
		_Filter_Hotkey->TextChanged += gcnew EventHandler(this, &Form_Settings_Hotkeys::Filter_Changed);
		Filter_Panel->Controls->Add(_Filter_Hotkey, 2, 1);

		Filter_Group->Controls->Add(Filter_Panel);
		
		_Main_Layout->Controls->Add(Filter_Group, 1, 0);
	}

	void Form_Settings_Hotkeys::Load_Hotkeys()
	{
		_All_Rows = gcnew List<DataGridViewRow^>();
		_Grid_Hotkeys->Rows->Clear();
		_All_Rows->Clear();

		List<Hotkey_Definition^>^ hotkeys = Hotkey_Manager::Instance->Definitions;

		// Sort the hotkeys by category and action
		hotkeys->Sort(gcnew Comparison<Hotkey_Definition^>(CompareHotkeyDefinitions));

		// Add all hotkeys to the grid
		for each(Hotkey_Definition ^ def in hotkeys) {
			if (def == nullptr || def->Category == nullptr) continue;
			String^ currentBinding = Hotkey_Manager::Instance->Get_Binding(def->Action);
			_Grid_Hotkeys->Rows->Add(def->Action, def->Category, currentBinding);
		}

		PopulateCategories();
	}

	void Form_Settings_Hotkeys::Save_Hotkeys()
	{
		Dictionary<String^, String^>^ new_bindings = gcnew Dictionary<String^, String^>();

		for each(DataGridViewRow ^ row in _Grid_Hotkeys->Rows) {
			String^ action = safe_cast<String^>(row->Cells[0]->Value);
			String^ key = safe_cast<String^>(row->Cells[2]->Value);
			if (!String::IsNullOrEmpty(key)) {
				new_bindings->Add(action, key);
			}
		}

		Hotkey_Manager::Instance->Set_Bindings(new_bindings);
		Settings::Get_Instance()->Save_To_File();
	}

	void Form_Settings_Hotkeys::PopulateCategories()
	{
		SortedSet<String^>^ categories = gcnew SortedSet<String^>();
		categories->Add("All");  // Add "All" as first item

		for each (DataGridViewRow ^ row in _Grid_Hotkeys->Rows) {
			String^ category = safe_cast<String^>(row->Cells[1]->Value);
			if (!String::IsNullOrEmpty(category)) {
				categories->Add(category);
			}
		}

		_Filter_Category->Items->Clear();
		for each (String ^ category in categories) {
			_Filter_Category->Items->Add(category);
		}
		_Filter_Category->SelectedIndex = 0;  // Select "All" by default
	}

	bool Form_Settings_Hotkeys::Check_Hotkey_Uniqueness(int current_row)
	{
		String^ current_hotkey = safe_cast<String^>(_Grid_Hotkeys->Rows[current_row]->Cells[2]->Value);  // Changed index from 1 to 2
		if (String::IsNullOrEmpty(current_hotkey)) return true;

		for (int i = 0; i < _Grid_Hotkeys->Rows->Count; i++) {
			if (i == current_row) continue;
			String^ other_hotkey = safe_cast<String^>(_Grid_Hotkeys->Rows[i]->Cells[2]->Value);  // Changed index from 1 to 2
			if (current_hotkey->Equals(other_hotkey) && !String::IsNullOrEmpty(other_hotkey)) {
				return false;
			}
		}
		return true;
	}

	void Form_Settings_Hotkeys::Update_Row_Status_Icons()
	{
		Drawing::Image^ Tick_Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Tick")));
		Drawing::Image^ Error_Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Error")));

		for (int i = 0; i < _Grid_Hotkeys->Rows->Count; i++) {
			bool is_unique = Check_Hotkey_Uniqueness(i);

			// Clear existing value
			_Grid_Hotkeys->Rows[i]->HeaderCell->Value = nullptr;

			// Set image in tag for use in RowPostPaint
			_Grid_Hotkeys->Rows[i]->HeaderCell->Tag = is_unique ? Tick_Image : Error_Image;
		}

		_Grid_Hotkeys->Invalidate();
	}

	void Form_Settings_Hotkeys::Button_OK_Click(System::Object^ sender, System::EventArgs^ e) {
		Save_Hotkeys();
		safe_cast<Form_Main^>(this->Owner)->Update_Hotkeys();
		this->Close();
	}

	void Form_Settings_Hotkeys::Grid_CellClick(Object^ sender, DataGridViewCellEventArgs^ e)
	{
		if (e->RowIndex < 0) return;

		if (e->ColumnIndex == 3) {  // Updated index for Set button
			DataGridViewRow^ row = _Grid_Hotkeys->Rows[e->RowIndex];
			this->ActiveControl = _Grid_Hotkeys;
			_Current_Edit_Row = e->RowIndex;
			this->KeyPreview = true;
			row->Cells[2]->Value = "Press any key...";  // Updated index for hotkey
		}
		else if (e->ColumnIndex == 4) {  // Updated index for Del button
			_Grid_Hotkeys->Rows[e->RowIndex]->Cells[2]->Value = "";  // Updated index for hotkey
			Update_Row_Status_Icons();
		}
	}

	void Form_Settings_Hotkeys::Grid_Hotkeys_RowPostPaint(Object^ sender, DataGridViewRowPostPaintEventArgs^ e) {
		Image^ icon = safe_cast<Image^>(_Grid_Hotkeys->Rows[e->RowIndex]->HeaderCell->Tag);
		if (icon != nullptr) {
			Rectangle rect = Rectangle(
				e->RowBounds.X + 20,
				e->RowBounds.Y + 2,  // Small vertical adjustment for centering
				16,
				16
			);
			e->Graphics->DrawImage(icon, rect);
		}
	}

	void Form_Settings_Hotkeys::Form_KeyDown(Object^ sender, KeyEventArgs^ e)
	{
		if (_Current_Edit_Row >= 0) {
			String^ hotkey = "";
			if (e->Control) hotkey += "Ctrl + ";
			if (e->Shift) hotkey += "Shift + ";
			if (e->Alt) hotkey += "Alt + ";

			if (!IsModifierKey(e->KeyCode)) {
				hotkey += GetDisplayKeyName(e->KeyCode);
				_Grid_Hotkeys->Rows[_Current_Edit_Row]->Cells[2]->Value = hotkey;
				Update_Row_Status_Icons();
				_Current_Edit_Row = -1;
				this->KeyPreview = false;
			}
			e->Handled = true;
		}
	}

	void Form_Settings_Hotkeys::Filter_Changed(Object^ sender, EventArgs^ e)
	{
		// Store all rows if not already stored
		if (_All_Rows->Count == 0) {
			for each (DataGridViewRow ^ row in _Grid_Hotkeys->Rows) {
				_All_Rows->Add(row);
			}
		}

		String^ action_filter = _Filter_Action->Text->ToLower();
		String^ hotkey_filter = _Filter_Hotkey->Text->ToLower();
		String^ category_filter = safe_cast<String^>(_Filter_Category->SelectedItem);

		_Grid_Hotkeys->Rows->Clear();

		for each (DataGridViewRow ^ row in _All_Rows) {
			String^ action = safe_cast<String^>(row->Cells[0]->Value)->ToLower();
			String^ category = safe_cast<String^>(row->Cells[1]->Value);
			String^ hotkey = safe_cast<String^>(row->Cells[2]->Value);
			if (hotkey == nullptr) hotkey = "";  // Handle null hotkeys

			bool action_match = String::IsNullOrEmpty(action_filter) || action->Contains(action_filter);
			bool category_match = category_filter == "All" || category_filter == category;
			bool hotkey_match = String::IsNullOrEmpty(hotkey_filter) || hotkey->ToLower()->Contains(hotkey_filter);

			if (action_match && category_match && hotkey_match) {
				// Create a new row with the original values instead of cloning
				array<Object^>^ row_values = gcnew array<Object^>(5);  // 5 columns total
				row_values[0] = row->Cells[0]->Value;  // Action
				row_values[1] = row->Cells[1]->Value;  // Category
				row_values[2] = row->Cells[2]->Value;  // Hotkey
				// Columns 3 and 4 are button columns, no need to set values

				_Grid_Hotkeys->Rows->Add(row_values);
			}
		}

		Update_Row_Status_Icons();
	}

	bool Form_Settings_Hotkeys::IsModifierKey(Keys key) {
		return key == Keys::ControlKey ||
			key == Keys::ShiftKey ||
			key == Keys::Alt ||
			key == Keys::LControlKey ||
			key == Keys::RControlKey ||
			key == Keys::LShiftKey ||
			key == Keys::RShiftKey ||
			key == Keys::LMenu ||
			key == Keys::RMenu;
	}

	String^ Form_Settings_Hotkeys::GetDisplayKeyName(Keys key) {
		//if (key >= Keys::D0 && key <= Keys::D9) {
		//	return ((int)key - (int)Keys::D0).ToString();
		//}
		return key.ToString();
	}

	int Form_Settings_Hotkeys::CompareHotkeyDefinitions(Hotkey_Definition^ a, Hotkey_Definition^ b) {
		if (a == nullptr || b == nullptr) return 0;

		if (a->Category == nullptr || b->Category == nullptr) return 0;
		int categoryCompare = String::Compare(a->Category, b->Category);
		if (categoryCompare != 0) return categoryCompare;

		if (a->Action == nullptr || b->Action == nullptr) return 0;
		return String::Compare(a->Action, b->Action);
	}
}
