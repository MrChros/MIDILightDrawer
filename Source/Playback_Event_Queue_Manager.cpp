#include "Playback_Event_Queue_Manager.h"
#include "Widget_Timeline.h"
#include "MIDI_Writer.h"

namespace MIDILightDrawer
{
	Playback_Event_Queue_Manager::Playback_Event_Queue_Manager(Playback_MIDI_Engine^ midi_engine, MIDI_Event_Raster^ midi_event_raster, Form_MIDI_Log^ form_midi_log)
	{
		_MIDI_Engine = midi_engine;
		_MIDI_Event_Raster = midi_event_raster;
		_Form_MIDI_Log = form_midi_log;
		_Unfiltered_Events = gcnew List<Playback_MIDI_Event^>();
		_Filtered_Events = gcnew List<Playback_MIDI_Event^>();
		_Current_Muted_Tracks = gcnew List<int>();
		_Current_Soloed_Tracks = gcnew List<int>();
		_Active_Notes = gcnew List<Active_Note>();
		_Active_Notes_Lock = gcnew Object();
		_Timeline_Tracks = nullptr;
		_Timeline_Measures = nullptr;
		_Global_MIDI_Channel = 0;
		_Cache_Valid = false;
	}

	Playback_Event_Queue_Manager::~Playback_Event_Queue_Manager()
	{
		// Send Note Off for any remaining active notes
		Send_All_Active_Notes_Off();
		Clear_Cache();
	}

	bool Playback_Event_Queue_Manager::Raster_And_Cache_Events(List<Track^>^ tracks, List<Measure^>^ measures, List<int>^ muted_tracks, List<int>^ soloed_tracks, uint8_t global_midi_channel)
	{
		if (!tracks || !measures)
		{
			return false;
		}

		try
		{
			// Store timeline references for real-time filtering
			_Timeline_Tracks = tracks;
			_Timeline_Measures = measures;
			_Global_MIDI_Channel = global_midi_channel;

			// Raster ALL tracks WITHOUT filtering (pass empty lists)
			// This allows us to filter dynamically during playback
			List<int>^ Empty_Muted = gcnew List<int>();
			List<int>^ Empty_Soloed = gcnew List<int>();

			List<Playback_MIDI_Event^>^ Rastered_Events = _MIDI_Event_Raster->Raster_Timeline_For_Playback();
			//List<Playback_MIDI_Event^>^ Rastered_Events = _MIDI_Event_Raster->Get_Timeline_PreRastered_Playback_Events(
			//	tracks,
			//	Empty_Muted,    // Don't filter during rastering
			//	Empty_Soloed,   // We'll filter later
			//	global_midi_channel
			//);

			if (!Rastered_Events) {
				return false;
			}

			// Store the unfiltered events
			_Unfiltered_Events->Clear();
			_Unfiltered_Events->AddRange(Rastered_Events);

			// Update current track state
			_Current_Muted_Tracks->Clear();
			_Current_Soloed_Tracks->Clear();

			if (muted_tracks) {
				_Current_Muted_Tracks->AddRange(muted_tracks);
			}

			if (soloed_tracks) {
				_Current_Soloed_Tracks->AddRange(soloed_tracks);
			}

			// Apply initial filtering
			Apply_Track_Filtering(_Current_Muted_Tracks, _Current_Soloed_Tracks);

			// Clear any old active notes
			System::Threading::Monitor::Enter(_Active_Notes_Lock);
			try
			{
				_Active_Notes->Clear();
			}
			finally
			{
				System::Threading::Monitor::Exit(_Active_Notes_Lock);
			}

			// Mark cache as valid
			_Cache_Valid = true;

			return true;
		}
		catch (...)
		{
			// Log error if needed
			_Cache_Valid = false;
			return false;
		}
	}

	bool Playback_Event_Queue_Manager::Queue_All_Cached_Events(double start_position_ms)
	{
		if (!_Cache_Valid || !_MIDI_Engine) {
			return false;
		}

		if (_Filtered_Events->Count == 0) {
			// No events to queue (empty timeline or all tracks muted)
			return true;
		}

		try
		{
			// Clear any existing events in the MIDI engine
			_MIDI_Engine->Clear_Event_Queue();

			// Only queue events that occur at or after the start position
			for each (Playback_MIDI_Event^ Event in _Filtered_Events)
			{
				if (Event->Timestamp_ms >= start_position_ms)
				{
					_MIDI_Engine->Queue_Event(Event);
				}
			}
			return true;
		}
		catch (...)
		{
			// Log error if needed
			return false;
		}
	}

