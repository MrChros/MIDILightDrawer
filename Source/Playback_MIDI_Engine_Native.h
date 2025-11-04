#pragma once

namespace MIDILightDrawer
{
	class Playback_MIDI_Engine_Native
	{
	private:
		static void* _MIDI_Handle;
		static bool _Is_Initialized;

	public:
		struct MIDI_Event
		{
			double Timestamp_Ms;      // Timestamp in milliseconds
			int Track;                // Track number
			int Channel;              // MIDI channel (0-15)
			unsigned char Command;    // MIDI command byte
			unsigned char Data1;      // First data byte
			unsigned char Data2;      // Second data byte
		};

		static bool Initialize(int device_id);
		static void Cleanup();
		static bool Send_MIDI_Event(const MIDI_Event& event);
		static bool Send_All_Notes_Off(int channel);
		static bool Is_Device_Open();
	};
}
