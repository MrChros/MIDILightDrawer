#pragma once

#include "Playback_MIDI_Engine.h"
#include "MIDI_Event_Raster.h"
#include "Form_MIDI_Log.h"

using namespace System;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward declarations
	ref class Track;
	ref class Measure;

	private value struct Active_Note
	{
		int Track_Index;       // Which timeline track
		uint8_t MIDI_Channel;  // MIDI channel (0-15)
		uint8_t Note_Number;   // Note number (0-127)
		double Note_On_Time;   // When the Note On was sent (for debugging)

		bool Equals(Active_Note other)
		{
			return Track_Index == other.Track_Index && MIDI_Channel == other.MIDI_Channel && Note_Number == other.Note_Number;
		}
	};


	public ref class Playback_Event_Queue_Manager
	{
	private:
		List<Playback_MIDI_Event^>^ _Unfiltered_Events;
		List<Playback_MIDI_Event^>^ _Filtered_Events;

		bool _Cache_Valid;

		Playback_MIDI_Engine^ _MIDI_Engine;
		MIDI_Event_Raster^ _MIDI_Event_Raster;
		Form_MIDI_Log^ _Form_MIDI_Log;

		// Track filtering state
		List<int>^ _Current_Muted_Tracks;
		List<int>^ _Current_Soloed_Tracks;

		// Timeline references (needed for re-filtering during playback)
		List<Track^>^ _Timeline_Tracks;
		List<Measure^>^ _Timeline_Measures;
		uint8_t _Global_MIDI_Channel;

		// Active note tracking (notes that are currently "on" and need "off")
		List<Active_Note>^ _Active_Notes;
		Object^ _Active_Notes_Lock;

	public:
		Playback_Event_Queue_Manager(Playback_MIDI_Engine^ midi_engine, MIDI_Event_Raster^ midi_event_raster, Form_MIDI_Log^ form_midi_log);
		~Playback_Event_Queue_Manager();

		bool Raster_And_Cache_Events(List<Track^>^ tracks, List<Measure^>^ measures, List<int>^ muted_tracks, List<int>^ soloed_tracks, uint8_t global_midi_channel);
		bool Queue_All_Cached_Events(double start_position_ms);
		bool Update_Track_State_During_Playback(double current_position_ms, List<int>^ new_muted_tracks, List<int>^ new_soloed_tracks);

		void On_Event_Sent(Playback_MIDI_Event^ event);
		void Send_All_Active_Notes_Off();
		void Send_Active_Notes_Off_For_Tracks(List<int>^ track_indices);
		int Get_Active_Note_Count();

		void Invalidate_Cache();
		bool Should_Invalidate_Cache_For_Track_State(List<int>^ current_muted_tracks, List<int>^ current_soloed_tracks);
		int Get_Cached_Event_Count();
		int Get_Active_Event_Count();
		bool Is_Cache_Valid();
		void Clear_Cache();

	private:
		void Apply_Track_Filtering(List<int>^ muted_tracks, List<int>^ soloed_tracks);
		bool Should_Track_Play(int track_index, List<int>^ muted_tracks, List<int>^ soloed_tracks);
		List<int>^ Get_Changed_Tracks(List<int>^ old_muted, List<int>^ old_soloed, List<int>^ new_muted, List<int>^ new_soloed);
		void Track_Note_On(int track_index, uint8_t midi_channel, uint8_t note_number, double timestamp_ms);
		void Track_Note_Off(int track_index, uint8_t midi_channel, uint8_t note_number);
		void Send_Note_Off_Immediate(uint8_t midi_channel, uint8_t note_number);
		bool Track_Lists_Equal(List<int>^ list_a, List<int>^ list_b);
	};
}