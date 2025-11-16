#include "MIDI_Event_Raster.h"

#include "MIDI_Writer.h"
#include "Playback_MIDI_Engine.h"

namespace MIDILightDrawer
{
	MIDI_Event_Raster::MIDI_Event_Raster(Widget_Timeline^ timeline)
	{
		this->_Timeline = timeline;
		this->_Additional_Offset = 0;
		this->_Last_End_Tick = -1;
		this->_Next_Start_Tick = -1;
		this->_Last_Color = Color();
	}

	uint64_t MIDI_Event_Raster::Convert_Microseconds_To_Samples(uint64_t microseconds, uint32_t sample_rate)
	{
		return (microseconds * sample_rate) / 1000000;
	}

	uint64_t MIDI_Event_Raster::Convert_Samples_To_Microseconds(uint64_t samples, uint32_t sample_rate)
	{
		return (samples * 1000000) / sample_rate;
	}

	List<Raw_Rasterized_Event>^ MIDI_Event_Raster::Raster_Bar_For_Export(BarEvent^ bar)
	{
		List<Raw_Rasterized_Event>^ RasteredEvents = gcnew List<Raw_Rasterized_Event>();

		switch (bar->Type)
		{
		case BarEventType::Solid:	RasterBarSolid(RasteredEvents, bar);	break;
		case BarEventType::Fade:	RasterBarFade(RasteredEvents, bar);		break;
		case BarEventType::Strobe:	RasterBarStrobe(RasteredEvents, bar);	break;
		}

		return RasteredEvents;
	}

	Export_MIDI_Track^ MIDI_Event_Raster::Raster_Track_For_Export(Track^ track)
	{
		Export_MIDI_Track^ Export_Track = gcnew Export_MIDI_Track(track);
		
		for (int i = 0; i < track->Events->Count; i++)
		{
			BarEvent^ Bar = track->Events[i];

			List<Raw_Rasterized_Event>^ BarEvents = Raster_Bar_For_Export(Bar);

			for each (Raw_Rasterized_Event E in BarEvents)
			{
				uint8_t Value_Red	= (E.Color.R >> 1);
				uint8_t Value_Green = (E.Color.G >> 1);
				uint8_t Value_Blue	= (E.Color.B >> 1);

				if (Value_Red > 0)
				{
					Export_MIDI_Color_Note^ Note_Event = gcnew Export_MIDI_Color_Note(E.TickStart, E.TickLength, Value_Red, Settings::Get_Instance()->MIDI_Note_Red);

					if (Settings::Get_Instance()->MIDI_Export_Anti_Flicker && (Export_Track->Notes_Red->Count > 0) && (Export_Track->Notes_Red[Export_Track->Notes_Red->Count - 1]->Tick_End == Note_Event->Tick_Start)) {
						Note_Event->Has_Offset = !Export_Track->Notes_Red[Export_Track->Notes_Red->Count - 1]->Has_Offset;
						Note_Event->Is_Direct_Follower = true;
					}

					Export_Track->Notes_Red->Add(Note_Event);
				}

				if (Value_Green > 0)
				{
					Export_MIDI_Color_Note^ Note_Event = gcnew Export_MIDI_Color_Note(E.TickStart, E.TickLength, Value_Green, Settings::Get_Instance()->MIDI_Note_Green);

					if (Settings::Get_Instance()->MIDI_Export_Anti_Flicker && (Export_Track->Notes_Green->Count > 0) && (Export_Track->Notes_Green[Export_Track->Notes_Green->Count - 1]->Tick_End == Note_Event->Tick_Start)) {
						Note_Event->Has_Offset = !Export_Track->Notes_Green[Export_Track->Notes_Green->Count - 1]->Has_Offset;
						Note_Event->Is_Direct_Follower = true;
					}

					Export_Track->Notes_Green->Add(Note_Event);
				}

				if (Value_Blue > 0)
				{
					Export_MIDI_Color_Note^ Note_Event = gcnew Export_MIDI_Color_Note(E.TickStart, E.TickLength, Value_Blue, Settings::Get_Instance()->MIDI_Note_Blue);

					if (Settings::Get_Instance()->MIDI_Export_Anti_Flicker && (Export_Track->Notes_Blue->Count > 0) && (Export_Track->Notes_Blue[Export_Track->Notes_Blue->Count - 1]->Tick_End == Note_Event->Tick_Start)) {
						Note_Event->Has_Offset = !Export_Track->Notes_Blue[Export_Track->Notes_Blue->Count - 1]->Has_Offset;
						Note_Event->Is_Direct_Follower = true;
					}

					Export_Track->Notes_Blue->Add(Note_Event);
				}
			}

			Export_Track->Raw_Events->AddRange(BarEvents);
		}

		return Export_Track;
	}

