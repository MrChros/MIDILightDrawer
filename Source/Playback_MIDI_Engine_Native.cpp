#pragma managed(push, off)

#include "Playback_MIDI_Engine_Native.h"
#include <Windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <chrono>

#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "winmm.lib")

namespace MIDILightDrawer
{
	// Static member initialization
	void* Playback_MIDI_Engine_Native::_MIDI_Handle = nullptr;
	bool Playback_MIDI_Engine_Native::_Is_Initialized = false;
	void (*Playback_MIDI_Engine_Native::_Event_Sent_Callback)(const MIDI_Event&) = nullptr;

	// Threading members initialization
	std::thread* Playback_MIDI_Engine_Native::_Midi_Thread = nullptr;
	std::atomic<bool> Playback_MIDI_Engine_Native::_Is_Playing(false);
	std::atomic<bool> Playback_MIDI_Engine_Native::_Should_Stop(false);
	std::atomic<bool> Playback_MIDI_Engine_Native::_Reset_Timing(false);
	std::atomic<int64_t> Playback_MIDI_Engine_Native::_Current_Position_us(0);
	std::deque<Playback_MIDI_Engine_Native::Scheduled_MIDI_Event> Playback_MIDI_Engine_Native::_Event_Queue;
	std::mutex Playback_MIDI_Engine_Native::_Event_Queue_Mutex;

	std::atomic<bool> Playback_MIDI_Engine_Native::_Audio_Is_Available(false);
	std::atomic<int64_t> Playback_MIDI_Engine_Native::_Audio_Position_us(0);

	bool Playback_MIDI_Engine_Native::Initialize(int device_id)
	{
		if (_Is_Initialized) {
			Cleanup();
		}

		HMIDIOUT Midi_Out;
		MMRESULT Result = midiOutOpen(&Midi_Out, device_id, 0, 0, CALLBACK_NULL);

		if (Result == MMSYSERR_NOERROR)
		{
			_MIDI_Handle = (void*)Midi_Out;
			_Is_Initialized = true;
			return true;
		}

		return false;
	}

	void Playback_MIDI_Engine_Native::Cleanup()
	{
		// Stop playback thread first
		Stop_Playback_Thread();

		if (_MIDI_Handle)
		{
			midiOutClose((HMIDIOUT)_MIDI_Handle);
			_MIDI_Handle = nullptr;
		}

		_Is_Initialized = false;
	}

	void Playback_MIDI_Engine_Native::Set_Event_Sent_Callback(void (*callback)(const MIDI_Event&))
	{
		_Event_Sent_Callback = callback;
	}

	bool Playback_MIDI_Engine_Native::Send_MIDI_Event(const MIDI_Event& event)
	{
		if (!_Is_Initialized || !_MIDI_Handle) {
			return false;
		}

		// Pack MIDI message into DWORD (status | data1 << 8 | data2 << 16)
		DWORD Midi_Message = (event.Command | event.Channel) | (event.Data1 << 8) | (event.Data2 << 16);

		MMRESULT Result = midiOutShortMsg((HMIDIOUT)_MIDI_Handle, Midi_Message);
		return (Result == MMSYSERR_NOERROR);
	}

	bool Playback_MIDI_Engine_Native::Send_All_Notes_Off(int channel)
	{
		if (!_Is_Initialized || !_MIDI_Handle) {
			return false;
		}

		// Send CC 123 (All Notes Off) on the specified channel
		DWORD Midi_Message = (0xB0 | channel) | (123 << 8) | (0 << 16);
		MMRESULT Result = midiOutShortMsg((HMIDIOUT)_MIDI_Handle, Midi_Message);

		return (Result == MMSYSERR_NOERROR);
	}

	bool Playback_MIDI_Engine_Native::Is_Device_Open()
	{
		return _Is_Initialized && (_MIDI_Handle != nullptr);
	}

	void Playback_MIDI_Engine_Native::Set_Audio_Available(bool available)
	{
		_Audio_Is_Available.store(available, std::memory_order_release);

		if (!available) {
			// Clear audio position when audio stops
			_Audio_Position_us.store(0, std::memory_order_release);
		}
	}

