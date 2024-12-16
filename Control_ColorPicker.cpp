#include "Control_ColorPicker.h"

namespace MIDILightDrawer
{
	Control_ColorPicker::Control_ColorPicker()
	{
		this->DoubleBuffered = true;
		this->MinimumSize = System::Drawing::Size(200, 140);
		this->Size = System::Drawing::Size(300, 200 + 2 * (SLIDER_HEIGHT + SPACING));
		this->BackColor = Color::Transparent;

		_Current_Hue = 0.0f;
		_Current_Saturation = 1.0f;
		_Current_Value = 1.0f;
		_Is_Dragging_Wheel = false;
		_Updating_Text_Boxes = false;

		// Initialize all controls first
		Initialize_Text_Boxes();
		Initialize_Sliders();

		this->MouseDown += gcnew MouseEventHandler(this, &Control_ColorPicker::OnMouseDown);
		this->MouseMove += gcnew MouseEventHandler(this, &Control_ColorPicker::OnMouseMove);
		this->MouseUp += gcnew MouseEventHandler(this, &Control_ColorPicker::OnMouseUp);
		this->Resize += gcnew EventHandler(this, &Control_ColorPicker::OnResize);

		// Create wheel bitmap last
		Create_Wheel_Bitmap();
	}

	void Control_ColorPicker::Initialize_Sliders()
	{
		// Initialize Saturation Slider
		_Saturation_Slider = gcnew Control_ColorSlider(SliderType::Saturation);
		_Saturation_Slider->Height = SLIDER_HEIGHT;
		_Saturation_Slider->ValueChanged += gcnew EventHandler(this, &Control_ColorPicker::OnSliderValueChanged);

		// Initialize Value Slider
		_Value_Slider = gcnew Control_ColorSlider(SliderType::Value);
		_Value_Slider->Height = SLIDER_HEIGHT;
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

		_Label_Green = gcnew Label();
		_Label_Green->Text = "G:";
		_Label_Green->AutoSize = true;

		_Label_Blue = gcnew Label();
		_Label_Blue->Text = "B:";
		_Label_Blue->AutoSize = true;

		// Create textboxes
		_TextBox_Red = gcnew TextBox();
		_TextBox_Red->Width = TEXT_BOX_WIDTH;
		_TextBox_Red->Height = TEXT_BOX_HEIGHT;
		_TextBox_Red->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnTextBoxValueChanged);