	List<Export_MIDI_Track^>^ MIDI_Event_Raster::Raster_Timeline_For_Export()
	{
		List<Export_MIDI_Track^>^ Timeline_Export_Events = gcnew List<Export_MIDI_Track^>;
		
		for each(Track ^ T in this->_Timeline->Tracks)
		{
			Timeline_Export_Events->Add(this->Raster_Track_For_Export(T));
		}

		return Timeline_Export_Events;
	}
	
	List<Playback_MIDI_Event^>^ MIDI_Event_Raster::Raster_Bar_For_Playback(BarEvent^ bar, int track_index, uint8_t midi_channel, int octave_note_offset, bool use_anti_flicker)
	{
		List<Playback_MIDI_Event^>^ PlaybackEvents = gcnew List<Playback_MIDI_Event^>();

		// First raster to export format
		List<Raw_Rasterized_Event>^ ExportEvents = Raster_Bar_For_Export(bar);

		// Then convert to playback format
		for each (Raw_Rasterized_Event ExportEvent in ExportEvents)
		{
			Color_To_MIDI_Events(PlaybackEvents, ExportEvent.Color, ExportEvent.TickStart, ExportEvent.TickLength, track_index, midi_channel, octave_note_offset);
		}

		return PlaybackEvents;
	}
	
	List<Playback_MIDI_Event^>^ MIDI_Event_Raster::Raster_Timeline_For_Playback()
	{
		List<int>^ Muted_Tracks = _Timeline->TrackNumbersMuted;
		List<int>^ Soloed_Tracks = _Timeline->TrackNumbersSoloed;
		uint8_t Global_MIDI_Channel = Settings::Get_Instance()->Global_MIDI_Output_Channel;
		
		List<Playback_MIDI_Event^>^ AllEvents = gcnew List<Playback_MIDI_Event^>();

		for (int i = 0; i < _Timeline->Tracks->Count; i++)
		{
			// Check if track should play based on mute/solo
			if (!Should_Track_Play(i, Muted_Tracks, Soloed_Tracks)) {
				continue;
			}

			Export_MIDI_Track^ Export_Track = Raster_Track_For_Export(_Timeline->Tracks[i]);
			List<Playback_MIDI_Event^>^ Track_Playback_Events = Export_Track_To_Playback_Events(Export_Track);
			
			AllEvents->AddRange(Track_Playback_Events);
		}

		// Sort by timestamp for sequential playback
		AllEvents->Sort(gcnew Comparison<Playback_MIDI_Event^>(&MIDI_Event_Raster::Compare_Events_By_Timestamp));

		return AllEvents;
	}

	List<Playback_MIDI_Event^>^ MIDI_Event_Raster::Get_Timeline_PreRastered_Playback_Events(List<Track^>^ tracks, List<int>^ muted_tracks, List<int>^ soloed_tracks)
	{
		List<Playback_MIDI_Event^>^ AllEvents = gcnew List<Playback_MIDI_Event^>();

		for (int i = 0; i < tracks->Count; i++)
		{
			// Check if track should play based on mute/solo
			if (!Should_Track_Play(i, muted_tracks, soloed_tracks)) {
				continue;
			}

			List<Playback_MIDI_Event^>^ TrackEvents = gcnew List<Playback_MIDI_Event^>();

			for each(BarEvent ^ E in tracks[i]->Events) {
				List<Playback_MIDI_Event^>^ BarEvents = E->Playback_Rastered_Events;

				TrackEvents->AddRange(BarEvents);
			}

			AllEvents->AddRange(TrackEvents);
		}

		// Sort by timestamp for sequential playback
		AllEvents->Sort(gcnew Comparison<Playback_MIDI_Event^>(&MIDI_Event_Raster::Compare_Events_By_Timestamp));

		return AllEvents;
	}

	void MIDI_Event_Raster::RasterBarSolid(List<Raw_Rasterized_Event>^ rastered_events, BarEvent^ bar)
	{
		Raw_Rasterized_Event Event;
		Event.TickStart = bar->StartTick;
		Event.TickLength = bar->Duration;
		Event.Color = bar->Color;

		rastered_events->Add(Event);
	}

