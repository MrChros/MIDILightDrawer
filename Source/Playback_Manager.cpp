#include "Playback_Manager.h"

namespace MIDILightDrawer
{
	Playback_Manager::Playback_Manager()
	{
		_MIDI_Engine = gcnew Playback_MIDI_Engine();
		_Audio_Engine = gcnew Playback_Audio_Engine();
		_Current_State = Playback_State::Stopped;
		_Current_Position_Ms = 0.0;
		_Playback_Speed = 1.0;
		_Playback_Cursor_Position_Ms = 0.0;
	}

	Playback_Manager::~Playback_Manager()
	{
		Cleanup();
	}

	bool Playback_Manager::Initialize_MIDI(int device_id)
	{
		return _MIDI_Engine->Initialize(device_id);
	}

	bool Playback_Manager::Initialize_Audio(String^ device_id, int buffer_size)
	{
		bool Success = _Audio_Engine->Initialize(device_id, buffer_size);

		if (Success) {
			// Enable MIDI synchronization for audio
			_Audio_Engine->Enable_MIDI_Sync(true);
		}

		return Success;
	}

	void Playback_Manager::Cleanup()
	{
		Stop();
		_MIDI_Engine->Cleanup();
		_Audio_Engine->Cleanup();
	}

	bool Playback_Manager::Load_Audio_File(String^ file_path)
	{
		return _Audio_Engine->Load_Audio_File(file_path);
	}

	void Playback_Manager::Unload_Audio_File()
	{
		_Audio_Engine->Unload_Audio_File();
	}

	bool Playback_Manager::Is_Audio_Loaded()
	{
		return _Audio_Engine->Is_Audio_Loaded();
	}

	bool Playback_Manager::Play()
	{
		if (_Current_State == Playback_State::Playing) {
			return true;
		}

		bool Success = true;

		if (_Current_State == Playback_State::Stopped) {
			_Current_Position_Ms = _Playback_Cursor_Position_Ms;
		}

		// Set MIDI engine position before starting
		_MIDI_Engine->Set_Current_Position_Ms(_Current_Position_Ms);

		// Set audio engine position (if loaded)
		if (Is_Audio_Loaded()) {
			_Audio_Engine->Seek_To_Position(_Current_Position_Ms);
		}

		// Start MIDI playback thread (MIDI is the master clock)
		Success &= _MIDI_Engine->Start_Playback();

		// Start audio if loaded
		if (Is_Audio_Loaded())
		{
			if (_Current_State == Playback_State::Paused)
				Success &= _Audio_Engine->Resume_Playback();
			else
				Success &= _Audio_Engine->Start_Playback();
		}

		if (Success) {
			_Current_State = Playback_State::Playing;
		}

		return Success;
	}

	bool Playback_Manager::Pause()
	{
		if (_Current_State != Playback_State::Playing)
			return false;

		bool Success = true;

		// Stop MIDI playback thread
		Success &= _MIDI_Engine->Stop_Playback();

		// Send all notes off for MIDI
		Send_All_Notes_Off();

		// Pause audio if loaded
		if (Is_Audio_Loaded())
			Success &= _Audio_Engine->Pause_Playback();

		// Update position from MIDI engine (master clock)
		_Current_Position_Ms = _MIDI_Engine->Get_Current_Position_Ms();

		if (Success)
			_Current_State = Playback_State::Paused;

		return Success;
	}

	bool Playback_Manager::Stop()
	{
		if (_Current_State == Playback_State::Stopped)
			return true;

		bool Success = true;

		// Stop MIDI playback thread
		Success &= _MIDI_Engine->Stop_Playback();

		// Send all notes off for MIDI
		Send_All_Notes_Off();

		// Clear any queued MIDI events
		_MIDI_Engine->Clear_Event_Queue();

		// Stop audio if loaded
		if (Is_Audio_Loaded())
			Success &= _Audio_Engine->Stop_Playback();

		_Current_Position_Ms = 0.0;
		_Current_State = Playback_State::Stopped;

		return Success;
	}

	bool Playback_Manager::Seek_To_Position(double position_ms)
	{
		bool Was_Playing = (_Current_State == Playback_State::Playing);

		// Stop playback during seek
		if (Was_Playing)
		{
			_MIDI_Engine->Stop_Playback();
			if (Is_Audio_Loaded()) {
				_Audio_Engine->Pause_Playback();
			}
		}

		// Send all notes off before seeking
		Send_All_Notes_Off();

		// Clear event queue
		_MIDI_Engine->Clear_Event_Queue();

		// Update position
		_Current_Position_Ms = position_ms;
		_MIDI_Engine->Set_Current_Position_Ms(position_ms);

		// Seek audio if loaded
		if (Is_Audio_Loaded())
		{
			_Audio_Engine->Seek_To_Position(position_ms);
		}

		// Resume playback if it was playing
		if (Was_Playing)
		{
			_MIDI_Engine->Start_Playback();
			if (Is_Audio_Loaded()) {
				_Audio_Engine->Resume_Playback();
			}
		}

		return true;
	}

	Playback_State Playback_Manager::Get_State()
	{
		return _Current_State;
	}

	double Playback_Manager::Get_Current_Position_Ms()
	{
		// Get position from MIDI engine if playing (it's the master clock)
		if (_Current_State == Playback_State::Playing)
		{
			double MIDI_Pos = _MIDI_Engine->Get_Current_Position_Ms();

			// Sync audio engine with MIDI position
			if (Is_Audio_Loaded()) {
				int64_t MIDI_Pos_Us = (int64_t)(MIDI_Pos * 1000.0);
				_Audio_Engine->Set_MIDI_Position_Us(MIDI_Pos_Us);
			}

			return MIDI_Pos;
		}

		// Otherwise use stored position
		return _Current_Position_Ms;
	}

	double Playback_Manager::Get_Audio_Duration_Ms()
	{
		return _Audio_Engine->Get_Audio_Duration_Ms();
	}

	bool Playback_Manager::Is_Playing()
	{
		return _Current_State == Playback_State::Playing;
	}

	void Playback_Manager::Set_Playback_Cursor_Position_Ms(double position_ms)
	{
		_Playback_Cursor_Position_Ms = position_ms;
	}

	double Playback_Manager::Get_Playback_Cursor_Position_Ms()
	{
		return _Playback_Cursor_Position_Ms;
	}

	void Playback_Manager::Set_Playback_Speed(double speed)
	{
		if (speed > 0.0)
			_Playback_Speed = speed;
	}

	double Playback_Manager::Get_Playback_Speed()
	{
		return _Playback_Speed;
	}

	bool Playback_Manager::Send_MIDI_Event(MIDI_Event_Info^ event)
	{
		return _MIDI_Engine->Send_Event(event);
	}

	bool Playback_Manager::Send_All_Notes_Off()
	{
		bool Success = true;
		for (int i = 0; i < 16; ++i)
		{
			Success &= _MIDI_Engine->Send_All_Notes_Off(i);
		}
		return Success;
	}
}