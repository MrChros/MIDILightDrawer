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

			String^ Export(String^ filename, gp_parser::Parser* _tab);

			std::string ConvertToStdString(System::String^ input_string);

		private:
			void WriteBarSolid(MIDI_Writer* writer, BarEvent^ bar, int octave_note_offset);
			void WriteBarSolid(MIDI_Writer* writer, int start_tick, int tick_length, Color color, int octave_note_offset);
			void WriteBarFade(MIDI_Writer* writer, BarEvent^ bar, int octave_note_offset);
			void WriteBarStrobe(MIDI_Writer* writer, BarEvent^ bar, int octave_note_offset);

			Color InterpolateFadeColor(Color start, Color end, float ratio);

			void ToggleAdditionalOffset();
	};
}
