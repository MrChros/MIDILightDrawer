#pragma managed(push, off)

#include "Device_Manager_MIDI_Native.h"
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

Device_Manager_MIDI_Native::MIDI_Device_Native* Device_Manager_MIDI_Native::Enumerate_MIDI_Devices(int& count)
{
	UINT Num_Devices = midiOutGetNumDevs();
	count = Num_Devices;

	if (Num_Devices == 0)
		return nullptr;

	MIDI_Device_Native* Devices = new MIDI_Device_Native[Num_Devices];

	for (UINT i = 0; i < Num_Devices; ++i)
	{
		MIDIOUTCAPSW Caps;
		MMRESULT Result = midiOutGetDevCapsW(i, &Caps, sizeof(MIDIOUTCAPSW));

		if (Result == MMSYSERR_NOERROR)
		{
			Devices[i].Device_ID = static_cast<int>(i);
			Devices[i].Device_Name = _wcsdup(Caps.szPname);
			Devices[i].Is_Available = true;
		}
		else
		{
			Devices[i].Device_ID = -1;
			Devices[i].Device_Name = nullptr;
			Devices[i].Is_Available = false;
		}
	}

	return Devices;
}

void Device_Manager_MIDI_Native::Free_Device_List(MIDI_Device_Native* devices, int count)
{
	for (int i = 0; i < count; ++i)
	{
		if (devices[i].Device_Name)
			free(devices[i].Device_Name);
	}
	delete[] devices;
}

#pragma managed(pop)