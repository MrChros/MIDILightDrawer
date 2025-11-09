#include "Playback_Audio_File_Manager.h"
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

		if (Playback_Audio_File_Manager_Native::Get_Waveform_Segment(segment_index, Min_Val, Max_Val))
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
		_Total_Segments = Playback_Audio_File_Manager_Native::Get_Total_Waveform_Segments();
		_Samples_Per_Segment = Playback_Audio_File_Manager_Native::Get_Samples_Per_Segment();
	}

	////////////////////////////////////////////////
	// Playback_Audio_File_Manager Implementation //
	////////////////////////////////////////////////

	Playback_Audio_File_Manager::Playback_Audio_File_Manager()
	{
		_File_Path = nullptr;
		_Waveform_Data = gcnew Waveform_Render_Data();
		_Is_Initialized = false;
	}

	Playback_Audio_File_Manager::~Playback_Audio_File_Manager()
	{
		Cleanup();
	}

	bool Playback_Audio_File_Manager::Initialize()
	{
		if (_Is_Initialized) {
			return true;
		}

		_Is_Initialized = Playback_Audio_File_Manager_Native::Initialize();
		return _Is_Initialized;
	}

	void Playback_Audio_File_Manager::Cleanup()
	{
		if (_Is_Initialized)
		{
			Playback_Audio_File_Manager_Native::Cleanup();
			_Is_Initialized = false;
		}

		_File_Path = nullptr;
		_Waveform_Data = gcnew Waveform_Render_Data();
	}

	bool Playback_Audio_File_Manager::LoadAudioFile(String^ file_path, String^% error_message)
	{
		error_message = String::Empty;

		if (!_Is_Initialized)
		{
			if (!Initialize())
			{
				error_message = "Failed to initialize Media Foundation";
				return false;
			}
		}

		if (file_path == nullptr || file_path->Length == 0)
		{
			error_message = "Invalid file path";
			return false;
		}

		// Convert managed string to native wstring
		pin_ptr<const wchar_t> File_Path_Pin = PtrToStringChars(file_path);

		bool Success = Playback_Audio_File_Manager_Native::Load_Audio_File(File_Path_Pin);

		if (Success)
		{
			_File_Path = file_path;
		}
		else
		{
			error_message = "Failed to load audio file. File may be corrupted or format not supported.";
			_File_Path = nullptr;
		}

		return Success;
	}

	void Playback_Audio_File_Manager::ClearAudioFile()
	{
		Playback_Audio_File_Manager_Native::Clear_Audio_File();
		_File_Path = nullptr;
		_Waveform_Data = gcnew Waveform_Render_Data();
	}

	void Playback_Audio_File_Manager::CalculateWaveformData(int segments_per_second)
	{
		if (!HasAudioFile) {
			return;
		}

		if (segments_per_second <= 0) {
			segments_per_second = 100; // Default
		}

		Playback_Audio_File_Manager_Native::Calculate_Waveform_Data(segments_per_second);

		// Update managed waveform wrapper with native data
		_Waveform_Data->Update_From_Native();
	}

	array<float>^ Playback_Audio_File_Manager::GetPCMSamplesAtTime(double time_milliseconds, int sample_count)
	{
		if (!HasAudioFile || sample_count <= 0) {
			return nullptr;
		}

		int Channel_Count = Playback_Audio_File_Manager_Native::Get_Channel_Count();
		int Total_Values = sample_count * Channel_Count;

		// Create temporary native buffer
		float* Native_Buffer = new float[Total_Values];
		int Samples_Written = 0;

		bool Success = Playback_Audio_File_Manager_Native::Get_PCM_Samples_At_Time(
			time_milliseconds,
			sample_count,
			Native_Buffer,
			&Samples_Written
		);

		array<float>^ Result = nullptr;

		if (Success && Samples_Written > 0)
		{
			Result = gcnew array<float>(Samples_Written);

			// Copy from native to managed array
			for (int i = 0; i < Samples_Written; i++) {
				Result[i] = Native_Buffer[i];
			}
		}

		delete[] Native_Buffer;

		return Result;
	}
}