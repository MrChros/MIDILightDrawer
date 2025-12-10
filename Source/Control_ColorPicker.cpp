#include "Control_ColorPicker.h"

namespace MIDILightDrawer
{
	Control_ColorPicker::Control_ColorPicker()
	{
		this->DoubleBuffered	= true;
		this->MinimumSize		= System::Drawing::Size(200, 140);
		this->BackColor			= Color::Transparent;

		_Current_Hue			= 0.0f;
		_Current_Saturation		= 1.0f;
		_Current_Value			= 1.0f;
		_Is_Dragging_Wheel		= false;
		_Updating_Text_Boxes	= false;

		// Initialize all controls first
		Initialize_Text_Boxes();
		Initialize_Sliders();

		this->MouseDown += gcnew MouseEventHandler(this, &Control_ColorPicker::OnMouseDown);
		this->MouseMove += gcnew MouseEventHandler(this, &Control_ColorPicker::OnMouseMove);
		this->MouseUp	+= gcnew MouseEventHandler(this, &Control_ColorPicker::OnMouseUp);
		this->Resize	+= gcnew EventHandler(this, &Control_ColorPicker::OnResize);

		// Create wheel bitmap last
		Create_Wheel_Bitmap();
		Update_Text_Boxes();
	}

	void Control_ColorPicker::Initialize_Sliders()
	{
		// Initialize Saturation Slider
		_Saturation_Slider = gcnew Control_ColorSlider(SliderType::Saturation);
		_Saturation_Slider->Height = SLIDER_HEIGHT;
		_Saturation_Slider->ApplyTheme(Theme_Manager::Get_Instance()->BackgroundAlt);
		_Saturation_Slider->ValueChanged += gcnew EventHandler(this, &Control_ColorPicker::OnSliderValueChanged);

		// Initialize Value Slider
		_Value_Slider = gcnew Control_ColorSlider(SliderType::Value);
		_Value_Slider->Height = SLIDER_HEIGHT;
		_Value_Slider->ApplyTheme(Theme_Manager::Get_Instance()->BackgroundAlt);
		_Value_Slider->ValueChanged += gcnew EventHandler(this, &Control_ColorPicker::OnSliderValueChanged);

		this->Controls->Add(_Saturation_Slider);
		this->Controls->Add(_Value_Slider);

		Update_Slider_Positions();
	}

	void Control_ColorPicker::Initialize_Text_Boxes()
	{
		// Create labels
		_Label_Red = gcnew Label();
		_Label_Red->Text = "R:";
		_Label_Red->AutoSize = true;
		_Label_Red->ForeColor = Theme_Manager::Get_Instance()->ForegroundText;

		_Label_Green = gcnew Label();
		_Label_Green->Text = "G:";
		_Label_Green->AutoSize = true;
		_Label_Green->ForeColor = Theme_Manager::Get_Instance()->ForegroundText;
		
		_Label_Blue = gcnew Label();
		_Label_Blue->Text = "B:";
		_Label_Blue->AutoSize = true;
		_Label_Blue->ForeColor = Theme_Manager::Get_Instance()->ForegroundText;

		_Label_Hex = gcnew Label();
		_Label_Hex->Text = "#:";
		_Label_Hex->AutoSize = true;
		_Label_Hex->ForeColor = Theme_Manager::Get_Instance()->ForegroundText;

		// Create textboxes
		_TextBox_Red = gcnew TextBox();
		_TextBox_Red->Width = TEXT_BOX_WIDTH;
		_TextBox_Red->Height = TEXT_BOX_HEIGHT;
		_TextBox_Red->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnRGBTextBoxValueChanged);
		_TextBox_Red->KeyPress += gcnew KeyPressEventHandler(this, &Control_ColorPicker::OnRGBTextBoxKeyPress);

