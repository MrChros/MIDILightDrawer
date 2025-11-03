#pragma managed(push, off)

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

#include "Device_Manager_Audio_Native.h"

#include <Windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

bool Device_Manager_Audio_Native::_COM_Initialized = false;
void* Device_Manager_Audio_Native::_Enumerator = nullptr;

void Device_Manager_Audio_Native::Initialize()
{
	if (!_COM_Initialized)
	{
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), &_Enumerator);
		_COM_Initialized = true;
	}
}

void Device_Manager_Audio_Native::Cleanup()
{
	if (_Enumerator)
	{
		((IMMDeviceEnumerator*)_Enumerator)->Release();
		_Enumerator = nullptr;
	}
	if (_COM_Initialized)
	{
		CoUninitialize();
		_COM_Initialized = false;
	}
}

Device_Manager_Audio_Native::Audio_Device_Native* Device_Manager_Audio_Native::Enumerate_Audio_Devices(int& count)
{
	Initialize();
	count = 0;

	IMMDeviceEnumerator* Enumerator = (IMMDeviceEnumerator*)_Enumerator;
	if (!Enumerator) return nullptr;

	IMMDeviceCollection* Collection = nullptr;
	if (FAILED(Enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &Collection)))
		return nullptr;

	UINT Count = 0;
	Collection->GetCount(&Count);

	Audio_Device_Native* Devices = new Audio_Device_Native[Count];

	// Get default device ID
	IMMDevice* Default_Device = nullptr;
	wchar_t* Default_ID = nullptr;
	if (SUCCEEDED(Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Default_Device)))
	{
		Default_Device->GetId(&Default_ID);
		Default_Device->Release();
	}

	for (UINT i = 0; i < Count; ++i)
	{
		IMMDevice* Device = nullptr;
		if (SUCCEEDED(Collection->Item(i, &Device)))
		{
			LPWSTR Device_ID;
			if (SUCCEEDED(Device->GetId(&Device_ID)))
			{
				Devices[i].Device_ID = _wcsdup(Device_ID);
				Devices[i].Is_Default = (Default_ID && wcscmp(Device_ID, Default_ID) == 0);
				CoTaskMemFree(Device_ID);
			}

			IPropertyStore* Props = nullptr;
			if (SUCCEEDED(Device->OpenPropertyStore(STGM_READ, &Props)))
			{
				PROPVARIANT Var_Name;
				PropVariantInit(&Var_Name);
				if (SUCCEEDED(Props->GetValue(PKEY_Device_FriendlyName, &Var_Name)))
				{
					Devices[i].Device_Name = _wcsdup(Var_Name.pwszVal);
					PropVariantClear(&Var_Name);
				}
				Props->Release();
			}

			Devices[i].Is_Available = true;
			Device->Release();
		}
	}

	if (Default_ID) CoTaskMemFree(Default_ID);
	Collection->Release();

	count = Count;
	return Devices;
}

void Device_Manager_Audio_Native::Free_Device_List(Audio_Device_Native* devices, int count)
{
	for (int i = 0; i < count; ++i)
	{
		free(devices[i].Device_ID);
		free(devices[i].Device_Name);
	}
	delete[] devices;
}

#pragma managed(pop)