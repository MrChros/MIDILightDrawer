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
	std::atomic<int64_t> Playback_MIDI_Engine_Native::_Current_Position_us(0);
	std::vector<Playback_MIDI_Engine_Native::Scheduled_MIDI_Event> Playback_MIDI_Engine_Native::_Event_Queue;
	std::mutex Playback_MIDI_Engine_Native::_Event_Queue_Mutex;

	std::atomic<bool> Playback_MIDI_Engine_Native::_Audio_Is_Available(false);
	std::atomic<int64_t> Playback_MIDI_Engine_Native::_Audio_Position_Us(0);

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
		DWORD Midi_Message = event.Command | (event.Data1 << 8) | (event.Data2 << 16);

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
		_Audio_Is_Available.store(available, std::memory_order_relaxed);

		if (!available) {
			// Clear audio position when audio stops
			_Audio_Position_Us.store(0, std::memory_order_relaxed);
		}
	}

	void Playback_MIDI_Engine_Native::Set_Audio_Position_Us(int64_t position_us)
	{
		_Audio_Position_Us.store(position_us, std::memory_order_relaxed);
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

		_Should_Stop.store(false);
		_Is_Playing.store(true);

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
		_Should_Stop.store(true);
		_Is_Playing.store(false);

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

		// Keep queue sorted by execution time
		//std::sort(_Event_Queue.begin(), _Event_Queue.end(),
		//	[](const Scheduled_MIDI_Event& a, const Scheduled_MIDI_Event& b) {
		//		return a.Execute_Time_Us < b.Execute_Time_Us;
		//	});
	}

	void Playback_MIDI_Engine_Native::Clear_Event_Queue()
	{
		std::lock_guard<std::mutex> Lock(_Event_Queue_Mutex);
		_Event_Queue.clear();
	}

	int64_t Playback_MIDI_Engine_Native::Get_Current_Position_Us()
	{
		return _Current_Position_us.load(std::memory_order_relaxed);
	}

	void Playback_MIDI_Engine_Native::Set_Current_Position_Us(int64_t position_us)
	{
		_Current_Position_us.store(position_us, std::memory_order_relaxed);
	}

	bool Playback_MIDI_Engine_Native::Is_Playing_Threaded()
	{
		return _Is_Playing.load(std::memory_order_relaxed);
	}

	void Playback_MIDI_Engine_Native::MIDI_Playback_Thread_Function()
	{
		LARGE_INTEGER Frequency, Start_Time, Current_Time;
		QueryPerformanceFrequency(&Frequency);
		QueryPerformanceCounter(&Start_Time);

		int64_t Start_Position_Us = _Current_Position_us.load();

		// Sync state (only used when audio is available)
		auto last_sync_time = std::chrono::steady_clock::now();
		const int64_t SYNC_INTERVAL_MS = 1000;  // Sync every second
		const int64_t SYNC_THRESHOLD_US = 50000;  // 50ms threshold

		while (!_Should_Stop.load())
		{
			if (!_Is_Playing.load())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			// Calculate elapsed real time
			QueryPerformanceCounter(&Current_Time);
			int64_t Elapsed_Ticks = Current_Time.QuadPart - Start_Time.QuadPart;
			int64_t Elapsed_Us = (Elapsed_Ticks * 1000000LL) / Frequency.QuadPart;

			// Current position in real time (which matches musical time because
			// TicksToMilliseconds already accounted for tempo!)
			int64_t Current_Pos_Us = Start_Position_Us + Elapsed_Us;

			bool audio_available = _Audio_Is_Available.load(std::memory_order_relaxed);


			if (audio_available)
			{
				auto now = std::chrono::steady_clock::now();
				auto elapsed_since_sync = std::chrono::duration_cast(now - last_sync_time).count();

				if (elapsed_since_sync >= SYNC_INTERVAL_MS)
				{
					int64_t Audio_Pos_Us = _Audio_Position_Us.load(std::memory_order_relaxed);

					if (Audio_Pos_Us > 0)  // Validate audio position
					{
						int64_t Drift_Us = Current_Musical_Pos_Us - Audio_Pos_Us;

						// Debug output (rate-limited)
#ifdef _DEBUG
						std::wstringstream ss;
						ss << L"MIDI Sync Check - Drift: " << std::fixed << std::setprecision(2)
							<< (Drift_Us / 1000.0) << L" ms (MIDI: "
							<< (Current_Musical_Pos_Us / 1000.0) << L" ms, Audio: "
							<< (Audio_Pos_Us / 1000.0) << L" ms)\n";
						OutputDebugStringW(ss.str().c_str());
#endif

						// If drift exceeds threshold, adjust MIDI clock to match audio
						if (std::abs(Drift_Us) > SYNC_THRESHOLD_US)
						{
							// Reset MIDI clock to audio's position
							QueryPerformanceCounter(&Start_Time);
							Start_Position_Us = Audio_Pos_Us;
							Current_Musical_Pos_Us = Audio_Pos_Us;

#ifdef _DEBUG
							std::wstringstream sync_msg;
							sync_msg << L"*** MIDI CLOCK ADJUSTED: " << (Drift_Us / 1000.0)
								<< L" ms drift corrected ***\n";
							OutputDebugStringW(sync_msg.str().c_str());
#endif
						}
					}

					last_sync_time = now;
				}
			}


			// Update shared position
			_Current_Position_us.store(Current_Pos_Us, std::memory_order_relaxed);

			// Process events - their timestamps are already in real-world time!
			{
				std::lock_guard<std::mutex> Lock(_Event_Queue_Mutex);

				while (!_Event_Queue.empty())
				{
					const Scheduled_MIDI_Event& Next_Event = _Event_Queue.front();

					if (Next_Event.Execute_Time_Us <= Current_Pos_Us + 1000)
					{
						Send_MIDI_Event(Next_Event.Event);

						if (_Event_Sent_Callback != nullptr) {
							_Event_Sent_Callback(Next_Event.Event);
						}

						_Event_Queue.erase(_Event_Queue.begin());
					}
					else
					{
						break;
					}
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

#pragma managed(pop)