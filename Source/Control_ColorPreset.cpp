#include "Control_ColorPreset.h"

namespace MIDILightDrawer
{
	Control_ColorPreset::Control_ColorPreset(void)
	{
		this->_ShowSetButtons = true;

		Initialize_Component();
		Load_PresetColors();
	}

	Control_ColorPreset::Control_ColorPreset(bool showSetButtons)
	{
		this->_ShowSetButtons = showSetButtons;
		
		Initialize_Component();
		Load_PresetColors();
	}

	void Control_ColorPreset::Initialize_Component(void)
	{
		_HighlightColor	= Theme_Manager::Get_Instance()->AccentPrimary;
		_SelectedIndex	= -1;

		// Initialize color array
		_Colors = gcnew array<Color>(Control_ColorPreset::COUNT_PRESET_COLORS);

		for (int i = 0; i < Control_ColorPreset::COUNT_PRESET_COLORS; i++) {
			_Colors[i] = Color::White;
		}
		
		TableLayoutPanel^ TableLayout = gcnew TableLayoutPanel();
		TableLayout->ColumnCount = Control_ColorPreset::COUNT_PRESET_COLORS;
		TableLayout->RowCount = _ShowSetButtons ? 2 : 1;	// 1 Row for Color Buttons, 1 Row for Set Buttons
		TableLayout->AutoSize = true;
		TableLayout->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
		TableLayout->Dock = DockStyle::Fill;
		TableLayout->BackColor = Color::Transparent;
		TableLayout->Margin = System::Windows::Forms::Padding(0, 0, 0, 0);
		TableLayout->Padding = System::Windows::Forms::Padding(0, 0, 0, 0);

		// Configure columns with fixed widths
		for (int i = 0; i < Control_ColorPreset::COUNT_PRESET_COLORS; i++) {
			TableLayout->ColumnStyles->Add(gcnew ColumnStyle(SizeType::Absolute, 45));  // 40px button + 5px margin
		}

		// Configure rows with fixed heights
		TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 45));  // Color buttons row
		
		if (_ShowSetButtons) {
			TableLayout->RowStyles->Add(gcnew RowStyle(SizeType::Absolute, 30));  // Set buttons row
		}

		// Initialize button arrays
		_ColorPresetButtons = gcnew array<Button^>(Control_ColorPreset::COUNT_PRESET_COLORS);
		
		if (_ShowSetButtons) {
			_SetButtons = gcnew array<Button^>(Control_ColorPreset::COUNT_PRESET_COLORS);
		}

		// Create all buttons
		for (int i = 0; i < Control_ColorPreset::COUNT_PRESET_COLORS; i++)
		{
			// Color preset button
			Button^ Color_Button = gcnew Button();
			Color_Button->Size = System::Drawing::Size(40, 40);
			Color_Button->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			Color_Button->FlatAppearance->BorderSize = 1;
			Color_Button->FlatAppearance->BorderColor = Theme_Manager::Get_Instance()->BorderPrimary;
			Color_Button->Margin = System::Windows::Forms::Padding(0);
			Color_Button->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left);
			Color_Button->BackColor	= Theme_Manager::Get_Instance()->BackgroundLight;
			Color_Button->Tag = i;
			Color_Button->Click += gcnew EventHandler(this, &Control_ColorPreset::Color_Preset_Button_Click);
			Color_Button->Paint += gcnew PaintEventHandler(this, &Control_ColorPreset::Color_Button_Paint);
			Color_Button->MouseEnter += gcnew EventHandler(this, &Control_ColorPreset::Button_MouseEnter);
			Color_Button->MouseLeave += gcnew EventHandler(this, &Control_ColorPreset::Button_MouseLeave);

			_ColorPresetButtons[i] = Color_Button;
			
			UpdateButtonIcon(i, _Colors[i]);
				

			TableLayout->Controls->Add(Color_Button, i, 0);

			if (_ShowSetButtons)
			{
				// Set button
				Button^ Set_Button = gcnew Button();
				Set_Button->Text = "Set";
				Set_Button->Size = System::Drawing::Size(40, 25);
				Set_Button->Margin = System::Windows::Forms::Padding(1, 3, 0, 0);
				Set_Button->Font = gcnew System::Drawing::Font(Set_Button->Font->FontFamily, 8.0f);
				Set_Button->TextAlign = ContentAlignment::MiddleCenter;
				Set_Button->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left);
				Set_Button->Tag = i;
				Theme_Manager::Get_Instance()->ApplyThemeToButton(Set_Button);

				Set_Button->Click += gcnew System::EventHandler(this, &Control_ColorPreset::Set_Button_Click);

				_SetButtons[i] = Set_Button;

				TableLayout->Controls->Add(Set_Button, i, 1);
			}
		}

		this->BackColor = Theme_Manager::Get_Instance()->Background;
		this->Controls->Add(TableLayout);
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

		settings->ColorPresetsString = colorList;
	}

	void Control_ColorPreset::Load_PresetColors()
	{
		Settings^ Settings = Settings::Get_Instance();
		List<Color>^ ColorList = Settings->ColorPresetsColor;

		for (int i = 0; i < Control_ColorPreset::COUNT_PRESET_COLORS; i++)
		{
			UpdateButtonIcon(i, ColorList[i]);
		}
	}

	void Control_ColorPreset::UpdateButtonIcon(int index, Color color)
	{
		Button^ button = _ColorPresetButtons[index];

		// Update color in array
		_Colors[index] = color;

		// If button already has an icon, dispose it
		if (button->Image != nullptr) {
			delete button->Image;
		}

		// Create icon and set it
		Drawing::Icon^ icon = CreateColorIcon(color, Control_ColorPreset::COLOR_CIRCLE_DIAMETER);
		button->Image = icon->ToBitmap();
		delete icon;

		// Center the icon in the button
		button->ImageAlign = ContentAlignment::MiddleCenter;

		// Create and assign tooltip
		ToolTip^ tip = gcnew ToolTip();
		tip->SetToolTip(button, "#" + color.R.ToString("X2") + color.G.ToString("X2") + color.B.ToString("X2"));
	}

	void Control_ColorPreset::Color_Preset_Button_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Button^ Color_Button = safe_cast<Button^>(sender);
		int index = safe_cast<int>(Color_Button->Tag);

		_SelectedIndex = index;
		_SelectedColor = _Colors[index];

		// Refresh all buttons to update highlighting
		for each (Button ^ button in _ColorPresetButtons) {
			button->Invalidate();
		}

		OnSelectedColorChanged();
	}

	void Control_ColorPreset::Set_Button_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Button^ Set_Button = safe_cast<Button^>(sender);
		int index = safe_cast<int>(Set_Button->Tag);

		UpdateButtonIcon(index, _SelectedColor);
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

		if (buttonIndex == _SelectedIndex) {
			borderPen = gcnew Pen(_HighlightColor, 2);
		}
		else if (button->ClientRectangle.Contains(button->PointToClient(Control::MousePosition))) {
			borderPen = gcnew Pen(Color::FromArgb(180, _HighlightColor), 2);
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

	void Control_ColorPreset::OnPresetColorsChanged()
	{
		PresetColorsChanged(this, EventArgs::Empty);
	}

	void Control_ColorPreset::OnSelectedColorChanged()
	{
		SelectedColorChanged(this, EventArgs::Empty);
	}

	Drawing::Icon^ Control_ColorPreset::CreateColorIcon(Color color, int diameter)
	{
		Bitmap^ bmp = CreateColorBitmap(color, diameter);

		IntPtr hicon = bmp->GetHicon();
		Drawing::Icon^ icon = Drawing::Icon::FromHandle(hicon);

		return icon;
	}

	Drawing::Bitmap^ Control_ColorPreset::CreateColorBitmap(Color color, int diameter)
	{
		Bitmap^ bmp = gcnew Bitmap(diameter, diameter);
		Graphics^ g = Graphics::FromImage(bmp);

		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Draw circle
		g->FillEllipse(gcnew SolidBrush(color), 0, 0, diameter - 1, diameter - 1);

		// Draw circle border
		g->DrawEllipse(gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong), 0, 0, diameter - 1, diameter - 1);

		delete g;
		return bmp;
	}

	Color Control_ColorPreset::SelectedColor::get()
	{ 
		return _SelectedColor;
	}

	void Control_ColorPreset::SelectedColor::set(Color color)
	{
		_SelectedColor = color;
	}

	void Control_ColorPreset::SelectedIndex::set(int index)
	{
		// Check if index is within valid range
		if (index >= 0 && index < _ColorPresetButtons->Length)
		{
			_SelectedIndex = index;
			_SelectedColor = _Colors[index];

			// Refresh all buttons to update highlighting
			for each(Button ^ button in _ColorPresetButtons) {
				button->Invalidate();
			}

			OnSelectedColorChanged();
		}
	}
}