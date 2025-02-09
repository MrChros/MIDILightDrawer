#include "MIDI_Exporter.h"

#include <vcclr.h>

namespace MIDILightDrawer
{
	MIDI_Exporter::MIDI_Exporter(Widget_Timeline^ timeline)
	{
		this->_Timeline = timeline;

		_AdditionalOffset	= 0;
		_LastEndTick		= -1;
		_NextStartTick		= -1;
		_LastColor			= Color();
	}

	String^ MIDI_Exporter::Export(String^ filename, gp_parser::Parser* _tab)
	{
		Settings^ Settings = Settings::Get_Instance();
		List<Settings::Octave_Entry^>^ Octave_Entries = Settings->Octave_Entries;

		MIDI_Writer Writer(960);  // Use 960 ticks per quarter note
		
		if (_tab == NULL) {
			return "No Guitar Pro 5 file opened";
		}

		// First, add all measures from the Guitar Pro file
		for (auto i = 0; i < _tab->getTabFile().measureHeaders.size(); i++)
		{
			gp_parser::MeasureHeader* MH = &(_tab->getTabFile().measureHeaders.at(i));
			Writer.Add_Measure(MH->timeSignature.numerator, MH->timeSignature.denominator.value, MH->tempo.value);
		}

		// Then add all the notes
		for each (Track^ T in this->_Timeline->Tracks)
		{
			String^ Track_Name = T->Name;
			int Octave = T->Octave;
			int Octave_Note_Offset = (Octave + MIDI_Exporter::OCTAVE_OFFSET) * MIDI_Exporter::NOTES_PER_OCTAVE;

			List<RasteredEvent>^ RasteredEvents = gcnew List<RasteredEvent>;

			for each(BarEvent^ B in T->Events)
			{
				switch (B->Type)
				{
				case BarEventType::Solid:	RasterBarSolid(RasteredEvents, B);	break;
				case BarEventType::Fade:	RasterBarFade(RasteredEvents, B);	break;
				case BarEventType::Strobe:	RasterBarStrobe(RasteredEvents, B); break;
				}
			}

			for (int e = 0;e < RasteredEvents->Count;e++)
			{
				RasteredEvent E = RasteredEvents[e];
				
				if (e < RasteredEvents->Count - 1) {
					_NextStartTick = RasteredEvents[e + 1].TickStart;
				}

				WriteEventToMIDI(&Writer, E.TickStart, E.TickLength, E.Color, Octave_Note_Offset);
			}

			RasteredEvents->Clear();
		}

		if (!Writer.Save_To_File(ConvertToStdString(filename))) {
			return "Failed to write MIDI file";
		}

		return String::Empty;
	}

	std::string MIDI_Exporter::ConvertToStdString(System::String^ input_string)
	{
		if (input_string == nullptr)
			return "";

		pin_ptr<const wchar_t> wch = PtrToStringChars(input_string);
		int len = ((input_string->Length + 1) * 2);
		char* ch = new char[len];

		size_t convertedChars = 0;
		wcstombs_s(&convertedChars, ch, len, wch, _TRUNCATE);
		std::string standardString(ch);
		delete[] ch;

		return standardString;
	}

	void MIDI_Exporter::RasterBarSolid(List<RasteredEvent>^ rasteredEvents, BarEvent^ bar)
	{
		RasteredEvent Event;
		Event.TickStart = bar->StartTick;
		Event.TickLength = bar->Duration;
		Event.Color = bar->Color;

		rasteredEvents->Add(Event);
	}