		_TextBox_Green = gcnew TextBox();
		_TextBox_Green->Width = TEXT_BOX_WIDTH;
		_TextBox_Green->Height = TEXT_BOX_HEIGHT;
		_TextBox_Green->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnRGBTextBoxValueChanged);
		_TextBox_Green->KeyPress += gcnew KeyPressEventHandler(this, &Control_ColorPicker::OnRGBTextBoxKeyPress);

		_TextBox_Blue = gcnew TextBox();
		_TextBox_Blue->Width = TEXT_BOX_WIDTH;
		_TextBox_Blue->Height = TEXT_BOX_HEIGHT;
		_TextBox_Blue->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnRGBTextBoxValueChanged);
		_TextBox_Blue->KeyPress += gcnew KeyPressEventHandler(this, &Control_ColorPicker::OnRGBTextBoxKeyPress);

		_TextBox_Hex = gcnew TextBox();
		_TextBox_Hex->Width = (int)(TEXT_BOX_WIDTH * 1.2);
		_TextBox_Hex->Height = TEXT_BOX_HEIGHT;
		_TextBox_Hex->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnHexTextBoxValueChanged);
		_TextBox_Hex->Leave += gcnew EventHandler(this, &Control_ColorPicker::OnHexTextBoxValueChanged);

		// Add controls in a specific order
		this->Controls->Add(_Label_Red);
		this->Controls->Add(_TextBox_Red);
		this->Controls->Add(_Label_Green);
		this->Controls->Add(_TextBox_Green);
		this->Controls->Add(_Label_Blue);
		this->Controls->Add(_TextBox_Blue);
		this->Controls->Add(_Label_Hex);
		this->Controls->Add(_TextBox_Hex);
	}

	void Control_ColorPicker::Create_Wheel_Bitmap()
	{
		// Calculate available space for wheel
		int controlsWidth = SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int availableWidth = Width - controlsWidth - SPACING * 2;
		int availableHeight = Height - SPACING * 2;

		// Use the smaller dimension to ensure the wheel stays circular and fits
		int wheelSize = Math::Min(availableWidth, availableHeight);
		_Radius_Wheel = ((float)wheelSize / 2.0f);
		float innerRadius = _Radius_Wheel - RING_WIDTH; // Maintain exact 10-pixel ring width

		if (wheelSize <= 0) return;

		if (_Wheel_Bitmap != nullptr) {
			delete _Wheel_Bitmap;
			_Wheel_Bitmap = nullptr;
		}

		try {
			_Wheel_Bitmap = gcnew Bitmap(wheelSize, wheelSize);
			Graphics^ G = Graphics::FromImage(_Wheel_Bitmap);
			G->SmoothingMode = SmoothingMode::AntiAlias;
			G->Clear(Color::Transparent);

			// Draw color ring
			for (int x = 0; x < wheelSize; x++)
			{
				for (int y = 0; y < wheelSize; y++)
				{
					Point Center(wheelSize / 2, wheelSize / 2);
					float dx = (float)(x - Center.X);
					float dy = (float)(y - Center.Y);
					float Distance = (float)Math::Sqrt(dx * dx + dy * dy);

					if (Distance <= _Radius_Wheel && Distance >= innerRadius)
					{
						float Angle = (float)(Math::Atan2(dy, dx) * 180.0 / Math::PI);
						if (Angle < 0) {
							Angle += 360;
						}
						Color Col = ColorFromHSV(Angle, 1.0f, 1.0f);
						_Wheel_Bitmap->SetPixel(x, y, Col);
					}
				}
			}

			delete G;
		}
		catch (System::ArgumentException^)
		{
			if (_Wheel_Bitmap != nullptr) {
				delete _Wheel_Bitmap;
				_Wheel_Bitmap = nullptr;
			}
		}

		Update_Slider_Positions();
		Update_TextBox_Positions();
	}

	void Control_ColorPicker::Validate_RGBTextBox_Input(TextBox^ textBox)
	{
		String^ Text = textBox->Text;
		if (String::IsNullOrEmpty(Text)) {
			return;
		}

		int Value;
		if (Int32::TryParse(Text, Value)) {
			if (Value < 0) {
				textBox->Text = "0";
			}
			else if (Value > 255) {
				textBox->Text = "255";
			}
		}
		else {
			textBox->Text = "0";
		}
	}

	void Control_ColorPicker::Validate_HexTextBox_Input(TextBox^ textBox)
	{
		if (!textBox->Focused)
		{
			textBox->Text = Get_Valid_Hex_Color(textBox->Text);
		}
	}

	String^ Control_ColorPicker::Get_Valid_Hex_Color(String^ input)
	{
		// Remove any whitespace and convert to uppercase
		String^ text = input->Replace(" ", "")->ToUpper();

		// Filter out invalid characters
		String^ ValidText = "";
		for (int i = 0; i < text->Length && i < 6; i++)
		{
			if (Char::IsDigit(text[i]) || (text[i] >= 'A' && text[i] <= 'F'))
			{
				ValidText += text[i];
			}
		}

		// If empty, return black
		if (String::IsNullOrEmpty(ValidText))
		{
			return "000000";
		}
		else if (ValidText->Length > 6) {
			return ValidText->Substring(0, 6);
		}
		else while (ValidText->Length < 6) {
			ValidText = "0" + ValidText;
		}

		return ValidText;
	}

	void Control_ColorPicker::Update_Color_From_Mouse(int x, int y)
	{
		// Calculate wheel position (same as in OnPaint)
		int Controls_Width = SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int Wheel_X = Controls_Width;
		int Wheel_Y = SPACING;

		Point Center(Wheel_X + _Wheel_Bitmap->Width / 2, Wheel_Y + _Wheel_Bitmap->Height / 2);

		float dx = (float)(x - Center.X);
		float dy = (float)(y - Center.Y);
		float Angle = (float)(Math::Atan2(dy, dx) * 180.0 / Math::PI);
		if (Angle < 0) {
			Angle += 360;
		}

		_Current_Hue = Angle;

		Update_Sliders_Hue();

		this->Invalidate();

		OnColorChanged();
	}

	void Control_ColorPicker::Update_TextBox_Positions()
	{
		if (_Wheel_Bitmap == nullptr) {
			return;
		}

		// Calculate left-side margin
		int Left_Margin	= SPACING * 2;
		int Label_Width	= 25;
		int Text_Box_X	= Left_Margin + Label_Width;

		// Center RGB controls vertically relative to the wheel
		int Wheel_Center_Y = SPACING + _Wheel_Bitmap->Height / 2;
		int Total_Controls_Height = (TEXT_BOX_HEIGHT + SPACING) * 3 - SPACING;
		int Start_Y = Wheel_Center_Y - Total_Controls_Height / 2;

		// Position RGB controls
		_Label_Red->Location = Point(Left_Margin, Start_Y + (TEXT_BOX_HEIGHT - _Label_Red->Height) / 2);
		_TextBox_Red->Location = Point(Text_Box_X, Start_Y);

		_Label_Green->Location = Point(Left_Margin, Start_Y + TEXT_BOX_HEIGHT + SPACING + (TEXT_BOX_HEIGHT - _Label_Green->Height) / 2);
		_TextBox_Green->Location = Point(Text_Box_X, Start_Y + TEXT_BOX_HEIGHT + SPACING);

		_Label_Blue->Location	= Point(Left_Margin, Start_Y + 2 * (TEXT_BOX_HEIGHT + SPACING) + (TEXT_BOX_HEIGHT - _Label_Blue->Height) / 2);
		_TextBox_Blue->Location = Point(Text_Box_X, Start_Y + 2 * (TEXT_BOX_HEIGHT + SPACING));

		_Label_Hex->Location = Point(Left_Margin, (int)(Start_Y + 3.5 * (TEXT_BOX_HEIGHT + SPACING) + (TEXT_BOX_HEIGHT - _Label_Hex->Height) / 2));
		_TextBox_Hex->Location = Point(Text_Box_X, (int)(Start_Y + 3.5 * (TEXT_BOX_HEIGHT + SPACING)));
	}
	
	void Control_ColorPicker::Update_Slider_Positions()
	{
		if (_Wheel_Bitmap == nullptr) {
			return;
		}

		// Calculate wheel position
		int Controls_Width	= SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int Wheel_X			= Controls_Width;
		int Wheel_Y			= SPACING;
		int Wheel_Size		= _Wheel_Bitmap->Width;

		// Use exact inner radius based on RING_WIDTH
		float Inner_Radius = _Radius_Wheel - RING_WIDTH;

		// Make sliders fit comfortably inside the inner circle
		// Use 80% of inner diameter for slider width to leave some padding
		int Slider_Width = (int)(Inner_Radius * 1.6f);

		// Center point of the wheel
		int Center_X = Wheel_X + Wheel_Size / 2;
		int Center_Y = Wheel_Y + Wheel_Size / 2;

		// Position sliders in the center of the wheel with proper spacing
		_Saturation_Slider->Width = Slider_Width;
		_Saturation_Slider->Location = Point(Center_X - Slider_Width / 2, Center_Y - SLIDER_HEIGHT - SPACING / 2);

		_Value_Slider->Width = Slider_Width;
		_Value_Slider->Location = Point(Center_X - Slider_Width / 2, Center_Y + SPACING / 2);
	}

	void Control_ColorPicker::Update_Text_Boxes()
	{
		if (!_Updating_Text_Boxes)
		{
			_Updating_Text_Boxes = true;

			Color Current_Color = SelectedColor;

			_TextBox_Red->Text		= Current_Color.R.ToString();
			_TextBox_Green->Text	= Current_Color.G.ToString();
			_TextBox_Blue->Text		= Current_Color.B.ToString();

			if(!_TextBox_Hex->Focused) {
				_TextBox_Hex->Text		= Current_Color.R.ToString("X2") + Current_Color.G.ToString("X2") + Current_Color.B.ToString("X2");
			}

			_Updating_Text_Boxes = false;
		}
	}

	void Control_ColorPicker::Update_From_RGB(int r, int g, int b)
	{
		float H, S, V;
		RGBtoHSV(r, g, b, H, S, V);

		_Current_Hue = H;
		_Current_Saturation = S;
		_Current_Value = V;

		_Saturation_Slider->Value = _Current_Saturation;
		_Value_Slider->Value = _Current_Value;

		Update_Sliders_Hue();
		this->Invalidate();
		OnColorChanged();
	}

	void Control_ColorPicker::Update_Sliders_Hue()
	{
		if (_Saturation_Slider != nullptr)
		{
			_Saturation_Slider->Hue = _Current_Hue;
			_Saturation_Slider->Brightness = _Current_Value;

			_Saturation_Slider->Invalidate();
		}

		if (_Value_Slider != nullptr)
		{
			_Value_Slider->Hue = _Current_Hue;
			_Value_Slider->Saturation = _Current_Saturation;

			_Value_Slider->Invalidate();
		}
	}

	void Control_ColorPicker::OnPaint(PaintEventArgs^ e)
	{
		if (_Wheel_Bitmap == nullptr) {
			return;
		}

		int Controls_Width = SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int Wheel_X = Controls_Width;

		// Center wheel vertically
		int Wheel_Y = (Height - _Wheel_Bitmap->Height) / 2;

		if (this->Enabled) {
			e->Graphics->DrawImage(_Wheel_Bitmap, Wheel_X, Wheel_Y);
		}
		else
		{
			// Create grayscale version for disabled state
			ColorMatrix^ Gray_Matrix = gcnew ColorMatrix(
				gcnew array<array<float>^>{
					gcnew array<float>{ 0.3f, 0.3f, 0.3f, 0, 0 },
					gcnew array<float>{ 0.3f, 0.3f, 0.3f, 0, 0 },
					gcnew array<float>{ 0.3f, 0.3f, 0.3f, 0, 0 },
					gcnew array<float>{ 0, 0, 0, 1, 0 },
					gcnew array<float>{ 0.1f, 0.1f, 0.1f, 0, 1 }
				}
			);

			ImageAttributes^ Attributes = gcnew ImageAttributes();
			Attributes->SetColorMatrix(Gray_Matrix);

			e->Graphics->DrawImage(_Wheel_Bitmap, Rectangle(Wheel_X, Wheel_Y, _Wheel_Bitmap->Width, _Wheel_Bitmap->Height), 0, 0, _Wheel_Bitmap->Width, _Wheel_Bitmap->Height, GraphicsUnit::Pixel, Attributes);

			delete Attributes;
			delete Gray_Matrix;
		}

		// Draw the selected point on the ring
		float Angle_Rad = _Current_Hue * (float)Math::PI / 180.0f;
		int Center_X = Wheel_X + _Wheel_Bitmap->Width / 2;
		int Center_Y = Wheel_Y + _Wheel_Bitmap->Height / 2;
		float Mid_Radius = _Radius_Wheel - (RING_WIDTH / 2.0f); // Center of the ring
		int Point_X = Center_X + (int)(Mid_Radius * Math::Cos(Angle_Rad));
		int Point_Y = Center_Y + (int)(Mid_Radius * Math::Sin(Angle_Rad));

		// Create larger selector (20 pixels diameter)
		int Selector_Size = 20;
		Rectangle Selector_Rect(Point_X - Selector_Size / 2, Point_Y - Selector_Size / 2, Selector_Size, Selector_Size);

		// Fill with current hue color at full saturation and value
		Color Selector_Color = this->Enabled ? ColorFromHSV(_Current_Hue, 1.0f, 1.0f) : Color::Gray;

		SolidBrush^ Fill_Brush = gcnew SolidBrush(Selector_Color);
		e->Graphics->FillEllipse(Fill_Brush, Selector_Rect);
		delete Fill_Brush;

		// Draw borders for better visibility
		Color Border_Color = this->Enabled ? Color::Black : Color::LightGray;
		Pen^ White_Pen = gcnew Pen(Border_Color, 2);
		e->Graphics->DrawEllipse(White_Pen, Selector_Rect);
		delete White_Pen;

		Pen^ Black_Pen = gcnew Pen(this->Enabled ? Color::Black : Color::DarkGray, 1);
		e->Graphics->DrawEllipse(Black_Pen, Selector_Rect);
		delete Black_Pen;
	}

	void Control_ColorPicker::OnMouseDown(Object^ sender, MouseEventArgs^ e)
	{
		if (_Wheel_Bitmap == nullptr) {
			return;
		}

		// Calculate wheel position (same as in OnPaint)
		int Controls_Width = SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int Wheel_X = Controls_Width;
		int Wheel_Y = SPACING;

		// Calculate relative coordinates
		Point Center(Wheel_X + _Wheel_Bitmap->Width / 2, Wheel_Y + _Wheel_Bitmap->Height / 2);

		float dx = (float)(e->X - Center.X);
		float dy = (float)(e->Y - Center.Y);
		float Distance = (float)Math::Sqrt(dx * dx + dy * dy);
		float Inner_Radius = _Radius_Wheel - RING_WIDTH;

		if (Distance <= _Radius_Wheel && Distance >= Inner_Radius) {
			_Is_Dragging_Wheel = true;
			Update_Color_From_Mouse(e->X, e->Y);
		}
	}

	void Control_ColorPicker::OnMouseMove(Object^ sender, MouseEventArgs^ e)
	{
		if (_Is_Dragging_Wheel) {
			Update_Color_From_Mouse(e->X, e->Y);
		}
	}

	void Control_ColorPicker::OnMouseUp(Object^ sender, MouseEventArgs^ e)
	{
		_Is_Dragging_Wheel = false;
	}

	void Control_ColorPicker::OnResize(Object^ sender, EventArgs^ e)
	{
		// Update minimum size
		int Min_Size = 200; // Minimum reasonable size for the control
		if (Width < Min_Size) {
			Width = Min_Size;
		}
		if (Height < Min_Size) {
			Height = Min_Size;
		}

		Create_Wheel_Bitmap();
	}

	void Control_ColorPicker::OnColorChanged()
	{
		Update_Text_Boxes();
		ColorChanged(this, EventArgs::Empty);
	}

	void Control_ColorPicker::OnSliderValueChanged(Object^ sender, EventArgs^ e)
	{
		Control_ColorSlider^ Slider = safe_cast<Control_ColorSlider^>(sender);

		if (Slider == _Saturation_Slider) {
			_Current_Saturation = Slider->Value;
			_Value_Slider->Saturation = _Current_Saturation;
		}
		else if (Slider == _Value_Slider) {
			_Current_Value = Slider->Value;
			_Saturation_Slider->Brightness = _Current_Value;
		}

		OnColorChanged();
	}

	void Control_ColorPicker::OnRGBTextBoxValueChanged(Object^ sender, EventArgs^ e)
	{
		if (_Updating_Text_Boxes) {
			return;
		}

		TextBox^ Text_Box = safe_cast<TextBox^>(sender);
		
		Validate_RGBTextBox_Input(Text_Box);

		try {
			int R = Int32::Parse(_TextBox_Red->Text);
			int G = Int32::Parse(_TextBox_Green->Text);
			int B = Int32::Parse(_TextBox_Blue->Text);

			if ((R >= 0 && R <= 255) &&
				(G >= 0 && G <= 255) &&
				(B >= 0 && B <= 255))
			{
				Update_From_RGB(R, G, B);
			}
		}
		catch (FormatException^) {
			// Invalid input - ignore
		}
		catch (OverflowException^) {
			// Invalid input - ignore
		}
	}

	void Control_ColorPicker::OnRGBTextBoxKeyPress(Object^ sender, KeyPressEventArgs^ e)
	{
		if (_Updating_Text_Boxes) {
			return;
		}

		TextBox^ Text_Box = safe_cast<TextBox^>(sender);
		
		if ((int)e->KeyChar == (int)Keys::Back ||
			(int)e->KeyChar == (int)Keys::Delete)
		{
			return;
		}

		if ((int)e->KeyChar < (int)Keys::D0 || (int)e->KeyChar >(int)Keys::D9 || Text_Box->Text->Length >= 3) {
			e->Handled = true;
		}
	}

	void Control_ColorPicker::OnHexTextBoxValueChanged(Object^ sender, EventArgs^ e)
	{
		if (_Updating_Text_Boxes) {
			return;
		}

		TextBox^ Text_Box = safe_cast<TextBox^>(sender);
		String^ Valid_Color = Get_Valid_Hex_Color(Text_Box->Text);

		try {
			int R = Int32::Parse(Valid_Color->Substring(0, 2), System::Globalization::NumberStyles::HexNumber);
			int G = Int32::Parse(Valid_Color->Substring(2, 2), System::Globalization::NumberStyles::HexNumber);
			int B = Int32::Parse(Valid_Color->Substring(4, 2), System::Globalization::NumberStyles::HexNumber);

			if (R >= 0 && R <= 255 && G >= 0 && G <= 255 && B >= 0 && B <= 255)
			{
				Update_From_RGB(R, G, B);
			}
		}
		catch (FormatException^) {
			// Invalid input - ignore
		}
		catch (OverflowException^) {
			// Invalid input - ignore
		}
	}
	
	Color Control_ColorPicker::ColorFromHSV(float hue, float saturation, float value)
	{
		int Hi = (int)(hue / 60.0f) % 6;
		float F = (hue / 60.0f) - Hi;
		float P = value * (1.0f - saturation);
		float Q = value * (1.0f - F * saturation);
		float T = value * (1.0f - (1.0f - F) * saturation);

		switch (Hi)
		{
			case 0: return Color::FromArgb(255, (int)(value * 255), (int)(T * 255), (int)(P * 255));
			case 1: return Color::FromArgb(255, (int)(Q * 255), (int)(value * 255), (int)(P * 255));
			case 2: return Color::FromArgb(255, (int)(P * 255), (int)(value * 255), (int)(T * 255));
			case 3: return Color::FromArgb(255, (int)(P * 255), (int)(Q * 255), (int)(value * 255));
			case 4: return Color::FromArgb(255, (int)(T * 255), (int)(P * 255), (int)(value * 255));
			default: return Color::FromArgb(255, (int)(value * 255), (int)(P * 255), (int)(Q * 255));
		}
	}

	void Control_ColorPicker::RGBtoHSV(int r, int g, int b, float% h, float% s, float% v)
	{
		float Rf = r / 255.0f;
		float Gf = g / 255.0f;
		float Bf = b / 255.0f;

		float C_Max = Math::Max(Rf, Math::Max(Gf, Bf));
		float C_Min = Math::Min(Rf, Math::Min(Gf, Bf));
		float Delta = C_Max - C_Min;

		// Calculate hue
		if (Delta == 0) {
			h = 0;
		}
		else if (C_Max == Rf) {
			h = 60.0f * ((Gf - Bf) / Delta);
			if (h < 0) h += 360;
		}
		else if (C_Max == Gf) {
			h = 60.0f * (2 + (Bf - Rf) / Delta);
		}
		else {
			h = 60.0f * (4 + (Rf - Gf) / Delta);
		}

		// Calculate saturation
		s = (C_Max == 0) ? 0 : Delta / C_Max;

		// Calculate value
		v = C_Max;
	}


	////////////////
	// Properties //
	////////////////
	Color Control_ColorPicker::SelectedColor::get()
	{
		return ColorFromHSV(_Current_Hue, _Current_Saturation, _Current_Value);
	}

	bool Control_ColorPicker::IsTyping::get()
	{
		return _TextBox_Red->Focused || _TextBox_Green->Focused || _TextBox_Blue->Focused || _TextBox_Hex->Focused;
	}

	void Control_ColorPicker::SelectedColor::set(Color color)
	{
		float H, S, V;
		RGBtoHSV(color.R, color.G, color.B, H, S, V);

		_Current_Hue = H;
		_Current_Saturation = S;
		_Current_Value = V;

		// Update sliders
		if (_Saturation_Slider != nullptr)
		{
			_Saturation_Slider->Value = S;
			_Saturation_Slider->Hue = H;
			_Saturation_Slider->Brightness = V;
		}

		if (_Value_Slider != nullptr)
		{
			_Value_Slider->Value = V;
			_Value_Slider->Hue = H;
			_Value_Slider->Saturation = S;
		}

		// Force redraw
		this->Invalidate();

		// Update text boxes and trigger change event
		Update_Text_Boxes();
		OnColorChanged();
	}
}