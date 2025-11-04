#pragma once

#include "Playback_MIDI_Engine.h"
#include "Playback_Audio_Engine.h"

using namespace System;

namespace MIDILightDrawer
{
	public enum class Playback_State
	{
		Stopped,
		Playing,
		Paused
	};

	ref class Playback_Manager
	{
	private:
		Playback_MIDI_Engine^ _MIDI_Engine;
		Playback_Audio_Engine^ _Audio_Engine;
		Playback_State _Current_State;
		double _Current_Position_Ms;
		double _Playback_Speed;

	public:
		Playback_Manager();
		~Playback_Manager();

		// Initialization
		bool Initialize_MIDI(int device_id);
		bool Initialize_Audio(String^ device_id, int buffer_size);
		void Cleanup();

		// Audio file management
		bool Load_Audio_File(String^ file_path);
		void Unload_Audio_File();
		bool Is_Audio_Loaded();

		// Playback controls
		bool Play();
		bool Pause();
		bool Stop();
		bool Seek_To_Position(double position_ms);

		// State queries
		Playback_State Get_State();
		double Get_Current_Position_Ms();
		double Get_Audio_Duration_Ms();
		bool Is_Playing();

		// Playback speed
		void Set_Playback_Speed(double speed);
		double Get_Playback_Speed();

		// MIDI control
		bool Send_MIDI_Event(MIDI_Event_Info^ event);
		bool Send_All_Notes_Off();
	};
}