	void MIDI_Event_Raster::RasterBarFade(List<Raw_Rasterized_Event>^ rastered_events, BarEvent^ bar)
	{
		int Tick_Start = bar->StartTick;
		int Tick_Length = bar->Duration;

		// Calculate number of bars needed
		int NumBars = (int)Math::Ceiling((double)Tick_Length / bar->FadeInfo->QuantizationTicks);

		//FadeEasingsTest(NumBars, bar->FadeInfo->ColorStart, bar->FadeInfo->ColorEnd);

		if (NumBars == 0) {
			return;
		}

		Color ColorCenter;
		if (bar->FadeInfo->Type == FadeType::Two_Colors) {
			ColorCenter = Color::FromArgb(Math::Abs(bar->FadeInfo->ColorEnd.R - bar->FadeInfo->ColorStart.R) / 2,
				Math::Abs(bar->FadeInfo->ColorEnd.G - bar->FadeInfo->ColorStart.G) / 2,
				Math::Abs(bar->FadeInfo->ColorEnd.B - bar->FadeInfo->ColorStart.B) / 2);
		}
		else {
			ColorCenter = bar->FadeInfo->ColorCenter;
		}

		for (int i = 0; i < NumBars; i++)
		{
			float Ratio = (float)i / (NumBars - 1);

			if (NumBars == 1) { // Single bar case
				Ratio = 0;
			}

			Color BarColor;
			bool IsFirstHalf = Ratio <= 0.5f;

			Easing CurrentEasing = IsFirstHalf ? bar->FadeInfo->EaseIn : bar->FadeInfo->EaseOut;

			if (IsFirstHalf)
			{
				// First half: interpolate between start and center colors
				float AdjustedRatio = Ratio * 2.0f;

				int R = (int)Easings::ApplyEasing(AdjustedRatio, bar->FadeInfo->ColorStart.R, ColorCenter.R, CurrentEasing);
				int G = (int)Easings::ApplyEasing(AdjustedRatio, bar->FadeInfo->ColorStart.G, ColorCenter.G, CurrentEasing);
				int B = (int)Easings::ApplyEasing(AdjustedRatio, bar->FadeInfo->ColorStart.B, ColorCenter.B, CurrentEasing);

				BarColor = Color::FromArgb(255, R, G, B);
			}
			else
			{
				// Second half: interpolate between center and end colors
				float AdjustedRatio = (Ratio - 0.5f) * 2.0f;

				int R = (int)Easings::ApplyEasing(AdjustedRatio, ColorCenter.R, bar->FadeInfo->ColorEnd.R, CurrentEasing);
				int G = (int)Easings::ApplyEasing(AdjustedRatio, ColorCenter.G, bar->FadeInfo->ColorEnd.G, CurrentEasing);
				int B = (int)Easings::ApplyEasing(AdjustedRatio, ColorCenter.B, bar->FadeInfo->ColorEnd.B, CurrentEasing);

				BarColor = Color::FromArgb(255, R, G, B);
			}

			int BarTickStart = Tick_Start + (i * bar->FadeInfo->QuantizationTicks);
			int BarTickDuration = bar->FadeInfo->QuantizationTicks;

			Raw_Rasterized_Event Event;
			Event.TickStart = BarTickStart;
			Event.TickLength = BarTickDuration;
			Event.Color = BarColor;

			rastered_events->Add(Event);
		}
	}

