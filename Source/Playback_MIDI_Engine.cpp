#include "Playback_MIDI_Engine.h"
#include "Playback_Event_Queue_Manager.h"

namespace MIDILightDrawer
{
	ref class Engine_Callback_Helper {
	public:
		static Playback_MIDI_Engine^ Instance = nullptr;
	};

	void Native_Event_Sent_Callback(const Playback_MIDI_Engine_Native::MIDI_Event& event)
	{
		// Get the managed instance
		Playback_MIDI_Engine^ Managed_Instance = Engine_Callback_Helper::Instance;

		if (Managed_Instance != nullptr && Managed_Instance->_Event_Queue_Manager != nullptr)
		{
			// Convert native event to managed
			Playback_MIDI_Event^ Managed_Event = gcnew Playback_MIDI_Event();
			Managed_Event->Timestamp_ms = event.Timestamp_ms;
			Managed_Event->Timeline_Track_ID = event.Track;
			Managed_Event->MIDI_Channel = event.Channel;
			Managed_Event->MIDI_Command = event.Command;
			Managed_Event->MIDI_Data1 = event.Data1;
			Managed_Event->MIDI_Data2 = event.Data2;

			// Notify queue manager
			Managed_Instance->_Event_Queue_Manager->On_Event_Sent(Managed_Event);
		}
	}
	
	Playback_MIDI_Engine::Playback_MIDI_Engine()
	{
		_Is_Initialized = false;
	}

	Playback_MIDI_Engine::~Playback_MIDI_Engine()
	{
		Cleanup();

		if (Engine_Callback_Helper::Instance == this) {
			Engine_Callback_Helper::Instance = nullptr;
		}
	}

	void Playback_MIDI_Engine::Set_Event_Queue_Manager(Playback_Event_Queue_Manager^ manager)
	{
		_Event_Queue_Manager = manager;
		Engine_Callback_Helper::Instance = this;

		// Set native callback
		Playback_MIDI_Engine_Native::Set_Event_Sent_Callback(&Native_Event_Sent_Callback);
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
		Native_Event.Timestamp_ms = event->Timestamp_ms;
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

	void Playback_MIDI_Engine::Set_Audio_Available(bool available)
	{
		Playback_MIDI_Engine_Native::Set_Audio_Available(available);
	}

	void Playback_MIDI_Engine::Set_Audio_Position_us(int64_t position_us)
	{
		Playback_MIDI_Engine_Native::Set_Audio_Position_Us(position_us);
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

		Playback_MIDI_Engine_Native::Queue_MIDI_Event(Playback_MIDI_Engine::MIDI_Playback_Event_To_Native(event));
	}

	void Playback_MIDI_Engine::Queue_Event(Playback_MIDI_Engine_Native::MIDI_Event event)
	{
		Playback_MIDI_Engine_Native::Queue_MIDI_Event(event);
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

	double Playback_MIDI_Engine::Get_Current_Position_ms()
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

	Playback_MIDI_Engine_Native::MIDI_Event Playback_MIDI_Engine::MIDI_Playback_Event_To_Native(Playback_MIDI_Event^ event)
	{
		Playback_MIDI_Engine_Native::MIDI_Event Native_Event;
		Native_Event.Timestamp_ms = event->Timestamp_ms;
		Native_Event.Track = event->Timeline_Track_ID;
		Native_Event.Channel = event->MIDI_Channel;
		Native_Event.Command = event->MIDI_Command;
		Native_Event.Data1 = event->MIDI_Data1;
		Native_Event.Data2 = event->MIDI_Data2;

		return Native_Event;
	}
}