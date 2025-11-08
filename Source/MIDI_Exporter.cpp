#include "MIDI_Exporter.h"
#include "Easings.h"

#include <vcclr.h>

namespace MIDILightDrawer
{
	MIDI_Exporter::MIDI_Exporter(MIDI_Event_Raster^ midi_event_raster)
	{
		this->_MIDI_Event_Raster = midi_event_raster;

		_AdditionalOffset	= 0;
		_LastEndTick		= -1;
		_NextStartTick		= -1;
		_LastColor			= Color();
	}

	String^ MIDI_Exporter::Export(String^ filename, gp_parser::Parser* tab)
	{
		Settings^ Settings = Settings::Get_Instance();

		MIDI_Writer Writer(MIDI_Event_Raster::TICKS_PER_QUARTER);  // Use 960 ticks per quarter note
		
		if (tab == NULL) {
			return "No Guitar Pro 5 file opened";
		}

		// First, add all measures from the Guitar Pro file
		for (auto i = 0; i < tab->getTabFile().measureHeaders.size(); i++)
		{
			gp_parser::MeasureHeader* MH = &(tab->getTabFile().measureHeaders.at(i));
			Writer.Add_Measure(MH->timeSignature.numerator, MH->timeSignature.denominator.value, MH->tempo.value);
		}

		List<Export_MIDI_Track>^ Timeline_Export_Events = _MIDI_Event_Raster->Raster_Timeline_For_Export();

		for each(Export_MIDI_Track EMT in Timeline_Export_Events)
		{
			int Octave = EMT.Track->Octave;
			int Octave_Note_Offset = (Octave + MIDI_Event_Raster::OCTAVE_OFFSET) * MIDI_Event_Raster::NOTES_PER_OCTAVE;

			// Write all rastered events to MIDI file
			for each(Export_MIDI_Event EME in EMT.Events)
			{
				WriteEventToMIDI(&Writer, EME, Octave_Note_Offset);
			}
		}

		if (!Writer.Save_To_File(ConvertToStdString(filename))) {
			return "Failed to write MIDI file";
		}

		return String::Empty;
	}

	std::string MIDI_Exporter::ConvertToStdString(System::String^ input_string)
	{
		if (input_string == nullptr) {
			return "";
		}

		pin_ptr<const wchar_t> wch = PtrToStringChars(input_string);
		int len = ((input_string->Length + 1) * 2);
		char* ch = new char[len];

		size_t Converted_Chars = 0;
		wcstombs_s(&Converted_Chars, ch, len, wch, _TRUNCATE);
		std::string Standard_String(ch);
		delete[] ch;

		return Standard_String;
	}

	void MIDI_Exporter::WriteEventToMIDI(MIDI_Writer* writer, Export_MIDI_Event event, int octave_note_offset)
	{
		Settings^ Settings = Settings::Get_Instance();

		uint8_t Value_Red = (event.Color.R >> 1);
		uint8_t Value_Green = (event.Color.G >> 1);
		uint8_t Value_Blue = (event.Color.B >> 1);

		if (Value_Red > 0) {
			writer->Add_Note(event.TickStart, event.TickLength, 0, octave_note_offset + Settings->MIDI_Note_Red, Value_Red);
		}

		if (Value_Green > 0) {
			writer->Add_Note(event.TickStart, event.TickLength, 0, octave_note_offset + Settings->MIDI_Note_Green, Value_Green);
		}

		if (Value_Blue > 0) {
			writer->Add_Note(event.TickStart, event.TickLength, 0, octave_note_offset + Settings->MIDI_Note_Blue, Value_Blue);
		}
	}
}