		_TextBox_Green = gcnew TextBox();
		_TextBox_Green->Width = TEXT_BOX_WIDTH;
		_TextBox_Green->Height = TEXT_BOX_HEIGHT;
		_TextBox_Green->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnTextBoxValueChanged);

		_TextBox_Blue = gcnew TextBox();
		_TextBox_Blue->Width = TEXT_BOX_WIDTH;
		_TextBox_Blue->Height = TEXT_BOX_HEIGHT;
		_TextBox_Blue->TextChanged += gcnew EventHandler(this, &Control_ColorPicker::OnTextBoxValueChanged);

		// Add controls in a specific order
		this->Controls->Add(_Label_Red);
		this->Controls->Add(_TextBox_Red);
		this->Controls->Add(_Label_Green);
		this->Controls->Add(_TextBox_Green);
		this->Controls->Add(_Label_Blue);
		this->Controls->Add(_TextBox_Blue);

		// Set initial positions after all controls are created
		int textStartX = Width - TEXT_BOX_WIDTH - SPACING * 2;
		int textStartY = SPACING;

		_Label_Red->Location = Point(textStartX, textStartY);
		_TextBox_Red->Location = Point(textStartX + 25, textStartY);

		_Label_Green->Location = Point(textStartX, textStartY + TEXT_BOX_HEIGHT + SPACING);
		_TextBox_Green->Location = Point(textStartX + 25, textStartY + TEXT_BOX_HEIGHT + SPACING);

		_Label_Blue->Location = Point(textStartX, textStartY + 2 * (TEXT_BOX_HEIGHT + SPACING));
		_TextBox_Blue->Location = Point(textStartX + 25, textStartY + 2 * (TEXT_BOX_HEIGHT + SPACING));
	}

	void Control_ColorPicker::Create_Wheel_Bitmap()
	{
		// Make wheel fill the width of the control minus padding
		int wheelSize = Math::Min(Width - 2 * SPACING, Height - 3 * SPACING - TEXT_BOX_HEIGHT);
		_Radius_Wheel = (wheelSize / 2.0f);
		float innerRadius = _Radius_Wheel - RING_WIDTH;

		if (wheelSize <= 0) return;

		if (_Wheel_Bitmap != nullptr) {
			delete _Wheel_Bitmap;
			_Wheel_Bitmap = nullptr;
		}

		try {
			_Wheel_Bitmap = gcnew Bitmap(wheelSize, wheelSize);
			Graphics^ g = Graphics::FromImage(_Wheel_Bitmap);
			g->SmoothingMode = SmoothingMode::AntiAlias;
			g->Clear(Color::Transparent); // Use transparent background

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

	void Control_ColorPicker::Validate_TextBox_Input(TextBox^ textBox)
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

	void Control_ColorPicker::Update_Color_From_Mouse(int x, int y)
	{
		int wheelX = (Width - _Wheel_Bitmap->Width) / 2;
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

		int wheelBottom = SPACING + _Wheel_Bitmap->Height;

		// Calculate the total width needed for all controls
		int controlGroup = TEXT_BOX_WIDTH + 25; // Width of textbox + label
		int totalWidth = (controlGroup * 3) + (SPACING * 2); // 3 groups with 2 spaces between

		// Start position for first control group
		int startX = (Width - totalWidth) / 2;
		int y = wheelBottom + SPACING;

		// Calculate vertical alignment for labels
		int labelOffset = (TEXT_BOX_HEIGHT - _Label_Red->Height) / 2;

		// Position all controls in a row
		// R controls
		_Label_Red->Location = Point(startX, y + labelOffset);
		_TextBox_Red->Location = Point(startX + 25, y);

		// G controls
		startX += controlGroup + SPACING;
		_Label_Green->Location = Point(startX, y + labelOffset);
		_TextBox_Green->Location = Point(startX + 25, y);

		// B controls
		startX += controlGroup + SPACING;
		_Label_Blue->Location = Point(startX, y + labelOffset);
		_TextBox_Blue->Location = Point(startX + 25, y);
	}
	
	void Control_ColorPicker::Update_Slider_Positions()
	{
		if (_Wheel_Bitmap == nullptr) return;

		int wheelSize = _Wheel_Bitmap->Width;
		float innerRadius = _Radius_Wheel - RING_WIDTH;

		// Calculate slider width to fit inside the ring
		int sliderWidth = (int)(innerRadius * 1.4f);

		// Center point of the wheel (not the control)
		int wheelX = (Width - wheelSize) / 2;
		int centerX = wheelX + wheelSize / 2;
		int centerY = SPACING + wheelSize / 2;

		// Position sliders in the center of the ring
		_Saturation_Slider->Width = sliderWidth;
		_Saturation_Slider->Location = Point(
			centerX - sliderWidth / 2,
			centerY - SLIDER_HEIGHT - SPACING / 2
		);

		_Value_Slider->Width = sliderWidth;
		_Value_Slider->Location = Point(
			centerX - sliderWidth / 2,
			centerY + SPACING / 2
		);
	}

	void Control_ColorPicker::Update_Text_Boxes()
	{
		if (!_Updating_Text_Boxes) {
			_Updating_Text_Boxes = true;
			Color currentColor = SelectedColor;
			_TextBox_Red->Text = currentColor.R.ToString();
			_TextBox_Green->Text = currentColor.G.ToString();
			_TextBox_Blue->Text = currentColor.B.ToString();
			_Updating_Text_Boxes = false;
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
		if (_Wheel_Bitmap != nullptr) {
			// Center the wheel horizontally and vertically in the available space
			int x = (Width - _Wheel_Bitmap->Width) / 2;
			int y = SPACING;

			if (this->Enabled) {
				e->Graphics->DrawImage(_Wheel_Bitmap, x, y);
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
					Rectangle(x, y, _Wheel_Bitmap->Width, _Wheel_Bitmap->Height),
					0, 0, _Wheel_Bitmap->Width, _Wheel_Bitmap->Height,
					GraphicsUnit::Pixel, attributes);

				delete attributes;
				delete grayMatrix;
			}

			// Draw the selected point on the ring
			float angleRad = _Current_Hue * (float)Math::PI / 180.0f;
			int centerX = x + _Wheel_Bitmap->Width / 2;
			int centerY = y + _Wheel_Bitmap->Height / 2;
			int pointX = centerX + (int)((_Radius_Wheel - RING_WIDTH / 2) * Math::Cos(angleRad));
			int pointY = centerY + (int)((_Radius_Wheel - RING_WIDTH / 2) * Math::Sin(angleRad));

			// Create larger selector (20 pixels diameter)
			int selectorSize = 20;
			Rectangle selectorRect(pointX - selectorSize / 2, pointY - selectorSize / 2,
				selectorSize, selectorSize);

			// Fill with current hue color at full saturation and value
			Color selectorColor;
			if (this->Enabled) {
				selectorColor = ColorFromHSV(_Current_Hue, 1.0f, 1.0f);
			}
			else {
				selectorColor = Color::Gray;
			}

			SolidBrush^ fillBrush = gcnew SolidBrush(selectorColor);
			e->Graphics->FillEllipse(fillBrush, selectorRect);
			delete fillBrush;

			// Draw borders for better visibility
			Color borderColor = this->Enabled ? Color::White : Color::LightGray;
			Pen^ whitePen = gcnew Pen(borderColor, 2);
			e->Graphics->DrawEllipse(whitePen, selectorRect);
			delete whitePen;

			Pen^ blackPen = gcnew Pen(this->Enabled ? Color::Black : Color::DarkGray, 1);
			e->Graphics->DrawEllipse(blackPen, selectorRect);
			delete blackPen;
		}
	}

	void Control_ColorPicker::OnMouseDown(Object^ sender, MouseEventArgs^ e)
	{
		if (_Wheel_Bitmap == nullptr) return;

		int wheelX = (Width - _Wheel_Bitmap->Width) / 2;
		int wheelY = SPACING;
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

	void Control_ColorPicker::OnTextBoxValueChanged(Object^ sender, EventArgs^ e) {
		if (_Updating_Text_Boxes) return;

		TextBox^ textBox = safe_cast<TextBox^>(sender);
		Validate_TextBox_Input(textBox);

		try {
			int r = Int32::Parse(_TextBox_Red->Text);
			int g = Int32::Parse(_TextBox_Green->Text);
			int b = Int32::Parse(_TextBox_Blue->Text);

			if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
				Update_From_RGB(r, g, b);
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

		switch (hi) {
		case 0: return Color::FromArgb(255,
			(int)(value * 255), (int)(t * 255), (int)(p * 255));
		case 1: return Color::FromArgb(255,
			(int)(q * 255), (int)(value * 255), (int)(p * 255));
		case 2: return Color::FromArgb(255,
			(int)(p * 255), (int)(value * 255), (int)(t * 255));
		case 3: return Color::FromArgb(255,
			(int)(p * 255), (int)(q * 255), (int)(value * 255));
		case 4: return Color::FromArgb(255,
			(int)(t * 255), (int)(p * 255), (int)(value * 255));
		default: return Color::FromArgb(255,
			(int)(value * 255), (int)(p * 255), (int)(q * 255));
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