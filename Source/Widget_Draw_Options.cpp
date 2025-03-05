#include "Widget_Draw_Options.h"

namespace MIDILightDrawer
{
	Widget_Draw_Options::Widget_Draw_Options(Control_ColorPicker^ color_picker)
	{
		this->_Color_Picker = color_picker;

		Initialize_Component();
	}

	Widget_Draw_Options::~Widget_Draw_Options()
	{
		if (_Components) {
			delete _Components;
		}
	}

	void Widget_Draw_Options::Initialize_Component(void)
	{
		this->_Components = gcnew System::ComponentModel::Container();

		// Create main GroupBox
		this->_GroupBox = gcnew Control_GroupBox();
		this->_GroupBox->Text = "Draw Options";
		this->_GroupBox->Dock = DockStyle::Fill;
		this->_GroupBox->Padding = System::Windows::Forms::Padding(10, 15, 10, 10);
	
		// Create layout for GroupBox contents
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->RowCount = 2;
		Table_Layout_Main->ColumnCount = 3;
		Table_Layout_Main->Dock = DockStyle::Fill;
		Table_Layout_Main->Padding = System::Windows::Forms::Padding(5);

		// Configure row styles
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 45));	// Row for combo and color button
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 80));	// Row for color presets

		// Configure column styles
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 135));
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 250));  // Combo box column
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));  // Combo box column



		array<String^>^ Lines_First_Snapping = gcnew array<String^>{ "None", "Grid", "Events", "Tablature"};
		array<String^>^ Lines_Second_Snapping = gcnew array<String^>{ "", "", "", ""};
		array<int>^ Values_Snapping = gcnew array<int>{ (int)SnappingType::Snap_None, (int)SnappingType::Snap_Grid, (int)SnappingType::Snap_Events, (int)SnappingType::Snap_Tablature };

		this->_DropDown_Draw_Snapping = gcnew Control_DropDown();
		this->_DropDown_Draw_Snapping->Dock = DockStyle::Fill;
		this->_DropDown_Draw_Snapping->Set_Tile_Layout(126, 30, 1);
		this->_DropDown_Draw_Snapping->Title_Text = "Snap To";
		this->_DropDown_Draw_Snapping->Set_Title_Color(Color::DarkGray);
		this->_DropDown_Draw_Snapping->Set_Open_Direction(false);
		this->_DropDown_Draw_Snapping->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Draw_Snapping->Margin = System::Windows::Forms::Padding(1, 2, 5, 2);
		this->_DropDown_Draw_Snapping->Set_Items(Lines_First_Snapping, Lines_Second_Snapping, Values_Snapping);
		this->_DropDown_Draw_Snapping->Selected_Index = 1;
		this->_DropDown_Draw_Snapping->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Widget_Draw_Options::DropDown_Draw_Snapping_OnItem_Selected);


		array<String^>^ Lines_First_Length = gcnew array<String^>	{ "1/1"	, "1/2"	, "1/4"	, "1/8"	, "1/16", "1/32", "1/4"	, "1/8"	, "1/16", "1/32", "1/8"	, "1/16", "1/32", "1/8"	, "1/16", "1/32", "1/8"		, "1/8"		, "1/8"		, "1/8"		, "1/8"		, "1/16"	, "1/16"	, "1/16"	, "1/16"	, "1/16"	};
		array<String^>^ Lines_Second_Length = gcnew array<String^>	{ ""	, ""	, ""	, ""	, ""	, ""	, "T"	, "T"	, "T"	, "T"	, "5:4"	, "5:4"	, "5:4"	, "7:8"	, "7:8"	, "7:8"	, "20% SW"	, "40% SW"	, "60% SW"	, "80% SW"	, "100% SW"	, "20% SW"	, "40% SW"	, "60% SW"	, "80% SW"	, "100% SW" };
		array<int>^ Values_Length = gcnew array<int>				{ 3840	, 1920	, 960	, 480	, 240	, 120	, 640	,  320	,  160	,  80	,  384	,  192	,  96	,  619	,  278	,  137	,  542		,  610		,  672		,  734		,  802		,  257		,  271		,  288		,  305		,  319		};

		this->_DropDown_Draw_Length = gcnew Control_DropDown();
		this->_DropDown_Draw_Length->Dock = DockStyle::Fill;
		this->_DropDown_Draw_Length->Set_Tile_Layout(55, 55, 7);
		this->_DropDown_Draw_Length->Title_Text = "Draw Length";
		this->_DropDown_Draw_Length->Set_Title_Color(Color::DarkGray);
		this->_DropDown_Draw_Length->Set_Open_Direction(false);
		this->_DropDown_Draw_Length->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Draw_Length->Margin = System::Windows::Forms::Padding(1, 2, 2, 2);
		this->_DropDown_Draw_Length->Set_Items(Lines_First_Length, Lines_Second_Length, Values_Length);
		this->_DropDown_Draw_Length->Selected_Index = 0;
		this->_DropDown_Draw_Length->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Widget_Draw_Options::DropDown_Draw_Length_OnItem_Selected);


		// CheckBox to Draw based on Tablature
		this->_CheckBox_Consider_Tab = gcnew CheckBox();
		this->_CheckBox_Consider_Tab->Dock = DockStyle::Left;
		this->_CheckBox_Consider_Tab->Width = 300;
		this->_CheckBox_Consider_Tab->Text = "Length from Tablature";
		this->_CheckBox_Consider_Tab->Margin = System::Windows::Forms::Padding(10, 0, 0, 0);
		this->_CheckBox_Consider_Tab->ForeColor = Theme_Manager::Get_Instance()->ForegroundText;
		this->_CheckBox_Consider_Tab->CheckStateChanged += gcnew System::EventHandler(this, &Widget_Draw_Options::CheckBox_Consider_Tab_OnCheckStateChanged);


		// Color Picker Event Handler
		this->_Color_Picker->ColorChanged += gcnew EventHandler(this, &Widget_Draw_Options::Color_Picker_OnColorChanged);


		// Create color presets panel
		this->_Color_Presets = gcnew Control_ColorPreset();
		this->_Color_Presets->Dock			= DockStyle::Fill;
		this->_Color_Presets->BackColor		= Color::Transparent;
		this->_Color_Presets->SelectedColor = this->_Color_Picker->SelectedColor;
		this->_Color_Presets->Margin		= System::Windows::Forms::Padding(0, 5, 0, 0);
		this->_Color_Presets->SelectedColorChanged += gcnew EventHandler(this, &Widget_Draw_Options::PresetPanel_SelectedColorChanged);

		// Add controls to main layout
		Table_Layout_Main->Controls->Add(this->_DropDown_Draw_Snapping	, 0, 0);
		Table_Layout_Main->Controls->Add(this->_DropDown_Draw_Length	, 1, 0);
		Table_Layout_Main->Controls->Add(this->_CheckBox_Consider_Tab	, 2, 0);
		Table_Layout_Main->Controls->Add(this->_Color_Presets			, 0, 1);
		Table_Layout_Main->SetColumnSpan(this->_Color_Presets, Table_Layout_Main->ColumnCount);

		// Add main layout to GroupBox
		this->_GroupBox->Controls->Add(Table_Layout_Main);

		// Add GroupBox to UserControl
		this->Controls->Add(this->_GroupBox);
	}

	void Widget_Draw_Options::DropDown_Draw_Snapping_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		SnappingChanged(e->Value);
	}

	void Widget_Draw_Options::DropDown_Draw_Length_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		LengthChanged(e->Value);
	}

	void Widget_Draw_Options::CheckBox_Consider_Tab_OnCheckStateChanged(System::Object^ sender, System::EventArgs^ e)
	{
		ConsiderTabChanged(this->_CheckBox_Consider_Tab->Checked);
	}

	void Widget_Draw_Options::PresetPanel_SelectedColorChanged(System::Object^ sender, System::EventArgs^ e)
	{
		this->_Color_Picker->SelectedColor = this->_Color_Presets->SelectedColor;

		ColorChanged(this->_Color_Picker->SelectedColor);
	}

	void Widget_Draw_Options::Color_Picker_OnColorChanged(Object^ sender, EventArgs^ e)
	{
		this->_Color_Presets->SelectedColor = _Color_Picker->SelectedColor;

		ColorChanged(this->_Color_Picker->SelectedColor);
	}

	void Widget_Draw_Options::Select_Draw_Snapping_Next(void)
	{
		this->_DropDown_Draw_Snapping->Select_Next();
	}

	void Widget_Draw_Options::Select_Draw_Snapping_Previous(void)
	{
		this->_DropDown_Draw_Snapping->Select_Previous();
	}

	void Widget_Draw_Options::Select_Draw_Length_Next(void)
	{
		this->_DropDown_Draw_Length->Select_Next();
	}

	void Widget_Draw_Options::Select_Draw_Length_Previous(void)
	{
		this->_DropDown_Draw_Length->Select_Previous();
	}

	void Widget_Draw_Options::Toggle_LengthByTablature(void)
	{
		this->_CheckBox_Consider_Tab->Checked = !this->_CheckBox_Consider_Tab->Checked;
	}

	int Widget_Draw_Options::DrawSnapping::get() {
		return this->_DropDown_Draw_Snapping->Selected_Value;
	}

	void Widget_Draw_Options::DrawSnapping::set(int value) {
		this->_DropDown_Draw_Snapping->Select_By_Value(value);
	}

	int Widget_Draw_Options::DrawLength::get() {
		return this->_DropDown_Draw_Length->Selected_Value;
	}

	void Widget_Draw_Options::DrawLength::set(int value) {
		this->_DropDown_Draw_Length->Select_By_Value(value);
	}

	bool Widget_Draw_Options::LengthByTablature::get() {
		return this->_CheckBox_Consider_Tab->Checked;
	}

	void Widget_Draw_Options::LengthByTablature::set(bool value) {
		this->_CheckBox_Consider_Tab->Checked = value;
	}

	Color Widget_Draw_Options::SelectedColor::get() {
		return this->_Color_Picker->SelectedColor;
	}

	void Widget_Draw_Options::SelectedColor::set(Color color) {
		this->_Color_Picker->SelectedColor = color;
	}

	void Widget_Draw_Options::PresetColor::set(int index) {
		this->_Color_Presets->SelectedIndex = index;
	}
}


