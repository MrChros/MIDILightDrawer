#include "Device_Manager_Audio_Enumerator.h"

#include <Windows.h>
//#include <mmeapi.h>
#include <mmdeviceapi.h>

#include <functiondiscoverykeys_devpkey.h>

namespace MIDILightDrawer
{
	// Static member initialization
	bool Device_Manager_Audio_Enumerator::_COM_Initialized = false;
	void* Device_Manager_Audio_Enumerator::_Enumerator = nullptr;
	
	void Device_Manager_Audio_Enumerator::Initialize()
	{
		if (!_COM_Initialized)
		{
			CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&_Enumerator);
			_COM_Initialized = true;
		}
	}

	void Device_Manager_Audio_Enumerator::Cleanup()
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

	List<Audio_Device_Info^>^ Device_Manager_Audio_Enumerator::Enumerate_Audio_Devices()
	{
		Initialize();
		List<Audio_Device_Info^>^ Devices = gcnew List<Audio_Device_Info^>();

		IMMDeviceEnumerator* Enumerator = (IMMDeviceEnumerator*)_Enumerator;
		
		if (!Enumerator) {
			return Devices;
		}

		// Get device collection
		IMMDeviceCollection* Collection = nullptr;
		HRESULT Hr = Enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &Collection);

		if (FAILED(Hr)) {
			return Devices;
		}

		// Get device count
		UINT Count = 0;
		Collection->GetCount(&Count);

		// Get default device ID
		IMMDevice* Default_Device = nullptr;
		String^ Default_ID = nullptr;
		if (SUCCEEDED(Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Default_Device)))
		{
			LPWSTR Id;
			if (SUCCEEDED(Default_Device->GetId(&Id)))
			{
				Default_ID = gcnew String(Id);
				CoTaskMemFree(Id);
			}
			Default_Device->Release();
		}

		// Enumerate devices
		for (UINT i = 0; i < Count; ++i)
		{
			IMMDevice* Device = nullptr;
			Hr = Collection->Item(i, &Device);

			if (SUCCEEDED(Hr))
			{
				Audio_Device_Info^ Info = gcnew Audio_Device_Info();

				// Get device ID
				LPWSTR Device_ID;
				if (SUCCEEDED(Device->GetId(&Device_ID)))
				{
					Info->Device_ID = gcnew String(Device_ID);
					Info->Is_Default = (Default_ID != nullptr && Info->Device_ID == Default_ID);
					CoTaskMemFree(Device_ID);
				}

				// Get device friendly name
				IPropertyStore* Props = nullptr;
				if (SUCCEEDED(Device->OpenPropertyStore(STGM_READ, &Props)))
				{
					PROPVARIANT Var_Name;
					PropVariantInit(&Var_Name);

					if (SUCCEEDED(Props->GetValue(PKEY_Device_FriendlyName, &Var_Name)))
					{
						Info->Device_Name = gcnew String(Var_Name.pwszVal);
						PropVariantClear(&Var_Name);
					}
					Props->Release();
				}

				Info->Is_Available = true;
				Devices->Add(Info);

				Device->Release();
			}
		}

		Collection->Release();
		
		return Devices;
	}


}