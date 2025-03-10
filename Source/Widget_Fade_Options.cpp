#include "Widget_Fade_Options.h"



namespace MIDILightDrawer
{
	Widget_Fade_Options::Widget_Fade_Options(Control_ColorPicker^ color_picker)
	{
		this->_Color_Picker = color_picker;

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
		this->_GroupBox = gcnew Control_GroupBox();
		this->_GroupBox->Text = "Fade Options";
		this->_GroupBox->Dock = DockStyle::Fill;
		this->_GroupBox->Padding = System::Windows::Forms::Padding(10, 15, 10, 10);

		// Create layout for GroupBox contents
		TableLayoutPanel^ Table_Layout_Main = gcnew TableLayoutPanel();
		Table_Layout_Main->RowCount = 2;
		Table_Layout_Main->ColumnCount = 5;
		Table_Layout_Main->Dock = DockStyle::Fill;
		Table_Layout_Main->Padding = System::Windows::Forms::Padding(5);

		// Configure row styles
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 45));	// Row for combo and color button
		Table_Layout_Main->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 60));	// Row for fade preview

		// Configure column styles
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 250));	// Combo Box Column
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute,  50));	// Button Switch Column
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute,  50));	// Button Mode Column
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 100));	// ...
		Table_Layout_Main->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Percent, 100.0f));	// Padding Column


		array<String^>^ Lines_First_Quantization = TimeSignatures::TimeSignatureExtendedrStringMain->ToArray();
		array<String^>^ Lines_Second_Quantization = TimeSignatures::TimeSignatureExtendedStringSub->ToArray();
		array<int>^ Values_Quantization = TimeSignatures::TimeSignatureExtendedValues->ToArray();

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


		array<String^>^ Lines_First_Easings = gcnew array<String^>	{ "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ", "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ", "Linear", "Sine", "Quad", "Cubic", "Quart", "Quint", "Expo", "Circ" };
		array<String^>^ Lines_Second_Easings = gcnew array<String^>	{ "", "In", "In", "In", "In", "In", "In", "In", "", "Out", "Out", "Out", "Out", "Out", "Out", "Out", "", "InOut", "InOut", "InOut", "InOut", "InOut", "InOut", "InOut" };
		array<int>^ Values_Easings = gcnew array<int> {
			(int)FadeEasing::Linear,
			(int)FadeEasing::In_Sine,
			(int)FadeEasing::In_Quad,
			(int)FadeEasing::In_Cubic,
			(int)FadeEasing::In_Quart,
			(int)FadeEasing::In_Quint,
			(int)FadeEasing::In_Expo,
			(int)FadeEasing::In_Circ,
			(int)FadeEasing::Linear,
			(int)FadeEasing::Out_Sine,
			(int)FadeEasing::Out_Quad,
			(int)FadeEasing::Out_Cubic,
			(int)FadeEasing::Out_Quart,
			(int)FadeEasing::Out_Quint,
			(int)FadeEasing::Out_Expo,
			(int)FadeEasing::Out_Circ,
			(int)FadeEasing::Linear,
			(int)FadeEasing::InOut_Sine,
			(int)FadeEasing::InOut_Quad,
			(int)FadeEasing::InOut_Cubic,
			(int)FadeEasing::InOut_Quart,
			(int)FadeEasing::InOut_Quint,
			(int)FadeEasing::InOut_Expo,
			(int)FadeEasing::InOut_Circ
		};

		System::Resources::ResourceManager^ EasingImages = gcnew System::Resources::ResourceManager("MIDILightDrawer.Easing", System::Reflection::Assembly::GetExecutingAssembly());

		array<Image^>^ Images_Easings = gcnew array<Image^> {
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"00_LINEAR")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"01_IN_SINE")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"04_IN_QUAD")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"07_IN_CUBIC")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"10_IN_QUART")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"13_IN_QUINT")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"16_IN_EXPO")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"19_IN_CIRC")),

			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"00_LINEAR")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"02_OUT_SINE")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"05_OUT_QUAD")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"08_OUT_CUBIC")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"11_OUT_QUART")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"14_OUT_QUINT")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"17_OUT_EXPO")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"20_OUT_CIRC")),

			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"00_LINEAR")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"03_INOUT_SINE")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"06_INOUT_QUAD")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"09_INOUT_CUBIC")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"12_INOUT_QUART")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"15_INOUT_QUINT")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"18_INOUT_EXPO")),
			cli::safe_cast<System::Drawing::Image^>(EasingImages->GetObject(L"21_INOUT_CIRC"))
		};


		// DropDown Ease In
		this->_DropDown_Ease_In = gcnew Control_DropDown();
		this->_DropDown_Ease_In->Dock = DockStyle::Fill;
		this->_DropDown_Ease_In->Set_Tile_Layout(55, 55, 8);
		this->_DropDown_Ease_In->Title_Text = "Ease In";
		this->_DropDown_Ease_In->Set_Title_Color(Color::DarkGray);
		this->_DropDown_Ease_In->Set_Open_Direction(false);
		this->_DropDown_Ease_In->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Ease_In->Margin = System::Windows::Forms::Padding(1, 2, 2, 2);
		this->_DropDown_Ease_In->Set_Items(Lines_First_Easings, Lines_Second_Easings, Values_Easings, Images_Easings);
		this->_DropDown_Ease_In->Selected_Index = 0;
		this->_DropDown_Ease_In->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Widget_Fade_Options::DropDown_Easings_OnItem_Selected);

		// DropDown Ease Out
		this->_DropDown_Ease_Out = gcnew Control_DropDown();
		this->_DropDown_Ease_Out->Dock = DockStyle::Fill;
		this->_DropDown_Ease_Out->Set_Tile_Layout(55, 55, 8);
		this->_DropDown_Ease_Out->Title_Text = "Ease Out";
		this->_DropDown_Ease_Out->Set_Title_Color(Color::DarkGray);
		this->_DropDown_Ease_Out->Set_Open_Direction(false);
		this->_DropDown_Ease_Out->Set_Horizontal_Alignment(Panel_Horizontal_Alignment::Left);
		this->_DropDown_Ease_Out->Margin = System::Windows::Forms::Padding(1, 2, 2, 2);
		this->_DropDown_Ease_Out->Set_Items(Lines_First_Easings, Lines_Second_Easings, Values_Easings, Images_Easings);
		this->_DropDown_Ease_Out->Selected_Index = 0;
		this->_DropDown_Ease_Out->Item_Selected += gcnew Control_DropDown_Item_Selected_Event_Handler(this, &Widget_Fade_Options::DropDown_Easings_OnItem_Selected);


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
		Button_Switch_Colors->Click += gcnew System::EventHandler(this, &Widget_Fade_Options::Button_Switch_Colors_OnClick);
		Theme_Manager::Get_Instance()->ApplyThemeToButton(Button_Switch_Colors);
		Button_Switch_Colors->Image = (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Switch_White")));

		Button^ Button_Fade_Mode = gcnew Button();
		Button_Fade_Mode->Dock = DockStyle::Fill;
		Button_Fade_Mode->Margin = System::Windows::Forms::Padding(10, 25, 0, 15);
		Button_Fade_Mode->Click += gcnew System::EventHandler(this, &Widget_Fade_Options::Button_Fade_Mode_OnClick);
		Theme_Manager::Get_Instance()->ApplyThemeToButton(Button_Fade_Mode);
		Button_Fade_Mode->Image = (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Number_2_White")));
		

		// Add controls to main layout
		Table_Layout_Main->Controls->Add(this->_DropDown_Fade_Quantization, 0, 0);
		Table_Layout_Main->Controls->Add(this->_DropDown_Ease_In, 1, 0);
		Table_Layout_Main->SetColumnSpan(this->_DropDown_Ease_In, 2);
		Table_Layout_Main->Controls->Add(this->_DropDown_Ease_Out, 3, 0);
		Table_Layout_Main->Controls->Add(this->_Fade_Preview, 0, 1);
		Table_Layout_Main->Controls->Add(Button_Switch_Colors, 1, 1);
		Table_Layout_Main->Controls->Add(Button_Fade_Mode, 2, 1);


		// Add main layout to GroupBox
		this->_GroupBox->Controls->Add(Table_Layout_Main);

		// Add GroupBox to UserControl
		this->Controls->Add(this->_GroupBox);
	}

	void Widget_Fade_Options::DropDown_Fade_Quantization_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		QuantizationChanged(e->Value);
	}

	void Widget_Fade_Options::DropDown_Easings_OnItem_Selected(System::Object^ sender, Control_DropDown_Item_Selected_Event_Args^ e)
	{
		this->_Fade_Preview->EaseIn = static_cast<FadeEasing>(this->_DropDown_Ease_In->Selected_Value);
		this->_Fade_Preview->EaseOut = static_cast<FadeEasing>(this->_DropDown_Ease_Out->Selected_Value);

		EasingsChanged(static_cast<FadeEasing>(this->_DropDown_Ease_In->Selected_Value), (static_cast<FadeEasing>(this->_DropDown_Ease_Out->Selected_Value)));
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
		else if (this->_Fade_Preview->Center_Is_Selected()) {
			ColorCenterChanged(this->_Color_Picker->SelectedColor);
		}
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
		return this->_DropDown_Fade_Quantization->Selected_Value;
	}

	void Widget_Fade_Options::TickLength::set(int value) {
		this->_DropDown_Fade_Quantization->Select_By_Value(value);
	}

	FadeType Widget_Fade_Options::FadeMode::get() {
		return this->_Fade_Preview->Type;
	}

	Color Widget_Fade_Options::StartColor::get() {
		return this->_Fade_Preview->StartColor;
	}

	Color Widget_Fade_Options::EndColor::get() {
		return this->_Fade_Preview->EndColor;
	}

	Color Widget_Fade_Options::CenterColor::get() {
		return this->_Fade_Preview->CenterColor;
	}

	FadeEasing Widget_Fade_Options::EaseIn::get() {
		return static_cast<FadeEasing>(this->_DropDown_Ease_In->Selected_Value);
	}

	FadeEasing Widget_Fade_Options::EaseOut::get() {
		return static_cast<FadeEasing>(this->_DropDown_Ease_Out->Selected_Value);
	}

	void Widget_Fade_Options::Button_Switch_Colors_OnClick(System::Object^ sender, System::EventArgs^ e)
	{
		this->_Fade_Preview->Switch_Colors();

		ColorStartChanged(this->_Fade_Preview->StartColor);
		ColorEndChanged(this->_Fade_Preview->EndColor);
	}

	void Widget_Fade_Options::Button_Fade_Mode_OnClick(System::Object^ sender, System::EventArgs^ e)
	{
		this->_Fade_Preview->Toggle_Mode();

		Button^ Toggle_Button = (cli::safe_cast<Button^>(sender));

		System::Resources::ResourceManager^ Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());

		if (this->_Fade_Preview->Type == FadeType::Three_Colors) {
			Toggle_Button->Image = (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Number_3_White")));
		}
		else {
			Toggle_Button->Image = (cli::safe_cast<System::Drawing::Image^>(Resources->GetObject(L"Number_2_White")));
		}

		FadeModeChanged(this->_Fade_Preview->Type);
	}
}