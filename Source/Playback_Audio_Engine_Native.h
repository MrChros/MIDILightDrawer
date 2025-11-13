#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace MIDILightDrawer
{
	class Playback_Audio_Engine_Native
	{
	public:
		struct Waveform_Segment
		{
			float Min_Value;
			float Max_Value;
		};

	private:
		static bool _Is_Initialized;
		static std::atomic<bool> _Is_Playing;
		static void* _Audio_Client;				// IAudioClient*
		static void* _Render_Client;			// IAudioRenderClient*
		static void* _Audio_Event;				// HANDLE for event-driven mode
		static float* _Audio_Buffer;			// Decoded PCM buffer (interleaved)
		static int _Audio_Buffer_Size;			// WASAPI buffer size
		static int _Audio_Sample_Rate_WASAPI;	// WASAPI output sample rate
		static int _Audio_Sample_Rate_File;		// Audio file's original sample rate
		static int _Audio_Num_Channels;
		static int _Audio_Bit_Rate;
		static double _Audio_Duration_ms;
		static std::atomic<double> _Current_Position_ms;
		static double _Audio_Offset_ms;
		static std::atomic<double> _Audio_Volume;

		// Waveform visualization
		static std::vector<Waveform_Segment> _Waveform_Segments;
		static int _Samples_Per_Segment;
		static int _Segments_Per_Second;

		// Threading
		static std::thread* _Audio_Thread;
		static std::atomic<bool> _Should_Stop;
		static std::mutex _Buffer_Mutex;

		// Audio buffer management
		static int64_t _Total_Audio_Samples;       // Total samples in file (per channel)
		static int64_t _Current_Sample_Position;   // Current playback position (in samples)

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
		static double Get_Current_Position_ms();
		static double Get_Audio_Duration_ms();
		static bool Is_Playing();
		static bool Is_Audio_Loaded();
		static void Set_Offset(double offset_ms);
		static void Set_Volume(double volume_percent);
		static void Calculate_Waveform_Data(int segments_per_second);
		static bool Get_Waveform_Segment(int segment_index, float& min_value, float& max_value);
		static int Get_Total_Waveform_Segments();
		static int Get_Samples_Per_Segment();
		static int Get_Sample_Rate_File();
		static int Get_Sample_Rate_WASAPI();
		static int Get_Channel_Count();
		static int Get_Bit_Rate();
		static int64_t Get_Sample_Count();

	private:
		// Thread function
		static void Audio_Playback_Thread_Function();

		// Helper functions
		static bool Initialize_WASAPI(const wchar_t* device_id, int buffer_size_samples);
		static void Cleanup_WASAPI();
		static void Calculate_Waveform_Data_Internal(int segments_per_second);
	};
}