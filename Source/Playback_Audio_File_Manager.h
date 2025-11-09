#pragma once

#include "Playback_Audio_File_Manager_Native.h"

using namespace System;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	/// <summary>
	/// Stores pre-calculated waveform visualization data for efficient rendering
	/// Managed wrapper around native waveform data
	/// </summary>
	public ref class Waveform_Render_Data
	{
	private:
		int _Total_Segments;
		int _Samples_Per_Segment;

	public:
		Waveform_Render_Data();
		~Waveform_Render_Data();

		/// <summary>
		/// Gets the min/max sample values for a specific segment
		/// </summary>
		void GetSegmentMinMax(int segment_index, float% min_value, float% max_value);

		property int TotalSegments {
			int get() { return _Total_Segments; }
		}

		property int SamplesPerSegment {
			int get() { return _Samples_Per_Segment; }
		}

		void Update_From_Native();
	};

	/// <summary>
	/// Manages audio file loading, storage, and waveform data
	/// Managed wrapper around native audio file manager
	/// </summary>
	public ref class Playback_Audio_File_Manager
	{
	private:
		String^ _File_Path;
		Waveform_Render_Data^ _Waveform_Data;
		bool _Is_Initialized;

	public:
		Playback_Audio_File_Manager();
		~Playback_Audio_File_Manager();

		/// <summary>
		/// Initializes the audio file manager and Media Foundation
		/// </summary>
		bool Initialize();

		/// <summary>
		/// Cleans up resources and shuts down Media Foundation
		/// </summary>
		void Cleanup();

		/// <summary>
		/// Loads a WAV or MP3 file and decodes it to PCM data
		/// </summary>
		/// <param name="file_path">Full path to the audio file</param>
		/// <param name="error_message">Output parameter for error description</param>
		/// <returns>True if successful, false otherwise</returns>
		bool LoadAudioFile(String^ file_path, String^% error_message);

		/// <summary>
		/// Clears the currently loaded audio file and frees memory
		/// </summary>
		void ClearAudioFile();

		/// <summary>
		/// Pre-calculates waveform rendering data for timeline visualization
		/// </summary>
		/// <param name="segments_per_second">Desired rendering resolution (default: 100)</param>
		void CalculateWaveformData(int segments_per_second);

		/// <summary>
		/// Gets PCM sample data at a specific time offset
		/// </summary>
		/// <param name="time_milliseconds">Time position in milliseconds</param>
		/// <param name="sample_count">Number of samples to retrieve</param>
		/// <returns>Array of PCM samples (interleaved if stereo), or nullptr if failed</returns>
		array<float>^ GetPCMSamplesAtTime(double time_milliseconds, int sample_count);

		/// <summary>
		/// Checks if an audio file is currently loaded
		/// </summary>
		property bool HasAudioFile {
			bool get() { return Playback_Audio_File_Manager_Native::Has_Audio_File(); }
		}

		property String^ FilePath {
			String^ get() { return _File_Path; }
		}

		property int SampleRate {
			int get() { return Playback_Audio_File_Manager_Native::Get_Sample_Rate(); }
		}

		property int ChannelCount {
			int get() { return Playback_Audio_File_Manager_Native::Get_Channel_Count(); }
		}

		property int64_t TotalSamples {
			int64_t get() { return Playback_Audio_File_Manager_Native::Get_Total_Samples(); }
		}

		property int BitDepth {
			int get() { return Playback_Audio_File_Manager_Native::Get_Bit_Depth(); }
		}

		property double DurationMilliseconds {
			double get() { return Playback_Audio_File_Manager_Native::Get_Duration_Milliseconds(); }
		}

		property Waveform_Render_Data^ WaveformData {
			Waveform_Render_Data^ get() { return _Waveform_Data; }
		}

		/// <summary>
		/// Gets direct access to the native PCM data pointer (for playback engine)
		/// Use with caution - this is unmanaged memory
		/// </summary>
		property const float* NativePCMDataPointer {
			const float* get() { return Playback_Audio_File_Manager_Native::Get_PCM_Data_Pointer(); }
		}
	};
}