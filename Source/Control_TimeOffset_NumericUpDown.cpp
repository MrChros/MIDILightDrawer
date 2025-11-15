#include "Control_TimeOffset_NumericUpDown.h"
#include "Theme_Manager.h"

namespace MIDILightDrawer
{
	Control_TimeOffset_NumericUpDown::Control_TimeOffset_NumericUpDown()
	{
		_Total_Milliseconds = 0.0;
		_Updating_From_Total = false;
		_Background_Color = Theme_Manager::Get_Instance()->BackgroundAlt;
		_Foreground_Color = Color::White;
		_Border_Color = Theme_Manager::Get_Instance()->AccentPrimary;

		Initialize_Components();
		Apply_Theme();
	}

	void Control_TimeOffset_NumericUpDown::Initialize_Components()
	{
		// Set control size
		//this->Size = Drawing::Size(140, 26);

		// Create TableLayoutPanel for layout
		TableLayoutPanel^ Layout = gcnew TableLayoutPanel();
		Layout->Dock = DockStyle::Fill;
		Layout->ColumnCount = 2;
		Layout->RowCount = 1;
		Layout->Padding = System::Windows::Forms::Padding(0);
		Layout->Margin = System::Windows::Forms::Padding(0);
		Layout->BackColor = Color::Transparent;

		// Column styles: [Seconds] [:] [Milliseconds] [ms]
		//Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 45.0f));  // Seconds
		//Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 12.0f));  // Colon
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 70.0f));  // Milliseconds
		Layout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));  // "ms" label

		// Row style
		Layout->RowStyles->Add(gcnew RowStyle(SizeType::Percent, 100.0f));

		// Create Seconds NumericUpDown
		_NumericUpDown_Seconds = gcnew NumericUpDown();
		_NumericUpDown_Seconds->Minimum = -999;
		_NumericUpDown_Seconds->Maximum = 999;
		_NumericUpDown_Seconds->Value = 0;
		_NumericUpDown_Seconds->DecimalPlaces = 0;
		_NumericUpDown_Seconds->Dock = DockStyle::Fill;
		_NumericUpDown_Seconds->TextAlign = HorizontalAlignment::Center;
		_NumericUpDown_Seconds->BorderStyle = System::Windows::Forms::BorderStyle::None;
		_NumericUpDown_Seconds->Margin = System::Windows::Forms::Padding(2, 3, 0, 3);
		_NumericUpDown_Seconds->ValueChanged += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_Component_ValueChanged);

		// Create separator label ":"
		_Label_Separator = gcnew Label();
		_Label_Separator->Text = ":";
		_Label_Separator->Dock = DockStyle::Fill;
		_Label_Separator->TextAlign = ContentAlignment::MiddleCenter;
		_Label_Separator->Font = gcnew Drawing::Font("Segoe UI", 9.0f, FontStyle::Bold);
		_Label_Separator->Margin = System::Windows::Forms::Padding(0);

		// Create Milliseconds NumericUpDown
		_NumericUpDown_Milliseconds = gcnew NumericUpDown();
		_NumericUpDown_Milliseconds->Minimum = 0;
		_NumericUpDown_Milliseconds->Maximum = 999;
		_NumericUpDown_Milliseconds->Value = 0;
		_NumericUpDown_Milliseconds->DecimalPlaces = 0;
		_NumericUpDown_Milliseconds->Dock = DockStyle::Fill;
		_NumericUpDown_Milliseconds->TextAlign = HorizontalAlignment::Center;
		_NumericUpDown_Milliseconds->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
		_NumericUpDown_Milliseconds->Margin = System::Windows::Forms::Padding(0, 3, 2, 3);
		_NumericUpDown_Milliseconds->ValueChanged += gcnew EventHandler(this, &Control_TimeOffset_NumericUpDown::On_Component_ValueChanged);

		// Create "ms" unit label
		_Label_Unit = gcnew Label();
		_Label_Unit->Text = "ms";
		_Label_Unit->Dock = DockStyle::Fill;
		_Label_Unit->TextAlign = ContentAlignment::MiddleLeft;
		_Label_Unit->Font = gcnew Drawing::Font("Segoe UI", 8.0f);
		_Label_Unit->Margin = System::Windows::Forms::Padding(2, 0, 0, 2);

		// Add controls to layout
		//Layout->Controls->Add(_NumericUpDown_Seconds, 0, 0);
		//Layout->Controls->Add(_Label_Separator, 1, 0);
		//Layout->Controls->Add(_NumericUpDown_Milliseconds, 2, 0);
		//Layout->Controls->Add(_Label_Unit, 3, 0);

		Layout->Controls->Add(_NumericUpDown_Milliseconds, 0, 0);
		Layout->Controls->Add(_Label_Unit, 1, 0);

		this->Controls->Add(Layout);
	}

	void Control_TimeOffset_NumericUpDown::Apply_Theme()
	{
		this->BackColor = Color::Transparent;

		_NumericUpDown_Seconds->BackColor = _Background_Color;
		_NumericUpDown_Seconds->ForeColor = _Foreground_Color;

		_NumericUpDown_Milliseconds->BackColor = _Background_Color;
		_NumericUpDown_Milliseconds->ForeColor = _Foreground_Color;

		_Label_Separator->BackColor = Color::Transparent;;
		_Label_Separator->ForeColor = _Foreground_Color;

		_Label_Unit->BackColor = Color::Transparent;
		_Label_Unit->ForeColor = _Foreground_Color;
	}

	void Control_TimeOffset_NumericUpDown::Value_ms::set(double value)
	{
		if (Math::Abs(_Total_Milliseconds - value) < 0.001) {
			return; // No change
		}

		_Total_Milliseconds = value;
		Update_From_Total_Milliseconds();
		ValueChanged(this, EventArgs::Empty);
	}

	double Control_TimeOffset_NumericUpDown::Minimum_ms::get()
	{
		return (double)_NumericUpDown_Seconds->Minimum * 1000.0;
	}

	void Control_TimeOffset_NumericUpDown::Minimum_ms::set(double value)
	{
		int Seconds_Min = (int)(value / 1000.0);
		_NumericUpDown_Seconds->Minimum = Seconds_Min;
	}

	double Control_TimeOffset_NumericUpDown::Maximum_ms::get()
	{
		return (double)_NumericUpDown_Seconds->Maximum * 1000.0 + 999.0;
	}

	void Control_TimeOffset_NumericUpDown::Maximum_ms::set(double value)
	{
		int Seconds_Max = (int)(value / 1000.0);
		_NumericUpDown_Seconds->Maximum = Seconds_Max;
	}

	void Control_TimeOffset_NumericUpDown::BackColor_Custom::set(Color value)
	{
		_Background_Color = value;
		Apply_Theme();
	}

	void Control_TimeOffset_NumericUpDown::ForeColor_Custom::set(Color value)
	{
		_Foreground_Color = value;
		Apply_Theme();
	}

	void Control_TimeOffset_NumericUpDown::Update_From_Total_Milliseconds()
	{
		_Updating_From_Total = true;

		// Calculate seconds and milliseconds
		bool Is_Negative = _Total_Milliseconds < 0;
		double Abs_Milliseconds = Math::Abs(_Total_Milliseconds);

		int Seconds = (int)(Abs_Milliseconds / 1000.0);
		int Milliseconds = (int)(Abs_Milliseconds) % 1000;

		// Apply sign to seconds
		if (Is_Negative) {
			Seconds = -Seconds;
		}

		// Update controls
		_NumericUpDown_Seconds->Value = Seconds;
		_NumericUpDown_Milliseconds->Value = Milliseconds;

		_Updating_From_Total = false;
	}

	void Control_TimeOffset_NumericUpDown::Update_Total_From_Components()
	{
		if (_Updating_From_Total) {
			return; // Avoid recursion
		}

		// Calculate total milliseconds
		int Seconds = (int)_NumericUpDown_Seconds->Value;
		int Milliseconds = (int)_NumericUpDown_Milliseconds->Value;

		// Handle negative seconds correctly
		if (Seconds < 0) {
			_Total_Milliseconds = (double)Seconds * 1000.0 - (double)Milliseconds;
		}
		else {
			_Total_Milliseconds = (double)Seconds * 1000.0 + (double)Milliseconds;
		}

		// Raise event
		ValueChanged(this, EventArgs::Empty);
	}

	void Control_TimeOffset_NumericUpDown::On_Component_ValueChanged(Object^ sender, EventArgs^ e)
	{
		Update_Total_From_Components();
	}
}