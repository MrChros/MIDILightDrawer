#pragma once

#include "Playback_MIDI_Engine_Native.h"

using namespace System;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	public ref struct MIDI_Event_Info
	{
		double Timestamp_Ms;
		int Track;
		int Channel;
		unsigned char Command;
		unsigned char Data1;
		unsigned char Data2;

		MIDI_Event_Info() : Timestamp_Ms(0), Track(0), Channel(0), Command(0), Data1(0), Data2(0) {
		}
	};
	
	ref class Playback_MIDI_Engine
	{
	private:
		bool _Is_Initialized;

	public:
		Playback_MIDI_Engine();
		~Playback_MIDI_Engine();

		bool Initialize(int device_id);
		void Cleanup();
		bool Send_Event(MIDI_Event_Info^ event);
		bool Send_All_Notes_Off(int channel);
		bool Is_Device_Open();
	};
}

