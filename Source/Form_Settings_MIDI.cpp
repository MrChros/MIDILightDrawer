#include "Form_Settings_MIDI.h"
#include "Settings.h"

namespace MIDILightDrawer
{
	Form_Settings_MIDI::Form_Settings_MIDI()
	{
		Initialize_Note_Names();
		Initialize_Component();
		Initialize_Octaves_Section();
		Load_Current_Settings();
		Load_Octave_Entries();

		Theme_Manager::Get_Instance()->ApplyTheme(this);
	}

	void Form_Settings_MIDI::Initialize_Note_Names()
	{
		_Note_Names = gcnew array<Note_Entry>{
			Note_Entry("C" , 0),
			Note_Entry("C#", 1),
			Note_Entry("D" , 2),
			Note_Entry("D#", 3),
			Note_Entry("E" , 4),
			Note_Entry("F" , 5),
			Note_Entry("F#", 6),
			Note_Entry("G" , 7),
			Note_Entry("G#", 8),
			Note_Entry("A" , 9),
			Note_Entry("A#", 10),
			Note_Entry("B" , 11)
		};
	}

	void Form_Settings_MIDI::Initialize_Component()
	{
		this->Text = "MIDI Note Settings";
		this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
		this->MaximizeBox = false;
		this->MinimizeBox = false;
		this->ShowInTaskbar = false;
		this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
		this->Size = System::Drawing::Size(400, 630);

		_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());

		_Main_Layout = gcnew System::Windows::Forms::TableLayoutPanel();
		_Main_Layout->Dock = System::Windows::Forms::DockStyle::Fill;
		_Main_Layout->ColumnCount = 1;
		_Main_Layout->RowCount = 3;
		_Main_Layout->Padding = System::Windows::Forms::Padding(10);

