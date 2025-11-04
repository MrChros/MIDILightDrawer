#pragma once

namespace MIDILightDrawer
{
	class Playback_Audio_Engine_Native
	{
	private:
		static bool _Is_Initialized;
		static bool _Is_Playing;
		static void* _Audio_Client;
		static void* _Render_Client;
		static float* _Audio_Buffer;
		static int _Buffer_Size_Samples;
		static int _Sample_Rate;
		static int _Num_Channels;
		static double _Current_Position_Ms;
		static double _Audio_Duration_Ms;

	public:
		static bool Initialize(const wchar_t* device_id, int buffer_size_samples);
		static void Cleanup();
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
	};
}

