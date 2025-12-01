#pragma managed(push, off)

#define NOMINMAX

#include "Playback_Audio_Engine_Native.h"
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <chrono>
#include <algorithm>
#include <vector>

#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace MIDILightDrawer
{
	// Static member initialization
	bool Playback_Audio_Engine_Native::_Is_Initialized = false;
	std::atomic<bool> Playback_Audio_Engine_Native::_Is_Playing(false);
	void* Playback_Audio_Engine_Native::_Audio_Client = nullptr;
	void* Playback_Audio_Engine_Native::_Render_Client = nullptr;
	void* Playback_Audio_Engine_Native::_Audio_Event = nullptr;
	float* Playback_Audio_Engine_Native::_Audio_Buffer = nullptr;
	int Playback_Audio_Engine_Native::_Audio_Buffer_Size = 0;
	int Playback_Audio_Engine_Native::_Audio_Sample_Rate_WASAPI = 0;
	int Playback_Audio_Engine_Native::_Audio_Sample_Rate_File = 0;
	int Playback_Audio_Engine_Native::_Audio_Num_Channels = 0;
	int Playback_Audio_Engine_Native::_Audio_Bit_Rate = 0;
	std::atomic<double> Playback_Audio_Engine_Native::_Current_Position_ms(0.0);
	double Playback_Audio_Engine_Native::_Audio_Duration_ms = 0.0;
	double Playback_Audio_Engine_Native::_Audio_Offset_ms = 0.0;
	std::atomic<double> Playback_Audio_Engine_Native::_Audio_Volume(1.0);

	std::vector<Playback_Audio_Engine_Native::Waveform_Segment> Playback_Audio_Engine_Native::_Waveform_Segments;
	int Playback_Audio_Engine_Native::_Samples_Per_Segment = 0;
	int Playback_Audio_Engine_Native::_Segments_Per_Second = 0;

	// Threading members
	std::thread* Playback_Audio_Engine_Native::_Audio_Thread = nullptr;
	std::atomic<bool> Playback_Audio_Engine_Native::_Should_Stop(false);
	std::mutex Playback_Audio_Engine_Native::_Buffer_Mutex;
	int64_t Playback_Audio_Engine_Native::_Total_Audio_Samples = 0;
	int64_t Playback_Audio_Engine_Native::_Current_Sample_Position = 0;

	bool Playback_Audio_Engine_Native::Initialize(const wchar_t* device_id, int buffer_size_samples)
	{
		if (_Is_Initialized) {
			Cleanup();
		}

		return Initialize_WASAPI(device_id, buffer_size_samples);
	}

	void Playback_Audio_Engine_Native::Cleanup()
	{
		// Stop playback thread
		Stop_Playback();

		// Unload audio file
		Unload_Audio_File();

		// Release WASAPI resources
		Cleanup_WASAPI();

		_Is_Initialized = false;
	}

	std::vector<float> Playback_Audio_Engine_Native::Resample_Audio_Linear(const std::vector<float>& input_buffer, int input_sample_rate, int output_sample_rate, int num_channels)
	{
		if (input_sample_rate == output_sample_rate) {
			return input_buffer; // No resampling needed
		}

		int64_t Input_Samples = input_buffer.size() / num_channels;
		double Ratio = (double)output_sample_rate / (double)input_sample_rate;
		int64_t Output_Samples = (int64_t)(Input_Samples * Ratio);

		std::vector<float> Output_Buffer(Output_Samples * num_channels);

		// Use higher-quality cubic interpolation instead of linear
		// This significantly reduces distortion when converting between sample rates
		for (int64_t i = 0; i < Output_Samples; i++) {
			double Source_Pos = i / Ratio;
			int64_t Source_Index = (int64_t)Source_Pos;
			double Fraction = Source_Pos - Source_Index;

			for (int ch = 0; ch < num_channels; ch++) {
				// Get 4 points for cubic interpolation (with bounds checking)
				int64_t Index_Minus1 = std::max((int64_t)0, Source_Index - 1) * num_channels + ch;
				int64_t Index_0 = Source_Index * num_channels + ch;
				int64_t Index_1 = std::min(Source_Index + 1, Input_Samples - 1) * num_channels + ch;
				int64_t Index_2 = std::min(Source_Index + 2, Input_Samples - 1) * num_channels + ch;

				// Ensure indices are within bounds
				Index_Minus1 = std::min(Index_Minus1, (int64_t)input_buffer.size() - 1);
				Index_0 = std::min(Index_0, (int64_t)input_buffer.size() - 1);
				Index_1 = std::min(Index_1, (int64_t)input_buffer.size() - 1);
				Index_2 = std::min(Index_2, (int64_t)input_buffer.size() - 1);

				float Sample_Minus1 = input_buffer[Index_Minus1];
				float Sample_0 = input_buffer[Index_0];
				float Sample_1 = input_buffer[Index_1];
				float Sample_2 = input_buffer[Index_2];

				// Catmull-Rom cubic interpolation
				// This provides much smoother resampling than linear interpolation
				float t = (float)Fraction;
				float t2 = t * t;
				float t3 = t2 * t;

				float a0 = -0.5f * Sample_Minus1 + 1.5f * Sample_0 - 1.5f * Sample_1 + 0.5f * Sample_2;
				float a1 = Sample_Minus1 - 2.5f * Sample_0 + 2.0f * Sample_1 - 0.5f * Sample_2;
				float a2 = -0.5f * Sample_Minus1 + 0.5f * Sample_1;
				float a3 = Sample_0;

				float Interpolated_Sample = a0 * t3 + a1 * t2 + a2 * t + a3;

				// Clamp to prevent any potential overflow
				Interpolated_Sample = std::max(-1.0f, std::min(1.0f, Interpolated_Sample));

				Output_Buffer[i * num_channels + ch] = Interpolated_Sample;
			}
		}

		return Output_Buffer;
	}

	bool Playback_Audio_Engine_Native::Load_Audio_File(const wchar_t* file_path)
	{
		if (!_Is_Initialized) {
			return false;
		}

		// Unload any existing audio
		Unload_Audio_File();

		HRESULT Hr;

		// Initialize Media Foundation
		Hr = MFStartup(MF_VERSION);
		if (FAILED(Hr)) {
			return false;
		}

		// Create source reader
		IMFSourceReader* Source_Reader = nullptr;
		Hr = MFCreateSourceReaderFromURL(file_path, nullptr, &Source_Reader);
		if (FAILED(Hr)) {
			MFShutdown();
			return false;
		}

		// Configure source reader to output PCM float
		// NOTE: We don't force sample rate here - let it decode at native rate
		IMFMediaType* Media_Type = nullptr;
		Hr = MFCreateMediaType(&Media_Type);
		if (FAILED(Hr)) {
			Source_Reader->Release();
			MFShutdown();
			return false;
		}

		Media_Type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		Media_Type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);

		Hr = Source_Reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, Media_Type);
		Media_Type->Release();

		if (FAILED(Hr)) {
			Source_Reader->Release();
			MFShutdown();
			return false;
		}

		// Get actual format from the file
		IMFMediaType* Actual_Type = nullptr;
		Hr = Source_Reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &Actual_Type);
		if (FAILED(Hr)) {
			Source_Reader->Release();
			MFShutdown();
			return false;
		}

		UINT32 File_Channels = 0;
		UINT32 File_Sample_Rate = 0;
		UINT32 File_Bit_Rate = 0;
		Actual_Type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &File_Channels);
		Actual_Type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &File_Sample_Rate);
		Actual_Type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &File_Bit_Rate);
		Actual_Type->Release();

		// Store the file's original sample rate
		_Audio_Sample_Rate_File = File_Sample_Rate;
		_Audio_Bit_Rate = File_Bit_Rate;

		// Read all samples into temporary buffer
		std::vector<float> Temp_Buffer;

		while (true)
		{
			IMFSample* Sample = nullptr;
			DWORD Stream_Flags = 0;

			Hr = Source_Reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &Stream_Flags, nullptr, &Sample);

			if (FAILED(Hr) || (Stream_Flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
				if (Sample) Sample->Release();
				break;
			}

			if (Sample)
			{
				IMFMediaBuffer* Buffer = nullptr;
				Hr = Sample->ConvertToContiguousBuffer(&Buffer);

				if (SUCCEEDED(Hr))
				{
					BYTE* Audio_Data = nullptr;
					DWORD Data_Length = 0;

					Hr = Buffer->Lock(&Audio_Data, nullptr, &Data_Length);
					if (SUCCEEDED(Hr))
					{
						float* Float_Data = (float*)Audio_Data;
						int Sample_Count = Data_Length / sizeof(float);

						Temp_Buffer.insert(Temp_Buffer.end(), Float_Data, Float_Data + Sample_Count);

						Buffer->Unlock();
					}

					Buffer->Release();
				}

				Sample->Release();
			}
		}

		Source_Reader->Release();
		MFShutdown();

		if (Temp_Buffer.empty()) {
			return false;
		}

		// Resample if file sample rate doesn't match WASAPI output
		if (File_Sample_Rate != _Audio_Sample_Rate_WASAPI) {
			// Resample the audio to match WASAPI's sample rate
			Temp_Buffer = Resample_Audio_Linear(Temp_Buffer, _Audio_Sample_Rate_File, _Audio_Sample_Rate_WASAPI, File_Channels);

			// After resampling, use WASAPI's sample rate for all calculations
			_Audio_Sample_Rate_File = _Audio_Sample_Rate_WASAPI;
		}

		// Allocate audio buffer with resampled data
		_Audio_Buffer = new float[Temp_Buffer.size()];
		memcpy(_Audio_Buffer, Temp_Buffer.data(), Temp_Buffer.size() * sizeof(float));

		_Total_Audio_Samples = Temp_Buffer.size() / File_Channels;
		_Audio_Duration_ms = (_Total_Audio_Samples * 1000.0) / _Audio_Sample_Rate_File;
		_Current_Sample_Position = 0;
		_Current_Position_ms.store(0.0);

		return true;
	}

	void Playback_Audio_Engine_Native::Unload_Audio_File()
	{
		std::lock_guard<std::mutex> Lock(_Buffer_Mutex);

		if (_Audio_Buffer)
		{
			delete[] _Audio_Buffer;
			_Audio_Buffer = nullptr;
		}
		_Audio_Duration_ms = 0.0;
		_Total_Audio_Samples = 0;
		_Current_Sample_Position = 0;

		_Waveform_Segments.clear();
		_Samples_Per_Segment = 0;
		_Segments_Per_Second = 0;
	}

	bool Playback_Audio_Engine_Native::Start_Playback()
	{
		if (!_Is_Initialized || !_Audio_Buffer || !_Audio_Client) {
			return false;
		}

		if (_Audio_Thread != nullptr) {
			return false;  // Already playing
		}

		// Reset position if starting from stopped state
		if (_Current_Sample_Position == 0) {
			_Current_Position_ms.store(0.0);
		}

		_Should_Stop.store(false);
		_Is_Playing.store(true);

		// Start WASAPI audio client
		HRESULT Hr = ((IAudioClient*)_Audio_Client)->Start();
		if (FAILED(Hr)) {
			_Is_Playing.store(false);
			return false;
		}

		// Create playback thread
		_Audio_Thread = new std::thread(Audio_Playback_Thread_Function);

		return true;
	}

	bool Playback_Audio_Engine_Native::Stop_Playback()
	{
		if (_Audio_Thread == nullptr) {
			return true;  // Already stopped
		}

		// Signal thread to stop
		_Should_Stop.store(true);
		_Is_Playing.store(false);

		// Set event to wake up thread if waiting
		if (_Audio_Event) {
			SetEvent((HANDLE)_Audio_Event);
		}

		// Wait for thread to finish
		if (_Audio_Thread->joinable()) {
			_Audio_Thread->join();
		}

		delete _Audio_Thread;
		_Audio_Thread = nullptr;

		// Stop WASAPI
		if (_Audio_Client) {
			((IAudioClient*)_Audio_Client)->Stop();
			((IAudioClient*)_Audio_Client)->Reset();
		}

		// Reset position
		_Current_Sample_Position = 0;
		_Current_Position_ms.store(0.0);

		return true;
	}

	bool Playback_Audio_Engine_Native::Pause_Playback()
	{
		if (!_Audio_Buffer || !_Audio_Client) {
			return false;
		}

		_Is_Playing.store(false);

		// Stop WASAPI (but don't reset position)
		((IAudioClient*)_Audio_Client)->Stop();

		// Do NOT set _Should_Stop or kill thread
		// Thread stays alive, waiting for resume

		return true;
	}

	bool Playback_Audio_Engine_Native::Resume_Playback()
	{
		if (!_Audio_Buffer || !_Audio_Client) {
			return false;
		}

		// If thread doesn't exist, start it
		if (_Audio_Thread == nullptr) {
			// Thread was stopped - need to restart it completely
			_Should_Stop.store(false);
			_Is_Playing.store(true);

			// Start WASAPI audio client
			HRESULT Hr = ((IAudioClient*)_Audio_Client)->Start();
			if (FAILED(Hr)) {
				_Is_Playing.store(false);
				return false;
			}

			// Create playback thread
			_Audio_Thread = new std::thread(Audio_Playback_Thread_Function);
			return true;
		}

		// Thread exists, just resume playback
		_Is_Playing.store(true);

		// Restart WASAPI from current position
		HRESULT Hr = ((IAudioClient*)_Audio_Client)->Start();
		if (FAILED(Hr)) {
			_Is_Playing.store(false);
			return false;
		}

		return true;
	}

	bool Playback_Audio_Engine_Native::Seek_To_Position(double position_ms)
	{
		if (!_Audio_Buffer || position_ms < 0.0 || position_ms > _Audio_Duration_ms) {
			return false;
		}

		bool Was_Playing = _Is_Playing.load();

		// Temporarily pause if playing
		if (Was_Playing) {
			_Is_Playing.store(false);
			if (_Audio_Client) {
				((IAudioClient*)_Audio_Client)->Stop();
				((IAudioClient*)_Audio_Client)->Reset();
			}
		}
		
		{
			std::lock_guard<std::mutex> Lock(_Buffer_Mutex);

			// Calculate sample position using FILE's sample rate
			int64_t New_Sample_Position = (int64_t)((position_ms / 1000.0) * _Audio_Sample_Rate_File);
			New_Sample_Position = std::min(New_Sample_Position, _Total_Audio_Samples);

			_Current_Sample_Position = New_Sample_Position;
			_Current_Position_ms.store(position_ms);
		}

		// Resume if it was playing
		if (Was_Playing) {
			_Is_Playing.store(true);
			if (_Audio_Client) {
				((IAudioClient*)_Audio_Client)->Start();
			}
		}

		return true;
	}

	double Playback_Audio_Engine_Native::Get_Current_Position_ms()
	{
		return _Current_Position_ms.load();
	}

	double Playback_Audio_Engine_Native::Get_Audio_Duration_ms()
	{
		return _Audio_Duration_ms;
	}

	bool Playback_Audio_Engine_Native::Is_Playing()
	{
		return _Is_Playing.load();
	}

	bool Playback_Audio_Engine_Native::Is_Audio_Loaded()
	{
		return _Audio_Buffer != nullptr;
	}

	void Playback_Audio_Engine_Native::Set_Offset(double offset_ms)
	{
		std::lock_guard<std::mutex> Lock(_Buffer_Mutex);
		_Audio_Offset_ms = offset_ms;
	}

	void Playback_Audio_Engine_Native::Set_Volume(double volume_percent)
	{
		// Clamp to valid range
		if (volume_percent < 0.0) {
			volume_percent = 0.0;
		}

		if (volume_percent > 1.0) {
			volume_percent = 1.0;
		}

		_Audio_Volume.store(volume_percent);
	}

	void Playback_Audio_Engine_Native::Calculate_Waveform_Data(int segments_per_second)
	{
		if (!Is_Audio_Loaded() || segments_per_second <= 0) {
			return;
		}

		_Segments_Per_Second = segments_per_second;
		Calculate_Waveform_Data_Internal(segments_per_second);
	}

	bool Playback_Audio_Engine_Native::Get_Waveform_Segment(int segment_index, float& min_value, float& max_value)
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

	int Playback_Audio_Engine_Native::Get_Total_Waveform_Segments()
	{
		return (int)_Waveform_Segments.size();
	}

	int Playback_Audio_Engine_Native::Get_Samples_Per_Segment()
	{
		return _Samples_Per_Segment;
	}

	int Playback_Audio_Engine_Native::Get_Sample_Rate_File()
	{
		return _Audio_Sample_Rate_File;
	}

	int Playback_Audio_Engine_Native::Get_Sample_Rate_WASAPI()
	{
		return _Audio_Sample_Rate_WASAPI;
	}

	int Playback_Audio_Engine_Native::Get_Channel_Count()
	{
		return _Audio_Num_Channels;
	}

	int Playback_Audio_Engine_Native::Get_Bit_Rate()
	{
		return _Audio_Bit_Rate;
	}

	bool Playback_Audio_Engine_Native::Get_Audio_Samples(int64_t start_sample, int64_t sample_count, float* output_buffer)
	{
		if (!_Audio_Buffer || !output_buffer || start_sample < 0) {
			return false;
		}

		std::lock_guard<std::mutex> Lock(_Buffer_Mutex);

		int64_t Total_Samples = _Total_Audio_Samples * _Audio_Num_Channels;
		int64_t Start_Index = start_sample * _Audio_Num_Channels;
		int64_t Copy_Count = sample_count * _Audio_Num_Channels;

		if (Start_Index >= Total_Samples)
			return false;

		// Clamp to available data
		if (Start_Index + Copy_Count > Total_Samples) {
			Copy_Count = Total_Samples - Start_Index;
		}

		memcpy(output_buffer, _Audio_Buffer + Start_Index, Copy_Count * sizeof(float));
		return true;
	}

	float* Playback_Audio_Engine_Native::Get_Audio_Buffer_Pointer()
	{
		return _Audio_Buffer;
	}

	int64_t Playback_Audio_Engine_Native::Get_Sample_Count()
	{
		return _Total_Audio_Samples;
	}

	void Playback_Audio_Engine_Native::Audio_Playback_Thread_Function()
	{
		IAudioClient* Audio_Client = (IAudioClient*)_Audio_Client;
		IAudioRenderClient* Render_Client = (IAudioRenderClient*)_Render_Client;
		HANDLE Audio_Event = (HANDLE)_Audio_Event;

		if (!Audio_Client || !Render_Client || !Audio_Event) {
			return;
		}

		// Get buffer frame count
		UINT32 Buffer_Frame_Count = 0;
		Audio_Client->GetBufferSize(&Buffer_Frame_Count);

		// Real-time reference clock
		LARGE_INTEGER Frequency, Playback_Start_Time, Current_Time;
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&Playback_Start_Time);

		// Remember starting sample position
		int64_t Start_Sample_Position = _Current_Sample_Position;

		// Calculate approximate hardware latency
		// WASAPI typically buffers 10-30ms depending on settings
		int64_t Hardware_Latency_Samples = Buffer_Frame_Count / 2;

		while (!_Should_Stop.load())
		{
			// Wait for buffer ready event (or 100ms timeout)
			DWORD Wait_Result = WaitForSingleObject(Audio_Event, 100);

			if (Wait_Result != WAIT_OBJECT_0) {
				// Timeout or error
				if (_Should_Stop.load()) break;
				continue;
			}

			// Check if we should stop before doing any work
			if (_Should_Stop.load()) {
				break;
			}

			if (!_Is_Playing.load())
			{
				// Paused - fill buffer with silence to prevent clicks
				UINT32 Padding_Frames = 0;
				HRESULT Hr = Audio_Client->GetCurrentPadding(&Padding_Frames);
				if (SUCCEEDED(Hr))
				{
					UINT32 Frames_Available = Buffer_Frame_Count - Padding_Frames;

					if (Frames_Available > 0)
					{
						BYTE* Buffer_Data = nullptr;
						Hr = Render_Client->GetBuffer(Frames_Available, &Buffer_Data);
						if (SUCCEEDED(Hr)) {
							memset(Buffer_Data, 0, Frames_Available * _Audio_Num_Channels * sizeof(float));
							Render_Client->ReleaseBuffer(Frames_Available, 0);
						}
					}
				}
				continue;
			}

			// Get available buffer space
			UINT32 Padding_Frames = 0;
			HRESULT Hr = Audio_Client->GetCurrentPadding(&Padding_Frames);
			if (FAILED(Hr)) {
				break;
			}

			UINT32 Frames_Available = Buffer_Frame_Count - Padding_Frames;

			if (Frames_Available == 0) {
				continue;  // Buffer full, wait for next event
			}

			// Get buffer
			BYTE* Buffer_Data = nullptr;
			Hr = Render_Client->GetBuffer(Frames_Available, &Buffer_Data);
			if (FAILED(Hr)) {
				break;
			}

			// Fill buffer with audio data
			float* Float_Buffer = (float*)Buffer_Data;

			{
				std::lock_guard<std::mutex> Lock(_Buffer_Mutex);

				// Convert offset from milliseconds to samples
				int64_t Offset_Samples = (int64_t)((_Audio_Offset_ms / 1000.0) * _Audio_Sample_Rate_File);

				// Calculate adjusted position
				int64_t Adjusted_Sample_Position = _Current_Sample_Position + Offset_Samples;

				// Case 1: Offset is positive and we're before the audio starts
				// Case 2: Offset is negative and we're before the beginning
				// Case 3: We're past the end of the audio
				if (Adjusted_Sample_Position < 0 || Adjusted_Sample_Position >= _Total_Audio_Samples || _Current_Sample_Position >= _Total_Audio_Samples)
				{
					// Play silence (zero samples)
					memset(Float_Buffer, 0, Frames_Available * _Audio_Num_Channels * sizeof(float));

					// Update position normally (ignore offset for position tracking)
					_Current_Sample_Position += Frames_Available;
				}
				else
				{
					// Calculate remaining samples from the adjusted position
					int64_t Remaining_Samples = _Total_Audio_Samples - Adjusted_Sample_Position;
					UINT32 Frames_Can_Read = (UINT32)std::min((int64_t)Frames_Available, Remaining_Samples);

					// Ensure we don't read past the end
					if (Frames_Can_Read > 0 && Adjusted_Sample_Position + Frames_Can_Read <= _Total_Audio_Samples)
					{
						// Copy audio data from adjusted position (interleaved)
						int64_t Source_Index = Adjusted_Sample_Position * _Audio_Num_Channels;
						memcpy(Float_Buffer, &_Audio_Buffer[Source_Index], Frames_Can_Read * _Audio_Num_Channels * sizeof(float));

						double Current_Volume = _Audio_Volume.load();
						if (Current_Volume != 1.0) {  // Optimize: skip if volume is 100%
							for (UINT32 i = 0; i < Frames_Can_Read * _Audio_Num_Channels; i++) {
								Float_Buffer[i] *= (float)Current_Volume;
							}
						}

						// Fill remainder with silence if we couldn't read enough
						if (Frames_Can_Read < Frames_Available) {
							memset(&Float_Buffer[Frames_Can_Read * _Audio_Num_Channels], 0, (Frames_Available - Frames_Can_Read) * _Audio_Num_Channels * sizeof(float));
						}
					}
					else
					{
						// Can't read any frames (shouldn't happen due to earlier check, but be safe)
						memset(Float_Buffer, 0, Frames_Available * _Audio_Num_Channels * sizeof(float));
					}

					// Update position (WITHOUT offset - position tracks playback time, not audio read position)
					_Current_Sample_Position += Frames_Available;
				}

				double Position_Ms = (_Current_Sample_Position * 1000.0) / _Audio_Sample_Rate_File;

				// Account for hardware latency
				int64_t Hardware_Latency_Samples = Buffer_Frame_Count / 2;
				Position_Ms -= (Hardware_Latency_Samples * 1000.0) / _Audio_Sample_Rate_File;

				if (Position_Ms < 0.0) Position_Ms = 0.0;

				_Current_Position_ms.store(Position_Ms, std::memory_order_release);

				if (_Current_Sample_Position >= _Total_Audio_Samples) {
					_Is_Playing.store(false);
				}
			}

			// Release buffer
			Hr = Render_Client->ReleaseBuffer(Frames_Available, 0);
			if (FAILED(Hr)) {
				break;
			}
		}

		// Thread cleanup - stop audio
		if (Audio_Client) {
			Audio_Client->Stop();
		}
	}

	bool Playback_Audio_Engine_Native::Initialize_WASAPI(const wchar_t* device_id, int buffer_size_samples)
	{
		HRESULT Hr;

		// Initialize COM
		Hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(Hr) && Hr != RPC_E_CHANGED_MODE) {
			return false;
		}

		// Get device enumerator
		IMMDeviceEnumerator* Device_Enumerator = nullptr;
		Hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&Device_Enumerator);
		if (FAILED(Hr)) {
			return false;
		}

		// Get audio device
		IMMDevice* Audio_Device = nullptr;
		if (device_id == nullptr || wcslen(device_id) == 0) {
			// Use default device
			Hr = Device_Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Audio_Device);
		}
		else {
			// Use specific device by ID
			Hr = Device_Enumerator->GetDevice(device_id, &Audio_Device);
		}

		Device_Enumerator->Release();

		if (FAILED(Hr)) {
			return false;
		}

		// Activate audio client
		IAudioClient* Audio_Client = nullptr;
		Hr = Audio_Device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&Audio_Client);
		Audio_Device->Release();

		if (FAILED(Hr)) {
			return false;
		}

		// Get mix format
		WAVEFORMATEX* Wave_Format = nullptr;
		Hr = Audio_Client->GetMixFormat(&Wave_Format);
		if (FAILED(Hr)) {
			Audio_Client->Release();
			return false;
		}

		// Store format info
		_Audio_Sample_Rate_WASAPI = Wave_Format->nSamplesPerSec;
		_Audio_Num_Channels = Wave_Format->nChannels;

		// Initialize audio client in shared mode with event-driven callback
		REFERENCE_TIME Buffer_Duration = (REFERENCE_TIME)(buffer_size_samples * 10000000.0 / _Audio_Sample_Rate_WASAPI);

		Hr = Audio_Client->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
			Buffer_Duration,
			0,
			Wave_Format,
			nullptr
		);

		CoTaskMemFree(Wave_Format);

		if (FAILED(Hr)) {
			Audio_Client->Release();
			return false;
		}

		// Create event for callback
		HANDLE Audio_Event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!Audio_Event) {
			Audio_Client->Release();
			return false;
		}

		Hr = Audio_Client->SetEventHandle(Audio_Event);
		if (FAILED(Hr)) {
			CloseHandle(Audio_Event);
			Audio_Client->Release();
			return false;
		}

		// Get actual buffer size
		UINT32 Actual_Buffer_Size = 0;
		Hr = Audio_Client->GetBufferSize(&Actual_Buffer_Size);
		if (FAILED(Hr)) {
			CloseHandle(Audio_Event);
			Audio_Client->Release();
			return false;
		}

		_Audio_Buffer_Size = Actual_Buffer_Size;

		// Get render client
		IAudioRenderClient* Render_Client = nullptr;
		Hr = Audio_Client->GetService(__uuidof(IAudioRenderClient), (void**)&Render_Client);
		if (FAILED(Hr)) {
			CloseHandle(Audio_Event);
			Audio_Client->Release();
			return false;
		}

		// Store interfaces as void pointers
		_Audio_Client = (void*)Audio_Client;
		_Render_Client = (void*)Render_Client;
		_Audio_Event = (void*)Audio_Event;

		_Is_Initialized = true;
		return true;
	}

	void Playback_Audio_Engine_Native::Cleanup_WASAPI()
	{
		// Release WASAPI resources
		if (_Render_Client) {
			((IAudioRenderClient*)_Render_Client)->Release();
			_Render_Client = nullptr;
		}

		if (_Audio_Client) {
			((IAudioClient*)_Audio_Client)->Release();
			_Audio_Client = nullptr;
		}

		if (_Audio_Event) {
			CloseHandle((HANDLE)_Audio_Event);
			_Audio_Event = nullptr;
		}
	}

	void Playback_Audio_Engine_Native::Calculate_Waveform_Data_Internal(int segments_per_second)
	{
		_Waveform_Segments.clear();

		// Create mono data for visualization
		std::vector<float> Mono_Data;

		if (_Audio_Num_Channels == 1)
		{
			// Already mono, use data as-is
			for (int64_t i = 0; i < _Total_Audio_Samples; i++) {
				Mono_Data.push_back(_Audio_Buffer[i]);
			}
		}
		else if (_Audio_Num_Channels == 2)
		{
			// Mix stereo to mono by averaging
			for (int64_t i = 0; i < _Total_Audio_Samples * 2; i += 2)
			{
				float Left = _Audio_Buffer[i];
				float Right = _Audio_Buffer[i + 1];
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
}

#pragma managed(pop)