	void MIDI_Event_Raster::RasterBarStrobe(List<Raw_Rasterized_Event>^ rastered_events, BarEvent^ bar)
	{
		int Tick_Start = bar->StartTick;
		int Tick_Length = bar->Duration;

		// Calculate number of bars needed
		int NumBars = (int)Math::Ceiling((double)Tick_Length / bar->StrobeInfo->QuantizationTicks) >> 1;

		for (int i = 0; i < NumBars; i++)
		{
			int BarTickStart = Tick_Start + (i * 2 * bar->StrobeInfo->QuantizationTicks);
			int BarTickDuration = bar->StrobeInfo->QuantizationTicks;

			Raw_Rasterized_Event Event;
			Event.TickStart = BarTickStart;
			Event.TickLength = BarTickDuration;
			Event.Color = bar->Color;

			rastered_events->Add(Event);
		}
	}

	
	void MIDI_Event_Raster::Color_To_MIDI_Events(List<Playback_MIDI_Event^>^ output, Color color, int tick_start, int tick_length, int track_index, uint8_t midi_channel, int octave_note_offset)
	{
		Settings^ Settings = Settings::Get_Instance();

		int AppliedTickLength = tick_length;

		// Apply anti-flicker logic if enabled
		if (Settings->MIDI_Export_Anti_Flicker == true)
		{
			if (_Last_End_Tick == tick_start) {
				Toggle_Additional_Offset();
			}

			if (_Next_Start_Tick == tick_start + tick_length) {
				AppliedTickLength += 1;
			}

			octave_note_offset += _Additional_Offset;
		}

		// Convert RGB color to MIDI note values (halved to fit 0-127 range)
		uint8_t ValueRed = (color.R >> 1);
		uint8_t ValueGreen = (color.G >> 1);
		uint8_t ValueBlue = (color.B >> 1);

		// Convert tick times to microseconds
		double Timestamp_Start_ms	= _Timeline->TicksToMilliseconds(tick_start);
		double Timestamp_End_ms		= _Timeline->TicksToMilliseconds(tick_start + AppliedTickLength);

		int Applicable_MIDI_Channel = Math::Max(midi_channel - 1, 0);	// Ensure minimum MIDI Channel is 0 (Channel 1 in terms of 1 to 16)

		// Create Note On events for each color channel
		if (ValueRed > 0)
		{
			Playback_MIDI_Event^ NoteOn = gcnew Playback_MIDI_Event();
			NoteOn->Timestamp_ms = Timestamp_Start_ms;
			NoteOn->Tick = tick_start;
			NoteOn->Timeline_Track_ID = track_index;
			NoteOn->MIDI_Channel = Applicable_MIDI_Channel;
			NoteOn->MIDI_Command = MIDI_Writer::MIDI_EVENT_NOTE_ON;
			NoteOn->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Red;
			NoteOn->MIDI_Data2 = ValueRed;
			
			output->Add(NoteOn);


			Playback_MIDI_Event^ NoteOff = gcnew Playback_MIDI_Event();
			NoteOff->Timestamp_ms = Timestamp_End_ms;
			NoteOff->Tick = tick_start + AppliedTickLength;
			NoteOff->Timeline_Track_ID = track_index;
			NoteOff->MIDI_Channel = Applicable_MIDI_Channel;
			NoteOff->MIDI_Command = MIDI_Writer::MIDI_EVENT_NOTE_OFF;  // Note Off
			NoteOff->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Red;
			NoteOff->MIDI_Data2 = 0;
			
			output->Add(NoteOff);
		}

		if (ValueGreen > 0)
		{
			Playback_MIDI_Event^ NoteOn = gcnew Playback_MIDI_Event();
			NoteOn->Timestamp_ms = Timestamp_Start_ms;
			NoteOn->Tick = tick_start;
			NoteOn->Timeline_Track_ID = track_index;
			NoteOn->MIDI_Channel = Applicable_MIDI_Channel;
			NoteOn->MIDI_Command = MIDI_Writer::MIDI_EVENT_NOTE_ON;
			NoteOn->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Green;
			NoteOn->MIDI_Data2 = ValueGreen;
			
			output->Add(NoteOn);


			Playback_MIDI_Event^ NoteOff = gcnew Playback_MIDI_Event();
			NoteOff->Timestamp_ms = Timestamp_End_ms;
			NoteOff->Tick = tick_start + AppliedTickLength;
			NoteOff->MIDI_Channel = Applicable_MIDI_Channel;
			NoteOff->MIDI_Command = MIDI_Writer::MIDI_EVENT_NOTE_OFF;
			NoteOff->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Green;
			NoteOff->MIDI_Data2 = 0;
			NoteOff->Timeline_Track_ID = track_index;
			
			output->Add(NoteOff);
		}

		if (ValueBlue > 0)
		{
			Playback_MIDI_Event^ NoteOn = gcnew Playback_MIDI_Event();
			NoteOn->Timestamp_ms = Timestamp_Start_ms;
			NoteOn->Tick = tick_start;
			NoteOn->Timeline_Track_ID = track_index;
			NoteOn->MIDI_Channel = Applicable_MIDI_Channel;
			NoteOn->MIDI_Command = MIDI_Writer::MIDI_EVENT_NOTE_ON;
			NoteOn->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Blue;
			NoteOn->MIDI_Data2 = ValueBlue;
			
			output->Add(NoteOn);


			Playback_MIDI_Event^ NoteOff = gcnew Playback_MIDI_Event();
			NoteOff->Timestamp_ms = Timestamp_End_ms;
			NoteOff->Tick = tick_start + AppliedTickLength;
			NoteOff->Timeline_Track_ID = track_index;
			NoteOff->MIDI_Channel = Applicable_MIDI_Channel;
			NoteOff->MIDI_Command = MIDI_Writer::MIDI_EVENT_NOTE_OFF;
			NoteOff->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Blue;
			NoteOff->MIDI_Data2 = 0;

			output->Add(NoteOff);
		}
	}
	

