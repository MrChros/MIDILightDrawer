#include "Control_VolumeSlider.h"

namespace MIDILightDrawer
{
	Control_VolumeSlider::Control_VolumeSlider()
	{
		SetStyle(ControlStyles::UserPaint |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::ResizeRedraw,
			true);

		_Value = 100;
		_Minimum = 0;
		_Maximum = 100;
		_Is_Dragging = false;
		_Is_Hovering = false;
		_Previous_Volume_Before_Mute = 100;
		_Value_Changed = nullptr;

		// Default size
		this->Size = System::Drawing::Size(150, 35);
		this->BackColor = Color::Transparent;
	}

	void Control_VolumeSlider::On_Value_Changed(EventArgs^ e)
	{
		if (_Value_Changed != nullptr) {
			_Value_Changed(this, e);
		}
	}

	void Control_VolumeSlider::OnPaint(PaintEventArgs^ e)
	{
		Control::OnPaint(e);

		Graphics^ g = e->Graphics;
		g->SmoothingMode = SmoothingMode::AntiAlias;
		g->PixelOffsetMode = PixelOffsetMode::HighQuality;

		Theme_Manager^ theme = Theme_Manager::Get_Instance();

		// Calculate layout
		int icon_x = PADDING;
		int track_x = icon_x + ICON_SIZE + PADDING * 2;
		int track_width = Width - track_x - PADDING;
		int center_y = Height / 2;

		_Icon_Rect = Rectangle(icon_x, center_y - ICON_SIZE / 2, ICON_SIZE, ICON_SIZE);
		_Track_Rect = Rectangle(track_x, center_y - TRACK_HEIGHT / 2, track_width, TRACK_HEIGHT);

		// Calculate thumb position based on value
		float value_percent = (_Value - _Minimum) / (float)(_Maximum - _Minimum);
		int thumb_x = track_x + (int)(track_width * value_percent) - THUMB_SIZE / 2;
		_Thumb_Rect = Rectangle(thumb_x, center_y - THUMB_SIZE / 2, THUMB_SIZE, THUMB_SIZE);

		// Draw volume icon
		Draw_Volume_Icon(g, _Icon_Rect, value_percent);

		// Draw track background with rounded corners
		GraphicsPath^ track_path = gcnew GraphicsPath();
		int radius = TRACK_HEIGHT / 2;
		track_path->AddArc(_Track_Rect.X, _Track_Rect.Y, radius * 2, radius * 2, 90, 180);
		track_path->AddArc(_Track_Rect.Right - radius * 2, _Track_Rect.Y, radius * 2, radius * 2, 270, 180);
		track_path->CloseFigure();

		// Background track
		SolidBrush^ track_bg_brush = gcnew SolidBrush(Color::FromArgb(50, 255, 255, 255));
		g->FillPath(track_bg_brush, track_path);

		// Filled portion with gradient
		if (value_percent > 0) {
			Rectangle filled_rect = Rectangle(
				_Track_Rect.X,
				_Track_Rect.Y,
				(int)(_Track_Rect.Width * value_percent),
				_Track_Rect.Height
			);

			if (filled_rect.Width > 0) {
				GraphicsPath^ filled_path = gcnew GraphicsPath();
				filled_path->AddArc(filled_rect.X, filled_rect.Y, radius * 2, radius * 2, 90, 180);

				if (filled_rect.Width > radius * 2) {
					filled_path->AddLine(filled_rect.X + radius, filled_rect.Bottom, filled_rect.Right, filled_rect.Bottom);
					filled_path->AddLine(filled_rect.Right, filled_rect.Bottom, filled_rect.Right, filled_rect.Top);
					filled_path->AddLine(filled_rect.Right, filled_rect.Top, filled_rect.X + radius, filled_rect.Top);
				}
				else {
					// For very small widths, just do an arc
					int arc_width = Math::Min(filled_rect.Width, radius * 2);
					filled_path->AddArc(filled_rect.X, filled_rect.Y, arc_width, radius * 2, 270, 180);
				}
				filled_path->CloseFigure();

				// Create gradient from accent color to lighter shade
				Color accent = theme->AccentPrimary;
				Color accent_light = Color::FromArgb(
					Math::Min(255, accent.R + 40),
					Math::Min(255, accent.G + 40),
					Math::Min(255, accent.B + 40)
				);

				LinearGradientBrush^ gradient = gcnew LinearGradientBrush(
					filled_rect,
					accent_light,
					accent,
					LinearGradientMode::Horizontal
				);

				g->FillPath(gradient, filled_path);
				delete gradient;
				delete filled_path;
			}
		}

		delete track_bg_brush;
		delete track_path;

		// Draw thumb (circular handle)
		Color thumb_color = _Is_Hovering || _Is_Dragging
			? Color::White
			: Color::FromArgb(240, 240, 240);

		// Thumb shadow for depth
		Rectangle shadow_rect = Rectangle(
			_Thumb_Rect.X + 1,
			_Thumb_Rect.Y + 1,
			_Thumb_Rect.Width,
			_Thumb_Rect.Height
		);
		SolidBrush^ shadow_brush = gcnew SolidBrush(Color::FromArgb(60, 0, 0, 0));
		g->FillEllipse(shadow_brush, shadow_rect);
		delete shadow_brush;

		// Thumb main circle
		SolidBrush^ thumb_brush = gcnew SolidBrush(thumb_color);
		g->FillEllipse(thumb_brush, _Thumb_Rect);
		delete thumb_brush;

		// Thumb border
		Pen^ thumb_border = gcnew Pen(theme->AccentPrimary, _Is_Hovering || _Is_Dragging ? 2.5f : 1.5f);
		g->DrawEllipse(thumb_border, _Thumb_Rect);
		delete thumb_border;

		// Draw percentage text on thumb when hovering or dragging
		if (_Is_Hovering || _Is_Dragging) {
			String^ percent_text = _Value.ToString() + "%";
			System::Drawing::Font^ small_font = gcnew System::Drawing::Font("Segoe UI", 7.0f, FontStyle::Bold);
			SizeF text_size = g->MeasureString(percent_text, small_font);

			PointF text_pos = PointF(
				_Thumb_Rect.X + (_Thumb_Rect.Width - text_size.Width) / 2,
				_Thumb_Rect.Y - text_size.Height - 5
			);

			// Background for text
			Rectangle text_bg = Rectangle(
				(int)text_pos.X - 3,
				(int)text_pos.Y - 1,
				(int)text_size.Width + 6,
				(int)text_size.Height + 2
			);
			SolidBrush^ text_bg_brush = gcnew SolidBrush(Color::FromArgb(200, 0, 0, 0));
			g->FillRectangle(text_bg_brush, text_bg);
			delete text_bg_brush;

			SolidBrush^ text_brush = gcnew SolidBrush(Color::White);
			g->DrawString(percent_text, small_font, text_brush, text_pos);
			delete text_brush;
			delete small_font;
		}
	}

