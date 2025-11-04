#pragma managed(push, off)

#include "Playback_MIDI_Engine_Native.h"
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

namespace MIDILightDrawer
{
	void* Playback_MIDI_Engine_Native::_MIDI_Handle = nullptr;
	bool Playback_MIDI_Engine_Native::_Is_Initialized = false;

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
		if (_MIDI_Handle)
		{
			// Send all notes off on all channels before closing
			for (int i = 0; i < 16; ++i)
			{
				Send_All_Notes_Off(i);
			}

			midiOutClose((HMIDIOUT)_MIDI_Handle);
			_MIDI_Handle = nullptr;
		}

		_Is_Initialized = false;
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
}

#pragma managed(pop)