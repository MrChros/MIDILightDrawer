#include "Control_TrackBar_Zoom.h"

namespace MIDILightDrawer
{
	Control_TrackBar_Zoom::Control_TrackBar_Zoom()
	{
		SetStyle(ControlStyles::UserPaint |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::OptimizedDoubleBuffer,
			true);

		_Current_Step = 0;
		_Is_Dragging = false;
		_Drag_Offset = 0;
		_Value_Changed = nullptr;
		_Slider_Color = Color::DarkBlue;

		// Default size
		Size = System::Drawing::Size(200, 40);

		// Default values (can be changed using Set_Values)
		_Values = gcnew array<double> { 0.0, 1.0, 2.0, 3.0, 4.0 };
	}

	double Control_TrackBar_Zoom::Value::get()
	{
		return _Values[_Current_Step];
	}

	void Control_TrackBar_Zoom::Value::set(double value)
	{
		// Find closest step
		int Closest_Step = 0;
		double Min_Diff = Math::Abs(_Values[0] - value);

		for (int i = 1; i < _Values->Length; i++) {
			double Diff = Math::Abs(_Values[i] - value);
			if (Diff < Min_Diff) {
				Min_Diff = Diff;
				Closest_Step = i;
			}
		}

		if (_Current_Step != Closest_Step) {
			_Current_Step = Closest_Step;
			Invalidate();
			On_Value_Changed(gcnew Track_Bar_Value_Changed_Event_Args(_Values[_Current_Step]));
		}
	}

	void Control_TrackBar_Zoom::Set_Values(array<double>^ values_array)
	{
		if (values_array == nullptr || values_array->Length < 2)
			throw gcnew ArgumentException("At least two values are required");

		_Values = values_array;
		_Current_Step = 0;
		Invalidate();
	}

	bool Control_TrackBar_Zoom::Move_To_Next_Value()
	{
		if (_Current_Step < _Values->Length - 1) {
			_Current_Step++;
			Invalidate();
			On_Value_Changed(gcnew Track_Bar_Value_Changed_Event_Args(_Values[_Current_Step]));
			return true;
		}
		return false;
	}

	bool Control_TrackBar_Zoom::Move_To_Previous_Value()
	{
		if (_Current_Step > 0) {
			_Current_Step--;
			Invalidate();
			On_Value_Changed(gcnew Track_Bar_Value_Changed_Event_Args(_Values[_Current_Step]));
			return true;
		}
		return false;
	}

	void Control_TrackBar_Zoom::On_Value_Changed(Track_Bar_Value_Changed_Event_Args^ e)
	{
		if (_Value_Changed != nullptr) {
			_Value_Changed(this, e);
		}
	}