	void Control_VolumeSlider::Draw_Volume_Icon(Graphics^ g, Rectangle bounds, float volume_percent)
	{
		// Draw speaker icon
		SolidBrush^ icon_brush = gcnew SolidBrush(Color::White);
		Pen^ icon_pen = gcnew Pen(Color::White, 1.5f);

		// Speaker cone (trapezoid)
		array<Point>^ speaker_points = gcnew array<Point> {
			Point(bounds.X + 4, bounds.Y + bounds.Height / 2 - 3),
			Point(bounds.X + 8, bounds.Y + bounds.Height / 2 - 6),
			Point(bounds.X + 8, bounds.Y + bounds.Height / 2 + 6),
			Point(bounds.X + 4, bounds.Y + bounds.Height / 2 + 3)
		};
		g->FillPolygon(icon_brush, speaker_points);

		// Speaker base (rectangle)
		Rectangle speaker_base = Rectangle(bounds.X + 2, bounds.Y + bounds.Height / 2 - 2, 3, 4);
		g->FillRectangle(icon_brush, speaker_base);

		// Draw sound waves based on volume
		int wave_start_x = bounds.X + 10;
		int center_y = bounds.Y + bounds.Height / 2;

		if (volume_percent > 0) {
			// First wave (always shown if volume > 0)
			g->DrawArc(icon_pen, wave_start_x, center_y - 3, 4, 6, -90, 180);
		}

		if (volume_percent > 0.33f) {
			// Second wave
			g->DrawArc(icon_pen, wave_start_x + 3, center_y - 5, 6, 10, -90, 180);
		}

		if (volume_percent > 0.66f) {
			// Third wave
			g->DrawArc(icon_pen, wave_start_x + 6, center_y - 7, 8, 14, -90, 180);
		}

		// Draw mute X if volume is 0
		if (volume_percent == 0) {
			Pen^ mute_pen = gcnew Pen(Color::FromArgb(255, 200, 50, 50), 2.0f);
			g->DrawLine(mute_pen,
				bounds.X + 2, bounds.Y + 6,
				bounds.X + bounds.Width - 2, bounds.Y + bounds.Height - 6);
			g->DrawLine(mute_pen,
				bounds.X + 2, bounds.Y + bounds.Height - 6,
				bounds.X + bounds.Width - 2, bounds.Y + 6);
			delete mute_pen;
		}

		delete icon_brush;
		delete icon_pen;
	}

