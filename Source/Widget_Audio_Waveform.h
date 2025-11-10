#pragma once

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref class Playback_Audio_File_Manager;
	
	ref class Widget_Audio_Waveform : UserControl
	{
	private:
		Playback_Audio_File_Manager^ _Audio_File_Manager;

		double _Cursor_Poisiton_ms;

	public:
		Widget_Audio_Waveform();

		void Set_Audio_File_Manager(Playback_Audio_File_Manager^ audio_file_manager);
		void Set_Cursor_Position_ms(double cursor_position_ms);

	protected:
		virtual void OnPaint(PaintEventArgs^ e) override;
		virtual void OnResize(EventArgs^ e) override;

	private:
	};
}

