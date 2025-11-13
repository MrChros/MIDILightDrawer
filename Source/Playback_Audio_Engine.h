#pragma once

#include "Playback_Audio_Engine_Native.h"

using namespace System;

namespace MIDILightDrawer
{
	
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
	
	ref class Playback_Audio_Engine
	{
	private:
		String^ _File_Path;
		Waveform_Render_Data^ _Waveform_Data;
		bool _Is_Initialized;

	public:
		Playback_Audio_Engine();
		~Playback_Audio_Engine();

		bool Initialize(String^ device_id, int buffer_size_samples);
		void Cleanup();
		bool Load_Audio_File(String^ file_path);
		void Unload_Audio_File();
		void CalculateWaveformData(int segments_per_second);

		bool Start_Playback();
		bool Stop_Playback();
		bool Pause_Playback();
		bool Resume_Playback();
		bool Seek_To_Position(double position_ms);
		double Get_Current_Position_ms();
		double Get_Audio_Duration_ms();
		bool Is_Playing();
		void Set_Offset(double offset_ms);
		void Set_Volume(double volume_percent);

		property bool Is_Audio_Loaded {
			bool get() { return Playback_Audio_Engine_Native::Is_Audio_Loaded(); }
		}

		property String^ File_Path {
			String^ get() { return _File_Path; }
		}

		property int Sample_Rate_Audio_Interface {
			int get() { return Playback_Audio_Engine_Native::Get_Sample_Rate_WASAPI(); }
		}

		property int Sample_Rate_File {
			int get() { return Playback_Audio_Engine_Native::Get_Sample_Rate_File(); }
		}

		property int Channel_Count {
			int get() { return Playback_Audio_Engine_Native::Get_Channel_Count(); }
		}

		property int Bit_Rate {
			int get() { return Playback_Audio_Engine_Native::Get_Bit_Rate(); }
		}

		property int64_t Sample_Count {
			int64_t get() { return Playback_Audio_Engine_Native::Get_Sample_Count(); }
		}

		property double Duration_ms {
			double get() { return Playback_Audio_Engine_Native::Get_Audio_Duration_ms(); }
		}

		property Waveform_Render_Data^ Waveform_Data {
			Waveform_Render_Data^ get() { return _Waveform_Data; }
		}
	};
}