	void Control_VolumeSlider::OnMouseDown(MouseEventArgs^ e)
	{
		Control::OnMouseDown(e);

		// Check if clicking on speaker icon to toggle mute
		if (e->Button == System::Windows::Forms::MouseButtons::Left && _Icon_Rect.Contains(e->Location)) {
			if (_Value == 0) {
				// Unmute: restore previous volume
				Value = _Previous_Volume_Before_Mute;
			}
			else {
				// Mute: save current volume and set to 0
				_Previous_Volume_Before_Mute = _Value;
				Value = 0;
			}
		}
		else if (_Thumb_Rect.Contains(e->Location) || _Track_Rect.Contains(e->Location)) {
			_Is_Dragging = true;

			// Update value based on click position
			float value_percent = (e->X - _Track_Rect.X) / (float)_Track_Rect.Width;
			value_percent = Math::Max(0.0f, Math::Min(1.0f, value_percent));
			Value = _Minimum + (int)((_Maximum - _Minimum) * value_percent);
		}
	}

	void Control_VolumeSlider::OnMouseMove(MouseEventArgs^ e)
	{
		Control::OnMouseMove(e);

		// Check if hovering over thumb
		bool was_hovering = _Is_Hovering;
		_Is_Hovering = _Thumb_Rect.Contains(e->Location);

		if (_Is_Dragging) {
			_Is_Hovering = true;

			// Update value based on drag position
			float value_percent = (e->X - _Track_Rect.X) / (float)_Track_Rect.Width;
			value_percent = Math::Max(0.0f, Math::Min(1.0f, value_percent));
			Value = _Minimum + (int)((_Maximum - _Minimum) * value_percent);
		}

		if (was_hovering != _Is_Hovering) {
			Invalidate();
		}
	}

	void Control_VolumeSlider::OnMouseUp(MouseEventArgs^ e)
	{
		Control::OnMouseUp(e);
		_Is_Dragging = false;
		Invalidate();
	}

	void Control_VolumeSlider::OnMouseWheel(MouseEventArgs^ e)
	{
		Control::OnMouseWheel(e);

		// Mouse wheel delta is typically 120 per "notch"
		// Positive delta = scroll up = increase volume
		// Negative delta = scroll down = decrease volume
		int steps = e->Delta / 120;
		int new_value = _Value + (steps * WHEEL_STEP_SIZE);

		// Clamp to min/max range
		if (new_value < _Minimum) new_value = _Minimum;
		if (new_value > _Maximum) new_value = _Maximum;

		Value = new_value;
	}

	void Control_VolumeSlider::OnMouseEnter(EventArgs^ e)
	{
		Control::OnMouseEnter(e);
		this->Cursor = Cursors::Hand;
	}

	void Control_VolumeSlider::OnMouseLeave(EventArgs^ e)
	{
		Control::OnMouseLeave(e);
		_Is_Hovering = false;
		this->Cursor = Cursors::Default;
		Invalidate();
	}
}
