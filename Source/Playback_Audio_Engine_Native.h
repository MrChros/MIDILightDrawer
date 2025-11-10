#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace MIDILightDrawer
{
	class Playback_Audio_Engine_Native
	{
	private:
		static bool _Is_Initialized;
		static std::atomic<bool> _Is_Playing;
		static void* _Audio_Client;			// IAudioClient*
		static void* _Render_Client;		// IAudioRenderClient*
		static void* _Audio_Event;			// HANDLE for event-driven mode
		static float* _Audio_Buffer;		// Decoded PCM buffer (interleaved)
		static int _Buffer_Size_Samples;	// WASAPI buffer size
		static int _Sample_Rate;			// WASAPI output sample rate
		static int _File_Sample_Rate;		// Audio file's original sample rate
		static int _Num_Channels;
		static std::atomic<double> _Current_Position_Ms;
		static double _Audio_Duration_Ms;

		// Threading
		static std::thread* _Audio_Thread;
		static std::atomic<bool> _Should_Stop;
		static std::mutex _Buffer_Mutex;

		// Audio buffer management
		static int64_t _Total_Audio_Samples;       // Total samples in file (per channel)
		static int64_t _Current_Sample_Position;   // Current playback position (in samples)

		// Sync with MIDI master clock
		static std::atomic<int64_t> _MIDI_Position_Us;  // MIDI position for sync
		static bool _Sync_To_MIDI;                      // Enable MIDI sync

	public:
		static bool Initialize(const wchar_t* device_id, int buffer_size_samples);
		static void Cleanup();
		static std::vector<float> Resample_Audio_Linear(const std::vector<float>& input_buffer, int input_sample_rate, int output_sample_rate, int num_channels);
		static bool Load_Audio_File(const wchar_t* file_path);
		static void Unload_Audio_File();
		static bool Start_Playback();
		static bool Stop_Playback();
		static bool Pause_Playback();
		static bool Resume_Playback();
		static bool Seek_To_Position(double position_ms);
		static double Get_Current_Position_Ms();
		static double Get_Audio_Duration_Ms();
		static bool Is_Playing();
		static bool Is_Audio_Loaded();

		// Sync with MIDI master clock
		static void Set_MIDI_Position_Us(int64_t position_us);
		static void Enable_MIDI_Sync(bool enable);

	private:
		// Thread function
		static void Audio_Playback_Thread_Function();

		// Helper functions
		static bool Initialize_WASAPI(const wchar_t* device_id, int buffer_size_samples);
		static void Cleanup_WASAPI();
	};
}