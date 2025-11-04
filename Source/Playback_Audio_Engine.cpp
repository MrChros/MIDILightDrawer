#include "Playback_Audio_Engine.h"

#include <vcclr.h>

namespace MIDILightDrawer
{
	Playback_Audio_Engine::Playback_Audio_Engine()
	{
		_Is_Initialized = false;
	}

	Playback_Audio_Engine::~Playback_Audio_Engine()
	{
		Cleanup();
	}

	bool Playback_Audio_Engine::Initialize(String^ device_id, int buffer_size_samples)
	{
		if (!device_id) {
			return false;
		}

		pin_ptr<const wchar_t> Device_ID_Pin = PtrToStringChars(device_id);
		_Is_Initialized = Playback_Audio_Engine_Native::Initialize(Device_ID_Pin, buffer_size_samples);
		return _Is_Initialized;
	}

	void Playback_Audio_Engine::Cleanup()
	{
		if (_Is_Initialized)
		{
			Playback_Audio_Engine_Native::Cleanup();
			_Is_Initialized = false;
		}
	}

	bool Playback_Audio_Engine::Load_Audio_File(String^ file_path)
	{
		if (!file_path) {
			return false;
		}

		pin_ptr<const wchar_t> File_Path_Pin = PtrToStringChars(file_path);
		return Playback_Audio_Engine_Native::Load_Audio_File(File_Path_Pin);
	}

	void Playback_Audio_Engine::Unload_Audio_File()
	{
		Playback_Audio_Engine_Native::Unload_Audio_File();
	}

	bool Playback_Audio_Engine::Start_Playback()
	{
		return Playback_Audio_Engine_Native::Start_Playback();
	}

	bool Playback_Audio_Engine::Stop_Playback()
	{
		return Playback_Audio_Engine_Native::Stop_Playback();
	}

	bool Playback_Audio_Engine::Pause_Playback()
	{
		return Playback_Audio_Engine_Native::Pause_Playback();
	}

	bool Playback_Audio_Engine::Resume_Playback()
	{
		return Playback_Audio_Engine_Native::Resume_Playback();
	}

	bool Playback_Audio_Engine::Seek_To_Position(double position_ms)
	{
		return Playback_Audio_Engine_Native::Seek_To_Position(position_ms);
	}

	double Playback_Audio_Engine::Get_Current_Position_Ms()
	{
		return Playback_Audio_Engine_Native::Get_Current_Position_Ms();
	}

	double Playback_Audio_Engine::Get_Audio_Duration_Ms()
	{
		return Playback_Audio_Engine_Native::Get_Audio_Duration_Ms();
	}

	bool Playback_Audio_Engine::Is_Playing()
	{
		return Playback_Audio_Engine_Native::Is_Playing();
	}

	bool Playback_Audio_Engine::Is_Audio_Loaded()
	{
		return Playback_Audio_Engine_Native::Is_Audio_Loaded();
	}
}
