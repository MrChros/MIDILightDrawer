#pragma once

#include <string>

#include "gp_parser.h"
#include "Settings.h"
#include "MIDI_Writer.h"
#include "Widget_Timeline.h"
#include "MIDI_Event_Raster.h"

using namespace System;
using namespace System::Drawing;

namespace MIDILightDrawer
{
	public ref class MIDI_Exporter
	{
		value struct RasteredEvent
		{
			int TickStart;
			int TickLength;
			Color Color;
		};
	
	public:
		
		
	private:
		MIDI_Event_Raster^ _MIDI_Event_Raster;

		int _AdditionalOffset;
		int _LastEndTick;
		int _NextStartTick;
		Color _LastColor;

	public:
		MIDI_Exporter(MIDI_Event_Raster^ midi_event_raster);

		String^ Export(String^ filename, gp_parser::Parser* tab);

		std::string ConvertToStdString(System::String^ input_string);

	private:
		void WriteEventToMIDI(MIDI_Writer* writer, Export_MIDI_Event event, int octave_note_offset);
 	};
}
