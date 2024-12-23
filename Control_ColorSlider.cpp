#include "Control_ColorSlider.h"

namespace MIDILightDrawer
{
	Control_ColorSlider::Control_ColorSlider(SliderType type)
		: _Type(type), _Value(100), _Hue(0), _Saturation(1.0f), _Brightness(1.0f)
	{
		this->SetStyle(ControlStyles::UserPaint |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::ResizeRedraw, true);

		this->Height = 20;
		this->MinimumSize = Drawing::Size(50, 20);
		_IsDragging = false;
	}

	void Control_ColorSlider::ApplyTheme(Color backgroundColor)
	{
		this->BackColor = backgroundColor;
	}

	void Control_ColorSlider::OnPaint(PaintEventArgs^ e)
	{
		Graphics^ g = e->Graphics;
		g->SmoothingMode = SmoothingMode::AntiAlias;

		// Draw track
		int trackHeight = 4;
		int trackY = (Height - trackHeight) / 2;
		Rectangle trackRect(0, trackY, Width, trackHeight);

		if (this->Enabled)
		{
			// Create gradient based on slider type
			LinearGradientBrush^ brush;
			if (_Type == SliderType::Saturation) {
				brush = gcnew LinearGradientBrush(
					trackRect,
					ColorFromHSV(_Hue, 0.0f, _Brightness),
					ColorFromHSV(_Hue, 1.0f, _Brightness),
					LinearGradientMode::Horizontal
				);
			}
			else {
				brush = gcnew LinearGradientBrush(
					trackRect,
					ColorFromHSV(_Hue, _Saturation, 0.0f),
					ColorFromHSV(_Hue, _Saturation, 1.0f),
					LinearGradientMode::Horizontal
				);
			}

			// Draw track with gradient
			g->FillRectangle(brush, trackRect);
			delete brush;
		}
		else {
			// Draw disabled state with gray gradient
			LinearGradientBrush^ brush = gcnew LinearGradientBrush(
				trackRect,
				Color::LightGray,
				Color::DarkGray,
				LinearGradientMode::Horizontal
			);
			g->FillRectangle(brush, trackRect);
			delete brush;
		}

		// Draw thumb
		int thumbX = (int)((Width - 8) * _Value / 100.0f);
		int thumbY = Height / 2;
		Rectangle thumbRect(thumbX, thumbY - 6, 8, 12);

		// Draw thumb with appropriate colors for enabled/disabled state
		Color thumbColor = this->Enabled ? Color::White : Color::LightGray;
		Color borderColor = this->Enabled ? Color::Gray : Color::DarkGray;

		g->FillEllipse(gcnew SolidBrush(thumbColor), thumbRect);
		g->DrawEllipse(gcnew Pen(borderColor), thumbRect);
	}

	void Control_ColorSlider::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left) {
			_IsDragging = true;
			UpdateValueFromMouse(e->X);
		}
		Control::OnMouseDown(e);
	}

	void Control_ColorSlider::OnMouseMove(MouseEventArgs^ e)
	{
		if (_IsDragging) {
			UpdateValueFromMouse(e->X);
		}
		Control::OnMouseMove(e);
	}

	void Control_ColorSlider::OnMouseUp(MouseEventArgs^ e)
	{
		_IsDragging = false;
		Control::OnMouseUp(e);
	}

	void Control_ColorSlider::UpdateValueFromMouse(int x)
	{
		int newValue = (int)((x * 100.0f) / Width);
		Value = Math::Max(0, Math::Min(100, newValue));
	}

	Color Control_ColorSlider::ColorFromHSV(float hue, float saturation, float value)
	{
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

	// Property implementations
	int Control_ColorSlider::Value::get()
	{
		return _Value;
	}

	void Control_ColorSlider::Value::set(int value)
	{
		if (value != _Value) {
			_Value = Math::Max(0, Math::Min(100, value));
			this->Invalidate();
			ValueChanged(this, EventArgs::Empty);
		}
	}

	float Control_ColorSlider::Hue::get()
	{
		return _Hue;
	}

	void Control_ColorSlider::Hue::set(float value)
	{
		if (value != _Hue) {
			_Hue = value;
			this->Invalidate();
		}
	}

	float Control_ColorSlider::Saturation::get()
	{
		return _Saturation;
	}

	void Control_ColorSlider::Saturation::set(float value)
	{
		if (value != _Saturation) {
			_Saturation = value;
			this->Invalidate();
		}
	}

	float Control_ColorSlider::Brightness::get()
	{
		return _Brightness;
	}

	void Control_ColorSlider::Brightness::set(float value)
	{
		if (value != _Brightness) {
			_Brightness = value;
			this->Invalidate();
		}
	}
}