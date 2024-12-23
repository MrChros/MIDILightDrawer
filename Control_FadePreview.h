#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Drawing::Drawing2D;

#include "Theme_Manager.h"

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
		void Switch_Colors(void);

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnMouseDown(MouseEventArgs^ e) override;

	private:
		System::Drawing::Color _Color_Start;
		System::Drawing::Color _Color_End;

		Rectangle _Rect_Pad_Left;
		Rectangle _Rect_Pad_Right;

		const int BORDER_PADDING_Y	= 4;
		const int PAD_FRAME_WIDTH	= 3;

		int _Color_Pad_Width;

		bool _Start_Selected;
		bool _End_Selected;

	public:
		property Color StartColor {
			Color get() { return _Color_Start;  }
		}

		property Color EndColor {
			Color get() { return _Color_End; }
		}

		event PreviewSideSelectedHandler^ PreviewSideSelected;
	};
}