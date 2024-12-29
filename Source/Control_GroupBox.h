#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	public ref class Control_GroupBox : public GroupBox
	{
	public:
		Control_GroupBox();

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
	};
}