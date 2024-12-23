#include "Widget_Fade_Options.h"


namespace MIDILightDrawer
{
	Widget_Fade_Options::Widget_Fade_Options(Control_ColorPicker^ color_picker)
	{
		this->_Color_Picker = color_picker;
		this->_Fade_Quantization_Ticks = 960 * 4;

		Initialize_Component(); 
	}

	Widget_Fade_Options::~Widget_Fade_Options()
	{
		if (_Components) {
			delete _Components;
		}
	}

	void Widget_Fade_Options::OnVisibleChanged(EventArgs^ e)
	{
		__super::OnVisibleChanged(e);
		
		if (this->Visible == false)
		{
			this->_Fade_Preview->Deselect_All();
		}
	}

	void Widget_Fade_Options::Initialize_Component(void)
	{
		System::Resources::ResourceManager^ Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());
		
		this->_Components = gcnew System::ComponentModel::Container();

		// Create main GroupBox
		this->_GroupBox = gcnew GroupBox();
		this->_GroupBox->Text = "Fade Options";
		this->_GroupBox->Dock = DockStyle::Fill;
		this->_GroupBox->Paint += gcnew PaintEventHandler(this, &Widget_Fade_Options::GroupBox_Paint);
		this->_GroupBox->Padding = System::Windows::Forms::Padding(10, 15, 10, 10);

		// Create layout for GroupBox contents
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->RowCount = 2;
		Table_Layout_Main->ColumnCount = 3;
		Table_Layout_Main->Dock = DockStyle::Fill;
		Table_Layout_Main->Padding = System::Windows::Forms::Padding(5);

