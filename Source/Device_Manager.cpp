#include "Device_Manager.h"
//#include <functiondiscoverykeys_devpkey.h>

namespace MIDILightDrawer
{
	Device_Manager::Device_Manager()
	{
		_MIDI_Devices = gcnew List<MIDI_Device_Info^>();
		_Audio_Devices = gcnew List<Audio_Device_Info^>();
	}

	void Device_Manager::Enumerate_All_Devices()
	{
		Enumerate_MIDI_Devices();
		Enumerate_Audio_Devices();
	}

	void Device_Manager::Enumerate_MIDI_Devices()
	{
		_MIDI_Devices->Clear();

		int Count = 0;
		auto* Native_Devices = Device_Manager_MIDI_Native::Enumerate_MIDI_Devices(Count);

		if (Native_Devices)
		{
			for (int i = 0; i < Count; ++i)
			{
				if (Native_Devices[i].Is_Available)
				{
					MIDI_Device_Info^ Info = gcnew MIDI_Device_Info();
					Info->Device_ID = Native_Devices[i].Device_ID;
					Info->Device_Name = gcnew String(Native_Devices[i].Device_Name);
					Info->Is_Available = Native_Devices[i].Is_Available;
					_MIDI_Devices->Add(Info);
				}
			}

			Device_Manager_MIDI_Native::Free_Device_List(Native_Devices, Count);
		}
	}

	void Device_Manager::Enumerate_Audio_Devices()
	{
		_Audio_Devices->Clear();

		int Count = 0;
		auto* Native_Devices = Device_Manager_Audio_Native::Enumerate_Audio_Devices(Count);

		if (Native_Devices)
		{
			for (int i = 0; i < Count; ++i)
			{
				Audio_Device_Info^ Info = gcnew Audio_Device_Info();
				Info->Device_ID = gcnew String(Native_Devices[i].Device_ID);
				Info->Device_Name = gcnew String(Native_Devices[i].Device_Name);
				Info->Is_Default = Native_Devices[i].Is_Default;
				Info->Is_Available = Native_Devices[i].Is_Available;
				_Audio_Devices->Add(Info);
			}

			Device_Manager_Audio_Native::Free_Device_List(Native_Devices, Count);
		}
	}

	List<MIDI_Device_Info^>^ Device_Manager::Get_MIDI_Devices()
	{
		return _MIDI_Devices;
	}

	List<Audio_Device_Info^>^ Device_Manager::Get_Audio_Devices()
	{
		return _Audio_Devices;
	}

	int Device_Manager::Find_MIDI_Device_By_Name(String^ device_name)
	{
		for each (MIDI_Device_Info ^ Device in _MIDI_Devices)
		{
			if (Device->Device_Name == device_name)
				return Device->Device_ID;
		}

		return -1;
	}

	String^ Device_Manager::Find_Audio_Device_By_Name(String^ device_name)
	{
		for each (Audio_Device_Info ^ Device in _Audio_Devices)
		{
			if (Device->Device_Name == device_name)
				return Device->Device_ID;
		}

		return nullptr;
	}

	int Device_Manager::Get_Default_MIDI_Device_ID()
	{
		if (_MIDI_Devices->Count > 0) {
			return _MIDI_Devices[0]->Device_ID;
		}
		return -1;
	}

	String^ Device_Manager::Get_Default_Audio_Device_ID()
	{
		for each (Audio_Device_Info ^ Device in _Audio_Devices)
		{
			if (Device->Is_Default)
				return Device->Device_ID;
		}

		if (_Audio_Devices->Count > 0)
			return _Audio_Devices[0]->Device_ID;

		return nullptr;
	}

	String^ Device_Manager::Get_MIDI_Device_Name(int device_id)
	{
		for each (MIDI_Device_Info ^ Device in _MIDI_Devices)
		{
			if (Device->Device_ID == device_id) {
				return Device->Device_Name;
			}
		}
		return nullptr;
	}

	String^ Device_Manager::Get_Audio_Device_Name(String^ device_id)
	{
		for each (Audio_Device_Info ^ Device in _Audio_Devices)
		{
			if (Device->Device_ID == device_id)
				return Device->Device_Name;
		}
		return nullptr;
	}
}