	void Playback_MIDI_Engine_Native::Set_Audio_Position_us(int64_t position_us)
	{
		_Audio_Position_us.store(position_us, std::memory_order_release);
	}

	bool Playback_MIDI_Engine_Native::Start_Playback_Thread()
	{
		if (!_Is_Initialized || !_MIDI_Handle) {
			return false;
		}

		if (_Midi_Thread != nullptr) {
			// Thread already running
			return false;
		}

		// Set Windows timer resolution to 1ms for better precision
		timeBeginPeriod(1);

		_Should_Stop.store(false, std::memory_order_release);
		_Is_Playing.store(true, std::memory_order_release);

		_Reset_Timing.store(true, std::memory_order_release);

		// Create and start the thread
		_Midi_Thread = new std::thread(MIDI_Playback_Thread_Function);

		return true;
	}

	bool Playback_MIDI_Engine_Native::Stop_Playback_Thread()
	{
		if (_Midi_Thread == nullptr) {
			return true;  // Already stopped
		}

		// Signal thread to stop
		_Should_Stop.store(true, std::memory_order_release);
		_Is_Playing.store(false, std::memory_order_release);

		// Wait for thread to finish
		if (_Midi_Thread->joinable()) {
			_Midi_Thread->join();
		}

		delete _Midi_Thread;
		_Midi_Thread = nullptr;

		// Clear any remaining events
		Clear_Event_Queue();

		// Restore timer resolution
		timeEndPeriod(1);

		return true;
	}

	void Playback_MIDI_Engine_Native::Queue_MIDI_Event(const MIDI_Event& event)
	{
		std::lock_guard<std::mutex> Lock(_Event_Queue_Mutex);

		Scheduled_MIDI_Event Scheduled;
		Scheduled.Execute_Time_Us = static_cast<int64_t>(event.Timestamp_ms * 1000.0);
		Scheduled.Event = event;

		_Event_Queue.push_back(Scheduled);
	}

	void Playback_MIDI_Engine_Native::Clear_Event_Queue()
	{
		std::lock_guard<std::mutex> Lock(_Event_Queue_Mutex);
		_Event_Queue.clear();
	}

	int64_t Playback_MIDI_Engine_Native::Get_Current_Position_us()
	{
		return _Current_Position_us.load(std::memory_order_acquire);
	}

	void Playback_MIDI_Engine_Native::Set_Current_Position_us(int64_t position_us)
	{
		_Current_Position_us.store(position_us, std::memory_order_release);

		if (_Audio_Is_Available.load()) {
			_Audio_Position_us.store(position_us, std::memory_order_release);
		}
	}

	bool Playback_MIDI_Engine_Native::Is_Playing_Threaded()
	{
		return _Is_Playing.load(std::memory_order_acquire);
	}

	// Precise sleep function for better timing
	void Precise_Sleep_us(int64_t microseconds)
	{
		if (microseconds <= 0) return;

		// For very short sleeps, use pure busy-wait
		if (microseconds < 500)
		{
			LARGE_INTEGER Frequency, Start, Current;
			QueryPerformanceFrequency(&Frequency);
			QueryPerformanceCounter(&Start);

			int64_t Target_Ticks = (microseconds * Frequency.QuadPart) / 1000000LL;

			do {
				QueryPerformanceCounter(&Current);
			} while ((Current.QuadPart - Start.QuadPart) < Target_Ticks);

			return;
		}

		// For longer sleeps, sleep for most of the time, then busy-wait
		if (microseconds > 1000)
		{
			int64_t Sleep_Time_Us = microseconds - 500;
			std::this_thread::sleep_for(std::chrono::microseconds(Sleep_Time_Us));
		}

		// Busy-wait for the last portion
		LARGE_INTEGER Frequency, Start, Current;
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&Start);

		int64_t Remaining_Us = (microseconds > 1000) ? 500 : microseconds;
		int64_t Target_Ticks = (Remaining_Us * Frequency.QuadPart) / 1000000LL;

