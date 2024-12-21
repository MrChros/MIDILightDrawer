#include "Widget_Draw_Options.h"

namespace MIDILightDrawer
{
	Widget_Draw_Options::Widget_Draw_Options(Control_ColorPicker^ color_picker)
	{
		this->_Color_Picker = color_picker;
		this->_Draw_Quantization_Ticks = 960 * 4;

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
		this->_GroupBox = gcnew GroupBox();
		this->_GroupBox->Text = "Draw Options";
		this->_GroupBox->Dock = DockStyle::Fill;
		this->_GroupBox->Paint += gcnew PaintEventHandler(this, &Widget_Draw_Options::GroupBox_Paint);
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



		array<String^>^ Lines_First_Quantization = gcnew array<String^>	{ "1/1"	, "1/2"	, "1/4"	, "1/8"	, "1/16", "1/32", "1/4"	, "1/8"	, "1/16", "1/32", "1/8"	, "1/16", "1/32", "1/8"	, "1/16", "1/32", "1/8"		, "1/8"		, "1/8"		, "1/8"		, "1/8"		, "1/16"	, "1/16"	, "1/16"	, "1/16"	, "1/16"	};
		array<String^>^ Lines_Second_Quantization = gcnew array<String^>{ ""	, ""	, ""	, ""	, ""	, ""	, "T"	, "T"	, "T"	, "T"	, "5:4"	, "5:4"	, "5:4"	, "7:8"	, "7:8"	, "7:8"	, "20% SW"	, "40% SW"	, "60% SW"	, "80% SW"	, "100% SW"	, "20% SW"	, "40% SW"	, "60% SW"	, "80% SW"	, "100% SW" };
		array<int>^ Values_Quantization = gcnew array<int>				{ 3840	, 1920	, 960	, 480	, 240	, 120	, 640	,  320	,  160	,  80	,  384	,  192	,  96	,  619	,  278	,  137	,  542		,  610		,  672		,  734		,  802		,  257		,  271		,  288		,  305		,  319		};

		this->_DropDown_Draw_Quantization = gcnew Control_DropDown();
		this->_DropDown_Draw_Quantization->Dock = DockStyle::Fill;
		this->_DropDown_Draw_Quantization->Set_Tile_Layout(55, 55, 7);
		this->_DropDown_Draw_Quantization->Title_Text = "Draw Quantization";
		this->_DropDown_Draw_Quantization->Set_Title_Color(Color::DarkGray);
		this->_DropDown_Draw_Quantization->Set_Open_Direction(false);
		this->_DropDown_Draw_Quantization->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Draw_Quantization->Margin = System::Windows::Forms::Padding(1, 2, 2, 2);
		this->_DropDown_Draw_Quantization->Set_Items(Lines_First_Quantization, Lines_Second_Quantization, Values_Quantization);
		this->_DropDown_Draw_Quantization->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Widget_Draw_Options::DropDown_Draw_Quantization_OnItem_Selected);

		this->_Color_Picker->ColorChanged += gcnew EventHandler(this, &Widget_Draw_Options::Color_Picker_OnColorChanged);

		// Create color presets panel
		this->_Color_Presets = gcnew Control_ColorPreset();
		this->_Color_Presets->Dock			= DockStyle::Fill;
		this->_Color_Presets->BackColor		= Color::Transparent;
		this->_Color_Presets->SelectedColor = this->_Color_Picker->SelectedColor;
		this->_Color_Presets->Margin		= System::Windows::Forms::Padding(0, 5, 0, 0);
		this->_Color_Presets->SelectedColorChanged += gcnew EventHandler(this, &Widget_Draw_Options::PresetPanel_SelectedColorChanged);

		// Add controls to main layout
		Table_Layout_Main->Controls->Add(this->_DropDown_Draw_Quantization	, 0, 0);
		Table_Layout_Main->Controls->Add(this->_Color_Presets				, 0, 1);
		Table_Layout_Main->SetColumnSpan(this->_Color_Presets, Table_Layout_Main->ColumnCount);

		// Add main layout to GroupBox
		this->_GroupBox->Controls->Add(Table_Layout_Main);

		// Add GroupBox to UserControl
		this->Controls->Add(this->_GroupBox);
	}

	void Widget_Draw_Options::DropDown_Draw_Quantization_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		_Draw_Quantization_Ticks = e->Value;
		
		QuantizationChanged(_Draw_Quantization_Ticks);
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

	void Widget_Draw_Options::GroupBox_Paint(Object^ sender, PaintEventArgs^ e)
	{
		GroupBox^ box = safe_cast<GroupBox^>(sender);
		Theme_Manager^ theme = Theme_Manager::Get_Instance();

		Graphics^ g = e->Graphics;
		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Calculate text size and positions
		Drawing::Font^ titleFont = gcnew Drawing::Font("Segoe UI Semibold", 9.5f);
		SizeF textSize = g->MeasureString(box->Text, titleFont);

		// Define header rect
		Rectangle headerRect = Rectangle(0, 0, box->Width, 28);

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

	void Widget_Draw_Options::Select_Next_Draw_Value(void)
	{
		this->_DropDown_Draw_Quantization->Select_Next();
	}

	void Widget_Draw_Options::Select_Previous_Draw_Value(void)
	{
		this->_DropDown_Draw_Quantization->Select_Previous();
	}

	int Widget_Draw_Options::Value::get() {
		return this->_Draw_Quantization_Ticks;
	}

	void Widget_Draw_Options::Value::set(int value) {
		this->_DropDown_Draw_Quantization->Select_By_Value(value);
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