		_Main_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 180));
		_Main_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 350));
		_Main_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 50));

		_Group_Box_Notes = gcnew System::Windows::Forms::GroupBox();
		_Group_Box_Notes->Text		= "Color MIDI Notes";
		_Group_Box_Notes->Dock		= System::Windows::Forms::DockStyle::Fill;
		_Group_Box_Notes->Height	= 150;
		_Group_Box_Notes->Padding	= System::Windows::Forms::Padding(10, 15, 10, 10);
		_Group_Box_Notes->Paint += gcnew PaintEventHandler(this, &Form_Settings_MIDI::GroupBox_Paint);
		

		_Notes_Layout = gcnew System::Windows::Forms::TableLayoutPanel();
		_Notes_Layout->Dock = System::Windows::Forms::DockStyle::Fill;
		_Notes_Layout->ColumnCount = 3;
		_Notes_Layout->RowCount = 4;
		_Notes_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 35));
		_Notes_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 35));
		_Notes_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 35));
		_Notes_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 35));

		_Notes_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 35));
		_Notes_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 35));
		_Notes_Layout->RowStyles->Add(gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 35));

		System::Windows::Forms::Label^ Label_Red = gcnew System::Windows::Forms::Label();
		Label_Red->Text = "Red Note:";
		Label_Red->AutoSize = true;
		Label_Red->Anchor = System::Windows::Forms::AnchorStyles::Right;
		_Notes_Layout->Controls->Add(Label_Red, 0, 0);

		_Combo_Box_Red = gcnew System::Windows::Forms::ComboBox();
		_Combo_Box_Red->Anchor = System::Windows::Forms::AnchorStyles::Left | System::Windows::Forms::AnchorStyles::Right;
		_Combo_Box_Red->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
		_Combo_Box_Red->SelectedIndexChanged += gcnew System::EventHandler(this, &Form_Settings_MIDI::Combo_Box_Selected_Index_Changed);
		_Notes_Layout->Controls->Add(_Combo_Box_Red, 1, 0);

		_Icon_Red = gcnew System::Windows::Forms::PictureBox();
		_Icon_Red->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
		_Icon_Red->Anchor = System::Windows::Forms::AnchorStyles::None;
		_Icon_Red->Size = System::Drawing::Size(16, 16);
		_Notes_Layout->Controls->Add(_Icon_Red, 2, 0);

		System::Windows::Forms::Label^ Label_Green = gcnew System::Windows::Forms::Label();
		Label_Green->Text = "Green Note:";
		Label_Green->AutoSize = true;
		Label_Green->Anchor = System::Windows::Forms::AnchorStyles::Right;
		_Notes_Layout->Controls->Add(Label_Green, 0, 1);

		_Combo_Box_Green = gcnew System::Windows::Forms::ComboBox();
		_Combo_Box_Green->Anchor = System::Windows::Forms::AnchorStyles::Left | System::Windows::Forms::AnchorStyles::Right;
		_Combo_Box_Green->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
		_Combo_Box_Green->SelectedIndexChanged += gcnew System::EventHandler(this, &Form_Settings_MIDI::Combo_Box_Selected_Index_Changed);
		_Notes_Layout->Controls->Add(_Combo_Box_Green, 1, 1);

		_Icon_Green = gcnew System::Windows::Forms::PictureBox();
		_Icon_Green->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
		_Icon_Green->Anchor = System::Windows::Forms::AnchorStyles::None;
		_Icon_Green->Size = System::Drawing::Size(16, 16);
		_Notes_Layout->Controls->Add(_Icon_Green, 2, 1);

		System::Windows::Forms::Label^ Label_Blue = gcnew System::Windows::Forms::Label();
		Label_Blue->Text = "Blue Note:";
		Label_Blue->AutoSize = true;
		Label_Blue->Anchor = System::Windows::Forms::AnchorStyles::Right;
		_Notes_Layout->Controls->Add(Label_Blue, 0, 2);

		_Combo_Box_Blue = gcnew System::Windows::Forms::ComboBox();
		_Combo_Box_Blue->Anchor = System::Windows::Forms::AnchorStyles::Left | System::Windows::Forms::AnchorStyles::Right;
		_Combo_Box_Blue->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
		_Combo_Box_Blue->SelectedIndexChanged += gcnew System::EventHandler(this, &Form_Settings_MIDI::Combo_Box_Selected_Index_Changed);
		_Notes_Layout->Controls->Add(_Combo_Box_Blue, 1, 2);

		_Icon_Blue = gcnew System::Windows::Forms::PictureBox();
		_Icon_Blue->SizeMode = System::Windows::Forms::PictureBoxSizeMode::CenterImage;
		_Icon_Blue->Anchor = System::Windows::Forms::AnchorStyles::None;
		_Icon_Blue->Size = System::Drawing::Size(16, 16);
		_Notes_Layout->Controls->Add(_Icon_Blue, 2, 2);

		// Add Anti Flicker checkbox
		_Checkbox_Anti_Flicker = gcnew CheckBox();
		_Checkbox_Anti_Flicker->Text = "Enable Anti-Flicker mode for MIDI Export";
		_Checkbox_Anti_Flicker->AutoSize = true;
		_Checkbox_Anti_Flicker->Padding = System::Windows::Forms::Padding(0, 0, 20, 5);
		_Checkbox_Anti_Flicker->Anchor = System::Windows::Forms::AnchorStyles::Bottom;
		_Notes_Layout->Controls->Add(_Checkbox_Anti_Flicker, 0, 3);
		_Notes_Layout->SetColumnSpan(_Checkbox_Anti_Flicker, 3);

		_Group_Box_Notes->Controls->Add(_Notes_Layout);
		_Main_Layout->Controls->Add(_Group_Box_Notes, 0, 0);

		for each (Note_Entry E in _Note_Names)
		{
			_Combo_Box_Red->Items->Add(E.Name);
			_Combo_Box_Green->Items->Add(E.Name);
			_Combo_Box_Blue->Items->Add(E.Name);
		}

		///////////////////////
		// Group Box Octaves //
		///////////////////////
		_Group_Box_Octaves = gcnew System::Windows::Forms::GroupBox();
		_Group_Box_Octaves->Text	= "Define Light Track Octaves";
		_Group_Box_Octaves->Dock	= System::Windows::Forms::DockStyle::Fill;
		_Group_Box_Octaves->Padding = System::Windows::Forms::Padding(10, 15, 10, 10);
		_Group_Box_Octaves->Paint += gcnew PaintEventHandler(this, &Form_Settings_MIDI::GroupBox_Paint);

		_Octaves_Layout = gcnew System::Windows::Forms::TableLayoutPanel();
		_Octaves_Layout->Dock = System::Windows::Forms::DockStyle::Fill;
		_Group_Box_Octaves->Controls->Add(_Octaves_Layout);
		_Main_Layout->Controls->Add(_Group_Box_Octaves, 0, 1);

		System::Windows::Forms::Panel^ Button_Panel = gcnew System::Windows::Forms::Panel();
		Button_Panel->Dock = System::Windows::Forms::DockStyle::Fill;

		_Button_OK = gcnew System::Windows::Forms::Button();
		_Button_OK->Text = "OK";
		_Button_OK->DialogResult = System::Windows::Forms::DialogResult::OK;
		_Button_OK->Click += gcnew System::EventHandler(this, &Form_Settings_MIDI::Button_OK_Click);
		Theme_Manager::Get_Instance()->ApplyThemeToButton(_Button_OK);

		_Button_Cancel = gcnew System::Windows::Forms::Button();
		_Button_Cancel->Text = "Cancel";
		_Button_Cancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
		Theme_Manager::Get_Instance()->ApplyThemeToButton(_Button_Cancel);

		Button_Panel->Controls->Add(_Button_Cancel);
		Button_Panel->Controls->Add(_Button_OK);

		_Button_Cancel->Location = System::Drawing::Point(Button_Panel->Width - 160, 10);
		_Button_OK->Location = System::Drawing::Point(Button_Panel->Width - 80, 10);

		_Main_Layout->Controls->Add(Button_Panel, 0, 2);

		this->Controls->Add(_Main_Layout);
		this->AcceptButton = _Button_OK;
		this->CancelButton = _Button_Cancel;

		this->_Tool_Tip = gcnew ToolTip();
		this->_Tool_Tip->InitialDelay = 200;
		this->_Tool_Tip->ReshowDelay = 100;

		this->Size = System::Drawing::Size(400, 630);
	}

	void Form_Settings_MIDI::Load_Current_Settings()
	{
		Settings^ Current_Settings = Settings::Get_Instance();
	
		// Find and select items by their values
		for each (Note_Entry Note in _Note_Names)
		{
			if (Note.Value == Current_Settings->MIDI_Note_Red) {
				_Combo_Box_Red->SelectedIndex = Array::IndexOf(_Note_Names, Note);
			}

			if (Note.Value == Current_Settings->MIDI_Note_Green) {
				_Combo_Box_Green->SelectedIndex = Array::IndexOf(_Note_Names, Note);
			}

			if (Note.Value == Current_Settings->MIDI_Note_Blue) {
				_Combo_Box_Blue->SelectedIndex = Array::IndexOf(_Note_Names, Note);
			}
		}

		_Checkbox_Anti_Flicker->Checked = Current_Settings->MIDI_Export_Anti_Flicker;

		Update_Status_Icons();
	}

	void Form_Settings_MIDI::Save_Settings()
	{
		Settings^ Current_Settings = Settings::Get_Instance();

		Current_Settings->MIDI_Note_Red		= Find_Note_Index_By_Name((String^)_Combo_Box_Red->SelectedItem);
		Current_Settings->MIDI_Note_Green	= Find_Note_Index_By_Name((String^)_Combo_Box_Green->SelectedItem);
		Current_Settings->MIDI_Note_Blue	= Find_Note_Index_By_Name((String^)_Combo_Box_Blue->SelectedItem);

		Current_Settings->MIDI_Export_Anti_Flicker = _Checkbox_Anti_Flicker->Checked;
	}

	void Form_Settings_MIDI::Update_Status_Icons()
	{
		System::Drawing::Image^ Valid_Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Tick")));
		System::Drawing::Image^ Invalid_Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Error")));
		System::Drawing::Image^ Warning_Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Warning")));

		// Clear existing tooltips
		_Tool_Tip->SetToolTip(_Icon_Red, "");
		_Tool_Tip->SetToolTip(_Icon_Green, "");
		_Tool_Tip->SetToolTip(_Icon_Blue, "");

		// First check if any notes are unselected
		if (_Combo_Box_Red->SelectedIndex == -1 || _Combo_Box_Green->SelectedIndex == -1 || _Combo_Box_Blue->SelectedIndex == -1)
		{
			_Icon_Red->Image = (_Combo_Box_Red->SelectedIndex != -1) ? Valid_Image : Invalid_Image;
			_Icon_Green->Image = (_Combo_Box_Green->SelectedIndex != -1) ? Valid_Image : Invalid_Image;
			_Icon_Blue->Image = (_Combo_Box_Blue->SelectedIndex != -1) ? Valid_Image : Invalid_Image;
			
			return;
		}

		if (!_Checkbox_Anti_Flicker->Checked) {
			// If anti-flicker is disabled, just show valid icons
			_Icon_Red->Image = Valid_Image;
			_Icon_Green->Image = Valid_Image;
			_Icon_Blue->Image = Valid_Image;

			return;
		}
		
		// Get note values
		int RedValue	= _Note_Names[_Combo_Box_Red->SelectedIndex].Value;
		int GreenValue	= _Note_Names[_Combo_Box_Green->SelectedIndex].Value;
		int BlueValue	= _Note_Names[_Combo_Box_Blue->SelectedIndex].Value;

		// Red note checks
		if (Math::Abs(RedValue - GreenValue) == 1 || Math::Abs(RedValue - BlueValue) == 1) {
			_Icon_Red->Image = Warning_Image;
			_Tool_Tip->SetToolTip(_Icon_Red, "When using the Anti-Flicker option for the MIDI export,\nthe color's note should be two steps away.");
		}
		else {
			_Icon_Red->Image = Valid_Image;
		}

		// Green note checks
		if (Math::Abs(GreenValue - RedValue) == 1 || Math::Abs(GreenValue - BlueValue) == 1) {
			_Icon_Green->Image = Warning_Image;
			_Tool_Tip->SetToolTip(_Icon_Green, "When using the Anti-Flicker option for the MIDI export,\nthe color's note should be two steps away.");
		}
		else {
			_Icon_Green->Image = Valid_Image;
		}

		// Blue note checks
		if (Math::Abs(BlueValue - RedValue) == 1 || Math::Abs(BlueValue - GreenValue) == 1) {
			_Icon_Blue->Image = Warning_Image;
			_Tool_Tip->SetToolTip(_Icon_Blue, "When using the Anti-Flicker option for the MIDI export,\nthe color's note should be two steps away.");
		}
		else {
			_Icon_Blue->Image = Valid_Image;
		}
	}

	int Form_Settings_MIDI::Find_Note_Index_By_Value(int value)
	{
		for (int i = 0; i < _Note_Names->Length; i++)
		{
			if (_Note_Names[i].Value == value) {
				return i;
			}
		}

		return -1;
	}

	int Form_Settings_MIDI::Find_Note_Index_By_Name(String^ name)
	{
		for (int i = 0; i < _Note_Names->Length; i++)
		{
			if (_Note_Names[i].Name == name) {
				return i;
			}
		}

		return -1;
	}

	void Form_Settings_MIDI::Combo_Box_Selected_Index_Changed(System::Object^ sender, System::EventArgs^ e)
	{
		ComboBox^ Current_Combo_Box = safe_cast<ComboBox^>(sender);

		if (Current_Combo_Box->SelectedIndex == -1) {
			Update_Status_Icons();
			return;
		}

		int NoteArrayIndex = Find_Note_Index_By_Name((String^)Current_Combo_Box->SelectedItem);
		
		if (NoteArrayIndex == -1) {
			return;
		}

		Note_Entry SelectedEntry = _Note_Names[NoteArrayIndex];

		array<ComboBox^>^ All_Combo_Boxes = { _Combo_Box_Red, _Combo_Box_Green, _Combo_Box_Blue };

		for each (ComboBox^ Other_Combo_Box in All_Combo_Boxes)
		{
			NoteArrayIndex = Find_Note_Index_By_Name((String^)Other_Combo_Box->SelectedItem);
			if (NoteArrayIndex == -1) {
				continue;
			}

			int OtherValue = _Note_Names[NoteArrayIndex].Value;

			if (Other_Combo_Box != Current_Combo_Box && Other_Combo_Box->SelectedIndex != -1 && OtherValue == SelectedEntry.Value)
			{
				Other_Combo_Box->SelectedIndex = -1;
			}
		}

		Update_Status_Icons();
	}

	void Form_Settings_MIDI::Button_OK_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (_Combo_Box_Red->SelectedIndex == -1 ||
			_Combo_Box_Green->SelectedIndex == -1 ||
			_Combo_Box_Blue->SelectedIndex == -1) {
			MessageBox::Show("Please assign a unique note to each color.",
				"Validation Error",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning);
			this->DialogResult = System::Windows::Forms::DialogResult::None;
			return;
		}

		//for each(DataGridViewRow ^ Row in _Grid_Octaves->Rows) {
		//	if (String::IsNullOrWhiteSpace(Row->Cells[0]->Value->ToString())) {
		//		MessageBox::Show("Please provide names for all octave entries.",
		//			"Validation Error",
		//			MessageBoxButtons::OK,
		//			MessageBoxIcon::Warning);
		//		this->DialogResult = System::Windows::Forms::DialogResult::None;
		//		return;
		//	}
		//}

		Save_Settings();
		Save_Octave_Entries();

		Settings^ Current_Settings = Settings::Get_Instance();
		Current_Settings->Save_To_File();

		On_Settings_Accepted();

		this->Close();
	}

	void Form_Settings_MIDI::Initialize_Octaves_Section() {
		_Octaves_Layout->ColumnCount = 1;
		_Octaves_Layout->RowCount = 2;
		_Octaves_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 51));
		_Octaves_Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100));

		Panel^ buttonPanel = gcnew Panel();
		buttonPanel->Dock = DockStyle::Fill;

		// Set fixed size for all buttons to accommodate 16x16 icon plus padding
		int buttonWidth		= 33;	// 16px icon + 8px padding on each side
		int buttonHeight	= 33;	// 16px icon + 8px padding on top and bottom + 1px extra
		int buttonSpacing	= 8;	// Space between buttons

		// Calculate total width needed for all buttons
		int totalButtonsWidth = (buttonWidth * 4) + (buttonSpacing * 3); // 4 buttons, 3 gaps
		// Calculate starting X to center the buttons in the panel
		int startX = (buttonPanel->Width - totalButtonsWidth) / 2;

		// Center buttons vertically in the 41px high panel (excluding the 10px gap)
		int startY = (41 - buttonHeight) / 2;

		_Button_Add_Octave = gcnew Button();
		_Button_Add_Octave->Size = System::Drawing::Size(buttonWidth, buttonHeight);
		_Button_Add_Octave->Location = Point(startX, startY);
		_Button_Add_Octave->Click += gcnew EventHandler(this, &Form_Settings_MIDI::Add_Octave_Entry_Click);
		Theme_Manager::Get_Instance()->ApplyThemeToButton(_Button_Add_Octave);
		_Button_Add_Octave->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Add")));

		_Button_Remove = gcnew Button();
		_Button_Remove->Size = System::Drawing::Size(buttonWidth, buttonHeight);
		_Button_Remove->Location = Point(startX + buttonWidth + buttonSpacing, startY);
		_Button_Remove->Click += gcnew EventHandler(this, &Form_Settings_MIDI::Remove_Entry_Click);
		_Button_Remove->Enabled = false;
		Theme_Manager::Get_Instance()->ApplyThemeToButton(_Button_Remove);
		_Button_Remove->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Delete")));

		_Button_Move_Up = gcnew Button();
		_Button_Move_Up->Size = System::Drawing::Size(buttonWidth, buttonHeight);
		_Button_Move_Up->Location = Point(startX + (buttonWidth + buttonSpacing) * 2, startY);
		_Button_Move_Up->Click += gcnew EventHandler(this, &Form_Settings_MIDI::Move_Up_Click);
		_Button_Move_Up->Enabled = false;
		Theme_Manager::Get_Instance()->ApplyThemeToButton(_Button_Move_Up);
		_Button_Move_Up->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Arrow_Up")));

		_Button_Move_Down = gcnew Button();
		_Button_Move_Down->Size = System::Drawing::Size(buttonWidth, buttonHeight);
		_Button_Move_Down->Location = Point(startX + (buttonWidth + buttonSpacing) * 3, startY);
		_Button_Move_Down->Click += gcnew EventHandler(this, &Form_Settings_MIDI::Move_Down_Click);
		_Button_Move_Down->Enabled = false;
		Theme_Manager::Get_Instance()->ApplyThemeToButton(_Button_Move_Down);
		_Button_Move_Down->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Arrow_Down")));
		
		
		buttonPanel->Controls->Add(_Button_Add_Octave);
		buttonPanel->Controls->Add(_Button_Remove);
		buttonPanel->Controls->Add(_Button_Move_Up);
		buttonPanel->Controls->Add(_Button_Move_Down);

		_Octaves_Layout->Controls->Add(buttonPanel, 0, 0);

		// Initialize DataGridView
		_Grid_Octaves = gcnew Control_DataGrid();
		_Grid_Octaves->Dock = DockStyle::Fill;
		_Grid_Octaves->AllowUserToAddRows = false;
		_Grid_Octaves->AllowUserToDeleteRows = false;
		_Grid_Octaves->SelectionMode = DataGridViewSelectionMode::FullRowSelect;
		_Grid_Octaves->MultiSelect = false;
		_Grid_Octaves->RowHeadersVisible = false;
		_Grid_Octaves->AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode::Fill;
		_Grid_Octaves->AllowUserToResizeRows = false;
		_Grid_Octaves->RowTemplate->Height = 24;  // Set a fixed height for all rows
		_Grid_Octaves->ColumnHeadersHeight = 32;
		_Grid_Octaves->ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode::DisableResizing;
		_Grid_Octaves->EditMode = DataGridViewEditMode::EditOnEnter;
		_Grid_Octaves->AllowUserToOrderColumns = false;
		_Grid_Octaves->AllowUserToResizeColumns = false;

		// Name column
		_Column_Name = gcnew DataGridViewTextBoxColumn();
		_Column_Name->Name				= "Light Track Name";
		_Column_Name->HeaderText		= "Light Track Name";
		_Column_Name->DataPropertyName	= "Light Track Name";
		_Column_Name->FillWeight		= 70;
		_Column_Name->SortMode = DataGridViewColumnSortMode::NotSortable;

		// Octave column with ComboBox
		_Column_Octave = gcnew DataGridViewComboBoxColumn();
		_Column_Octave->Name				= "Octave";
		_Column_Octave->HeaderText			= "Octave";
		_Column_Octave->DataPropertyName	= "Octave";
		_Column_Octave->FillWeight			= 30;
		_Column_Octave->SortMode = DataGridViewColumnSortMode::NotSortable;

		// Add octave range options
		for each (int octave in VALID_OCTAVES) {
			_Column_Octave->Items->Add(octave.ToString());
		}

		_Grid_Octaves->Columns->Add(_Column_Name);
		_Grid_Octaves->Columns->Add(_Column_Octave);

		_Grid_Octaves->SelectionChanged += gcnew EventHandler(this, &Form_Settings_MIDI::Grid_Octaves_Selection_Changed);
		_Grid_Octaves->CellValidating += gcnew DataGridViewCellValidatingEventHandler(this, &Form_Settings_MIDI::Grid_Octaves_Cell_Validating);

		Theme_Manager::Get_Instance()->ApplyThemeToDataGridView(_Grid_Octaves);

		_Octaves_Layout->Controls->Add(_Grid_Octaves, 0, 1);
	}

	void Form_Settings_MIDI::Add_Octave_Entry_Click(System::Object^ sender, System::EventArgs^ e)
	{
		int rowIndex = _Grid_Octaves->Rows->Add("New Entry", "4");
		_Grid_Octaves->Rows[rowIndex]->Selected = true;
		_Grid_Octaves->CurrentCell = _Grid_Octaves->Rows[rowIndex]->Cells[0];
		_Grid_Octaves->BeginEdit(true);
	}

	void Form_Settings_MIDI::Remove_Entry_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (_Grid_Octaves->SelectedRows->Count > 0) {
			_Grid_Octaves->Rows->RemoveAt(_Grid_Octaves->SelectedRows[0]->Index);
			Update_Button_States();
		}
	}

	void Form_Settings_MIDI::Move_Up_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (_Grid_Octaves->SelectedRows->Count > 0) {
			int selectedIndex = _Grid_Octaves->SelectedRows[0]->Index;
			if (selectedIndex > 0) {
				DataGridViewRow^ row = _Grid_Octaves->Rows[selectedIndex];
				array<Object^>^ values = gcnew array<Object^>(row->Cells->Count);
				for (int i = 0; i < row->Cells->Count; i++) {
					values[i] = row->Cells[i]->Value;
				}

				_Grid_Octaves->Rows->RemoveAt(selectedIndex);
				_Grid_Octaves->Rows->Insert(selectedIndex - 1, values);
				_Grid_Octaves->Rows[selectedIndex - 1]->Selected = true;
			}
		}
	}

	void Form_Settings_MIDI::Move_Down_Click(System::Object^ sender, System::EventArgs^ e) {
		if (_Grid_Octaves->SelectedRows->Count > 0) {
			int selectedIndex = _Grid_Octaves->SelectedRows[0]->Index;
			if (selectedIndex < _Grid_Octaves->RowCount - 1) {
				DataGridViewRow^ row = _Grid_Octaves->Rows[selectedIndex];
				array<Object^>^ values = gcnew array<Object^>(row->Cells->Count);
				for (int i = 0; i < row->Cells->Count; i++) {
					values[i] = row->Cells[i]->Value;
				}

				_Grid_Octaves->Rows->RemoveAt(selectedIndex);
				_Grid_Octaves->Rows->Insert(selectedIndex + 1, values);
				_Grid_Octaves->Rows[selectedIndex + 1]->Selected = true;
			}
		}
	}

	void Form_Settings_MIDI::Load_Octave_Entries() {
		Settings^ settings = Settings::Get_Instance();
		_Grid_Octaves->Rows->Clear();

		for each (Settings::Octave_Entry ^ entry in settings->Octave_Entries) {
			_Grid_Octaves->Rows->Add(entry->Name, entry->Octave_Number.ToString());
		}
	}

	void Form_Settings_MIDI::Save_Octave_Entries() {
		Settings^ settings = Settings::Get_Instance();
		settings->Octave_Entries->Clear();

		for each (DataGridViewRow ^ row in _Grid_Octaves->Rows) {
			String^ name = row->Cells[0]->Value->ToString()->Trim();
			if (!String::IsNullOrWhiteSpace(name)) {
				int octave = Int32::Parse(row->Cells[1]->Value->ToString());
				settings->Octave_Entries->Add(gcnew Settings::Octave_Entry(name, octave));
			}
		}
	}

	void Form_Settings_MIDI::Update_Button_States()
	{
		Grid_Octaves_Selection_Changed(nullptr, nullptr);
	}

	void Form_Settings_MIDI::Grid_Octaves_Selection_Changed(System::Object^ sender, System::EventArgs^ e)
	{
		bool hasSelection = _Grid_Octaves->SelectedRows->Count > 0;
		int selectedIndex = hasSelection ? _Grid_Octaves->SelectedRows[0]->Index : -1;

		_Button_Remove->Enabled = hasSelection;
		_Button_Move_Up->Enabled = hasSelection && selectedIndex > 0;
		_Button_Move_Down->Enabled = hasSelection && selectedIndex < _Grid_Octaves->RowCount - 1;
	}

	void Form_Settings_MIDI::Grid_Octaves_Cell_Validating(System::Object^ sender, DataGridViewCellValidatingEventArgs^ e)
	{
		// Name validation
		if (e->ColumnIndex == _Column_Name->Index) {
			String^ name = e->FormattedValue->ToString();
			if (String::IsNullOrWhiteSpace(name)) {
				_Grid_Octaves->Rows[e->RowIndex]->ErrorText = "Name is required";
				e->Cancel = true;
			}
			else {
				_Grid_Octaves->Rows[e->RowIndex]->ErrorText = "";
			}
		}
		// Octave validation
		//else if (e->ColumnIndex == _Column_Octave->Index) {
		//	String^ newOctave = e->FormattedValue->ToString();

		//	// Check all other rows for duplicate octave
		//	for (int i = 0; i < _Grid_Octaves->RowCount; i++) {
		//		if (i != e->RowIndex &&
		//			_Grid_Octaves->Rows[i]->Cells[_Column_Octave->Index]->Value->ToString() == newOctave) {
		//			_Grid_Octaves->Rows[e->RowIndex]->ErrorText = "Octave already in use";
		//			MessageBox::Show("This octave is already assigned to another track.",
		//				"Duplicate Octave",
		//				MessageBoxButtons::OK,
		//				MessageBoxIcon::Warning);
		//			e->Cancel = true;
		//			return;
		//		}
		//	}
		//	_Grid_Octaves->Rows[e->RowIndex]->ErrorText = "";
		//}
	}

	void Form_Settings_MIDI::GroupBox_Paint(Object^ sender, PaintEventArgs^ e)
	{
		GroupBox^ box = safe_cast<GroupBox^>(sender);
		Theme_Manager^ theme = Theme_Manager::Get_Instance();

		Graphics^ g = e->Graphics;
		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Calculate text size and positions
		Drawing::Font^ titleFont = gcnew Drawing::Font("Segoe UI Semibold", 9.5f);
		SizeF textSize = g->MeasureString(box->Text, titleFont);

		// Define header rect
		System::Drawing::Rectangle headerRect = System::Drawing::Rectangle(0, 0, box->Width, 28);

		// Draw header background with gradient
		Drawing2D::LinearGradientBrush^ headerBrush = gcnew Drawing2D::LinearGradientBrush(
			headerRect,
			theme->BackgroundAlt,
			theme->Background,
			Drawing2D::LinearGradientMode::Vertical);

		g->FillRectangle(headerBrush, headerRect);

		// Draw title
		g->DrawString(box->Text, titleFont, gcnew SolidBrush(theme->ForegroundText), Point(12, 6));

		// Draw border
		Pen^ borderPen = gcnew Pen(theme->BorderStrong);
		g->DrawRectangle(borderPen, 0, 0, box->Width - 1, box->Height - 1);

		// Draw header bottom line with accent
		Pen^ accentPen = gcnew Pen(theme->AccentPrimary, 1);
		g->DrawLine(accentPen, 0, headerRect.Bottom, box->Width, headerRect.Bottom);

		// Clean up
		delete headerBrush;
		delete borderPen;
		delete accentPen;
		delete titleFont;
	}
}