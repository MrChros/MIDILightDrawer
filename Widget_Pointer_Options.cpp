#include "Widget_Pointer_Options.h"

namespace MIDILightDrawer
{
	Widget_Pointer_Options::Widget_Pointer_Options()
	{
		Initialize_Component();
	}

	Widget_Pointer_Options::~Widget_Pointer_Options()
	{
		if (_Components) {
			delete _Components;
		}
	}

	void Widget_Pointer_Options::Initialize_Component(void)
	{
		this->_Components = gcnew System::ComponentModel::Container();

		// Create main GroupBox
		this->_GroupBox = gcnew GroupBox();
		this->_GroupBox->Text = "Pointer Options";
		this->_GroupBox->Dock = DockStyle::Fill;
		this->_GroupBox->Paint += gcnew PaintEventHandler(this, &Widget_Pointer_Options::GroupBox_Paint);
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



		array<String^>^ Lines_First_Snapping = gcnew array<String^>{ "None", "Grid", "Bars", "Tablature"};
		array<String^>^ Lines_Second_Snapping = gcnew array<String^>{ "", "", "", ""};
		array<int>^ Values_Snapping = gcnew array<int>{ (int)SnappingType::Snap_None, (int)SnappingType::Snap_Grid, (int)SnappingType::Snap_Bars, (int)SnappingType::Snap_Tablature };

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
		this->_DropDown_Draw_Snapping->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Widget_Pointer_Options::DropDown_Draw_Snapping_OnItem_Selected);


		// Add controls to main layout
		Table_Layout_Main->Controls->Add(this->_DropDown_Draw_Snapping, 0, 0);

		// Add main layout to GroupBox
		this->_GroupBox->Controls->Add(Table_Layout_Main);

		// Add GroupBox to UserControl
		this->Controls->Add(this->_GroupBox);
	}

	void Widget_Pointer_Options::DropDown_Draw_Snapping_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		SnappingChanged(e->Value);
	}

	void Widget_Pointer_Options::GroupBox_Paint(Object^ sender, PaintEventArgs^ e)
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

	void Widget_Pointer_Options::Select_Pointer_Snapping_Next(void)
	{
		this->_DropDown_Draw_Snapping->Select_Next();
	}

	void Widget_Pointer_Options::Select_Pointer_Snapping_Previous(void)
	{
		this->_DropDown_Draw_Snapping->Select_Previous();
	}

	int Widget_Pointer_Options::PointerSnapping::get() {
		return this->_DropDown_Draw_Snapping->Selected_Value;
	}

	void Widget_Pointer_Options::PointerSnapping::set(int value) {
		this->_DropDown_Draw_Snapping->Select_By_Value(value);
	}
}


