#pragma once

#include <Windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "Mfuuid.lib")

namespace MIDILightDrawer
{
	/*
	// Forward declaration
	class Playback_Manager;
	
	ref class Playback_Audio_Engine
	{
	private:
		// Audio device (WASAPI)
		IMMDeviceEnumerator* _Device_Enumerator;
		IMMDevice* _Audio_Device;
		IAudioClient* _Audio_Client;
		IAudioRenderClient* _Render_Client;
		std::string _Device_Name;
		bool _Device_Open;

		// Audio file (Media Foundation)
		IMFSourceReader* _Source_Reader;
		std::wstring _Audio_File_Path;
		bool _Audio_Loaded;

		// Audio format
		WAVEFORMATEX* _wave_format;
		uint32_t _sample_rate;
		uint16_t _num_channels;
		uint16_t _bits_per_sample;
		uint64_t _duration_ms;

		// Playback state
		std::atomic<bool> _is_playing;
		std::atomic<bool> _should_stop;
		std::thread _playback_thread;
		std::mutex _state_mutex;

		// Position tracking
		std::atomic<uint64_t> _current_position_ms;
		uint64_t _playback_start_position_ms;
		double _playback_speed;

		// Buffer settings
		uint32_t _buffer_size_frames;
		REFERENCE_TIME _buffer_duration;

	public:
		Playback_Audio_Engine();
		~Playback_Audio_Engine();

		// Device management
		bool Initialize(const std::wstring& device_id, uint32_t buffer_size_frames);
		void Shutdown();
		bool Is_Device_Open() const { return _Device_Open; }

		// Audio file management
		bool Load_Audio_File(const std::wstring& file_path);
		void Clear_Audio_File();
		bool Is_Audio_Loaded() const { return _Audio_Loaded; }

		// Playback control
		void Start(uint64_t start_position_ms);
		void Stop();
		void Pause();
		void Resume();
		bool Is_Playing() const { return _is_playing; }

		// Position control
		void Seek(uint64_t position_ms);
		uint64_t Get_Current_Position_Ms() const { return _current_position_ms; }

		// Settings
		void Set_Playback_Speed(double speed);
		double Get_Playback_Speed() const { return _playback_speed; }

		// Audio info
		uint64_t Get_Duration_Ms() const { return _duration_ms; }
		uint32_t Get_Sample_Rate() const { return _sample_rate; }
		std::string Get_Device_Name() const { return _Device_Name; }

	private:
		// Playback thread function
		void Playback_Thread_Function();

		// Helper methods
		bool Initialize_WASAPI(const std::wstring& device_id);
		void Shutdown_WASAPI();
		bool Load_Audio_File_Internal(const std::wstring& file_path);
		void Unload_Audio_File();
		bool Read_Audio_Samples(BYTE* buffer, uint32_t num_frames, uint32_t& frames_read);
		void Seek_Audio_File(uint64_t position_ms);
	};
	*/
}