	List<Playback_MIDI_Event^>^ MIDI_Event_Raster::Export_Track_To_Playback_Events(Export_MIDI_Track^ export_track)
	{
		Settings^ Settings = Settings::Get_Instance();
		
		List<Playback_MIDI_Event^>^ Playback_Events = gcnew List<Playback_MIDI_Event^>();
		
		List<Export_MIDI_Color_Note^>^ All_Notes = gcnew List<Export_MIDI_Color_Note^>();

		All_Notes->AddRange(export_track->Notes_Red);
		All_Notes->AddRange(export_track->Notes_Green);
		All_Notes->AddRange(export_track->Notes_Blue);

		int Octave = export_track->Timeline_Track->Octave;
		int Octave_Note_Offset = (Octave + MIDI_Event_Raster::OCTAVE_OFFSET) * MIDI_Event_Raster::NOTES_PER_OCTAVE;
		
		for each (Export_MIDI_Color_Note^ Note in All_Notes)
		{
			Playback_OnOff_Pair OnOff_Pair = Color_Note_To_Playback_Events(Note, Octave_Note_Offset, export_track->Timeline_Track->Index);
			
			Playback_Events->Add(OnOff_Pair.Note_On);
			Playback_Events->Add(OnOff_Pair.Note_Off);
		}

		return Playback_Events;
	}

	Playback_OnOff_Pair MIDI_Event_Raster::Color_Note_To_Playback_Events(Export_MIDI_Color_Note^ note, int note_octave_offset, int track_index)
	{
		Settings^ Settings = Settings::Get_Instance();

		int Note_Number = note->Base_Note_In_Octave + note_octave_offset + (int)note->Has_Offset;

		double Time_Start_ms = _Timeline->TicksToMilliseconds(note->Tick_Start - (int)note->Is_Direct_Follower);
		double Time_End_ms = _Timeline->TicksToMilliseconds(note->Tick_End);

		int Applicable_MIDI_Channel = Math::Max(Settings->Global_MIDI_Output_Channel - 1, 0);	// Ensure minimum MIDI Channel is 0 (Channel 1 in terms of 1 to 16) 

		Playback_MIDI_Event^ Note_On = gcnew Playback_MIDI_Event();
		Note_On->Timestamp_ms = Time_Start_ms;
		Note_On->Tick = note->Tick_Start - (int)note->Is_Direct_Follower;
		Note_On->Timeline_Track_ID = track_index;
		Note_On->MIDI_Channel = Applicable_MIDI_Channel;
		Note_On->MIDI_Command = MIDI_Writer::MIDI_EVENT_NOTE_ON;
		Note_On->MIDI_Data1 = Note_Number;
		Note_On->MIDI_Data2 = note->Color_Value;

		Playback_MIDI_Event^ Note_Off = gcnew Playback_MIDI_Event();
		Note_Off->Timestamp_ms = Time_End_ms;
		Note_Off->Tick = note->Tick_End;
		Note_Off->Timeline_Track_ID = track_index;
		Note_Off->MIDI_Channel = Applicable_MIDI_Channel;
		Note_Off->MIDI_Command = MIDI_Writer::MIDI_EVENT_NOTE_OFF;
		Note_Off->MIDI_Data1 = Note_Number;
		Note_Off->MIDI_Data2 = 0;

		Playback_OnOff_Pair OnOff_Pair;
		OnOff_Pair.Note_On = Note_On;
		OnOff_Pair.Note_Off = Note_Off;

		return OnOff_Pair;
	}

	void MIDI_Event_Raster::Toggle_Additional_Offset()
	{
		this->_Additional_Offset = (this->_Additional_Offset + 1) & 1;
	}

	bool MIDI_Event_Raster::Should_Track_Play(int track_index, List<int>^ muted_tracks, List<int>^ soloed_tracks)
	{
		// Check if muted
		if (muted_tracks->Contains(track_index)) {
			return false;
		}

		// If any solos active, only play soloed tracks
		if (soloed_tracks->Count > 0) {
			return soloed_tracks->Contains(track_index);
		}

		return true;
	}

	int MIDI_Event_Raster::Compare_Events_By_Timestamp(Playback_MIDI_Event^ a, Playback_MIDI_Event^ b)
	{
		if (a->Timestamp_ms < b->Timestamp_ms) {
			return -1;
		}
		else if (a->Timestamp_ms > b->Timestamp_ms) {
			return 1;
		}
		else {
			return 0;
		}
	}
}
