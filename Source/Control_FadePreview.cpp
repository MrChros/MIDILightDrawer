#include "Control_FadePreview.h"

namespace MIDILightDrawer
{
	Control_FadePreview::Control_FadePreview(int color_pad_width)
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint, true);
		
		this->_Current_Type = FadeType::Two_Colors;

		this->_Color_Start		= Color::Red;
		this->_Color_End		= Color::Green;
		this->_Color_Center		= Color::Blue;
		this->_Color_Pad_Width	= color_pad_width;

		this->_Ease_In			= FadeEasing::Linear;
		this->_Ease_Out			= FadeEasing::Linear;

		Deselect_All();
	}

	void Control_FadePreview::Set_Color(Color color)
	{
		if (_Start_Selected) {
			_Color_Start = color;
		}
		else if (_End_Selected) {
			_Color_End = color;
		}
		else if (_Center_Selected) {
			_Color_Center = color;
		}

		this->Invalidate();
	}

	void Control_FadePreview::Deselect_All(void)
	{
		_Start_Selected = false;
		_End_Selected = false;
		_Center_Selected = false;
	}

	bool Control_FadePreview::Start_Is_Selected(void)
	{
		return _Start_Selected;
	}

	bool Control_FadePreview::End_Is_Selected(void)
	{
		return _End_Selected;
	}

	bool Control_FadePreview::Center_Is_Selected(void)
	{
		if(this->_Current_Type == FadeType::Three_Colors) {
			return _Center_Selected;
		}

		return false;
	}

	void Control_FadePreview::Switch_Colors(void)
	{
		Color Color_Backup	= _Color_Start;
		_Color_Start		= _Color_End;
		_Color_End			= Color_Backup;

		this->Invalidate();
	}

	void Control_FadePreview::Toggle_Mode(void)
	{
		if (this->_Current_Type == FadeType::Two_Colors) {
			this->_Current_Type = FadeType::Three_Colors;
		}
		else {
			this->_Current_Type = FadeType::Two_Colors;
			_Center_Selected = false;
		}

		this->Invalidate();
	}

	void Control_FadePreview::OnPaint(PaintEventArgs^ e)
	{
		Graphics^ g = e->Graphics;
		g->SmoothingMode = SmoothingMode::AntiAlias;

		Rectangle bounds	= this->ClientRectangle;
		_Rect_Pad_Left		= Get_Color_Pad_Rect(PAD_FRAME_WIDTH);
		_Rect_Pad_Right		= Get_Color_Pad_Rect(Bounds.Width - 1 - _Color_Pad_Width - PAD_FRAME_WIDTH);
		_Rect_Pad_Center	= Get_Color_Pad_Rect((Bounds.Width - _Color_Pad_Width) / 2);


		Draw_Border(g);

		// Draw Rectange for Start Color
		Draw_Color_Pad(g, _Color_Start, PAD_FRAME_WIDTH);

		// Draw Rectange for End Color
		Draw_Color_Pad(g, _Color_End, bounds.Width - 1 - _Color_Pad_Width - PAD_FRAME_WIDTH);

		if (this->_Current_Type == FadeType::Three_Colors)
		{
			Draw_Color_Pad(g, _Color_Center, (bounds.Width - _Color_Pad_Width) / 2);
			Draw_Color_Gradient(g, _Color_Start, _Color_Center, _Rect_Pad_Left.Right + PAD_FRAME_WIDTH, _Rect_Pad_Center.Left - PAD_FRAME_WIDTH);
			Draw_Color_Gradient(g, _Color_Center, _Color_End, _Rect_Pad_Center.Right + PAD_FRAME_WIDTH, _Rect_Pad_Right.Left - PAD_FRAME_WIDTH);

			// Draw ease-in curve between start and center
			Draw_Easing_Curve(g, _Rect_Pad_Left.Right + PAD_FRAME_WIDTH, _Rect_Pad_Center.Left - PAD_FRAME_WIDTH, _Ease_In, false);

			// Draw ease-out curve between center and end
			Draw_Easing_Curve(g, _Rect_Pad_Center.Right + PAD_FRAME_WIDTH, _Rect_Pad_Right.Left - PAD_FRAME_WIDTH, _Ease_Out, true);
		}
		else // Two colors mode
		{
			Draw_Color_Gradient(g, _Color_Start, _Color_End, _Rect_Pad_Left.Right + PAD_FRAME_WIDTH, _Rect_Pad_Right.Left - PAD_FRAME_WIDTH);

			int middle_x = (_Rect_Pad_Left.Right + _Rect_Pad_Right.Left) / 2;

			// Draw ease-in curve from start to middle
			Draw_Easing_Curve(g, _Rect_Pad_Left.Right + PAD_FRAME_WIDTH, middle_x, _Ease_In, false);

			// Draw ease-out curve from middle to end
			Draw_Easing_Curve(g, middle_x, _Rect_Pad_Right.Left - PAD_FRAME_WIDTH, _Ease_Out, true);
		}

		// Draw Frame for Start Color
		Draw_Color_Pad_Frame(g, _Start_Selected, _Rect_Pad_Left.Left - PAD_FRAME_WIDTH + 1);
		
		// Draw Frame for End Color
		Draw_Color_Pad_Frame(g, _End_Selected, _Rect_Pad_Right.Left - PAD_FRAME_WIDTH + 1);

		// Draw Frame for Center Color
		if (this->_Current_Type == FadeType::Three_Colors) {
			Draw_Color_Pad_Frame(g, _Center_Selected, _Rect_Pad_Center.Left - PAD_FRAME_WIDTH + 1);
		}
	}

	void Control_FadePreview::OnMouseDown(MouseEventArgs^ e)
	{
		_Start_Selected = false;
		_End_Selected = false;
		_Center_Selected = false;

		if (e->Button == Windows::Forms::MouseButtons::Left) {
			if (e->X > _Rect_Pad_Left.Left && e->X < _Rect_Pad_Left.Right) {
				_Start_Selected = true;
				PreviewSideSelected(_Color_Start);
			}
			else if (e->X > _Rect_Pad_Right.Left && e->X < _Rect_Pad_Right.Right) {
				_End_Selected = true;
				PreviewSideSelected(_Color_End);
			}
			else if (e->X > _Rect_Pad_Center.Left && e->X < _Rect_Pad_Center.Right && _Current_Type == FadeType::Three_Colors) {
				_Center_Selected = true;
				PreviewSideSelected(_Color_Center);
			}
		}
		
		this->Invalidate();
		Control::OnMouseDown(e);
	}

	void Control_FadePreview::OnMouseMove(MouseEventArgs^ e)
	{
		if (e->X > _Rect_Pad_Left.Left && e->X < _Rect_Pad_Left.Right) {
			this->Cursor = Cursors::Hand;
		}
		else if (e->X > _Rect_Pad_Right.Left && e->X < _Rect_Pad_Right.Right) {
			this->Cursor = Cursors::Hand;
		}
		else if (e->X > _Rect_Pad_Center.Left && e->X < _Rect_Pad_Center.Right && _Current_Type == FadeType::Three_Colors) {
			this->Cursor = Cursors::Hand;
		}
		else {
			this->Cursor = Cursors::Default;
		}
		
		this->Invalidate();
		Control::OnMouseMove(e);
	}

	void Control_FadePreview::Draw_Border(Graphics^ g)
	{
		Rectangle bounds = this->ClientRectangle;

		Pen^ Border_Pen = gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong, 1);
		g->DrawRectangle(Border_Pen, 0, BORDER_PADDING_Y, bounds.Width - 1, bounds.Height - 1 - 2 * BORDER_PADDING_Y);
		delete Border_Pen;
	}

	void Control_FadePreview::Draw_Color_Pad(Graphics^ g, Color color, int x_offset)
	{
		SolidBrush^ Brush_Start = gcnew SolidBrush(color);
		g->FillRectangle(Brush_Start, Get_Color_Pad_Rect(x_offset));
		delete Brush_Start;
	}

	void Control_FadePreview::Draw_Color_Pad_Frame(Graphics^ g, bool is_selected, int x_offset)
	{
		Rectangle Bounds = this->ClientRectangle;
		
		int Pad_Frame_X = x_offset;
		int Pad_Frame_Y = BORDER_PADDING_Y - PAD_FRAME_WIDTH + 1;
		int Pad_Frame_W = _Color_Pad_Width + 2 * PAD_FRAME_WIDTH - 2;
		int Pad_Frame_H = Bounds.Height - 3 - 2 * BORDER_PADDING_Y + 2 * PAD_FRAME_WIDTH;

		Pen^ Frame_Pen;
		if (is_selected) {
			Frame_Pen = gcnew Pen(Theme_Manager::Get_Instance()->AccentPrimary, (float)PAD_FRAME_WIDTH);
		}
		else {
			Frame_Pen = gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong, (float)PAD_FRAME_WIDTH);
		}
		g->DrawRectangle(Frame_Pen, Pad_Frame_X, Pad_Frame_Y, Pad_Frame_W, Pad_Frame_H);
		delete Frame_Pen;
	}

	void Control_FadePreview::Draw_Color_Gradient(Graphics^ g, Color color_start, Color color_end, int x_start, int x_end)
	{
		Rectangle Bounds = this->ClientRectangle;
		
		Rectangle Rect_Gradient(x_start, BORDER_PADDING_Y + 1, x_end - x_start, Bounds.Height - 1 - 2 * BORDER_PADDING_Y - 2);
		
		LinearGradientBrush^ Brush_Gradient = gcnew LinearGradientBrush(Rect_Gradient, color_start, color_end, LinearGradientMode::Horizontal);
		g->FillRectangle(Brush_Gradient, Rect_Gradient);
		delete Brush_Gradient;
	}

	void Control_FadePreview::Draw_Easing_Curve(Graphics^ g, int x_start, int x_end, FadeEasing easing, bool isSecondHalf)
	{
		int Width = x_end - x_start;
		
		if (Width <= 0) {
			return;
		}

		// Create points array for the curve
		array<Point>^ Points = gcnew array<Point>(Width);

		int Height = (this->Height - 3 * BORDER_PADDING_Y) / 2;

		int Y_Offset = 2*Height;

		if (isSecondHalf) {
			Y_Offset = Y_Offset / 2 + 1;
		}

		// Calculate curve points
		for (int i = 0; i < Width; i++)
		{
			float Ratio = (float)i / (Width - 1);
			float Y;

			Y = Easings::ApplyEasing(Ratio, easing);

			// Invert Y coordinate since screen coordinates go down
			Points[i] = Point(x_start + i, BORDER_PADDING_Y + BORDER_PADDING_Y/4 + Y_Offset - (int)(Y * Height));
		}

		// Draw the curve
		Pen^ CurvePen = gcnew Pen(Color::FromArgb(180, Color::White), 2.0f);
		g->DrawLines(CurvePen, Points);
		delete CurvePen;
	}

	Rectangle Control_FadePreview::Get_Color_Pad_Rect(int x_offset)
	{
		Rectangle Bounds = this->ClientRectangle;
		Rectangle Rect_Pad = Rectangle(0 + x_offset, BORDER_PADDING_Y, _Color_Pad_Width, Bounds.Height - 1 - 2 * BORDER_PADDING_Y);

		return Rect_Pad;
	}

	void Control_FadePreview::EaseIn::set(FadeEasing value)
	{
			_Ease_In = value;
			this->Invalidate();
	}

	void Control_FadePreview::EaseOut::set(FadeEasing value)
	{
		_Ease_Out = value;
		this->Invalidate();
	}
}