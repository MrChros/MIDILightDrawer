#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public ref class Control_DataGrid : public DataGridView
	{
	public:
		Control_DataGrid()
		{
			this->DoubleBuffered = true;
			this->SetStyle(ControlStyles::OptimizedDoubleBuffer |
				ControlStyles::AllPaintingInWmPaint |
				ControlStyles::UserPaint, true);
		}
	};
}