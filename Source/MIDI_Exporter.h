#pragma once

#include <string>

#include "gp_parser.h"
#include "Settings.h"
#include "MIDI_Writer.h"
#include "Widget_Timeline.h"

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
		static const int OCTAVE_OFFSET		= 2;
		static const int NOTES_PER_OCTAVE	= 12;
		
	private:
		Widget_Timeline^ _Timeline;

		int _AdditionalOffset;
		int _LastEndTick;
		int _NextStartTick;
		Color _LastColor;

	public:
		MIDI_Exporter(Widget_Timeline^ timeline);

		String^ Export(String^ filename, gp_parser::Parser* tab);

		std::string ConvertToStdString(System::String^ input_string);

	private:
		void RasterBarSolid(List<RasteredEvent>^ rasteredEvents, BarEvent^ bar);
		void RasterBarFade(List<RasteredEvent>^ rasteredEvents, BarEvent^ bar);
		void RasterBarStrobe(List<RasteredEvent>^ rasteredEvents, BarEvent^ bar);

		void WriteEventToMIDI(MIDI_Writer* writer, int tick_start, int tick_length, Color color, int octave_note_offset);

		void ToggleAdditionalOffset();
	};
}
