#include "Control_ColorPreset.h"

namespace MIDILightDrawer {
	Control_ColorPreset::Control_ColorPreset(void)
	{
		Initialize_Component();
		Load_PresetColors();
	}

	void Control_ColorPreset::SelectedIndex::set(int index)
	{
		// Check if index is within valid range
		if (index >= 0 && index < _Color_Preset_Buttons->Length)
		{
			_Selected_Index = index;
			_Selected_Color = _Colors[index];

			// Refresh all buttons to update highlighting
			for each (Button ^ button in _Color_Preset_Buttons) {
				button->Invalidate();
			}

			OnSelectedColorChanged();
		}
	}

	void Control_ColorPreset::Initialize_Component(void)
	{
		_Highlight_Color	= Theme_Manager::Get_Instance()->AccentPrimary;
		_Selected_Index		= -1;

		// Initialize color array
		_Colors = gcnew array<Color>(10);
		for (int i = 0; i < 10; i++) {
			_Colors[i] = Color::White;
		}
		
		TableLayoutPanel^ Panel = gcnew TableLayoutPanel();
		Panel->ColumnCount	= 10;	// 10 columns for all buttons
		Panel->RowCount		= 2;	// 1 row for color buttons, 1 row for set buttons
		Panel->AutoSize		= true;
		Panel->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		Panel->Dock			= DockStyle::Fill;
		Panel->BackColor	= Color::Transparent;
		Panel->Margin		= System::Windows::Forms::Padding(0, 0, 0, 0);
		Panel->Padding		= System::Windows::Forms::Padding(0, 0, 0, 0);

		// Configure columns with fixed widths
		for (int i = 0; i < 10; i++) {
			Panel->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 45));  // 40px button + 5px margin
		}

		// Configure rows with fixed heights
		Panel->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 45));  // Color buttons row
		Panel->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));  // Set buttons row

		// Initialize button arrays
		_Color_Preset_Buttons = gcnew array<Button^>(10);
		_Set_Buttons = gcnew array<Button^>(10);

		// Create all buttons
		for (int i = 0; i < 10; i++)
		{
			// Color preset button
			Button^ Color_Button = gcnew Button();
			Color_Button->Size							= System::Drawing::Size(40, 40);
			//Color_Button->Dock							= DockStyle::Left;
			Color_Button->FlatStyle						= System::Windows::Forms::FlatStyle::Flat;
			Color_Button->FlatAppearance->BorderSize	= 1;
			Color_Button->FlatAppearance->BorderColor	= Theme_Manager::Get_Instance()->BorderPrimary;
			//Color_Button->Margin						= System::Windows::Forms::Padding(2);
			Color_Button->Margin						= System::Windows::Forms::Padding(0);
			Color_Button->Anchor						= static_cast<System::Windows::Forms::AnchorStyles>
														   (System::Windows::Forms::AnchorStyles::Top |
															System::Windows::Forms::AnchorStyles::Left);
			Color_Button->BackColor						= Theme_Manager::Get_Instance()->BackgroundLight;
			Color_Button->Tag							= i;
			Color_Button->Click			+= gcnew EventHandler		(this, &Control_ColorPreset::Color_Preset_Button_Click);
			Color_Button->Paint			+= gcnew PaintEventHandler	(this, &Control_ColorPreset::Color_Button_Paint);
			Color_Button->MouseEnter	+= gcnew EventHandler		(this, &Control_ColorPreset::Button_MouseEnter);
			Color_Button->MouseLeave	+= gcnew EventHandler		(this, &Control_ColorPreset::Button_MouseLeave);

			_Color_Preset_Buttons[i] = Color_Button;
			
			UpdateButtonIcon(i, _Colors[i]);
				

			Panel->Controls->Add(Color_Button, i, 0);

			// Set button
			Button^ Set_Button		= gcnew Button();
			Set_Button->Text		= "Set";
			Set_Button->Size		= System::Drawing::Size(40, 25);
			Set_Button->Margin		= System::Windows::Forms::Padding(1, 3, 0, 0);
			Set_Button->Font		= gcnew System::Drawing::Font(Set_Button->Font->FontFamily, 8.0f);
			Set_Button->TextAlign	= ContentAlignment::MiddleCenter;
			Set_Button->Anchor		= static_cast<System::Windows::Forms::AnchorStyles>
									   (System::Windows::Forms::AnchorStyles::Top |
										System::Windows::Forms::AnchorStyles::Left);
			Set_Button->Tag			= i;
			Theme_Manager::Get_Instance()->ApplyThemeToButton(Set_Button);

			Set_Button->Click += gcnew System::EventHandler(this, &Control_ColorPreset::Set_Button_Click);

			_Set_Buttons[i] = Set_Button;

			Panel->Controls->Add(Set_Button, i, 1);
		}

		this->BackColor = Theme_Manager::Get_Instance()->Background;
		this->Controls->Add(Panel);
	}

	void Control_ColorPreset::Save_PresetColors()
	{
		Settings^ settings = Settings::Get_Instance();
		System::Collections::Generic::List<String^>^ colorList = gcnew System::Collections::Generic::List<String^>();

		for (int i = 0; i < _Colors->Length; i++)
		{
			String^ colorString = String::Format("{0},{1},{2}", _Colors[i].R, _Colors[i].G, _Colors[i].B);
			colorList->Add(colorString);
		}

		settings->ColorPresets = colorList;
	}

	void Control_ColorPreset::Load_PresetColors()
	{
		Settings^ settings = Settings::Get_Instance();
		auto colorList = settings->ColorPresets;

		if (colorList != nullptr && colorList->Count == 10) {
			for (int i = 0; i < _Colors->Length; i++) {
				array<String^>^ rgb = colorList[i]->Split(',');
				if (rgb->Length == 3) {
					Color color = Color::FromArgb(
						Int32::Parse(rgb[0]),
						Int32::Parse(rgb[1]),
						Int32::Parse(rgb[2])
					);
					UpdateButtonIcon(i, color);
				}
			}
		}
	}

	void Control_ColorPreset::UpdateButtonIcon(int index, Color color)
	{
		Button^ button = _Color_Preset_Buttons[index];

		// Update color in array
		_Colors[index] = color;

		// If button already has an icon, dispose it
		if (button->Image != nullptr) {
			delete button->Image;
		}

		// Create icon and set it
		Drawing::Icon^ icon = CreateColorIcon(color);
		button->Image = icon->ToBitmap();
		delete icon;

		// Center the icon in the button
		button->ImageAlign = ContentAlignment::MiddleCenter;
	}

	void Control_ColorPreset::Color_Preset_Button_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Button^ Color_Button = safe_cast<Button^>(sender);
		int index = safe_cast<int>(Color_Button->Tag);

		_Selected_Index = index;
		_Selected_Color = _Colors[index];

		// Refresh all buttons to update highlighting
		for each (Button ^ button in _Color_Preset_Buttons) {
			button->Invalidate();
		}

		OnSelectedColorChanged();
	}

	void Control_ColorPreset::Set_Button_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Button^ Set_Button = safe_cast<Button^>(sender);
		int index = safe_cast<int>(Set_Button->Tag);

		UpdateButtonIcon(index, _Selected_Color);
		Save_PresetColors();
	}

	void Control_ColorPreset::Color_Button_Paint(Object^ sender, PaintEventArgs^ e)
	{
		Button^ button = safe_cast<Button^>(sender);
		Graphics^ g = e->Graphics;
		Rectangle rect = button->ClientRectangle;

		// Draw button border based on selection state
		Pen^ borderPen;
		int buttonIndex = safe_cast<int>(button->Tag);

		if (buttonIndex == _Selected_Index) {
			borderPen = gcnew Pen(_Highlight_Color, 2);
		}
		else if (button->ClientRectangle.Contains(button->PointToClient(Control::MousePosition))) {
			borderPen = gcnew Pen(Color::FromArgb(180, _Highlight_Color), 2);
		}
		else {
			borderPen = gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong);
		}

		g->DrawRectangle(borderPen, 1, 1, rect.Width - 2, rect.Height - 2);
		delete borderPen;
	}

	void Control_ColorPreset::Button_MouseEnter(Object^ sender, EventArgs^ e)
	{
		Button^ button = safe_cast<Button^>(sender);
		button->Invalidate();
	}

	void Control_ColorPreset::Button_MouseLeave(Object^ sender, EventArgs^ e)
	{
		Button^ button = safe_cast<Button^>(sender);
		button->Invalidate();
	}

	Drawing::Icon^ Control_ColorPreset::CreateColorIcon(Color color)
	{
		Bitmap^ bmp = gcnew Bitmap(COLOR_CIRCLE_DIAMETER, COLOR_CIRCLE_DIAMETER);
		Graphics^ g = Graphics::FromImage(bmp);

		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Draw circle
		g->FillEllipse(gcnew SolidBrush(color),
			0, 0, COLOR_CIRCLE_DIAMETER - 1, COLOR_CIRCLE_DIAMETER - 1);

		// Draw circle border
		g->DrawEllipse(gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong), 0, 0, COLOR_CIRCLE_DIAMETER - 1, COLOR_CIRCLE_DIAMETER - 1);

		IntPtr hicon = bmp->GetHicon();
		Drawing::Icon^ icon = Drawing::Icon::FromHandle(hicon);

		delete g;
		return icon;
	}
}