#pragma once

#include "Playback_MIDI_Engine.h"
#include "MIDI_Event_Raster.h"

using namespace System;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward declarations
	ref class Track;
	ref class Measure;

	/// <summary>
	/// Represents an active (playing) note that needs a Note Off
	/// </summary>
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

	/// <summary>
	/// Manages MIDI event caching and queuing for playback with real-time mute/solo support.
	/// Tracks active notes to ensure proper Note Off events are sent.
	/// </summary>
	public ref class Playback_Event_Queue_Manager
	{
	private:
		// Full unfiltered event cache (all tracks, no filtering)
		List<Playback_MIDI_Event^>^ _Unfiltered_Events;

		// Currently active filtered events (what's actually queued)
		List<Playback_MIDI_Event^>^ _Filtered_Events;

		// Flag indicating if cache is valid
		bool _Cache_Valid;

		// Reference to the MIDI engine for queuing
		Playback_MIDI_Engine^ _MIDI_Engine;

		// Reference to the raster utility
		MIDI_Event_Raster^ _Raster;

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
		Playback_Event_Queue_Manager(Playback_MIDI_Engine^ midi_engine);
		~Playback_Event_Queue_Manager();

		/// <summary>
		/// Raster all tracks from the timeline and cache ALL events (unfiltered).
		/// This allows for real-time mute/solo changes during playback.
		/// </summary>
		/// <param name="tracks">Timeline tracks to raster</param>
		/// <param name="measures">Measure list for tempo map initialization</param>
		/// <param name="muted_tracks">Initial list of muted track indices</param>
		/// <param name="soloed_tracks">Initial list of soloed track indices</param>
		/// <param name="global_midi_channel">MIDI channel to use for all events</param>
		/// <returns>True if rastering and caching succeeded</returns>
		bool Raster_And_Cache_Events(List<Track^>^ tracks, List<Measure^>^ measures, List<int>^ muted_tracks, List<int>^ soloed_tracks, uint8_t global_midi_channel);

		/// <summary>
		/// Queue all filtered events to the MIDI engine.
		/// Must be called after Raster_And_Cache_Events().
		/// </summary>
		/// <returns>True if events were queued successfully</returns>
		bool Queue_All_Cached_Events();

		/// <summary>
		/// Handle real-time mute/solo changes during playback.
		/// Sends proper Note Off events for affected tracks and updates the event queue.
		/// </summary>
		/// <param name="current_position_ms">Current playback position in milliseconds</param>
		/// <param name="new_muted_tracks">New list of muted track indices</param>
		/// <param name="new_soloed_tracks">New list of soloed track indices</param>
		/// <returns>True if update succeeded</returns>
		bool Update_Track_State_During_Playback(double current_position_ms, List<int>^ new_muted_tracks, List<int>^ new_soloed_tracks);

		/// <summary>
		/// Called when an event is about to be sent by the MIDI engine.
		/// Tracks Note On events to ensure proper Note Off later.
		/// </summary>
		/// <param name="event">The MIDI event being sent</param>
		void On_Event_Sent(Playback_MIDI_Event^ event);

		/// <summary>
		/// Send Note Off for all currently active notes.
		/// Called on Stop or when tracks are muted/soloed.
		/// </summary>
		void Send_All_Active_Notes_Off();

		/// <summary>
		/// Send Note Off for all active notes on specific tracks.
		/// </summary>
		/// <param name="track_indices">List of track indices to silence</param>
		void Send_Active_Notes_Off_For_Tracks(List<int>^ track_indices);

		/// <summary>
		/// Get count of currently active (playing) notes.
		/// </summary>
		int Get_Active_Note_Count();

		/// <summary>
		/// Invalidate the cache, forcing a re-raster on next playback.
		/// Call this when timeline content changes.
		/// </summary>
		void Invalidate_Cache();

		/// <summary>
		/// Check if the cache needs to be invalidated due to mute/solo changes.
		/// </summary>
		/// <param name="current_muted_tracks">Current muted track list</param>
		/// <param name="current_soloed_tracks">Current soloed track list</param>
		/// <returns>True if cache should be invalidated</returns>
		bool Should_Invalidate_Cache_For_Track_State(List<int>^ current_muted_tracks, List<int>^ current_soloed_tracks);

		/// <summary>
		/// Get the number of unfiltered cached events.
		/// </summary>
		int Get_Cached_Event_Count();

		/// <summary>
		/// Get the number of currently active filtered events.
		/// </summary>
		int Get_Active_Event_Count();

		/// <summary>
		/// Check if events are currently cached.
		/// </summary>
		bool Is_Cache_Valid();

		/// <summary>
		/// Clear all cached events and invalidate the cache.
		/// Also clears all active note tracking.
		/// </summary>
		void Clear_Cache();

	private:
		/// <summary>
		/// Apply mute/solo filtering to the unfiltered event cache.
		/// </summary>
		void Apply_Track_Filtering(
			List<int>^ muted_tracks,
			List<int>^ soloed_tracks
		);

		/// <summary>
		/// Determine which tracks should be audible based on mute/solo state.
		/// </summary>
		bool Should_Track_Play(
			int track_index,
			List<int>^ muted_tracks,
			List<int>^ soloed_tracks
		);

		/// <summary>
		/// Get list of tracks that changed state (newly muted or unmuted).
		/// </summary>
		List<int>^ Get_Changed_Tracks(List<int>^ old_muted, List<int>^ old_soloed, List<int>^ new_muted, List<int>^ new_soloed);

		/// <summary>
		/// Track a Note On event in the active notes list.
		/// </summary>
		void Track_Note_On(int track_index, uint8_t midi_channel, uint8_t note_number, double timestamp_ms);

		/// <summary>
		/// Remove a Note Off event from the active notes list.
		/// </summary>
		void Track_Note_Off(int track_index, uint8_t midi_channel, uint8_t note_number);

		/// <summary>
		/// Send a specific Note Off event immediately.
		/// </summary>
		void Send_Note_Off_Immediate(uint8_t midi_channel, uint8_t note_number);

		/// <summary>
		/// Helper to check if two track lists are equal.
		/// </summary>
		bool Track_Lists_Equal(List<int>^ list_a, List<int>^ list_b);
	};
}