	void Control_TrackBar_Zoom::OnPaint(PaintEventArgs^ e)
	{
		Control::OnPaint(e);

		Graphics^ g = e->Graphics;
		g->SmoothingMode = Drawing2D::SmoothingMode::AntiAlias;

		// Calculate dimensions
		int Track_Y = Height / 2;

		// Left circle position
		int Left_Circle_X = LEFT_CIRCLE_RADIUS;
		int Left_Circle_Width = LEFT_CIRCLE_RADIUS * 2;

		// Right circle position
		int Right_Circle_Width = RIGHT_CIRCLE_RADIUS * 2;
		int Right_Circle_X = Width - Right_Circle_Width - LEFT_CIRCLE_RADIUS;

		// Store circle rectangles for hit testing
		_Left_Circle_Rect = Rectangle(
			Left_Circle_X,
			Track_Y - LEFT_CIRCLE_RADIUS,
			Left_Circle_Width,
			Left_Circle_Width);

		_Right_Circle_Rect = Rectangle(
			Right_Circle_X,
			Track_Y - RIGHT_CIRCLE_RADIUS,
			Right_Circle_Width,
			Right_Circle_Width);

		// Use 1.5 times slider width as padding
		int Slider_Padding = (int)(SLIDER_WIDTH * 0.75);

		// Track start after left circle with moderate padding
		int Track_Start = Left_Circle_X + Left_Circle_Width + Slider_Padding;
		// Track end before right circle with moderate padding
		int Track_End = Right_Circle_X - Slider_Padding;
		int Track_Length = Track_End - Track_Start;

		// Draw track line (extends between circles)
		int Line_Start = Left_Circle_X + Left_Circle_Width;
		int Line_End = Right_Circle_X;
		g->DrawLine(Pens::Gray,
			Line_Start, Track_Y,
			Line_End, Track_Y);

		// Draw circles at ends
		g->FillEllipse(Brushes::DarkGray,
			Left_Circle_X,
			Track_Y - LEFT_CIRCLE_RADIUS,
			Left_Circle_Width,
			Left_Circle_Width);

		g->FillEllipse(Brushes::DarkGray,
			Right_Circle_X,
			Track_Y - RIGHT_CIRCLE_RADIUS,
			Right_Circle_Width,
			Right_Circle_Width);

		// Calculate and draw slider
		float Step_Size = (float)Track_Length / (_Values->Length - 1);
		int Slider_X = Track_Start + (int)(Step_Size * _Current_Step) - (SLIDER_WIDTH / 2);
		_Slider_Rect = Rectangle(Slider_X,
			Track_Y - SLIDER_HEIGHT / 2,
			SLIDER_WIDTH,
			SLIDER_HEIGHT);

		SolidBrush^ slider_brush = gcnew SolidBrush(_Slider_Color);
		g->FillRectangle(slider_brush, _Slider_Rect);
		delete slider_brush;
	}

	void Control_TrackBar_Zoom::OnMouseDown(MouseEventArgs^ e)
	{
		Control::OnMouseDown(e);

		if (_Slider_Rect.Contains(e->Location)) {
			_Is_Dragging = true;
			_Drag_Offset = e->X - _Slider_Rect.X;
		}
		else if (_Left_Circle_Rect.Contains(e->Location)) {
			Move_To_Previous_Value();
		}
		else if (_Right_Circle_Rect.Contains(e->Location)) {
			Move_To_Next_Value();
		}
	}

	void Control_TrackBar_Zoom::OnMouseMove(MouseEventArgs^ e)
	{
		Control::OnMouseMove(e);

		if (_Is_Dragging) {
			// Use same calculations as in OnPaint for consistency
			int Left_Circle_X = LEFT_CIRCLE_RADIUS;
			int Left_Circle_Width = LEFT_CIRCLE_RADIUS * 2;
			int Right_Circle_X = Width - (RIGHT_CIRCLE_RADIUS * 2) - LEFT_CIRCLE_RADIUS;

			int Slider_Padding = (int)(SLIDER_WIDTH * 0.75);
			int Track_Start = Left_Circle_X + Left_Circle_Width + Slider_Padding;
			int Track_End = Right_Circle_X - Slider_Padding;
			int Track_Length = Track_End - Track_Start;

			float Step_Size = (float)Track_Length / (_Values->Length - 1);

			int New_X = e->X - _Drag_Offset;
			int Slider_Center = New_X + SLIDER_WIDTH / 2;

			// Find closest step
			int New_Step = (int)((float)Math::Round((float)(Slider_Center - Track_Start) / Step_Size));
			New_Step = Math::Max(0, Math::Min(New_Step, _Values->Length - 1));

			if (New_Step != _Current_Step) {
				_Current_Step = New_Step;
				Invalidate();
				On_Value_Changed(gcnew Track_Bar_Value_Changed_Event_Args(_Values[_Current_Step]));
			}
		}
	}

	void Control_TrackBar_Zoom::OnMouseUp(MouseEventArgs^ e)
	{
		Control::OnMouseUp(e);
		_Is_Dragging = false;
	}
}