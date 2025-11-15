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
	ref class Widget_Audio_Container;
	ref class Form_MIDI_Log;
	
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
		Widget_Audio_Container^ _Audio_Container;
		
		Playback_State _Current_State;
		double _Playback_Position_ms;
		double _Playback_Speed;
		double _Audio_Offset_ms;
		double _Audio_Volume;

		System::Object^ _State_Lock;

	public:
		Playback_Manager(Widget_Timeline^ timeline, MIDI_Event_Raster^ midi_event_raster, Widget_Audio_Container^ audio_container, Form_MIDI_Log^ form_midi_log);
		~Playback_Manager();

		// Initialization
		bool Initialize_MIDI(int device_id);
		bool Initialize_Audio(String^ device_id, int buffer_size);
		void Stop_And_Cleanup();

		// Audio file management
		bool Load_Audio_File(String^ file_path, String^% error_message);
		void Unload_Audio_File();
		void Calculate_Waveform_Data(int segments_per_second);

		// Playback controls
		bool Play();
		bool Pause();
		bool Stop();
		bool Seek_To_Position(double position_ms);

		void On_Track_Mute_Changed(int track_index, bool is_muted);
		void On_Track_Solo_Changed(int track_index, bool is_soloed);

		// State queries
		Playback_State Get_State();
		bool Is_Playing();
		double Get_Playback_Position_ms();
		double Get_Audio_Duration_ms();
		double Get_MIDI_Duration_ms();

		// Playback speed
		void Set_Playback_Speed(double speed);
		double Get_Playback_Speed();

		// Audio Offest and Volume
		void Set_Audio_Offset(double offset_ms);
		double Get_Audio_Offset();
		void Set_Volume(double volume_percent);
		double Get_Volume();

		// MIDI control
		bool Send_MIDI_Event(Playback_MIDI_Event^ event);

	public:
		property bool Is_Audio_Loaded {
			bool get() { return _Audio_Engine->Is_Audio_Loaded; }
		}

		property String^ Audio_File_Path {
			String^ get() { return _Audio_Engine->File_Path; }
		}

		property int Audio_Sample_Rate_Audio_Interface {
			int get() { return _Audio_Engine->Sample_Rate_Audio_Interface; }
		}

		property int Audio_Sample_Rate_File {
			int get() { return _Audio_Engine->Sample_Rate_File; }
		}

		property int Audio_Channel_Count {
			int get() { return _Audio_Engine->Channel_Count; }
		}

		property int Audio_Bit_Rate {
			int get() { return _Audio_Engine->Bit_Rate; }
		}

		property int64_t Audio_Sample_Count {
			int64_t get() { return _Audio_Engine->Sample_Count; }
		}

		property double Audio_Duration_ms {
			double get() { return _Audio_Engine->Duration_ms; }
		}

		property Waveform_Render_Data^ Audio_Waveform_Data {
			Waveform_Render_Data^ get() { return _Audio_Engine->Waveform_Data; }
		}
	};
}

