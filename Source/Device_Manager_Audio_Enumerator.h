#pragma once

// Prevent Windows.h from including OLE headers that conflict with .NET
//#define _OLE2_H_
//#define _OLEIDL_H_

//#include <Windows.h>
//#include <mmeapi.h>
//#include <mmdeviceapi.h>

//#include <objbase.h>

using namespace System;
using namespace System::Collections::Generic;

// Forward declarations - no Windows.h needed in header
struct IMMDeviceEnumerator;

namespace MIDILightDrawer
{
	public ref struct Audio_Device_Info
	{
		String^ Device_ID;
		String^ Device_Name;
		bool Is_Default;
		bool Is_Available;

		Audio_Device_Info() : Is_Default(false), Is_Available(false) {}
	};
	
	class Device_Manager_Audio_Enumerator
	{
	private:
		static bool _COM_Initialized;
		static void* _Enumerator;

	public:
		static void Initialize();
		static void Cleanup();
		static List<Audio_Device_Info^>^ Enumerate_Audio_Devices();
	};
}