	bool Playback_Event_Queue_Manager::Update_Track_State_During_Playback(double current_position_ms, List<int>^ new_muted_tracks, List<int>^ new_soloed_tracks)
	{
		if (!_Cache_Valid)
		{
			return false;
		}

		try
		{
			// Get list of tracks that changed state
			List<int>^ Changed_Tracks = Get_Changed_Tracks(_Current_Muted_Tracks, _Current_Soloed_Tracks, new_muted_tracks, new_soloed_tracks);

			// Send Note Off for active notes on tracks that are being muted/de-soloed
			if (Changed_Tracks->Count > 0)
			{
				Send_Active_Notes_Off_For_Tracks(Changed_Tracks);
			}

			// Update current state
			_Current_Muted_Tracks->Clear();
			_Current_Soloed_Tracks->Clear();

			if (new_muted_tracks)
			{
				_Current_Muted_Tracks->AddRange(new_muted_tracks);
			}

			if (new_soloed_tracks)
			{
				_Current_Soloed_Tracks->AddRange(new_soloed_tracks);
			}

			// Re-filter events based on new state
			Apply_Track_Filtering(_Current_Muted_Tracks, _Current_Soloed_Tracks);

			// Clear MIDI engine queue
			_MIDI_Engine->Clear_Event_Queue();

			// Re-queue only future events (after current position)
			List<Playback_MIDI_Event^>^ Future_Events = gcnew List<Playback_MIDI_Event^>();

			for each (Playback_MIDI_Event ^ Event in _Filtered_Events)
			{
				// Only queue events that haven't played yet
				if (Event->Timestamp_ms >= current_position_ms)
				{
					Future_Events->Add(Event);
				}
			}

			// Queue future events
			if (Future_Events->Count > 0)
			{
				_MIDI_Engine->Queue_Events(Future_Events);
			}

			return true;
		}
		catch (...)
		{
			// Log error if needed
			return false;
		}
	}

	void Playback_Event_Queue_Manager::On_Event_Sent(Playback_MIDI_Event^ event)
	{
		if (!event)
		{
			return;
		}

		if (_Form_MIDI_Log != nullptr)
		{
			_Form_MIDI_Log->Add_MIDI_Event(
				event->Timestamp_ms,
				event->Timeline_Track_ID,
				event->MIDI_Channel,
				event->MIDI_Command,
				event->MIDI_Data1,
				event->MIDI_Data2
			);
		}

		unsigned char Command_Type = event->MIDI_Command & 0xF0;

		if (Command_Type == MIDI_Writer::MIDI_EVENT_NOTE_ON && event->MIDI_Data2 > 0)
		{
			// Note On event (velocity > 0)
			Track_Note_On(
				event->Timeline_Track_ID,
				event->MIDI_Channel,
				event->MIDI_Data1,  // Note number
				event->Timestamp_ms
			);
		}
		else if (Command_Type == MIDI_Writer::MIDI_EVENT_NOTE_OFF || (Command_Type == MIDI_Writer::MIDI_EVENT_NOTE_ON && event->MIDI_Data2 == 0))
		{
			// Note Off event (0x80) or Note On with velocity 0
			Track_Note_Off(
				event->Timeline_Track_ID,
				event->MIDI_Channel,
				event->MIDI_Data1  // Note number
			);
		}
	}

	void Playback_Event_Queue_Manager::Send_All_Active_Notes_Off()
	{
		System::Threading::Monitor::Enter(_Active_Notes_Lock);
		try
		{
			// Create a copy to iterate over (in case list is modified)
			List<Active_Note>^ Notes_To_Stop = gcnew List<Active_Note>(_Active_Notes);

			for each (Active_Note Note in Notes_To_Stop)
			{
				Send_Note_Off_Immediate(Note.MIDI_Channel, Note.Note_Number);
			}

			// Clear the active notes list
			_Active_Notes->Clear();
		}
		finally
		{
			System::Threading::Monitor::Exit(_Active_Notes_Lock);
		}
	}

