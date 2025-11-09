#pragma once

#include "Playback_MIDI_Engine.h"
#include "Playback_Audio_Engine.h"

using namespace System;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref class Widget_Timeline;
	ref class MIDI_Event_Raster;
	ref class Playback_Event_Queue_Manager;
	ref class Playback_Audio_File_Manager;
	
	public enum class Playback_State
	{
		Stopped,
		Playing,
		Paused
	};

	public ref class Playback_Manager
	{
	private:
		Widget_Timeline^ _Timeline;
		MIDI_Event_Raster^ _MIDI_Event_Raster;

		Playback_MIDI_Engine^ _MIDI_Engine;
		Playback_Event_Queue_Manager^ _Event_Queue_Manager;
		Playback_Audio_Engine^ _Audio_Engine;
		Playback_Audio_File_Manager^ _Audio_File_Manager;
		
		Playback_State _Current_State;
		double _Playback_Position_ms;
		double _Playback_Speed;

		System::Object^ _State_Lock;

	public:
		Playback_Manager(Widget_Timeline^ timeline, MIDI_Event_Raster^ midi_event_raster);
		~Playback_Manager();

		// Initialization
		bool Initialize_MIDI(int device_id);
		bool Initialize_Audio(String^ device_id, int buffer_size);
		void Cleanup();

		// Audio file management
		void Set_Audio_File_Manager(Playback_Audio_File_Manager^ audio_manager);
		void Unload_Audio_File();
		bool Is_Audio_Loaded();

		// Playback controls
		bool Play();
		bool Pause();
		bool Stop();
		bool Seek_To_Position(double position_ms);

		void On_Track_Mute_Changed(int track_index, bool is_muted);
		void On_Track_Solo_Changed(int track_index, bool is_soloed);

		// State queries
		Playback_State Get_State();
		double Get_Playback_Position_ms();
		void Set_Playback_Position_ms(double position_ms);
		double Get_Audio_Duration_Ms();
		bool Is_Playing();

		// Playback speed
		void Set_Playback_Speed(double speed);
		double Get_Playback_Speed();

		// MIDI control
		bool Send_MIDI_Event(Playback_MIDI_Event^ event);
		bool Send_All_Notes_Off();

	private:
		double CalculateAudioTimeFromBarPosition(int bar_position);

	public:
		property bool HasAudio {
			bool get();
		}
	};
}

