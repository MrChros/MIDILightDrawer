#pragma once

#include "Device_Manager_Audio_Native.h"
#include "Device_Manager_MIDI_Native.h"

using namespace System;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	public ref struct MIDI_Device_Info
	{
		int Device_ID;
		String^ Device_Name;
		bool Is_Available;

		MIDI_Device_Info() : Device_ID(-1), Is_Available(false) {}
	};

	public ref struct Audio_Device_Info
	{
		String^ Device_ID;
		String^ Device_Name;
		bool Is_Default;
		bool Is_Available;

		Audio_Device_Info() : Is_Default(false), Is_Available(false) {}
	};

	ref class Device_Manager
	{
	private:
		List<MIDI_Device_Info^>^ _MIDI_Devices;
		List<Audio_Device_Info^>^ _Audio_Devices;

	public:
		Device_Manager();

		void Enumerate_All_Devices();
		void Enumerate_MIDI_Devices();
		void Enumerate_Audio_Devices();

		List<MIDI_Device_Info^>^ Get_MIDI_Devices();
		List<Audio_Device_Info^>^ Get_Audio_Devices();

		int Find_MIDI_Device_By_Name(String^ device_name);
		String^ Find_Audio_Device_By_Name(String^ device_name);

		int Get_Default_MIDI_Device_ID();
		String^ Get_Default_Audio_Device_ID();

		String^ Get_MIDI_Device_Name(int device_id);
		String^ Get_Audio_Device_Name(String^ device_id);
	};
}