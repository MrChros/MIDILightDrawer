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

	bool Playback_MIDI_Engine::Send_Event(Playback_MIDI_Event^ event)
	{
		if (!event) {
			return false;
		}

		Playback_MIDI_Engine_Native::MIDI_Event Native_Event;
		Native_Event.Timestamp_Ms = event->Timestamp_ms;
		Native_Event.Track = event->Timeline_Track_ID;
		Native_Event.Channel = event->MIDI_Channel;
		Native_Event.Command = event->MIDI_Command;
		Native_Event.Data1 = event->MIDI_Data1;
		Native_Event.Data2 = event->MIDI_Data2;

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

	bool Playback_MIDI_Engine::Start_Playback()
	{
		if (!_Is_Initialized) {
			return false;
		}

		return Playback_MIDI_Engine_Native::Start_Playback_Thread();
	}

	bool Playback_MIDI_Engine::Stop_Playback()
	{
		return Playback_MIDI_Engine_Native::Stop_Playback_Thread();
	}

	void Playback_MIDI_Engine::Queue_Event(Playback_MIDI_Event^ event)
	{
		if (!event) {
			return;
		}

		Playback_MIDI_Engine_Native::MIDI_Event Native_Event;
		Native_Event.Timestamp_Ms = event->Timestamp_ms;
		Native_Event.Track = event->Timeline_Track_ID;
		Native_Event.Channel = event->MIDI_Channel;
		Native_Event.Command = event->MIDI_Command;
		Native_Event.Data1 = event->MIDI_Data1;
		Native_Event.Data2 = event->MIDI_Data2;

		Playback_MIDI_Engine_Native::Queue_MIDI_Event(Native_Event);
	}

	void Playback_MIDI_Engine::Queue_Events(List<Playback_MIDI_Event^>^ events)
	{
		for each(Playback_MIDI_Event ^ Event in events) {
			this->Queue_Event(Event);
		}
	}

	void Playback_MIDI_Engine::Clear_Event_Queue()
	{
		Playback_MIDI_Engine_Native::Clear_Event_Queue();
	}

	double Playback_MIDI_Engine::Get_Current_Position_Ms()
	{
		int64_t Position_Us = Playback_MIDI_Engine_Native::Get_Current_Position_Us();
		return ((double)Position_Us) / 1000.0;
	}

	void Playback_MIDI_Engine::Set_Current_Position_Ms(double position_ms)
	{
		int64_t Position_Us = static_cast<int64_t>(position_ms * 1000.0);
		Playback_MIDI_Engine_Native::Set_Current_Position_Us(Position_Us);
	}

	bool Playback_MIDI_Engine::Is_Playing()
	{
		return Playback_MIDI_Engine_Native::Is_Playing_Threaded();
	}
}