#include "Control_ColorSlider.h"

namespace MIDILightDrawer
{
	Control_ColorSlider::Control_ColorSlider(SliderType type) : _Type(type), _Value(1.0f), _Hue(0), _Saturation(1.0f), _Brightness(1.0f)
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
		Graphics^ G = e->Graphics;
		G->SmoothingMode = SmoothingMode::AntiAlias;

		// Draw track
		int Track_Height = 4;
		int Track_Y = (Height - Track_Height) / 2;
		Rectangle Track_Rect(0, Track_Y, Width, Track_Height);

		if (this->Enabled)
		{
			// Create gradient based on slider type
			LinearGradientBrush^ Brush;
			if (_Type == SliderType::Saturation) {
				Brush = gcnew LinearGradientBrush(Track_Rect, ColorFromHSV(_Hue, 0.0f, _Brightness), ColorFromHSV(_Hue, 1.0f, _Brightness), LinearGradientMode::Horizontal);
			}
			else {
				Brush = gcnew LinearGradientBrush(Track_Rect, ColorFromHSV(_Hue, _Saturation, 0.0f), ColorFromHSV(_Hue, _Saturation, 1.0f), LinearGradientMode::Horizontal);
			}

			// Draw track with gradient
			G->FillRectangle(Brush, Track_Rect);
			delete Brush;
		}
		else
		{
			// Draw disabled state with gray gradient
			LinearGradientBrush^ Brush = gcnew LinearGradientBrush(Track_Rect, Color::LightGray, Color::DarkGray, LinearGradientMode::Horizontal);
			G->FillRectangle(Brush, Track_Rect);
			delete Brush;
		}

		// Draw thumb
		int Thumb_X = (int)((Width - 8) * _Value);
		int Thumb_Y = Height / 2;
		Rectangle Thumb_Rect(Thumb_X, Thumb_Y - 6, 8, 12);

		// Draw thumb with appropriate colors for enabled/disabled state
		Color Thumb_Color = this->Enabled ? Color::White : Color::LightGray;
		Color Border_Color = this->Enabled ? Color::Gray : Color::DarkGray;

		G->FillEllipse(gcnew SolidBrush(Thumb_Color), Thumb_Rect);
		G->DrawEllipse(gcnew Pen(Border_Color), Thumb_Rect);
	}

	void Control_ColorSlider::OnMouseDown(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Left)
		{
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
		this->Value = (float)x / (float)this->Width;
	}

	Color Control_ColorSlider::ColorFromHSV(float hue, float saturation, float value)
	{
		int HI = (int)(hue / 60.0f) % 6;
		float F = (hue / 60.0f) - HI;
		float P = value * (1.0f - saturation);
		float Q = value * (1.0f - F * saturation);
		float T = value * (1.0f - (1.0f - F) * saturation);

		switch (HI) {
		case 0: return Color::FromArgb(255,
			(int)(value * 255), (int)(T * 255), (int)(P * 255));
		case 1: return Color::FromArgb(255,
			(int)(Q * 255), (int)(value * 255), (int)(P * 255));
		case 2: return Color::FromArgb(255,
			(int)(P * 255), (int)(value * 255), (int)(T * 255));
		case 3: return Color::FromArgb(255,
			(int)(P * 255), (int)(Q * 255), (int)(value * 255));
		case 4: return Color::FromArgb(255,
			(int)(T * 255), (int)(P * 255), (int)(value * 255));
		default: return Color::FromArgb(255,
			(int)(value * 255), (int)(P * 255), (int)(Q * 255));
		}
	}

	// Property implementations
	float Control_ColorSlider::Value::get()
	{
		return _Value;
	}

	void Control_ColorSlider::Value::set(float value)
	{
		if (value != _Value)
		{
			_Value = Math::Max(0.0f, Math::Min(1.0f, value));

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