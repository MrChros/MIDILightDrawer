#pragma once

#include <string>

#include "gp_parser.h"
#include "Settings.h"
#include "MIDI_Writer.h"
#include "Widget_Timeline.h"
#include "MIDI_Event_Raster.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref struct Playback_MIDI_Event;
	
	public ref class MIDI_Exporter
	{
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
		static int Compare_Events_By_TickStart(Export_MIDI_Color_Note^ a, Export_MIDI_Color_Note^ b);
 	};
}
