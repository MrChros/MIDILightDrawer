#pragma once

#include "Widget_Audio_Waveform.h"
#include "Control_TimeOffset_NumericUpDown.h"
#include "Control_VolumeSlider.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref class Widget_Timeline;
	ref class Playback_Manager;
	ref class Control_TrackBar_Zoom;
	
	public ref class Widget_Audio_Container : UserControl
	{
	private:
		Widget_Timeline^ _Timeline;
		Playback_Manager^ _Playback_Manager;
		Widget_Audio_Waveform^ _Audio_Waveform;
		Control_TrackBar_Zoom^ _TrackBar_Zoom;

		List<double>^ _Marker_Timestamps;

		Label^ _Label_Audio_Info_Length;
		Label^ _Label_Audio_Info_Channels;
		Label^ _Label_Audio_Info_Resolution;
		Label^ _Label_Audio_Info_Freqeuncy;
		Label^ _Label_Audio_Info_File;

		Control_TimeOffset_NumericUpDown^ _TimeOffset_Audio_Offset;
		Control_VolumeSlider^ _VolumeSlider;

	public:
		Widget_Audio_Container();

		void Set_Widget_Timeline(Widget_Timeline^ timeline);
		void Set_Playback_Manager(Playback_Manager^ playback_manager);
		void Set_TrackBar_Zoom(Control_TrackBar_Zoom^ trackbar_zoom);
		void Load_Audio_Data();
		void Unload_Audio_Data();

		void Load_MIDI_Information();

		void Update_Cursor();
		void Update_View_Range();
		void Refresh_Event_Display();

		Widget_Audio_Waveform^ Waveform();

	private:
		void Update_Audio_Information();
		void On_Audio_Offset_ValueChanged(Object^ sender, EventArgs^ e);
		void On_Volume_ValueChanged(Object^ sender, EventArgs^ e);
		void On_Cursor_Position_Changed(Object^ sender, double cursor_position_ms);
		void On_Viewport_Scroll_Changed(Object^ sender, double start_position_ms);
		void On_Viewport_Range_Changed(Object^ sender, array<double>^ range);
	};
}

