
#include "Playback_Manager.h"

#include "Settings.h"
#include "MIDI_Event_Raster.h"
#include "Playback_Event_Queue_Manager.h"
#include "Widget_Audio_Container.h"
#include "Form_MIDI_Log.h"

namespace MIDILightDrawer
{
	Playback_Manager::Playback_Manager(Widget_Timeline^ timeline, MIDI_Event_Raster^ midi_event_raster, Widget_Audio_Container^ audio_container, Form_MIDI_Log^ form_midi_log)
	{
		_Timeline = timeline;
		_MIDI_Event_Raster = midi_event_raster;
		_Audio_Container = audio_container;
		
		_MIDI_Engine = gcnew Playback_MIDI_Engine();
		_Audio_Engine = gcnew Playback_Audio_Engine();

		_Event_Queue_Manager = gcnew Playback_Event_Queue_Manager(_MIDI_Engine, _MIDI_Event_Raster, form_midi_log);
		_MIDI_Engine->Set_Event_Queue_Manager(_Event_Queue_Manager);

		_Current_State = Playback_State::Stopped;
		_Playback_Position_ms = 0.0;
		_Playback_Speed = 1.0;
		_Audio_Offset_ms = 0.0;
		_Audio_Volume = 1.0; // 100%

		_State_Lock = gcnew Object();
		_Event_Queue_Manager->Invalidate_Cache();
	}

	Playback_Manager::~Playback_Manager()
	{
		Stop_And_Cleanup();
	}

	bool Playback_Manager::Initialize_MIDI(int device_id)
	{
		return _MIDI_Engine->Initialize(device_id);
	}

	bool Playback_Manager::Initialize_Audio(String^ device_id, int buffer_size)
	{
		bool Success = _Audio_Engine->Initialize(device_id, buffer_size);

		return Success;
	}

	void Playback_Manager::Stop_And_Cleanup()
	{
		Stop();

		_MIDI_Engine->Cleanup();
		_Audio_Engine->Cleanup();
	}

	bool Playback_Manager::Load_Audio_File(String^ file_path, String^% error_message)
	{
		error_message = String::Empty;

		if (file_path == nullptr || file_path->Length == 0)
		{
			error_message = "Invalid file path";
			return false;
		}

		bool Success = _Audio_Engine->Load_Audio_File(file_path);

		if (Success) {
			this->Calculate_Waveform_Data(500);
			_Audio_Container->Load_Audio_Data();
		}
		else {
			error_message = "Failed to load audio file. File may be corrupted or format not supported.";
		}

		return Success;
	}

	void Playback_Manager::Unload_Audio_File()
	{
		// Stop playback if playing
		if (_Current_State == Playback_State::Playing) {
			Stop();
		}

		_Audio_Engine->Unload_Audio_File();
		_Audio_Container->Unload_Audio_Data();

		// NEW: Tell MIDI that audio is no longer available
		_MIDI_Engine->Set_Audio_Available(false);
	}

	void Playback_Manager::Calculate_Waveform_Data(int segments_per_second)
	{
		_Audio_Engine->CalculateWaveformData(segments_per_second);
	}

	bool Playback_Manager::Play()
	{
		System::Threading::Monitor::Enter(_State_Lock);

		try {
			if (_Current_State == Playback_State::Playing) {
				return true;
			}

			List<int>^ Muted_Tracks = _Timeline->TrackNumbersMuted;
			List<int>^ Soloed_Tracks = _Timeline->TrackNumbersSoloed;

			if (!_Event_Queue_Manager->Is_Cache_Valid())
			{
				bool Success = _Event_Queue_Manager->Raster_And_Cache_Events(_Timeline->Tracks, _Timeline->Measures, Muted_Tracks, Soloed_Tracks, Settings::Get_Instance()->Global_MIDI_Output_Channel);

				if (!Success) {
					return false;
				}
			}

			_MIDI_Engine->Clear_Event_Queue();
			_Event_Queue_Manager->Queue_All_Cached_Events(_Playback_Position_ms);

			bool Audio_Available = Is_Audio_Loaded && _Audio_Engine != nullptr;
			_MIDI_Engine->Set_Audio_Available(Audio_Available);

			bool Success = true;

			_Audio_Container->Update_Cursor();

			// Set MIDI engine position before starting
			_MIDI_Engine->Set_Current_Position_ms(_Playback_Position_ms);
			if (Audio_Available) {

			}

			// Start MIDI playback thread
			Success &= _MIDI_Engine->Start_Playback();

			if (Audio_Available) {
				_Audio_Engine->Set_Current_Position_ms(_Playback_Position_ms);

				if (_Current_State == Playback_State::Paused) {
					Success &= _Audio_Engine->Resume_Playback();
				}
				else {
					Success &= _Audio_Engine->Start_Playback();
				}
			}

			if (Success) {
				_Current_State = Playback_State::Playing;
			}

			return Success;
		}
		finally {
			System::Threading::Monitor::Exit(_State_Lock);
		}
	}

