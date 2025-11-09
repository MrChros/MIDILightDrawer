#pragma once

#include <vector>

namespace MIDILightDrawer
{
	/// <summary>
	/// Native C++ class for audio file management
	/// Handles Media Foundation operations without managed code
	/// </summary>
	class Playback_Audio_File_Manager_Native
	{
	public:
		struct Waveform_Segment
		{
			float Min_Value;
			float Max_Value;
		};

	private:
		// Audio file data
		static float* _PCM_Data;                  // Decoded PCM audio data (interleaved if stereo)
		static int _Sample_Rate;                  // Sample rate (e.g., 44100, 48000)
		static int _Channel_Count;                // Number of channels (1=mono, 2=stereo)
		static int64_t _Total_Samples;            // Total samples per channel
		static int _Bit_Depth;                    // Bits per sample (16, 24, 32)
		static double _Duration_Milliseconds;     // Total duration in milliseconds

		// Waveform visualization
		static std::vector<Waveform_Segment> _Waveform_Segments;
		static int _Samples_Per_Segment;
		static int _Segments_Per_Second;

		// Media Foundation COM initialization
		static bool _MF_Initialized;

		// Private helper methods
		static bool Initialize_Media_Foundation();
		static void Shutdown_Media_Foundation();
		static bool Load_Audio_File_Internal(const wchar_t* file_path);
		static bool Decode_PCM_Data(void* source_reader);  // IMFSourceReader*
		static void Calculate_Waveform_Data_Internal(int segments_per_second);

	public:
		static bool Initialize();
		static void Cleanup();

		/// <summary>
		/// Loads a WAV or MP3 file and decodes it to PCM data
		/// </summary>
		static bool Load_Audio_File(const wchar_t* file_path);

		/// <summary>
		/// Clears the currently loaded audio file and frees memory
		/// </summary>
		static void Clear_Audio_File();

		/// <summary>
		/// Pre-calculates waveform rendering data for timeline visualization
		/// </summary>
		static void Calculate_Waveform_Data(int segments_per_second);

		/// <summary>
		/// Gets PCM sample data at a specific time offset
		/// </summary>
		static bool Get_PCM_Samples_At_Time(double time_milliseconds, int sample_count, float* output_buffer, int* samples_written);

		/// <summary>
		/// Gets waveform segment data for visualization
		/// </summary>
		static bool Get_Waveform_Segment(int segment_index, float& min_value, float& max_value);

		// Property accessors
		static bool Has_Audio_File();
		static int Get_Sample_Rate();
		static int Get_Channel_Count();
		static int64_t Get_Total_Samples();
		static int Get_Bit_Depth();
		static double Get_Duration_Milliseconds();
		static int Get_Total_Waveform_Segments();
		static int Get_Samples_Per_Segment();
		static const float* Get_PCM_Data_Pointer();
	};
}