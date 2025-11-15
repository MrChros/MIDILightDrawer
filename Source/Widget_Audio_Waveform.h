#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref class Waveform_Render_Data;
	
	public ref class Widget_Audio_Waveform : UserControl
	{
	public:
		static const double NO_AUDIO = -1.0;
		static const double NO_MIDI = -1.0;

		event EventHandler<double>^ OnCursorPositionChanged;

	private:
		Waveform_Render_Data^ _Waveform_Data;

		double _Audio_Duration_ms;
		double _MIDI_Duration_ms;
		double _Maximum_Duration_ms;
		double _Cursor_Position_ms;

		List<double>^ _Marker_Timestamps;

	public:
		Widget_Audio_Waveform(List<double>^ marker_timestamp_list);

		void Set_Waveform_Data(Waveform_Render_Data^ waveform_data);
		void Set_Audio_Duration_ms(double audio_duration_ms);
		void Set_MIDI_Duration_ms(double midi_duration_ms);
		void Set_Cursor_Position_ms(double cursor_position_ms);

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnResize(EventArgs^ e) override;
		void OnMouseDown(MouseEventArgs^ e) override;

	private:
		void Update_Max_Duration();
	};
}