		// Configure row styles
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 45));	// Row for combo and color button
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 60));	// Row for fade preview

		// Configure column styles
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 250));	// Combo Box Column
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 50));	// Button Column
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));	// Padding Column



		array<String^>^ Lines_First_Quantization = gcnew array<String^>	{ "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4", "1/8", "1/16", "1/32", "1/8", "1/16", "1/32", "1/8", "1/16", "1/32", "1/8", "1/8", "1/8", "1/8", "1/8", "1/16", "1/16", "1/16", "1/16", "1/16"	};
		array<String^>^ Lines_Second_Quantization = gcnew array<String^>{ "", "", "", "", "", "", "T", "T", "T", "T", "5:4", "5:4", "5:4", "7:8", "7:8", "7:8", "20% SW", "40% SW", "60% SW", "80% SW", "100% SW", "20% SW", "40% SW", "60% SW", "80% SW", "100% SW" };
		array<int>^ Values_Quantization = gcnew array<int>				{ 3840, 1920, 960, 480, 240, 120, 640, 320, 160, 80, 384, 192, 96, 619, 278, 137, 542, 610, 672, 734, 802, 257, 271, 288, 305, 319		};

		// DropDown Fade Quantization
		this->_DropDown_Fade_Quantization = gcnew Control_DropDown();
		this->_DropDown_Fade_Quantization->Dock = DockStyle::Fill;
		this->_DropDown_Fade_Quantization->Set_Tile_Layout(55, 55, 7);
		this->_DropDown_Fade_Quantization->Title_Text = "Fade Quantization";
		this->_DropDown_Fade_Quantization->Set_Title_Color(Color::DarkGray);
		this->_DropDown_Fade_Quantization->Set_Open_Direction(false);
		this->_DropDown_Fade_Quantization->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Fade_Quantization->Margin = System::Windows::Forms::Padding(1, 2, 2, 2);
		this->_DropDown_Fade_Quantization->Set_Items(Lines_First_Quantization, Lines_Second_Quantization, Values_Quantization);
		this->_DropDown_Fade_Quantization->Selected_Index = 0;
		this->_DropDown_Fade_Quantization->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Widget_Fade_Options::DropDown_Fade_Quantization_OnItem_Selected);

		// Color Picker Event Handler
		this->_Color_Picker->ColorChanged += gcnew EventHandler(this, &Widget_Fade_Options::Color_Picker_OnColorChanged);

		// Fade Preview
		this->_Fade_Preview = gcnew Control_FadePreview(15);
		this->_Fade_Preview->Dock = DockStyle::Fill;
		this->_Fade_Preview->Margin = System::Windows::Forms::Padding(0, 20, 0, 10);
		this->_Fade_Preview->PreviewSideSelected += gcnew PreviewSideSelectedHandler(this, &Widget_Fade_Options::Fade_Preview_OnPreviewSideSelected);

		// Fade Preview Switch Color Button
		Button^ Button_Switch_Colors = gcnew Button();
		Button_Switch_Colors->Dock = DockStyle::Fill;
		Button_Switch_Colors->Margin = System::Windows::Forms::Padding(10, 25, 0, 15);
		//Button_Switch_Colors->Text = "Switch Colors";
		Button_Switch_Colors->Click += gcnew System::EventHandler(this, &Widget_Fade_Options::Button_Switch_Colors_OnClick);
		Theme_Manager::Get_Instance()->ApplyThemeToButton(Button_Switch_Colors);
		Button_Switch_Colors->Image = (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Switch_White")));
		

		// Add controls to main layout
		Table_Layout_Main->Controls->Add(this->_DropDown_Fade_Quantization, 0, 0);
		Table_Layout_Main->Controls->Add(this->_Fade_Preview, 0, 1);
		Table_Layout_Main->Controls->Add(Button_Switch_Colors, 1, 1);
		//Table_Layout_Main->SetColumnSpan(this->_Color_Presets, Table_Layout_Main->ColumnCount);

		// Add main layout to GroupBox
		this->_GroupBox->Controls->Add(Table_Layout_Main);

		// Add GroupBox to UserControl
		this->Controls->Add(this->_GroupBox);
	}

	void Widget_Fade_Options::DropDown_Fade_Quantization_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		_Fade_Quantization_Ticks = e->Value;

		QuantizationChanged(_Fade_Quantization_Ticks);
	}

	void Widget_Fade_Options::Color_Picker_OnColorChanged(Object^ sender, EventArgs^ e)
	{
		this->_Fade_Preview->Set_Color(this->_Color_Picker->SelectedColor);

		if (this->_Fade_Preview->Start_Is_Selected()) {
			ColorStartChanged(this->_Color_Picker->SelectedColor);
		}
		else if (this->_Fade_Preview->End_Is_Selected()) {
			ColorEndChanged(this->_Color_Picker->SelectedColor);
		}
	}

	void Widget_Fade_Options::GroupBox_Paint(Object^ sender, PaintEventArgs^ e)
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

	void Widget_Fade_Options::Fade_Preview_OnPreviewSideSelected(System::Drawing::Color color)
	{
		this->_Color_Picker->SelectedColor = color;
	}

	void Widget_Fade_Options::Select_Next_Fade_Value(void)
	{
		this->_DropDown_Fade_Quantization->Select_Next();
	}

	void Widget_Fade_Options::Select_Previous_Fade_Value(void)
	{
		this->_DropDown_Fade_Quantization->Select_Previous();
	}

	int Widget_Fade_Options::TickLength::get() {
		return this->_Fade_Quantization_Ticks;
	}

	void Widget_Fade_Options::TickLength::set(int value) {
		this->_DropDown_Fade_Quantization->Select_By_Value(value);
	}

	Color Widget_Fade_Options::StartColor::get() {
		return this->_Fade_Preview->StartColor;
	}

	Color Widget_Fade_Options::EndColor::get() {
		return this->_Fade_Preview->EndColor;
	}

	void Widget_Fade_Options::Button_Switch_Colors_OnClick(System::Object^ sender, System::EventArgs^ e)
	{
		this->_Fade_Preview->Switch_Colors();

		ColorStartChanged(this->_Fade_Preview->StartColor);
		ColorEndChanged(this->_Fade_Preview->EndColor);
	}
}