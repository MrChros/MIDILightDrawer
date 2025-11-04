#pragma once

namespace MIDILightDrawer
{
	class Device_Manager_Audio_Native
	{
	public:
		struct Audio_Device_Native
		{
			wchar_t* Device_ID;
			wchar_t* Device_Name;
			bool Is_Default;
			bool Is_Available;
		};

		static void Initialize();
		static void Cleanup();
		static Audio_Device_Native* Enumerate_Audio_Devices(int& count);
		static void Free_Device_List(Audio_Device_Native* devices, int count);

	private:
		static bool _COM_Initialized;
		static void* _Enumerator;
	};
}


