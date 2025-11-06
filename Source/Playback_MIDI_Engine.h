#pragma once

#include "Playback_MIDI_Engine_Native.h"

using namespace System;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	public ref struct Playback_MIDI_Event
	{
		double Timestamp_ms;
		int Start_Tick;
		int Timeline_Track_ID;
		int MIDI_Channel;
		unsigned char MIDI_Command;
		unsigned char MIDI_Data1;
		unsigned char MIDI_Data2;

		Playback_MIDI_Event() : Timestamp_ms(0), Timeline_Track_ID(0), MIDI_Channel(0), MIDI_Command(0), MIDI_Data1(0), MIDI_Data2(0) { }
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
		bool Send_Event(Playback_MIDI_Event^ event);
		bool Send_All_Notes_Off(int channel);
		bool Is_Device_Open();

		// Threading methods
		bool Start_Playback();
		bool Stop_Playback();
		void Queue_Event(Playback_MIDI_Event^ event);
		void Queue_Events(List<Playback_MIDI_Event^>^ events);
		void Clear_Event_Queue();
		double Get_Current_Position_Ms();
		void Set_Current_Position_Ms(double position_ms);
		bool Is_Playing();
	};
}