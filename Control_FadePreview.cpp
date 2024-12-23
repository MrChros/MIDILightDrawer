#include "Control_FadePreview.h"

namespace MIDILightDrawer
{
	Control_FadePreview::Control_FadePreview(int color_pad_width)
	{
		this->DoubleBuffered = true;
		this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint, true);
		
		_Color_Start		= Color::Red;
		_Color_End			= Color::Green;
		_Color_Pad_Width	= color_pad_width;

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

		this->Invalidate();
	}

	void Control_FadePreview::Deselect_All(void)
	{
		_Start_Selected = false;
		_End_Selected = false;
	}

	bool Control_FadePreview::Start_Is_Selected(void)
	{
		return _Start_Selected;
	}

	bool Control_FadePreview::End_Is_Selected(void)
	{
		return _End_Selected;
	}

	void Control_FadePreview::Switch_Colors(void)
	{
		Color Color_Backup	= _Color_Start;
		_Color_Start		= _Color_End;
		_Color_End			= Color_Backup;

		this->Invalidate();
	}

	void Control_FadePreview::OnPaint(PaintEventArgs^ e)
	{
		Graphics^ g = e->Graphics;
		g->SmoothingMode = SmoothingMode::AntiAlias;

		Rectangle bounds = this->ClientRectangle;

		_Rect_Pad_Left		= Rectangle(PAD_FRAME_WIDTH, BORDER_PADDING_Y, _Color_Pad_Width, bounds.Height - 1 - 2 * BORDER_PADDING_Y);
		_Rect_Pad_Right		= _Rect_Pad_Left;
		_Rect_Pad_Right.X	= bounds.Width - 1 - _Color_Pad_Width - PAD_FRAME_WIDTH;

		Pen^ Border_Pen = gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong, 1);
		g->DrawRectangle(Border_Pen, 0, BORDER_PADDING_Y, bounds.Width - 1, bounds.Height - 1 - 2 * BORDER_PADDING_Y);
		delete Border_Pen;

		// Draw Rectange for Start Color
		SolidBrush^ Brush_Start = gcnew SolidBrush(_Color_Start);
		g->FillRectangle(Brush_Start, _Rect_Pad_Left);
		delete Brush_Start;

		// Draw Rectange for End Color
		SolidBrush^ Brush_End = gcnew SolidBrush(_Color_End);
		g->FillRectangle(Brush_End, _Rect_Pad_Right);
		delete Brush_End;


		// Draw the Pad Frames
		int Pad_Frame_X = 1;
		int Pad_Frame_Y = BORDER_PADDING_Y - PAD_FRAME_WIDTH + 1;
		int Pad_Frame_W = _Color_Pad_Width + 2*PAD_FRAME_WIDTH - 2;
		int Pad_Frame_H = bounds.Height - 3 - 2*BORDER_PADDING_Y + 2* PAD_FRAME_WIDTH;


		// Draw the actual gradient
		Rectangle Rect_Gradient(2 * PAD_FRAME_WIDTH + _Color_Pad_Width, BORDER_PADDING_Y + 1, bounds.Width - 2 * (2*PAD_FRAME_WIDTH + _Color_Pad_Width), bounds.Height - 1 - 2 * BORDER_PADDING_Y - 2);
		LinearGradientBrush^ Brush_Gradient = gcnew LinearGradientBrush(Rect_Gradient, _Color_Start, _Color_End, LinearGradientMode::Horizontal);
		g->FillRectangle(Brush_Gradient, Rect_Gradient);
		delete Brush_Gradient;

		
		// Draw Frame around the Start Pad
		Pen^ Frame_Pen_Start;
		if (_Start_Selected) {
			Frame_Pen_Start = gcnew Pen(Theme_Manager::Get_Instance()->AccentPrimary, (float)PAD_FRAME_WIDTH);
		}
		else {
			Frame_Pen_Start = gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong, (float)PAD_FRAME_WIDTH);
		}
		g->DrawRectangle(Frame_Pen_Start, Pad_Frame_X, Pad_Frame_Y, Pad_Frame_W, Pad_Frame_H);
		delete Frame_Pen_Start;

		// Draw Frame around the Start Pad
		Pen^ Frame_Pen_End;
		if (_End_Selected) {
			Frame_Pen_End = gcnew Pen(Theme_Manager::Get_Instance()->AccentPrimary, (float)PAD_FRAME_WIDTH);
		}
		else {
			Frame_Pen_End = gcnew Pen(Theme_Manager::Get_Instance()->BorderStrong, (float)PAD_FRAME_WIDTH);
		}
		g->DrawRectangle(Frame_Pen_End, bounds.Width - 1 - Pad_Frame_W - Pad_Frame_X, Pad_Frame_Y, Pad_Frame_W, Pad_Frame_H);
		delete Frame_Pen_End;
	}

	void Control_FadePreview::OnMouseDown(MouseEventArgs^ e)
	{
		_Start_Selected = false;
		_End_Selected = false;

		if (e->Button == Windows::Forms::MouseButtons::Left) {
			if (e->X > _Rect_Pad_Left.Left && e->X < _Rect_Pad_Left.Right) {
				_Start_Selected = true;
				PreviewSideSelected(_Color_Start);
			}
			else if (e->X > _Rect_Pad_Right.Left && e->X < _Rect_Pad_Right.Right) {
				_End_Selected = true;
				PreviewSideSelected(_Color_End);
			}
		}
		
		this->Invalidate();
		Control::OnMouseDown(e);
	}
}