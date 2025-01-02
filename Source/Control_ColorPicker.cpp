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

		_TextBox_Green = gcnew TextBox();
		_TextBox_Green->Width = TEXT_BOX_WIDTH;
		_TextBox_Green->Height = TEXT_BOX_HEIGHT;
		_TextBox_Green->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnRGBTextBoxValueChanged);

		_TextBox_Blue = gcnew TextBox();
		_TextBox_Blue->Width = TEXT_BOX_WIDTH;
		_TextBox_Blue->Height = TEXT_BOX_HEIGHT;
		_TextBox_Blue->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnRGBTextBoxValueChanged);

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
		_Radius_Wheel = (wheelSize / 2.0f);
		float innerRadius = _Radius_Wheel - RING_WIDTH; // Maintain exact 10-pixel ring width

		if (wheelSize <= 0) return;

		if (_Wheel_Bitmap != nullptr) {
			delete _Wheel_Bitmap;
			_Wheel_Bitmap = nullptr;
		}

		try {
			_Wheel_Bitmap = gcnew Bitmap(wheelSize, wheelSize);
			Graphics^ g = Graphics::FromImage(_Wheel_Bitmap);
			g->SmoothingMode = SmoothingMode::AntiAlias;
			g->Clear(Color::Transparent);

			// Draw color ring
			for (int x = 0; x < wheelSize; x++) {
				for (int y = 0; y < wheelSize; y++) {
					Point center(wheelSize / 2, wheelSize / 2);
					float dx = (float)(x - center.X);
					float dy = (float)(y - center.Y);
					float distance = (float)Math::Sqrt(dx * dx + dy * dy);

					if (distance <= _Radius_Wheel && distance >= innerRadius) {
						float angle = (float)(Math::Atan2(dy, dx) * 180.0 / Math::PI);
						if (angle < 0) angle += 360;
						Color color = ColorFromHSV(angle, 1.0f, 1.0f);
						_Wheel_Bitmap->SetPixel(x, y, color);
					}
				}
			}

			delete g;
		}
		catch (System::ArgumentException^) {
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
		String^ text = textBox->Text;
		if (String::IsNullOrEmpty(text)) return;

		int value;
		if (Int32::TryParse(text, value)) {
			if (value < 0) textBox->Text = "0";
			else if (value > 255) textBox->Text = "255";
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
		String^ validText = "";
		for (int i = 0; i < text->Length && i < 6; i++)
		{
			if (Char::IsDigit(text[i]) || (text[i] >= 'A' && text[i] <= 'F'))
			{
				validText += text[i];
			}
		}

		// If empty, return black
		if (String::IsNullOrEmpty(validText))
		{
			return "000000";
		}
		else if (validText->Length > 6) {
			return validText->Substring(0, 6);
		}
		else while (validText->Length < 6) {
			validText = "0" + validText;
		}

		return validText;
	}

	void Control_ColorPicker::Update_Color_From_Mouse(int x, int y)
	{
		// Calculate wheel position (same as in OnPaint)
		int controlsWidth = SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int wheelX = controlsWidth;
		int wheelY = SPACING;

		Point center(wheelX + _Wheel_Bitmap->Width / 2, wheelY + _Wheel_Bitmap->Height / 2);

		float dx = (float)(x - center.X);
		float dy = (float)(y - center.Y);
		float angle = (float)(Math::Atan2(dy, dx) * 180.0 / Math::PI);
		if (angle < 0) angle += 360;

		_Current_Hue = angle;

		Update_Sliders_Hue();
		this->Invalidate();
		OnColorChanged();
	}

	void Control_ColorPicker::Update_TextBox_Positions()
	{
		if (_Wheel_Bitmap == nullptr) return;

		// Calculate left-side margin
		int leftMargin	= SPACING * 2;
		int labelWidth	= 25;
		int textBoxX	= leftMargin + labelWidth;

		// Center RGB controls vertically relative to the wheel
		int wheelCenterY = SPACING + _Wheel_Bitmap->Height / 2;
		int totalControlsHeight = (TEXT_BOX_HEIGHT + SPACING) * 3 - SPACING;
		int startY = wheelCenterY - totalControlsHeight / 2;

		// Position RGB controls
		_Label_Red->Location = Point(leftMargin, startY + (TEXT_BOX_HEIGHT - _Label_Red->Height) / 2);
		_TextBox_Red->Location = Point(textBoxX, startY);

		_Label_Green->Location = Point(leftMargin, startY + TEXT_BOX_HEIGHT + SPACING + (TEXT_BOX_HEIGHT - _Label_Green->Height) / 2);
		_TextBox_Green->Location = Point(textBoxX, startY + TEXT_BOX_HEIGHT + SPACING);

		_Label_Blue->Location	= Point(leftMargin, startY + 2 * (TEXT_BOX_HEIGHT + SPACING) + (TEXT_BOX_HEIGHT - _Label_Blue->Height) / 2);
		_TextBox_Blue->Location = Point(textBoxX, startY + 2 * (TEXT_BOX_HEIGHT + SPACING));

		_Label_Hex->Location	= Point(leftMargin, startY + 3.5 * (TEXT_BOX_HEIGHT + SPACING) + (TEXT_BOX_HEIGHT - _Label_Hex->Height) / 2);
		_TextBox_Hex->Location	= Point(textBoxX, startY + 3.5 * (TEXT_BOX_HEIGHT + SPACING));
	}
	
	void Control_ColorPicker::Update_Slider_Positions()
	{
		if (_Wheel_Bitmap == nullptr) return;

		// Calculate wheel position
		int controlsWidth	= SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int wheelX			= controlsWidth;
		int wheelY			= SPACING;
		int wheelSize		= _Wheel_Bitmap->Width;

		// Use exact inner radius based on RING_WIDTH
		float innerRadius = _Radius_Wheel - RING_WIDTH;

		// Make sliders fit comfortably inside the inner circle
		// Use 80% of inner diameter for slider width to leave some padding
		int sliderWidth = (int)(innerRadius * 1.6f);

		// Center point of the wheel
		int centerX = wheelX + wheelSize / 2;
		int centerY = wheelY + wheelSize / 2;

		// Position sliders in the center of the wheel with proper spacing
		_Saturation_Slider->Width = sliderWidth;
		_Saturation_Slider->Location = Point(centerX - sliderWidth / 2, centerY - SLIDER_HEIGHT - SPACING / 2);

		_Value_Slider->Width = sliderWidth;
		_Value_Slider->Location = Point(centerX - sliderWidth / 2, centerY + SPACING / 2);
	}

	void Control_ColorPicker::Update_Text_Boxes()
	{
		if (!_Updating_Text_Boxes)
		{
			_Updating_Text_Boxes = true;

			Color currentColor = SelectedColor;

			_TextBox_Red->Text		= currentColor.R.ToString();
			_TextBox_Green->Text	= currentColor.G.ToString();
			_TextBox_Blue->Text		= currentColor.B.ToString();

			if(!_TextBox_Hex->Focused) {
				_TextBox_Hex->Text		= currentColor.R.ToString("X2") + currentColor.G.ToString("X2") + currentColor.B.ToString("X2");
			}

			_Updating_Text_Boxes	= false;
		}
	}

	void Control_ColorPicker::Update_From_RGB(int r, int g, int b)
	{
		float h, s, v;
		RGBtoHSV(r, g, b, h, s, v);

		_Current_Hue = h;
		_Current_Saturation = s;
		_Current_Value = v;

		_Saturation_Slider->Value = (int)(_Current_Saturation * 100);
		_Value_Slider->Value = (int)(_Current_Value * 100);

		_Saturation_Slider->Hue = _Current_Hue;
		_Value_Slider->Hue = _Current_Hue;

		Update_Sliders_Hue();
		this->Invalidate();
		OnColorChanged();
	}

	void Control_ColorPicker::Update_Sliders_Hue()
	{
		if (_Saturation_Slider != nullptr) {
			_Saturation_Slider->Hue = _Current_Hue;
			_Saturation_Slider->Brightness = _Current_Value;
			_Saturation_Slider->Invalidate();
		}
		if (_Value_Slider != nullptr) {
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

		int controlsWidth = SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int wheelX = controlsWidth;

		// Center wheel vertically
		int wheelY = (Height - _Wheel_Bitmap->Height) / 2;

		if (this->Enabled) {
			e->Graphics->DrawImage(_Wheel_Bitmap, wheelX, wheelY);
		}
		else {
			// Create grayscale version for disabled state
			ColorMatrix^ grayMatrix = gcnew ColorMatrix(
				gcnew array<array<float>^>{
				gcnew array<float>{0.3f, 0.3f, 0.3f, 0, 0},
					gcnew array<float>{0.3f, 0.3f, 0.3f, 0, 0},
					gcnew array<float>{0.3f, 0.3f, 0.3f, 0, 0},
					gcnew array<float>{0, 0, 0, 1, 0},
					gcnew array<float>{0.1f, 0.1f, 0.1f, 0, 1}
			}
			);

			ImageAttributes^ attributes = gcnew ImageAttributes();
			attributes->SetColorMatrix(grayMatrix);

			e->Graphics->DrawImage(_Wheel_Bitmap,
				Rectangle(wheelX, wheelY, _Wheel_Bitmap->Width, _Wheel_Bitmap->Height),
				0, 0, _Wheel_Bitmap->Width, _Wheel_Bitmap->Height,
				GraphicsUnit::Pixel, attributes);

			delete attributes;
			delete grayMatrix;
		}

		// Draw the selected point on the ring
		float angleRad = _Current_Hue * (float)Math::PI / 180.0f;
		int centerX = wheelX + _Wheel_Bitmap->Width / 2;
		int centerY = wheelY + _Wheel_Bitmap->Height / 2;
		float midRadius = _Radius_Wheel - (RING_WIDTH / 2.0f); // Center of the ring
		int pointX = centerX + (int)(midRadius * Math::Cos(angleRad));
		int pointY = centerY + (int)(midRadius * Math::Sin(angleRad));

		// Create larger selector (20 pixels diameter)
		int selectorSize = 20;
		Rectangle selectorRect(pointX - selectorSize / 2, pointY - selectorSize / 2,
			selectorSize, selectorSize);

		// Fill with current hue color at full saturation and value
		Color selectorColor = this->Enabled ? ColorFromHSV(_Current_Hue, 1.0f, 1.0f) : Color::Gray;

		SolidBrush^ fillBrush = gcnew SolidBrush(selectorColor);
		e->Graphics->FillEllipse(fillBrush, selectorRect);
		delete fillBrush;

		// Draw borders for better visibility
		Color borderColor = this->Enabled ? Color::Black : Color::LightGray;
		Pen^ whitePen = gcnew Pen(borderColor, 2);
		e->Graphics->DrawEllipse(whitePen, selectorRect);
		delete whitePen;

		Pen^ blackPen = gcnew Pen(this->Enabled ? Color::Black : Color::DarkGray, 1);
		e->Graphics->DrawEllipse(blackPen, selectorRect);
		delete blackPen;
	}

	void Control_ColorPicker::OnMouseDown(Object^ sender, MouseEventArgs^ e)
	{
		if (_Wheel_Bitmap == nullptr) return;

		// Calculate wheel position (same as in OnPaint)
		int controlsWidth = SPACING * 2 + 25 + TEXT_BOX_WIDTH + SPACING * 2;
		int wheelX = controlsWidth;
		int wheelY = SPACING;

		// Calculate relative coordinates
		Point center(wheelX + _Wheel_Bitmap->Width / 2, wheelY + _Wheel_Bitmap->Height / 2);

		float dx = (float)(e->X - center.X);
		float dy = (float)(e->Y - center.Y);
		float distance = (float)Math::Sqrt(dx * dx + dy * dy);
		float innerRadius = _Radius_Wheel - RING_WIDTH;

		if (distance <= _Radius_Wheel && distance >= innerRadius) {
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
		int minSize = 200; // Minimum reasonable size for the control
		if (Width < minSize) Width = minSize;
		if (Height < minSize) Height = minSize;

		Create_Wheel_Bitmap();
	}

	void Control_ColorPicker::OnColorChanged()
	{
		Update_Text_Boxes();
		ColorChanged(this, EventArgs::Empty);
	}

	void Control_ColorPicker::OnSliderValueChanged(Object^ sender, EventArgs^ e)
	{
		Control_ColorSlider^ slider = safe_cast<Control_ColorSlider^>(sender);

		if (slider == _Saturation_Slider) {
			_Current_Saturation = slider->Value / 100.0f;
			_Value_Slider->Saturation = _Current_Saturation;
		}
		else if (slider == _Value_Slider) {
			_Current_Value = slider->Value / 100.0f;
			_Saturation_Slider->Brightness = _Current_Value;
		}

		OnColorChanged();
	}

	void Control_ColorPicker::OnRGBTextBoxValueChanged(Object^ sender, EventArgs^ e)
	{
		if (_Updating_Text_Boxes) return;

		TextBox^ textBox = safe_cast<TextBox^>(sender);
		Validate_RGBTextBox_Input(textBox);

		try {
			int R = Int32::Parse(_TextBox_Red->Text);
			int G = Int32::Parse(_TextBox_Green->Text);
			int B = Int32::Parse(_TextBox_Blue->Text);

			if (R >= 0 && R <= 255 && G >= 0 && G <= 255 && B >= 0 && B <= 255) {
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

	void Control_ColorPicker::OnHexTextBoxValueChanged(Object^ sender, EventArgs^ e)
	{
		if (_Updating_Text_Boxes) return;

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
	
	Color Control_ColorPicker::ColorFromHSV(float hue, float saturation, float value) {
		int hi = (int)(hue / 60.0f) % 6;
		float f = (hue / 60.0f) - hi;
		float p = value * (1.0f - saturation);
		float q = value * (1.0f - f * saturation);
		float t = value * (1.0f - (1.0f - f) * saturation);

		switch (hi)
		{
			case 0: return Color::FromArgb(255, (int)(value * 255), (int)(t * 255), (int)(p * 255));
			case 1: return Color::FromArgb(255, (int)(q * 255), (int)(value * 255), (int)(p * 255));
			case 2: return Color::FromArgb(255, (int)(p * 255), (int)(value * 255), (int)(t * 255));
			case 3: return Color::FromArgb(255, (int)(p * 255), (int)(q * 255), (int)(value * 255));
			case 4: return Color::FromArgb(255, (int)(t * 255), (int)(p * 255), (int)(value * 255));
			default: return Color::FromArgb(255, (int)(value * 255), (int)(p * 255), (int)(q * 255));
		}
	}

	void Control_ColorPicker::RGBtoHSV(int r, int g, int b, float% h, float% s, float% v) {
		float rf = r / 255.0f;
		float gf = g / 255.0f;
		float bf = b / 255.0f;

		float cmax = Math::Max(rf, Math::Max(gf, bf));
		float cmin = Math::Min(rf, Math::Min(gf, bf));
		float delta = cmax - cmin;

		// Calculate hue
		if (delta == 0) {
			h = 0;
		}
		else if (cmax == rf) {
			h = 60.0f * ((gf - bf) / delta);
			if (h < 0) h += 360;
		}
		else if (cmax == gf) {
			h = 60.0f * (2 + (bf - rf) / delta);
		}
		else {
			h = 60.0f * (4 + (rf - gf) / delta);
		}

		// Calculate saturation
		s = (cmax == 0) ? 0 : delta / cmax;

		// Calculate value
		v = cmax;
	}


	////////////////
	// Properties //
	////////////////
	Color Control_ColorPicker::SelectedColor::get() {
		return ColorFromHSV(_Current_Hue, _Current_Saturation, _Current_Value);
	}

	void Control_ColorPicker::SelectedColor::set(Color color) {
		float h, s, v;
		RGBtoHSV(color.R, color.G, color.B, h, s, v);

		_Current_Hue = h;
		_Current_Saturation = s;
		_Current_Value = v;

		// Update sliders
		if (_Saturation_Slider != nullptr) {
			_Saturation_Slider->Value = (int)(s * 100);
			_Saturation_Slider->Hue = h;
			_Saturation_Slider->Brightness = v;
		}

		if (_Value_Slider != nullptr) {
			_Value_Slider->Value = (int)(v * 100);
			_Value_Slider->Hue = h;
			_Value_Slider->Saturation = s;
		}

		// Force redraw
		this->Invalidate();

		// Update text boxes and trigger change event
		Update_Text_Boxes();
		OnColorChanged();
	}
}