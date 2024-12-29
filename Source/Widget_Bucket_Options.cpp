#include "Widget_Bucket_Options.h"


namespace MIDILightDrawer
{
	Widget_Bucket_Options::Widget_Bucket_Options(Control_ColorPicker^ color_picker)
	{
		this->_Color_Picker = color_picker;

		Initialize_Component();
	}

	void Widget_Bucket_Options::Initialize_Component(void)
	{
		this->_Components = gcnew System::ComponentModel::Container();

		// Create main GroupBox
		this->_GroupBox = gcnew GroupBox();
		this->_GroupBox->Text = "Bucket Options";
		this->_GroupBox->Dock = DockStyle::Fill;

		// Create layout for GroupBox contents
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->RowCount = 3;
		Table_Layout_Main->ColumnCount = 2;
		Table_Layout_Main->Dock = DockStyle::Fill;
		Table_Layout_Main->Padding = System::Windows::Forms::Padding(5);

		// Configure row styles
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 45));	// Row for combo and color button
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 10));	// Spacing
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 75));	// Row for color presets

		// Configure column styles
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 200));  // Combo box column
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));  // Combo box column

		this->_Color_Picker->ColorChanged += gcnew EventHandler(this, &Widget_Bucket_Options::Color_Picker_OnColorChanged);

		// Create color presets panel
		this->_Color_Presets = gcnew Control_ColorPreset();
		this->_Color_Presets->Dock = DockStyle::Fill;
		this->_Color_Presets->SelectedColor = this->_Color_Picker->SelectedColor;
		this->_Color_Presets->SelectedColorChanged += gcnew EventHandler(this, &Widget_Bucket_Options::PresetPanel_SelectedColorChanged);

		// Add controls to main layout
		Table_Layout_Main->Controls->Add(this->_Color_Presets, 0, 2);
		Table_Layout_Main->SetColumnSpan(this->_Color_Presets, 2);

		// Add main layout to GroupBox
		this->_GroupBox->Controls->Add(Table_Layout_Main);

		// Add GroupBox to UserControl
		this->Controls->Add(this->_GroupBox);
	}

	void Widget_Bucket_Options::PresetPanel_SelectedColorChanged(System::Object^ sender, System::EventArgs^ e) {
		this->_Color_Picker->SelectedColor = this->_Color_Presets->SelectedColor;

		ColorChanged(this->_Color_Picker->SelectedColor);
	}

	void Widget_Bucket_Options::Color_Picker_OnColorChanged(Object^ sender, EventArgs^ e)
	{
		this->_Color_Presets->SelectedColor = _Color_Picker->SelectedColor;

		ColorChanged(this->_Color_Picker->SelectedColor);
	}

	Color Widget_Bucket_Options::SelectedColor::get() {
		return this->_Color_Picker->SelectedColor;
	}

	void Widget_Bucket_Options::SelectedColor::set(Color color) {
		this->_Color_Picker->SelectedColor = color;
	}

	void Widget_Bucket_Options::PresetColor::set(int index) {
		this->_Color_Presets->SelectedIndex = index;
	}
}