	void MIDI_Exporter::RasterBarFade(List<RasteredEvent>^ rasteredEvents, BarEvent^ bar)
	{
		int Tick_Start = bar->StartTick;
		int Tick_Length = bar->Duration;

		// Calculate number of bars needed
		int NumBars = (int)Math::Ceiling((double)Tick_Length / bar->FadeInfo->QuantizationTicks);

		if (NumBars == 0) {
			return;
		}

		for (int i = 0; i < NumBars; i++)
		{
			float ratio = (float)i / (NumBars - 1);

			if (NumBars == 1) { // Single bar case
				ratio = 0;
			}

			Color BarColor;

			if (bar->FadeInfo->Type == FadeType::Two_Colors)
			{
				BarColor = InterpolateFadeColor(bar->FadeInfo->ColorStart, bar->FadeInfo->ColorEnd, ratio);	// Simple linear interpolation between start and end colors
			}
			else
			{ 
				if (ratio <= 0.5f) {
					BarColor = InterpolateFadeColor(bar->FadeInfo->ColorStart, bar->FadeInfo->ColorCenter, ratio * 2.0f);			// First half: interpolate between start and center colors
				}
				else {
					BarColor = InterpolateFadeColor(bar->FadeInfo->ColorCenter, bar->FadeInfo->ColorEnd, (ratio - 0.5f) * 2.0f);	// Second half: interpolate between center and end colors
				}
			}

			int BarTickStart	= Tick_Start + (i * bar->FadeInfo->QuantizationTicks);
			int BarTickDuration = bar->FadeInfo->QuantizationTicks;

			RasteredEvent Event;
			Event.TickStart = BarTickStart;
			Event.TickLength = BarTickDuration;
			Event.Color = BarColor;

			rasteredEvents->Add(Event);
		}
	}

	void MIDI_Exporter::RasterBarStrobe(List<RasteredEvent>^ rasteredEvents, BarEvent^ bar)
	{
		int Tick_Start = bar->StartTick;
		int Tick_Length = bar->Duration;

		// Calculate number of bars needed
		int NumBars = (int)Math::Ceiling((double)Tick_Length / bar->StrobeInfo->QuantizationTicks) >> 1;

		for (int i = 0; i < NumBars; i++)
		{
			int BarTickStart	= Tick_Start + (i * 2 * bar->StrobeInfo->QuantizationTicks);
			int BarTickDuration = bar->StrobeInfo->QuantizationTicks;

			RasteredEvent Event;
			Event.TickStart = BarTickStart;
			Event.TickLength = BarTickDuration;
			Event.Color = bar->Color;

			rasteredEvents->Add(Event);
		}
	}

	void MIDI_Exporter::WriteEventToMIDI(MIDI_Writer* writer, int tick_start, int tick_length, Color color, int octave_note_offset)
	{
		Settings^ Settings = Settings::Get_Instance();

		int AppliedTickLength = tick_length;

		if(Settings->MIDI_Export_Anti_Flicker == true)
		{
			if (this->_LastEndTick == tick_start) {
				ToggleAdditionalOffset();
			}

			if (_NextStartTick == tick_start + tick_length) {
				AppliedTickLength += 1;
			}
		
			octave_note_offset += this->_AdditionalOffset;
		}

		uint8_t Value_Red	= (color.R >> 1);
		uint8_t Value_Green = (color.G >> 1);
		uint8_t Value_Blue	= (color.B >> 1);

		if (Value_Red > 0) {
			writer->Add_Note(tick_start, AppliedTickLength, 0, octave_note_offset + Settings->MIDI_Note_Red, Value_Red);
		}

		if (Value_Green > 0) {
			writer->Add_Note(tick_start, AppliedTickLength, 0, octave_note_offset + Settings->MIDI_Note_Green, Value_Green);
		}

		if (Value_Blue > 0) {
			writer->Add_Note(tick_start, AppliedTickLength, 0, octave_note_offset + Settings->MIDI_Note_Blue, Value_Blue);
		}

		_LastEndTick = tick_start + tick_length;
		_LastColor = color;
	}

	Color MIDI_Exporter::InterpolateFadeColor(Color start, Color end, float ratio)
	{
		int R = start.R + (int)((end.R - start.R) * ratio);
		int G = start.G + (int)((end.G - start.G) * ratio);
		int B = start.B + (int)((end.B - start.B) * ratio);

		return Color::FromArgb(255, R, G, B);
	}

	void MIDI_Exporter::ToggleAdditionalOffset()
	{
		this->_AdditionalOffset = (this->_AdditionalOffset + 1) & 1;
	}
}