	bool Playback_Manager::Pause()
	{
		System::Threading::Monitor::Enter(_State_Lock);

		try {
			if (_Current_State != Playback_State::Playing) {
				return false;
			}

			bool Success = true;

			// Send Note Off for all active notes FIRST
			_Event_Queue_Manager->Send_All_Active_Notes_Off();
			_Event_Queue_Manager->Invalidate_Cache();

			// Stop MIDI playback thread
			Success &= _MIDI_Engine->Stop_Playback();

			// Pause audio if loaded
			if (Is_Audio_Loaded)
				Success &= _Audio_Engine->Pause_Playback();

			// Update position from MIDI engine
			_Playback_Position_ms = _MIDI_Engine->Get_Current_Position_ms();

			if (Success) {
				_Current_State = Playback_State::Paused;
			}

			return Success;
		}
		finally {
			System::Threading::Monitor::Exit(_State_Lock);
		}
	}

	bool Playback_Manager::Stop()
	{
		System::Threading::Monitor::Enter(_State_Lock);

		try {
			if (_Current_State == Playback_State::Stopped) {
				return true;
			}

			bool Success = true;

			// Send Note Off for all active notes FIRST
			_Event_Queue_Manager->Send_All_Active_Notes_Off();

			// Stop MIDI playback thread
			Success &= _MIDI_Engine->Stop_Playback();

			_MIDI_Engine->Clear_Event_Queue();
			_MIDI_Engine->Set_Audio_Available(false);

			// Stop audio if loaded
			if (Is_Audio_Loaded) {
				_Audio_Engine->Stop_Playback();
			}

			_Current_State = Playback_State::Stopped;

			return Success;
		}
		finally {
			System::Threading::Monitor::Exit(_State_Lock);
		}
	}

	bool Playback_Manager::Seek_To_Position(double position_ms)
	{
		if (position_ms < 0.0 || position_ms > Get_MIDI_Duration_ms()) {
			return false;
		}
		
		bool Was_Playing = (_Current_State == Playback_State::Playing);

		// Pause playback during seek
		if (Was_Playing) {
			Pause();
		}

		_Playback_Position_ms = position_ms;
		_Event_Queue_Manager->Invalidate_Cache();

		if (_MIDI_Engine) {
			_MIDI_Engine->Set_Current_Position_ms(position_ms);
		}
		if (_Audio_Engine && Is_Audio_Loaded) {
			_Audio_Engine->Set_Current_Position_ms(position_ms);
		}

		_Audio_Container->Update_Cursor();
		_Timeline->Playback_Auto_Scroll(true);

		// Resume playback if it was playing
		if (Was_Playing) {
			Play();
		}

		return true;
	}

	void Playback_Manager::On_Track_Mute_Changed(int track_index, bool is_muted)
	{
		if (_Timeline && track_index < _Timeline->Tracks->Count)
		{
			_Timeline->Tracks[track_index]->IsMuted = is_muted;
		}

		if (_Current_State == Playback_State::Playing)
		{
			double Current_Pos = Get_Playback_Position_ms();
			List<int>^ Muted = _Timeline->TrackNumbersMuted;
			List<int>^ Soloed = _Timeline->TrackNumbersSoloed;

			_Event_Queue_Manager->Update_Track_State_During_Playback(Current_Pos, Muted, Soloed);
		}
	}

