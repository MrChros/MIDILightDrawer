#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace MIDILightDrawer
{
	class Playback_MIDI_Engine_Native
	{
	public:
		struct MIDI_Event
		{
			double Timestamp_Ms;      // Timestamp in milliseconds
			int Track;                // Track number
			int Channel;              // MIDI channel (0-15)
			unsigned char Command;    // MIDI command byte
			unsigned char Data1;      // First data byte
			unsigned char Data2;      // Second data byte
		};

	private:
		static void* _MIDI_Handle;
		static bool _Is_Initialized;

		// Threading members
		static std::thread* _Midi_Thread;
		static std::atomic<bool> _Is_Playing;
		static std::atomic<bool> _Should_Stop;
		static std::atomic<int64_t> _Current_Position_Us;  // Microseconds

		// MIDI event queue (sorted by timestamp)
		struct Scheduled_MIDI_Event
		{
			int64_t Execute_Time_Us;  // When to send this event
			MIDI_Event Event;

			bool operator>(const Scheduled_MIDI_Event& other) const {
				return Execute_Time_Us > other.Execute_Time_Us;  // For min-heap
			}
		};

		static std::vector<Scheduled_MIDI_Event> _Event_Queue;
		static std::mutex _Event_Queue_Mutex;

	public:
		static bool Initialize(int device_id);
		static void Cleanup();
		static void Set_Event_Sent_Callback(void (*callback)(const MIDI_Event&));
		static bool Send_MIDI_Event(const MIDI_Event& event);
		static bool Send_All_Notes_Off(int channel);
		static bool Is_Device_Open();

		// Threading control
		static bool Start_Playback_Thread();
		static bool Stop_Playback_Thread();
		static void Queue_MIDI_Event(const MIDI_Event& event);
		static void Clear_Event_Queue();
		static int64_t Get_Current_Position_Us();
		static void Set_Current_Position_Us(int64_t position_us);
		static bool Is_Playing_Threaded();

	private:
		// Thread function
		static void MIDI_Playback_Thread_Function();
		static void (*_Event_Sent_Callback)(const MIDI_Event&);
	};
}
