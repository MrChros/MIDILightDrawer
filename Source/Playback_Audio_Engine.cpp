#include "Playback_Audio_Engine.h"

#include <vcclr.h>

namespace MIDILightDrawer
{
	//////////////////////////////////////////
	// Waveform_Render_Data Implementation //
	//////////////////////////////////////////

	Waveform_Render_Data::Waveform_Render_Data()
	{
		_Total_Segments = 0;
		_Samples_Per_Segment = 0;
	}

	Waveform_Render_Data::~Waveform_Render_Data()
	{
		// No cleanup needed - native class manages its own memory
	}

	void Waveform_Render_Data::GetSegmentMinMax(int segment_index, float% min_value, float% max_value)
	{
		float Min_Val = 0.0f;
		float Max_Val = 0.0f;

		if (Playback_Audio_Engine_Native::Get_Waveform_Segment(segment_index, Min_Val, Max_Val))
		{
			min_value = Min_Val;
			max_value = Max_Val;
		}
		else
		{
			min_value = 0.0f;
			max_value = 0.0f;
		}
	}

	void Waveform_Render_Data::Update_From_Native()
	{
		_Total_Segments = Playback_Audio_Engine_Native::Get_Total_Waveform_Segments();
		_Samples_Per_Segment = Playback_Audio_Engine_Native::Get_Samples_Per_Segment();
	}
	
	
	//////////////////////////////////////////
	// Playback_Audio_Engine Implementation //
	//////////////////////////////////////////

	Playback_Audio_Engine::Playback_Audio_Engine()
	{
		_File_Path = "";
		_Waveform_Data = gcnew Waveform_Render_Data();
		_Is_Initialized = false;
	}

	Playback_Audio_Engine::~Playback_Audio_Engine()
	{
		Cleanup();
	}

	bool Playback_Audio_Engine::Initialize(String^ device_id, int buffer_size_samples)
	{
		const wchar_t* Device_ID_Native = nullptr;

		if (device_id && device_id->Length > 0) {
			pin_ptr<const wchar_t> Device_ID_Pin = PtrToStringChars(device_id);
			Device_ID_Native = Device_ID_Pin;
		}

		_Is_Initialized = Playback_Audio_Engine_Native::Initialize(Device_ID_Native, buffer_size_samples);
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

		pin_ptr<const wchar_t> File_Path_WChar = PtrToStringChars(file_path);
		bool Success = Playback_Audio_Engine_Native::Load_Audio_File(File_Path_WChar);

		if (Success) {
			_File_Path = file_path;
		}

		return Success;
	}

	void Playback_Audio_Engine::Unload_Audio_File()
	{
		Playback_Audio_Engine_Native::Unload_Audio_File();
	}

	void Playback_Audio_Engine::CalculateWaveformData(int segments_per_second)
	{
		if (!Is_Audio_Loaded) {
			return;
		}

		if (segments_per_second <= 0) {
			segments_per_second = 100; // Default
		}

		Playback_Audio_Engine_Native::Calculate_Waveform_Data(segments_per_second);

		// Update managed waveform wrapper with native data
		_Waveform_Data->Update_From_Native();
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
		return Playback_Audio_Engine_Native::Get_Current_Position_ms();
	}

	double Playback_Audio_Engine::Get_Audio_Duration_Ms()
	{
		return Playback_Audio_Engine_Native::Get_Audio_Duration_ms();
	}

	bool Playback_Audio_Engine::Is_Playing()
	{
		return Playback_Audio_Engine_Native::Is_Playing();
	}

	void Playback_Audio_Engine::Set_MIDI_Position_Us(int64_t position_us)
	{
		Playback_Audio_Engine_Native::Set_MIDI_Position_Us(position_us);
	}

	void Playback_Audio_Engine::Enable_MIDI_Sync(bool enable)
	{
		Playback_Audio_Engine_Native::Enable_MIDI_Sync(enable);
	}
}