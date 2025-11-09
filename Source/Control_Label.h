#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public ref class Control_Label : public Label
	{
	public:
		Control_Label()
		{
			this->DoubleBuffered = true;
			this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
				ControlStyles::AllPaintingInWmPaint |
				ControlStyles::ResizeRedraw, true);
		}
	};
}