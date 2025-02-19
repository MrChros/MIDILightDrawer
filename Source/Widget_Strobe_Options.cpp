#include "Widget_Strobe_Options.h"

namespace MIDILightDrawer
{
	Widget_Strobe_Options::Widget_Strobe_Options(Control_ColorPicker^ color_picker)
	{
		this->_Color_Picker = color_picker;
		Initialize_Component();
	}

	Widget_Strobe_Options::~Widget_Strobe_Options()
	{
		if (_Components) {
			delete _Components;
		}
	}

	void Widget_Strobe_Options::Initialize_Component(void)
	{
		this->_Components = gcnew System::ComponentModel::Container();

		// Create main GroupBox
		this->_GroupBox = gcnew Control_GroupBox();
		this->_GroupBox->Text = "Strobe Options";
		this->_GroupBox->Dock = DockStyle::Fill;
		this->_GroupBox->Padding = System::Windows::Forms::Padding(10, 15, 10, 10);

		// Create layout for GroupBox contents
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->RowCount = 2;
		Table_Layout_Main->ColumnCount = 2;
		Table_Layout_Main->Dock = DockStyle::Fill;
		Table_Layout_Main->Padding = System::Windows::Forms::Padding(5);

		// Configure row styles
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 45));	// Row for combo and color button
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 80));	// Row for color presets

		// Configure column styles
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 250));  // Combo box column
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));  // Combo box column



		array<String^>^ Lines_First_Quantization = TimeSignatures::TimeSignatureExtendedrStringMain->ToArray();
		array<String^>^ Lines_Second_Quantization = TimeSignatures::TimeSignatureExtendedStringSub->ToArray();
		array<int>^ Values_Quantization = TimeSignatures::TimeSignatureExtendedValues->ToArray();

		this->_DropDown_Strobe_Quantization = gcnew Control_DropDown();
		this->_DropDown_Strobe_Quantization->Dock = DockStyle::Fill;
		this->_DropDown_Strobe_Quantization->Set_Tile_Layout(55, 55, 7);
		this->_DropDown_Strobe_Quantization->Title_Text = "Draw Quantization";
		this->_DropDown_Strobe_Quantization->Set_Title_Color(Color::DarkGray);
		this->_DropDown_Strobe_Quantization->Set_Open_Direction(false);
		this->_DropDown_Strobe_Quantization->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Strobe_Quantization->Margin = System::Windows::Forms::Padding(1, 2, 2, 2);
		this->_DropDown_Strobe_Quantization->Set_Items(Lines_First_Quantization, Lines_Second_Quantization, Values_Quantization);
		this->_DropDown_Strobe_Quantization->Selected_Index = 0;
		this->_DropDown_Strobe_Quantization->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Widget_Strobe_Options::DropDown_Strobe_Quantization_OnItem_Selected);

		this->_Color_Picker->ColorChanged += gcnew EventHandler(this, &Widget_Strobe_Options::Color_Picker_OnColorChanged);

		// Create color presets panel
		this->_Color_Presets = gcnew Control_ColorPreset();
		this->_Color_Presets->Dock = DockStyle::Fill;
		this->_Color_Presets->BackColor = Color::Transparent;
		this->_Color_Presets->SelectedColor = this->_Color_Picker->SelectedColor;
		this->_Color_Presets->Margin = System::Windows::Forms::Padding(0, 5, 0, 0);
		this->_Color_Presets->SelectedColorChanged += gcnew EventHandler(this, &Widget_Strobe_Options::PresetPanel_SelectedColorChanged);

		// Add controls to main layout
		Table_Layout_Main->Controls->Add(this->_DropDown_Strobe_Quantization, 0, 0);
		Table_Layout_Main->Controls->Add(this->_Color_Presets, 0, 1);
		Table_Layout_Main->SetColumnSpan(this->_Color_Presets, Table_Layout_Main->ColumnCount);

		// Add main layout to GroupBox
		this->_GroupBox->Controls->Add(Table_Layout_Main);

		// Add GroupBox to UserControl
		this->Controls->Add(this->_GroupBox);
	}

	void Widget_Strobe_Options::DropDown_Strobe_Quantization_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		QuantizationChanged(e->Value);
	}

	void Widget_Strobe_Options::PresetPanel_SelectedColorChanged(System::Object^ sender, System::EventArgs^ e)
	{
		this->_Color_Picker->SelectedColor = this->_Color_Presets->SelectedColor;

		ColorChanged(this->_Color_Picker->SelectedColor);
	}

	void Widget_Strobe_Options::Color_Picker_OnColorChanged(Object^ sender, EventArgs^ e)
	{
		this->_Color_Presets->SelectedColor = _Color_Picker->SelectedColor;

		ColorChanged(this->_Color_Picker->SelectedColor);
	}

	void Widget_Strobe_Options::Select_Next_Strobe_Value(void)
	{
		this->_DropDown_Strobe_Quantization->Select_Next();
	}

	void Widget_Strobe_Options::Select_Previous_Strobe_Value(void)
	{
		this->_DropDown_Strobe_Quantization->Select_Previous();
	}

	int Widget_Strobe_Options::TickLength::get() {
		return this->_DropDown_Strobe_Quantization->Selected_Value;
	}

	void Widget_Strobe_Options::TickLength::set(int value) {
		this->_DropDown_Strobe_Quantization->Select_By_Value(value);
	}

	Color Widget_Strobe_Options::SelectedColor::get() {
		return this->_Color_Picker->SelectedColor;
	}

	void Widget_Strobe_Options::SelectedColor::set(Color color) {
		this->_Color_Picker->SelectedColor = color;
	}

	void Widget_Strobe_Options::PresetColor::set(int index) {
		this->_Color_Presets->SelectedIndex = index;
	}
}