	void Playback_Manager::On_Track_Solo_Changed(int track_index, bool is_soloed)
	{
		if (_Timeline && track_index < _Timeline->Tracks->Count)
		{
			_Timeline->Tracks[track_index]->IsSoloed = is_soloed;
		}

		if (_Current_State == Playback_State::Playing)
		{
			double Current_Pos = Get_Playback_Position_ms();
			List<int>^ Muted = _Timeline->TrackNumbersMuted;
			List<int>^ Soloed = _Timeline->TrackNumbersSoloed;

			_Event_Queue_Manager->Update_Track_State_During_Playback(Current_Pos, Muted, Soloed);
		}
	}

	Playback_State Playback_Manager::Get_State()
	{
		System::Threading::Monitor::Enter(_State_Lock);

		try {
			return _Current_State;
		}
		finally {
			System::Threading::Monitor::Exit(_State_Lock);
		}
	}

	bool Playback_Manager::Is_Playing()
	{
		return _Current_State == Playback_State::Playing;
	}

	double Playback_Manager::Get_Playback_Position_ms()
	{
		System::Threading::Monitor::Enter(_State_Lock);
		try {
			Playback_State Current = _Current_State;
			System::Threading::Monitor::Exit(_State_Lock);

			// Get position from MIDI engine if playing (it's the master clock)
			if (Current == Playback_State::Playing)
			{
				double MIDI_Pos = _MIDI_Engine->Get_Current_Position_ms();

				// Sync audio engine with MIDI position
				if (Is_Audio_Loaded) {
					double Audio_Pos_ms = _Audio_Engine->Get_Current_Position_ms();
					int64_t Audio_Pos_us = (int64_t)(Audio_Pos_ms * 1000.0);

					// Feed audio position to MIDI engine for sync
					_MIDI_Engine->Set_Audio_Position_us(Audio_Pos_us);
				}

				if (MIDI_Pos >= Get_MIDI_Duration_ms()) {
					Stop();
					MIDI_Pos = Get_MIDI_Duration_ms();
					_Playback_Position_ms = MIDI_Pos;
				}

				return MIDI_Pos;
			}

			// Otherwise use stored position
			return _Playback_Position_ms;
		}
		catch(...) {
			System::Threading::Monitor::Exit(_State_Lock);
			throw;
		}
	}

	double Playback_Manager::Get_Audio_Duration_ms()
	{
		return _Audio_Engine->Get_Audio_Duration_ms();
	}

	double Playback_Manager::Get_MIDI_Duration_ms()
	{
		if (_Timeline->Measures->Count == 0) {
			return 0.0;
		}

		Measure^ Last_Measure = _Timeline->Measures[_Timeline->Measures->Count - 1];

		return Last_Measure->StartTime_ms + Last_Measure->Length_ms - 1;
	}

	void Playback_Manager::Set_Playback_Speed(double speed)
	{
		if (speed > 0.0) {
			_Playback_Speed = speed;
		}
	}

	double Playback_Manager::Get_Playback_Speed()
	{
		return _Playback_Speed;
	}

	void Playback_Manager::Set_Audio_Offset(double offset_ms)
	{
		_Audio_Offset_ms = offset_ms;
		if (_Audio_Engine) {
			_Audio_Engine->Set_Offset(offset_ms);
		}
	}

	double Playback_Manager::Get_Audio_Offset()
	{
		return _Audio_Offset_ms;
	}

	void Playback_Manager::Set_Volume(double volume_percent)
	{
		_Audio_Volume = volume_percent;
		if (_Audio_Engine) {
			_Audio_Engine->Set_Volume(volume_percent);
		}
	}

	double Playback_Manager::Get_Volume()
	{
		return _Audio_Volume;
	}

	bool Playback_Manager::Send_MIDI_Event(Playback_MIDI_Event^ event)
	{
		return _MIDI_Engine->Send_Event(event);
	}
}