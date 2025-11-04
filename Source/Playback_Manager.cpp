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
		return _Audio_Engine->Initialize(device_id, buffer_size);
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
		if (_Current_State == Playback_State::Playing)
			return true;

		bool Success = true;

		if (Is_Audio_Loaded())
		{
			if (_Current_State == Playback_State::Paused)
				Success &= _Audio_Engine->Resume_Playback();
			else
				Success &= _Audio_Engine->Start_Playback();
		}

		if (Success)
			_Current_State = Playback_State::Playing;

		return Success;
	}

	bool Playback_Manager::Pause()
	{
		if (_Current_State != Playback_State::Playing)
			return false;

		bool Success = true;

		// Send all notes off for MIDI
		Send_All_Notes_Off();

		if (Is_Audio_Loaded())
			Success &= _Audio_Engine->Pause_Playback();

		if (Success)
			_Current_State = Playback_State::Paused;

		return Success;
	}

	bool Playback_Manager::Stop()
	{
		if (_Current_State == Playback_State::Stopped)
			return true;

		bool Success = true;

		// Send all notes off for MIDI
		Send_All_Notes_Off();

		if (Is_Audio_Loaded())
			Success &= _Audio_Engine->Stop_Playback();

		_Current_Position_Ms = 0.0;
		_Current_State = Playback_State::Stopped;

		return Success;
	}

	bool Playback_Manager::Seek_To_Position(double position_ms)
	{
		// Send all notes off before seeking
		Send_All_Notes_Off();

		_Current_Position_Ms = position_ms;

		if (Is_Audio_Loaded())
			return _Audio_Engine->Seek_To_Position(position_ms);

		return true;
	}

	Playback_State Playback_Manager::Get_State()
	{
		return _Current_State;
	}

	double Playback_Manager::Get_Current_Position_Ms()
	{
		if (Is_Audio_Loaded() && _Current_State == Playback_State::Playing)
			return _Audio_Engine->Get_Current_Position_Ms();

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