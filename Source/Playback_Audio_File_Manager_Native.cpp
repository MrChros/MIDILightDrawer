#pragma managed(push, off)

#define NOMINMAX

#include "Playback_Audio_File_Manager_Native.h"
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <algorithm>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace MIDILightDrawer
{
	// Static member initialization
	float* Playback_Audio_File_Manager_Native::_PCM_Data = nullptr;
	int Playback_Audio_File_Manager_Native::_Sample_Rate = 0;
	int Playback_Audio_File_Manager_Native::_Channel_Count = 0;
	int64_t Playback_Audio_File_Manager_Native::_Total_Samples = 0;
	int Playback_Audio_File_Manager_Native::_Bit_Depth = 0;
	double Playback_Audio_File_Manager_Native::_Duration_Milliseconds = 0.0;
	std::vector<Playback_Audio_File_Manager_Native::Waveform_Segment> Playback_Audio_File_Manager_Native::_Waveform_Segments;
	int Playback_Audio_File_Manager_Native::_Samples_Per_Segment = 0;
	int Playback_Audio_File_Manager_Native::_Segments_Per_Second = 0;
	bool Playback_Audio_File_Manager_Native::_MF_Initialized = false;

	bool Playback_Audio_File_Manager_Native::Initialize()
	{
		if (_MF_Initialized) {
			return true;
		}

		return Initialize_Media_Foundation();
	}

	void Playback_Audio_File_Manager_Native::Cleanup()
	{
		Clear_Audio_File();
		Shutdown_Media_Foundation();
	}

	bool Playback_Audio_File_Manager_Native::Initialize_Media_Foundation()
	{
		if (_MF_Initialized) {
			return true;
		}

		HRESULT Hr = MFStartup(MF_VERSION);
		if (SUCCEEDED(Hr))
		{
			_MF_Initialized = true;
			return true;
		}

		return false;
	}

	void Playback_Audio_File_Manager_Native::Shutdown_Media_Foundation()
	{
		if (_MF_Initialized)
		{
			MFShutdown();
			_MF_Initialized = false;
		}
	}

	bool Playback_Audio_File_Manager_Native::Load_Audio_File(const wchar_t* file_path)
	{
		if (!_MF_Initialized)
		{
			if (!Initialize_Media_Foundation()) {
				return false;
			}
		}

		// Clear any previously loaded audio
		Clear_Audio_File();

		bool Success = Load_Audio_File_Internal(file_path);

		if (Success)
		{
			// Calculate duration
			if (_Sample_Rate > 0) {
				_Duration_Milliseconds = ((double)_Total_Samples / (double)_Sample_Rate) * 1000.0;
			}
		}
		else
		{
			Clear_Audio_File();
		}

		return Success;
	}

	bool Playback_Audio_File_Manager_Native::Load_Audio_File_Internal(const wchar_t* file_path)
	{
		// Create source reader
		IMFSourceReader* Source_Reader = nullptr;
		HRESULT Hr = MFCreateSourceReaderFromURL(file_path, nullptr, &Source_Reader);

		if (FAILED(Hr))
		{
			return false;
		}

		// Configure the source reader to output PCM audio
		IMFMediaType* PCM_Type = nullptr;
		Hr = MFCreateMediaType(&PCM_Type);

		if (SUCCEEDED(Hr))
		{
			Hr = PCM_Type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);

			if (SUCCEEDED(Hr)) {
				Hr = PCM_Type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
			}

			if (SUCCEEDED(Hr)) {
				Hr = Source_Reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, PCM_Type);
			}

			PCM_Type->Release();
		}

		if (FAILED(Hr))
		{
			Source_Reader->Release();
			return false;
		}

		// Get the complete media type (with sample rate, channels, etc.)
		IMFMediaType* Complete_Type = nullptr;
		Hr = Source_Reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &Complete_Type);

		if (SUCCEEDED(Hr))
		{
			// Extract audio format info
			UINT32 Channels = 0;
			UINT32 Sample_Rate = 0;
			UINT32 Bits_Per_Sample = 0;

			Complete_Type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &Channels);
			Complete_Type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &Sample_Rate);
			Complete_Type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &Bits_Per_Sample);

			_Channel_Count = Channels;
			_Sample_Rate = Sample_Rate;
			_Bit_Depth = Bits_Per_Sample;

			Complete_Type->Release();
		}

		if (FAILED(Hr) || _Sample_Rate == 0 || _Channel_Count == 0)
		{
			Source_Reader->Release();
			return false;
		}

		// Decode all PCM data
		bool Decode_Success = Decode_PCM_Data((void*)Source_Reader);

		Source_Reader->Release();

		return Decode_Success;
	}

	bool Playback_Audio_File_Manager_Native::Decode_PCM_Data(void* source_reader)
	{
		IMFSourceReader* Source_Reader = (IMFSourceReader*)source_reader;
		std::vector<float> PCM_Buffer;

		while (true)
		{
			DWORD Stream_Flags = 0;
			IMFSample* Sample = nullptr;

			HRESULT Hr = Source_Reader->ReadSample(
				(DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
				0,
				nullptr,
				&Stream_Flags,
				nullptr,
				&Sample
			);

			// Check for end of stream
			if (Stream_Flags & MF_SOURCE_READERF_ENDOFSTREAM) {
				break;
			}

			if (FAILED(Hr)) {
				return false;
			}

			if (Sample == nullptr) {
				continue;
			}

			// Get buffer from sample
			IMFMediaBuffer* Buffer = nullptr;
			Hr = Sample->ConvertToContiguousBuffer(&Buffer);

			if (SUCCEEDED(Hr))
			{
				BYTE* Audio_Data = nullptr;
				DWORD Audio_Data_Length = 0;

				Hr = Buffer->Lock(&Audio_Data, nullptr, &Audio_Data_Length);

				if (SUCCEEDED(Hr))
				{
					// Audio_Data contains float PCM samples
					int Sample_Count = Audio_Data_Length / sizeof(float);
					float* Float_Data = reinterpret_cast<float*>(Audio_Data);

					for (int i = 0; i < Sample_Count; i++) {
						PCM_Buffer.push_back(Float_Data[i]);
					}

					Buffer->Unlock();
				}

				Buffer->Release();
			}

			Sample->Release();
		}

		// Convert vector to array
		_Total_Samples = (int64_t)PCM_Buffer.size() / _Channel_Count; // Samples per channel

		if (_Total_Samples > 0)
		{
			_PCM_Data = new float[PCM_Buffer.size()];
			std::copy(PCM_Buffer.begin(), PCM_Buffer.end(), _PCM_Data);
		}

		return true;
	}

	void Playback_Audio_File_Manager_Native::Clear_Audio_File()
	{
		if (_PCM_Data != nullptr)
		{
			delete[] _PCM_Data;
			_PCM_Data = nullptr;
		}

		_Sample_Rate = 0;
		_Channel_Count = 0;
		_Total_Samples = 0;
		_Bit_Depth = 0;
		_Duration_Milliseconds = 0.0;

		_Waveform_Segments.clear();
		_Samples_Per_Segment = 0;
		_Segments_Per_Second = 0;
	}

	void Playback_Audio_File_Manager_Native::Calculate_Waveform_Data(int segments_per_second)
	{
		if (!Has_Audio_File() || segments_per_second <= 0) {
			return;
		}

		_Segments_Per_Second = segments_per_second;
		Calculate_Waveform_Data_Internal(segments_per_second);
	}

	void Playback_Audio_File_Manager_Native::Calculate_Waveform_Data_Internal(int segments_per_second)
	{
		_Waveform_Segments.clear();

		// Create mono data for visualization
		std::vector<float> Mono_Data;

		if (_Channel_Count == 1)
		{
			// Already mono, use data as-is
			for (int64_t i = 0; i < _Total_Samples; i++) {
				Mono_Data.push_back(_PCM_Data[i]);
			}
		}
		else if (_Channel_Count == 2)
		{
			// Mix stereo to mono by averaging
			for (int64_t i = 0; i < _Total_Samples * 2; i += 2)
			{
				float Left = _PCM_Data[i];
				float Right = _PCM_Data[i + 1];
				float Mono_Sample = (Left + Right) * 0.5f;
				Mono_Data.push_back(Mono_Sample);
			}
		}

		if (Mono_Data.empty()) {
			return;
		}

		// Calculate how many audio samples should be represented by each visual segment
		int64_t Mono_Sample_Count = (int64_t)Mono_Data.size();
		_Samples_Per_Segment = (int)std::max((int64_t)1, Mono_Sample_Count / segments_per_second);
		int Total_Segments = (int)((Mono_Sample_Count + _Samples_Per_Segment - 1) / _Samples_Per_Segment);

		// Pre-calculate min/max for each segment
		for (int Segment_Index = 0; Segment_Index < Total_Segments; Segment_Index++)
		{
			int64_t Start_Sample = (int64_t)Segment_Index * _Samples_Per_Segment;
			int64_t End_Sample = std::min(Start_Sample + _Samples_Per_Segment, Mono_Sample_Count);

			float Min_Value = 0.0f;
			float Max_Value = 0.0f;

			// Find min and max values in this segment
			for (int64_t i = Start_Sample; i < End_Sample; i++)
			{
				float Sample_Value = Mono_Data[i];

				if (Sample_Value < Min_Value) {
					Min_Value = Sample_Value;
				}
				if (Sample_Value > Max_Value) {
					Max_Value = Sample_Value;
				}
			}

			Waveform_Segment Segment;
			Segment.Min_Value = Min_Value;
			Segment.Max_Value = Max_Value;
			_Waveform_Segments.push_back(Segment);
		}
	}

	bool Playback_Audio_File_Manager_Native::Get_PCM_Samples_At_Time(double time_milliseconds, int sample_count, float* output_buffer, int* samples_written)
	{
		if (!Has_Audio_File() || _Sample_Rate == 0 || output_buffer == nullptr || samples_written == nullptr) {
			return false;
		}

		// Calculate starting sample index
		int64_t Start_Sample_Index = (int64_t)((time_milliseconds / 1000.0) * _Sample_Rate);
		Start_Sample_Index *= _Channel_Count; // Account for interleaved channels

		if (Start_Sample_Index < 0 || Start_Sample_Index >= _Total_Samples * _Channel_Count) {
			*samples_written = 0;
			return false;
		}

		int64_t Total_Samples_In_Buffer = _Total_Samples * _Channel_Count;
		int64_t Samples_To_Copy = std::min((int64_t)(sample_count * _Channel_Count), Total_Samples_In_Buffer - Start_Sample_Index);

		std::copy(_PCM_Data + Start_Sample_Index, _PCM_Data + Start_Sample_Index + Samples_To_Copy, output_buffer);
		*samples_written = (int)Samples_To_Copy;

		return true;
	}

	bool Playback_Audio_File_Manager_Native::Get_Waveform_Segment(int segment_index, float& min_value, float& max_value)
	{
		if (segment_index >= 0 && segment_index < (int)_Waveform_Segments.size())
		{
			min_value = _Waveform_Segments[segment_index].Min_Value;
			max_value = _Waveform_Segments[segment_index].Max_Value;
			return true;
		}

		min_value = 0.0f;
		max_value = 0.0f;
		return false;
	}

	bool Playback_Audio_File_Manager_Native::Has_Audio_File()
	{
		return _PCM_Data != nullptr && _Total_Samples > 0;
	}

	int Playback_Audio_File_Manager_Native::Get_Sample_Rate()
	{
		return _Sample_Rate;
	}

	int Playback_Audio_File_Manager_Native::Get_Channel_Count()
	{
		return _Channel_Count;
	}

	int64_t Playback_Audio_File_Manager_Native::Get_Total_Samples()
	{
		return _Total_Samples;
	}

	int Playback_Audio_File_Manager_Native::Get_Bit_Depth()
	{
		return _Bit_Depth;
	}

	double Playback_Audio_File_Manager_Native::Get_Duration_Milliseconds()
	{
		return _Duration_Milliseconds;
	}

	int Playback_Audio_File_Manager_Native::Get_Total_Waveform_Segments()
	{
		return (int)_Waveform_Segments.size();
	}

	int Playback_Audio_File_Manager_Native::Get_Samples_Per_Segment()
	{
		return _Samples_Per_Segment;
	}

	const float* Playback_Audio_File_Manager_Native::Get_PCM_Data_Pointer()
	{
		return _PCM_Data;
	}
}

#pragma managed(pop)