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
	int Playback_Audio_Engine_Native::_Buffer_Size_Samples = 0;
	int Playback_Audio_Engine_Native::_Sample_Rate = 44100;
	int Playback_Audio_Engine_Native::_Num_Channels = 2;
	std::atomic<double> Playback_Audio_Engine_Native::_Current_Position_Ms(0.0);
	double Playback_Audio_Engine_Native::_Audio_Duration_Ms = 0.0;

	// Threading members
	std::thread* Playback_Audio_Engine_Native::_Audio_Thread = nullptr;
	std::atomic<bool> Playback_Audio_Engine_Native::_Should_Stop(false);
	std::mutex Playback_Audio_Engine_Native::_Buffer_Mutex;
	int64_t Playback_Audio_Engine_Native::_Total_Audio_Samples = 0;
	int64_t Playback_Audio_Engine_Native::_Current_Sample_Position = 0;
	std::atomic<int64_t> Playback_Audio_Engine_Native::_MIDI_Position_Us(0);
	bool Playback_Audio_Engine_Native::_Sync_To_MIDI = false;

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
		IMFMediaType* Media_Type = nullptr;
		Hr = MFCreateMediaType(&Media_Type);
		if (FAILED(Hr)) {
			Source_Reader->Release();
			MFShutdown();
			return false;
		}

		Media_Type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		Media_Type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
		Media_Type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _Num_Channels);
		Media_Type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _Sample_Rate);

		Hr = Source_Reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, Media_Type);
		Media_Type->Release();

		if (FAILED(Hr)) {
			Source_Reader->Release();
			MFShutdown();
			return false;
		}

		// Get actual format
		IMFMediaType* Actual_Type = nullptr;
		Hr = Source_Reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &Actual_Type);
		if (FAILED(Hr)) {
			Source_Reader->Release();
			MFShutdown();
			return false;
		}

		UINT32 Channels = 0;
		UINT32 Sample_Rate = 0;
		Actual_Type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &Channels);
		Actual_Type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &Sample_Rate);
		Actual_Type->Release();

		// Read all samples into buffer
		std::vector<float> Temp_Buffer;

		while (true) {
			IMFSample* Sample = nullptr;
			DWORD Stream_Flags = 0;

			Hr = Source_Reader->ReadSample(
				(DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
				0,
				nullptr,
				&Stream_Flags,
				nullptr,
				&Sample
			);

			if (FAILED(Hr) || (Stream_Flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
				if (Sample) Sample->Release();
				break;
			}

			if (Sample) {
				IMFMediaBuffer* Buffer = nullptr;
				Hr = Sample->ConvertToContiguousBuffer(&Buffer);

				if (SUCCEEDED(Hr)) {
					BYTE* Audio_Data = nullptr;
					DWORD Data_Length = 0;

					Hr = Buffer->Lock(&Audio_Data, nullptr, &Data_Length);
					if (SUCCEEDED(Hr)) {
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

		// Allocate audio buffer
		_Audio_Buffer = new float[Temp_Buffer.size()];
		memcpy(_Audio_Buffer, Temp_Buffer.data(), Temp_Buffer.size() * sizeof(float));

		_Total_Audio_Samples = Temp_Buffer.size() / Channels;
		_Audio_Duration_Ms = (_Total_Audio_Samples * 1000.0) / Sample_Rate;
		_Current_Sample_Position = 0;

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
		_Audio_Duration_Ms = 0.0;
		_Total_Audio_Samples = 0;
		_Current_Sample_Position = 0;
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
			_Current_Position_Ms.store(0.0);
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
		_Current_Position_Ms.store(0.0);

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

		return true;
	}

	bool Playback_Audio_Engine_Native::Resume_Playback()
	{
		if (!_Audio_Buffer || !_Audio_Client) {
			return false;
		}

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
		if (!_Audio_Buffer || position_ms < 0.0 || position_ms > _Audio_Duration_Ms) {
			return false;
		}

		std::lock_guard<std::mutex> Lock(_Buffer_Mutex);

		// Calculate sample position
		int64_t New_Sample_Position = (int64_t)((position_ms / 1000.0) * _Sample_Rate);
		New_Sample_Position = std::min(New_Sample_Position, _Total_Audio_Samples);

		_Current_Sample_Position = New_Sample_Position;
		_Current_Position_Ms.store(position_ms);

		// Reset WASAPI buffer if playing
		if (_Is_Playing.load() && _Audio_Client) {
			((IAudioClient*)_Audio_Client)->Stop();
			((IAudioClient*)_Audio_Client)->Reset();
			((IAudioClient*)_Audio_Client)->Start();
		}

		return true;
	}

	double Playback_Audio_Engine_Native::Get_Current_Position_Ms()
	{
		return _Current_Position_Ms.load();
	}

	double Playback_Audio_Engine_Native::Get_Audio_Duration_Ms()
	{
		return _Audio_Duration_Ms;
	}

	bool Playback_Audio_Engine_Native::Is_Playing()
	{
		return _Is_Playing.load();
	}

	bool Playback_Audio_Engine_Native::Is_Audio_Loaded()
	{
		return _Audio_Buffer != nullptr;
	}

	void Playback_Audio_Engine_Native::Set_MIDI_Position_Us(int64_t position_us)
	{
		_MIDI_Position_Us.store(position_us, std::memory_order_relaxed);
	}

	void Playback_Audio_Engine_Native::Enable_MIDI_Sync(bool enable)
	{
		_Sync_To_MIDI = enable;
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

		while (!_Should_Stop.load())
		{
			// Wait for buffer ready event (or 100ms timeout)
			DWORD Wait_Result = WaitForSingleObject(Audio_Event, 100);

			if (Wait_Result != WAIT_OBJECT_0) {
				// Timeout or error
				if (_Should_Stop.load()) break;
				continue;
			}

			if (!_Is_Playing.load()) {
				// Paused - just continue waiting
				continue;
			}

			// Check if we need to sync with MIDI
			if (_Sync_To_MIDI) {
				int64_t MIDI_Pos_Us = _MIDI_Position_Us.load(std::memory_order_relaxed);
				double MIDI_Pos_Ms = MIDI_Pos_Us / 1000.0;
				double Audio_Pos_Ms = _Current_Position_Ms.load();

				// If audio is drifting more than 10ms from MIDI, adjust
				double Drift_Ms = Audio_Pos_Ms - MIDI_Pos_Ms;
				if (abs(Drift_Ms) > 10.0) {
					// Adjust audio position to match MIDI
					std::lock_guard<std::mutex> Lock(_Buffer_Mutex);
					int64_t New_Sample_Position = (int64_t)((MIDI_Pos_Ms / 1000.0) * _Sample_Rate);
					New_Sample_Position = std::min(New_Sample_Position, _Total_Audio_Samples);
					_Current_Sample_Position = New_Sample_Position;
					_Current_Position_Ms.store(MIDI_Pos_Ms);
				}
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
			UINT32 Frames_To_Write = Frames_Available;

			{
				std::lock_guard<std::mutex> Lock(_Buffer_Mutex);

				// Check if we've reached the end
				if (_Current_Sample_Position >= _Total_Audio_Samples) {
					// End of audio - fill with silence
					memset(Float_Buffer, 0, Frames_Available * _Num_Channels * sizeof(float));
					_Is_Playing.store(false);
				}
				else {
					// Calculate how many frames we can actually read
					int64_t Remaining_Samples = _Total_Audio_Samples - _Current_Sample_Position;
					UINT32 Frames_Can_Read = (UINT32)std::min((int64_t)Frames_Available, Remaining_Samples);

					// Copy audio data (interleaved)
					int64_t Source_Index = _Current_Sample_Position * _Num_Channels;
					memcpy(Float_Buffer, &_Audio_Buffer[Source_Index], Frames_Can_Read * _Num_Channels * sizeof(float));

					// Fill remainder with silence if needed
					if (Frames_Can_Read < Frames_Available) {
						memset(&Float_Buffer[Frames_Can_Read * _Num_Channels], 0,
							(Frames_Available - Frames_Can_Read) * _Num_Channels * sizeof(float));
					}

					// Update position
					_Current_Sample_Position += Frames_Can_Read;

					// Update position in milliseconds
					double Position_Ms = (_Current_Sample_Position * 1000.0) / _Sample_Rate;
					_Current_Position_Ms.store(Position_Ms);
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
		Hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator), (void**)&Device_Enumerator);
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
		_Sample_Rate = Wave_Format->nSamplesPerSec;
		_Num_Channels = Wave_Format->nChannels;

		// Initialize audio client in shared mode with event-driven callback
		REFERENCE_TIME Buffer_Duration = (REFERENCE_TIME)(buffer_size_samples * 10000000.0 / _Sample_Rate);

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

		_Buffer_Size_Samples = Actual_Buffer_Size;

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
}

#pragma managed(pop)