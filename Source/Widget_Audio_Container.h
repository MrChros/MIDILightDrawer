#pragma once

#include "Widget_Audio_Waveform.h"

using namespace System;
using namespace System::Windows::Forms;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref class Playback_Manager;
	
	public ref class Widget_Audio_Container : UserControl
	{
	private:
		Widget_Audio_Waveform^ _Audio_Waveform;
		Playback_Manager^ _Playback_Manager;

		Label^ _Label_Audio_Info_Length;
		Label^ _Label_Audio_Info_Channels;
		Label^ _Label_Audio_Info_Resolution;
		Label^ _Label_Audio_Info_Freqeuncy;
		Label^ _Label_Audio_Info_File;

	public:
		Widget_Audio_Container();

		void Set_Playback_Manager(Playback_Manager^ playback_manager);
		void Load_Audio_Data();
		void Unload_Audio_Data();

		void Update_Cursor();

		Widget_Audio_Waveform^ Waveform();

	private:
		void Update_Audio_Information();
	};
}

