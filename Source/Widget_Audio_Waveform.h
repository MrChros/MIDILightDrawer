#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref class Waveform_Render_Data;
	
	public ref class Widget_Audio_Waveform : UserControl
	{
	private:
		Waveform_Render_Data^ _Waveform_Data;

		double _Cursor_Position_ms;
		double _Audio_Duration_ms;

	public:
		Widget_Audio_Waveform();

		void Set_Waveform_Data(Waveform_Render_Data^ waveform_data);
		void Set_Cursor_Position_ms(double cursor_position_ms, double audio_duration_ms);

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnResize(EventArgs^ e) override;

	private:
	};
}

