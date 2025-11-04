#include "Playback_MIDI_Engine.h"


namespace MIDILightDrawer
{
	Playback_MIDI_Engine::Playback_MIDI_Engine()
	{
		_Is_Initialized = false;
	}

	Playback_MIDI_Engine::~Playback_MIDI_Engine()
	{
		Cleanup();
	}

	bool Playback_MIDI_Engine::Initialize(int device_id)
	{
		_Is_Initialized = Playback_MIDI_Engine_Native::Initialize(device_id);
		return _Is_Initialized;
	}

	void Playback_MIDI_Engine::Cleanup()
	{
		if (_Is_Initialized)
		{
			Playback_MIDI_Engine_Native::Cleanup();
			_Is_Initialized = false;
		}
	}

	bool Playback_MIDI_Engine::Send_Event(MIDI_Event_Info^ event)
	{
		if (!event) {
			return false;
		}

		Playback_MIDI_Engine_Native::MIDI_Event Native_Event;
		Native_Event.Timestamp_Ms = event->Timestamp_Ms;
		Native_Event.Track = event->Track;
		Native_Event.Channel = event->Channel;
		Native_Event.Command = event->Command;
		Native_Event.Data1 = event->Data1;
		Native_Event.Data2 = event->Data2;

		return Playback_MIDI_Engine_Native::Send_MIDI_Event(Native_Event);
	}

	bool Playback_MIDI_Engine::Send_All_Notes_Off(int channel)
	{
		return Playback_MIDI_Engine_Native::Send_All_Notes_Off(channel);
	}

	bool Playback_MIDI_Engine::Is_Device_Open()
	{
		return Playback_MIDI_Engine_Native::Is_Device_Open();
	}
}

