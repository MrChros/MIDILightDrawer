#pragma once

#include "Playback_Audio_Engine_Native.h"

using namespace System;

namespace MIDILightDrawer
{
	ref class Playback_Audio_Engine
	{
	private:
		bool _Is_Initialized;

	public:
		Playback_Audio_Engine();
		~Playback_Audio_Engine();

		bool Initialize(String^ device_id, int buffer_size_samples);
		void Cleanup();
		bool Load_Audio_File(String^ file_path);
		void Unload_Audio_File();
		bool Start_Playback();
		bool Stop_Playback();
		bool Pause_Playback();
		bool Resume_Playback();
		bool Seek_To_Position(double position_ms);
		double Get_Current_Position_Ms();
		double Get_Audio_Duration_Ms();
		bool Is_Playing();
		bool Is_Audio_Loaded();
	};
}

