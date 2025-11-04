#pragma once

namespace MIDILightDrawer
{
	class Device_Manager_MIDI_Native
	{
	public:
		struct MIDI_Device_Native
		{
			int Device_ID;
			wchar_t* Device_Name;
			bool Is_Available;
		};

		static MIDI_Device_Native* Enumerate_MIDI_Devices(int& count);
		static void Free_Device_List(MIDI_Device_Native* devices, int count);
	};
}