		do {
			QueryPerformanceCounter(&Current);
		} while ((Current.QuadPart - Start.QuadPart) < Target_Ticks);
	}

	void Playback_MIDI_Engine_Native::MIDI_Playback_Thread_Function()
	{
		// MIDI thread does not maintains its own clock
		// It now purely reads from audio position and processes events reactively (If Audio is available)

		// Use high-resolution timer only for precise sleeping, NOT for timekeeping
		LARGE_INTEGER Frequency;
		QueryPerformanceFrequency(&Frequency);

		// Lookahead time: Process events slightly ahead of current position
		// This compensates for MIDI output latency (~2-5ms typical)
		const int64_t LOOKAHEAD_US = 5000;  // 5ms lookahead

		// Polling interval: Check for events every 0.5ms
		const int64_t POLL_INTERVAL_US = 500;

		while (!_Should_Stop.load(std::memory_order_acquire))
		{
			if (!_Is_Playing.load(std::memory_order_acquire))
			{
				Precise_Sleep_us(5000);  // 5ms when paused
				continue;
			}

			// Read current position from audio (or fallback)
			int64_t Current_Pos_us = 0;

			bool Audio_Available = _Audio_Is_Available.load(std::memory_order_acquire);

			if (Audio_Available)
			{
				// Audio is master: Use its position directly
				Current_Pos_us = _Audio_Position_us.load(std::memory_order_acquire);
			}
			else
			{
				// MIDI-only playback: Use stored position
				Current_Pos_us = _Current_Position_us.load(std::memory_order_acquire);

				// In MIDI-only mode, we need to advance the position ourselves
				// This is acceptable because there's no drift to worry about
				static LARGE_INTEGER Last_Update_Time;
				static bool First_Update = true;

				if (_Reset_Timing.load(std::memory_order_acquire)) {
					First_Update = true;
					_Reset_Timing.store(false, std::memory_order_release);
				}

				LARGE_INTEGER Now;
				QueryPerformanceCounter(&Now);

				if (!First_Update)
				{
					int64_t Elapsed_Ticks = Now.QuadPart - Last_Update_Time.QuadPart;
					int64_t Elapsed_Us = (Elapsed_Ticks * 1000000LL) / Frequency.QuadPart;
					Current_Pos_us += Elapsed_Us;
				}

				Last_Update_Time = Now;
				First_Update = false;
			}

			// Update shared position for UI
			_Current_Position_us.store(Current_Pos_us, std::memory_order_release);

			// Process MIDI events based on audio's time
			{
				std::lock_guard<std::mutex> Lock(_Event_Queue_Mutex);

				// Batch process all events at the same timestamp
				while (!_Event_Queue.empty())
				{
					const Scheduled_MIDI_Event& Next_Event = _Event_Queue.front();

					// Send events that are due (with lookahead)
					if (Next_Event.Execute_Time_Us <= Current_Pos_us + LOOKAHEAD_US)
					{
						// Store the timestamp of the first event we're processing
						int64_t Current_Batch_Timestamp = Next_Event.Execute_Time_Us;

						// Send ALL events at this timestamp (within 100us tolerance)
						// This ensures simultaneous MIDI events are sent together
						while (!_Event_Queue.empty())
						{
							const Scheduled_MIDI_Event& Batch_Event = _Event_Queue.front();

							// Check if this event is at the same timestamp (within 100us tolerance)
							if (Batch_Event.Execute_Time_Us <= Current_Batch_Timestamp + 100)
							{
								Send_MIDI_Event(Batch_Event.Event);

								if (_Event_Sent_Callback != nullptr) {
									_Event_Sent_Callback(Batch_Event.Event);
								}

								_Event_Queue.pop_front();
							}
							else
							{
								break;  // Different timestamp, stop batching
							}
						}

						// Exit outer loop to re-check timing after batch
						break;
					}
					else
					{
						break;  // Events are sorted, so we're done
					}
				}
			}

			// Precise sleep until next poll
			Precise_Sleep_us(POLL_INTERVAL_US);
		}
	}
}

#pragma managed(pop)