	void Playback_Event_Queue_Manager::Send_Active_Notes_Off_For_Tracks(List<int>^ track_indices)
	{
		if (!track_indices || track_indices->Count == 0)
		{
			return;
		}

		System::Threading::Monitor::Enter(_Active_Notes_Lock);
		try
		{
			// Find all active notes on these tracks
			List<Active_Note>^ Notes_To_Stop = gcnew List<Active_Note>();

			for each (Active_Note Note in _Active_Notes)
			{
				if (track_indices->Contains(Note.Track_Index))
				{
					Notes_To_Stop->Add(Note);
				}
			}

			// Send Note Off for each one
			for each (Active_Note Note in Notes_To_Stop)
			{
				Send_Note_Off_Immediate(Note.MIDI_Channel, Note.Note_Number);

				// Remove from active notes list
				_Active_Notes->Remove(Note);
			}
		}
		finally
		{
			System::Threading::Monitor::Exit(_Active_Notes_Lock);
		}
	}

	int Playback_Event_Queue_Manager::Get_Active_Note_Count()
	{
		System::Threading::Monitor::Enter(_Active_Notes_Lock);
		try
		{
			return _Active_Notes->Count;
		}
		finally
		{
			System::Threading::Monitor::Exit(_Active_Notes_Lock);
		}
	}

	void Playback_Event_Queue_Manager::Invalidate_Cache()
	{
		_Cache_Valid = false;
	}

	bool Playback_Event_Queue_Manager::Should_Invalidate_Cache_For_Track_State(List<int>^ current_muted_tracks, List<int>^ current_soloed_tracks)
	{
		// If cache is already invalid, no need to check
		if (!_Cache_Valid)
		{
			return true;
		}

		// Check if mute/solo state has changed
		bool Mute_Changed = !Track_Lists_Equal(_Current_Muted_Tracks, current_muted_tracks);
		bool Solo_Changed = !Track_Lists_Equal(_Current_Soloed_Tracks, current_soloed_tracks);

		return Mute_Changed || Solo_Changed;
	}

	int Playback_Event_Queue_Manager::Get_Cached_Event_Count()
	{
		if (!_Cache_Valid)
		{
			return 0;
		}

		return _Unfiltered_Events->Count;
	}

	int Playback_Event_Queue_Manager::Get_Active_Event_Count()
	{
		if (!_Cache_Valid)
		{
			return 0;
		}

		return _Filtered_Events->Count;
	}

	bool Playback_Event_Queue_Manager::Is_Cache_Valid()
	{
		return _Cache_Valid;
	}

	void Playback_Event_Queue_Manager::Clear_Cache()
	{
		// Send Note Off for any active notes before clearing
		Send_All_Active_Notes_Off();

		_Unfiltered_Events->Clear();
		_Filtered_Events->Clear();
		_Current_Muted_Tracks->Clear();
		_Current_Soloed_Tracks->Clear();
		_Timeline_Tracks = nullptr;
		_Timeline_Measures = nullptr;
		_Cache_Valid = false;
	}

	void Playback_Event_Queue_Manager::Apply_Track_Filtering(List<int>^ muted_tracks, List<int>^ soloed_tracks)
	{
		_Filtered_Events->Clear();

		for each (Playback_MIDI_Event ^ Event in _Unfiltered_Events)
		{
			// Check if this track should play
			if (Should_Track_Play(Event->Timeline_Track_ID, muted_tracks, soloed_tracks))
			{
				_Filtered_Events->Add(Event);
			}
		}
	}

	bool Playback_Event_Queue_Manager::Should_Track_Play(int track_index, List<int>^ muted_tracks, List<int>^ soloed_tracks)
	{
		// Check if track is muted
		bool Is_Muted = muted_tracks && muted_tracks->Contains(track_index);

		if (Is_Muted)
		{
			return false;
		}

		// Check solo logic
		bool Has_Any_Solo = soloed_tracks && soloed_tracks->Count > 0;

		if (Has_Any_Solo)
		{
			// If any track is soloed, only soloed tracks play
			bool Is_Soloed = soloed_tracks->Contains(track_index);
			return Is_Soloed;
		}

		// No solos active, track is not muted, so it plays
		return true;
	}

