#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Drawing::Drawing2D;

#include "Easings.h"
#include "Theme_Manager.h"
#include "Widget_Timeline_Common.h"

namespace MIDILightDrawer
{
	public delegate void PreviewSideSelectedHandler(Color color);

	public ref class Control_FadePreview : public Control
	{
	public:
		Control_FadePreview(int color_pad_width);

		void Set_Color(Color color);
		void Deselect_All(void);
		bool Start_Is_Selected(void);
		bool End_Is_Selected(void);
		bool Center_Is_Selected(void);
		void Switch_Colors(void);
		void Toggle_Mode(void);

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;
		virtual void OnMouseMove(MouseEventArgs^ e) override;

	private:
		FadeType _Current_Type;

		Color _Color_Start;
		Color _Color_End;
		Color _Color_Center;

		FadeEasing _Ease_In;
		FadeEasing _Ease_Out;

		Rectangle _Rect_Pad_Left;
		Rectangle _Rect_Pad_Right;
		Rectangle _Rect_Pad_Center;

		const int BORDER_PADDING_Y	= 4;
		const int PAD_FRAME_WIDTH	= 3;

		int _Color_Pad_Width;

		bool _Start_Selected;
		bool _End_Selected;
		bool _Center_Selected;

		void Draw_Border(Graphics^ g);
		void Draw_Color_Pad(Graphics^ g, Color color, int x_offset);
		void Draw_Color_Pad_Frame(Graphics^ g, bool is_selected, int x_offset);
		void Draw_Color_Gradient(Graphics^ g, Color color_start, Color color_end, int x_start, int x_end);
		void Draw_Easing_Curve(Graphics^ g, int x_start, int x_end, FadeEasing easing, bool isSecondHalf);

		Rectangle Get_Color_Pad_Rect(int x_offset);

	public:
		property FadeType Type {
			FadeType get() { return _Current_Type; }
		}

		property Color StartColor {
			Color get() { return _Color_Start; }
		}

		property Color EndColor {
			Color get() { return _Color_End; }
		}

		property Color CenterColor {
			Color get() { return _Color_Center; }
		}

		property FadeEasing EaseIn {
			FadeEasing get() { return _Ease_In; }
			void set(FadeEasing value);
		}

		property FadeEasing EaseOut {
			FadeEasing get() { return _Ease_Out; }
			void set(FadeEasing value);
		}

		event PreviewSideSelectedHandler^ PreviewSideSelected;
	};
}