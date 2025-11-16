#include "Control_TimeOffset_NumericUpDown.h"

namespace MIDILightDrawer
{
	Control_TimeOffset_NumericUpDown::Control_TimeOffset_NumericUpDown()
	{
		_Total_Milliseconds = 0.0;
		_Minimum_ms = -60000.0;
		_Maximum_ms = 60000.0;
		_Is_Editing = false;

		Initialize_Components();
		Update_Display();
	}

	void Control_TimeOffset_NumericUpDown::Initialize_Components()
	{
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint, true);

		this->Size = System::Drawing::Size(200, DISPLAY_HEIGHT);
		this->BackColor = Color::Transparent;

		Theme_Manager^ theme = Theme_Manager::Get_Instance();

		// Create main layout
		TableLayoutPanel^ layout = gcnew TableLayoutPanel();
		layout->Dock = DockStyle::Fill;
		layout->RowCount = 1;
		layout->ColumnCount = 3;
		layout->Margin = System::Windows::Forms::Padding(0);
		layout->Padding = System::Windows::Forms::Padding(0);

		layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, BUTTON_WIDTH));
		layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));
		layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, BUTTON_WIDTH));

		layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));

		// Decrement button
		_Button_Decrement = gcnew Button();
		_Button_Decrement->Text = "-";
		_Button_Decrement->Dock = DockStyle::Fill;
		_Button_Decrement->FlatStyle = FlatStyle::Flat;
		_Button_Decrement->FlatAppearance->BorderColor = theme->AccentPrimary;
		_Button_Decrement->BackColor = theme->BackgroundAlt;
		_Button_Decrement->ForeColor = Color::White;
		_Button_Decrement->Font = gcnew System::Drawing::Font("Segoe UI", 10.0f, FontStyle::Bold);
		_Button_Decrement->Margin = System::Windows::Forms::Padding(0);
		_Button_Decrement->Click += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_Decrement_Click);

		// Increment button
		_Button_Increment = gcnew Button();
		_Button_Increment->Text = "+";
		_Button_Increment->Dock = DockStyle::Fill;
		_Button_Increment->FlatStyle = FlatStyle::Flat;
		_Button_Increment->FlatAppearance->BorderColor = theme->AccentPrimary;
		_Button_Increment->BackColor = theme->BackgroundAlt;
		_Button_Increment->ForeColor = Color::White;
		_Button_Increment->Font = gcnew System::Drawing::Font("Segoe UI", 10.0f, FontStyle::Bold);
		_Button_Increment->Margin = System::Windows::Forms::Padding(0);
		_Button_Increment->Click += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_Increment_Click);

		// Display panel
		_Display_Panel = gcnew Panel();
		_Display_Panel->Dock = DockStyle::Fill;
		_Display_Panel->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
		_Display_Panel->BackColor = theme->BackgroundAlt;
		_Display_Panel->Margin = System::Windows::Forms::Padding(1, 0, 1, 0);
		_Display_Panel->Cursor = Cursors::IBeam;
		_Display_Panel->Click += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_Display_Click);

		// Display label
		_Label_Display = gcnew Label();
		_Label_Display->Dock = DockStyle::Fill;
		_Label_Display->TextAlign = ContentAlignment::MiddleCenter;
		_Label_Display->ForeColor = Color::White;
		_Label_Display->BackColor = Color::Transparent;
		_Label_Display->Font = gcnew System::Drawing::Font("Consolas", 9.0f);
		_Label_Display->Padding = System::Windows::Forms::Padding(0, 3, 0, 0);
		_Label_Display->Cursor = Cursors::IBeam;
		_Label_Display->Click += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_Display_Click);

		// TextBox for editing (initially hidden)
		_TextBox_Input = gcnew TextBox();
		_TextBox_Input->Dock = DockStyle::Fill;
		_TextBox_Input->TextAlign = HorizontalAlignment::Center;
		_TextBox_Input->BackColor = theme->BackgroundAlt;
		_TextBox_Input->ForeColor = Color::White;
		_TextBox_Input->Font = gcnew System::Drawing::Font("Consolas", 9.0f);
		_TextBox_Input->BorderStyle = System::Windows::Forms::BorderStyle::None;
		_TextBox_Input->Margin = System::Windows::Forms::Padding(2, 5, 2, 5);
		_TextBox_Input->Visible = false;
		_TextBox_Input->KeyPress += gcnew KeyPressEventHandler(this, &Control_TimeOffset_NumericUpDown::On_TextBox_KeyPress);
		_TextBox_Input->KeyDown += gcnew KeyEventHandler(this, &Control_TimeOffset_NumericUpDown::On_TextBox_KeyDown);
		_TextBox_Input->LostFocus += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_TextBox_LostFocus);

		// Add controls to panel
		_Display_Panel->Controls->Add(_Label_Display);
		_Display_Panel->Controls->Add(_TextBox_Input);

		// Add to layout
		layout->Controls->Add(_Button_Decrement, 0, 0);
		layout->Controls->Add(_Display_Panel, 1, 0);
		layout->Controls->Add(_Button_Increment, 2, 0);

		this->Controls->Add(layout);
	}

	void Control_TimeOffset_NumericUpDown::Value_ms::set(double value)
	{
		// Clamp to min/max
		if (value < _Minimum_ms) value = _Minimum_ms;
		if (value > _Maximum_ms) value = _Maximum_ms;

		if (Math::Abs(_Total_Milliseconds - value) < 0.001) {
			return; // No change
		}

		_Total_Milliseconds = value;
		Update_Display();

		Value_Changed(this, EventArgs::Empty);
	}

	void Control_TimeOffset_NumericUpDown::Update_Display()
	{
		_Label_Display->Text = Format_Time(_Total_Milliseconds);
	}

	String^ Control_TimeOffset_NumericUpDown::Format_Time(double milliseconds)
	{
		bool is_negative = milliseconds < 0;
		double abs_ms = Math::Abs(milliseconds);

		// Always show sign: + for positive/zero, - for negative
		String^ sign = is_negative ? "-" : "+";

		// Show value in milliseconds
		String^ formatted = String::Format("{0}{1} ms", sign, (int)abs_ms);

		// If >= 1000ms, also show in seconds
		if (abs_ms >= 1000.0) {
			double seconds = abs_ms / 1000.0;
			formatted = String::Format("{0}{1} ms ({2:F2} s)", sign, (int)abs_ms, seconds);
		}

		return formatted;
	}

	double Control_TimeOffset_NumericUpDown::Parse_Time_Input(String^ input)
	{
		// Remove "ms" and "s" suffixes, and anything in parentheses
		input = input->Trim();

		// Remove content in parentheses (e.g., "(5.25 s)")
		int paren_start = input->IndexOf('(');
		if (paren_start >= 0) {
			input = input->Substring(0, paren_start)->Trim();
		}

		input = input->Replace("ms", "")->Replace("s", "")->Trim();

		bool is_negative = input->StartsWith("-");
		if (is_negative) {
			input = input->Substring(1)->Trim();
		}
		else if (input->StartsWith("+")) {
			// Remove leading + sign
			input = input->Substring(1)->Trim();
		}

		// Parse as integer (just milliseconds now)
		int milliseconds = 0;
		if (!Int32::TryParse(input, milliseconds)) {
			return _Total_Milliseconds; // Return current value if parsing fails
		}

		double result = (double)milliseconds;

		if (is_negative) {
			result = -result;
		}

		// Clamp to range
		if (result < _Minimum_ms) result = _Minimum_ms;
		if (result > _Maximum_ms) result = _Maximum_ms;

		return result;
	}

	void Control_TimeOffset_NumericUpDown::On_Display_Click(Object^ sender, EventArgs^ e)
	{
		Enter_Edit_Mode();
	}

	void Control_TimeOffset_NumericUpDown::On_TextBox_KeyPress(Object^ sender, KeyPressEventArgs^ e)
	{
		// Only allow digits, backspace, and plus/minus signs
		if (!Char::IsDigit(e->KeyChar) &&
			e->KeyChar != (Char)Keys::Back &&
			e->KeyChar != '-' &&
			e->KeyChar != '+') {
			e->Handled = true;
		}

		// Only allow plus or minus at the beginning
		if ((e->KeyChar == '-' || e->KeyChar == '+') && _TextBox_Input->SelectionStart != 0) {
			e->Handled = true;
		}
	}

	void Control_TimeOffset_NumericUpDown::On_TextBox_KeyDown(Object^ sender, KeyEventArgs^ e)
	{
		if (e->KeyCode == Keys::Enter || e->KeyCode == Keys::Return) {
			// Parse and apply the value
			Exit_Edit_Mode(true);

			// Shift focus away from textbox to prevent it from staying active
			if (this->Parent != nullptr) {
				this->Parent->Focus();
			}

			e->Handled = true;
			e->SuppressKeyPress = true;
		}
		else if (e->KeyCode == Keys::Escape) {
			// Cancel editing
			Exit_Edit_Mode(false);

			// Shift focus away
			if (this->Parent != nullptr) {
				this->Parent->Focus();
			}

			e->Handled = true;
		}
	}

	void Control_TimeOffset_NumericUpDown::On_TextBox_LostFocus(Object^ sender, EventArgs^ e)
	{
		Exit_Edit_Mode(true);
	}

	void Control_TimeOffset_NumericUpDown::On_Increment_Click(Object^ sender, EventArgs^ e)
	{
		Value_ms = _Total_Milliseconds + STEP_SIZE_MS;
	}

	void Control_TimeOffset_NumericUpDown::On_Decrement_Click(Object^ sender, EventArgs^ e)
	{
		Value_ms = _Total_Milliseconds - STEP_SIZE_MS;
	}

	void Control_TimeOffset_NumericUpDown::Enter_Edit_Mode()
	{
		if (_Is_Editing) return;

		_Is_Editing = true;

		// Prepare textbox with current value in milliseconds
		bool is_negative = _Total_Milliseconds < 0;
		double abs_ms = Math::Abs(_Total_Milliseconds);

		// Always show sign: + for positive/zero, - for negative
		String^ sign = is_negative ? "-" : "+";
		_TextBox_Input->Text = String::Format("{0}{1}", sign, (int)abs_ms);

		_Label_Display->Visible = false;
		_TextBox_Input->Visible = true;
		_TextBox_Input->Focus();
		_TextBox_Input->SelectAll();
	}

	void Control_TimeOffset_NumericUpDown::Exit_Edit_Mode(bool apply_changes)
	{
		if (!_Is_Editing) return;

		_Is_Editing = false;

		if (apply_changes) {
			double new_value = Parse_Time_Input(_TextBox_Input->Text);
			Value_ms = new_value;
		}

		_TextBox_Input->Visible = false;
		_Label_Display->Visible = true;
	}
}