	List<int>^ Playback_Event_Queue_Manager::Get_Changed_Tracks(List<int>^ old_muted, List<int>^ old_soloed, List<int>^ new_muted, List<int>^ new_soloed)
	{
		List<int>^ Changed = gcnew List<int>();

		// Get all unique track indices
		List<int>^ All_Tracks = gcnew List<int>();

		if (old_muted)
		{
			for each (int Track in old_muted)
			{
				All_Tracks->Add(Track);
			}
		}

		if (old_soloed)
		{
			for each (int Track in old_soloed)
			{
				All_Tracks->Add(Track);
			}
		}

		if (new_muted)
		{
			for each (int Track in new_muted)
			{
				All_Tracks->Add(Track);
			}
		}

		if (new_soloed)
		{
			for each (int Track in new_soloed)
			{
				All_Tracks->Add(Track);
			}
		}

		// Check each track to see if its play state changed
		for each (int Track_Index in All_Tracks)
		{
			bool Old_Should_Play = Should_Track_Play(Track_Index, old_muted, old_soloed);
			bool New_Should_Play = Should_Track_Play(Track_Index, new_muted, new_soloed);

			if (Old_Should_Play != New_Should_Play)
			{
				Changed->Add(Track_Index);
			}
		}

		return Changed;
	}

	void Playback_Event_Queue_Manager::Track_Note_On(
		int track_index,
		uint8_t midi_channel,
		uint8_t note_number,
		double timestamp_ms
	)
	{
		System::Threading::Monitor::Enter(_Active_Notes_Lock);
		try
		{
			Active_Note New_Note;
			New_Note.Track_Index = track_index;
			New_Note.MIDI_Channel = midi_channel;
			New_Note.Note_Number = note_number;
			New_Note.Note_On_Time = timestamp_ms;

			// Add to active notes list
			_Active_Notes->Add(New_Note);
		}
		finally
		{
			System::Threading::Monitor::Exit(_Active_Notes_Lock);
		}
	}

	void Playback_Event_Queue_Manager::Track_Note_Off(
		int track_index,
		uint8_t midi_channel,
		uint8_t note_number
	)
	{
		System::Threading::Monitor::Enter(_Active_Notes_Lock);
		try
		{
			// Find and remove the matching active note
			for (int i = 0; i < _Active_Notes->Count; i++)
			{
				Active_Note Note = _Active_Notes[i];

				if (Note.Track_Index == track_index &&
					Note.MIDI_Channel == midi_channel &&
					Note.Note_Number == note_number)
				{
					_Active_Notes->RemoveAt(i);
					break;  // Remove only the first match
				}
			}
		}
		finally
		{
			System::Threading::Monitor::Exit(_Active_Notes_Lock);
		}
	}

	void Playback_Event_Queue_Manager::Send_Note_Off_Immediate(
		uint8_t midi_channel,
		uint8_t note_number
	)
	{
		if (!_MIDI_Engine)
		{
			return;
		}

		// Create a Note Off event
		Playback_MIDI_Event^ Note_Off = gcnew Playback_MIDI_Event();
		Note_Off->Timestamp_ms = 0;  // Send immediately
		Note_Off->Timeline_Track_ID = -1;  // Not from timeline
		Note_Off->MIDI_Channel = midi_channel;
		Note_Off->MIDI_Command = 0x80;  // Note Off command
		Note_Off->MIDI_Data1 = note_number;
		Note_Off->MIDI_Data2 = 0;  // Velocity (not used for Note Off)

		// Send immediately (bypass queue)
		_MIDI_Engine->Send_Event(Note_Off);
	}

	bool Playback_Event_Queue_Manager::Track_Lists_Equal(List<int>^ list_a, List<int>^ list_b)
	{
		// Null check
		if (!list_a && !list_b)
		{
			return true;
		}

		if (!list_a || !list_b)
		{
			return false;
		}

		// Count check
		if (list_a->Count != list_b->Count)
		{
			return false;
		}

		// Create sorted copies for comparison
		List<int>^ Sorted_A = gcnew List<int>(list_a);
		List<int>^ Sorted_B = gcnew List<int>(list_b);

		Sorted_A->Sort();
		Sorted_B->Sort();

		// Element-by-element comparison
		for (int Index = 0; Index < Sorted_A->Count; Index++)
		{
			if (Sorted_A[Index] != Sorted_B[Index])
			{
				return false;
			}
		}

		return true;
	}
}