#include "MIDI_Event_Raster.h"

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

	List<Export_MIDI_Event>^ MIDI_Event_Raster::Raster_Bar_For_Export(BarEvent^ bar)
	{
		List<Export_MIDI_Event>^ RasteredEvents = gcnew List<Export_MIDI_Event>();

		switch (bar->Type)
		{
		case BarEventType::Solid:	RasterBarSolid(RasteredEvents, bar);	break;
		case BarEventType::Fade:	RasterBarFade(RasteredEvents, bar);		break;
		case BarEventType::Strobe:	RasterBarStrobe(RasteredEvents, bar);	break;
		}

		return RasteredEvents;
	}

	List<Export_MIDI_Event>^ MIDI_Event_Raster::Raster_Track_For_Export(Track^ track)
	{
		List<Export_MIDI_Event>^ AllRasteredEvents = gcnew List<Export_MIDI_Event>();

		// Reset anti-flicker state for new track
		this->_Additional_Offset = 0;
		this->_Last_End_Tick = -1;
		this->_Next_Start_Tick = -1;
		this->_Last_Color = Color();

		for (int i = 0; i < track->Events->Count; i++)
		{
			BarEvent^ Bar = track->Events[i];

			// Determine next start tick for anti-flicker
			if (i < track->Events->Count - 1) {
				_Next_Start_Tick = track->Events[i + 1]->StartTick;
			}
			else {
				_Next_Start_Tick = -1;
			}

			List<Export_MIDI_Event>^ BarEvents = Raster_Bar_For_Export(Bar);
			AllRasteredEvents->AddRange(BarEvents);

			if (BarEvents->Count > 0)
			{
				_Last_End_Tick = BarEvents[BarEvents->Count - 1].TickStart + BarEvents[BarEvents->Count - 1].TickLength;
				_Last_Color = BarEvents[BarEvents->Count - 1].Color;
			}
		}

		return AllRasteredEvents;
	}

	List<Export_MIDI_Track>^ MIDI_Event_Raster::Raster_Timeline_For_Export()
	{
		List<Export_MIDI_Track>^ Timeline_Export_Events = gcnew List<Export_MIDI_Track>;
		
		for each(Track ^ T in this->_Timeline->Tracks)
		{
			int Octave = T->Octave;
			int Octave_Note_Offset = (Octave + MIDI_Event_Raster::OCTAVE_OFFSET) * MIDI_Event_Raster::NOTES_PER_OCTAVE;

			List<Export_MIDI_Event>^ RasteredEvents = this->Raster_Track_For_Export(T);

			Export_MIDI_Track Export_Track;
			Export_Track.Track = T;
			Export_Track.Events = RasteredEvents;

			Timeline_Export_Events->Add(Export_Track);
		}

		return Timeline_Export_Events;
	}

	List<Playback_MIDI_Event^>^ MIDI_Event_Raster::Raster_Bar_For_Playback(BarEvent^ bar, int track_index, uint8_t midi_channel, int octave_note_offset, bool use_anti_flicker)
	{
		List<Playback_MIDI_Event^>^ PlaybackEvents = gcnew List<Playback_MIDI_Event^>();

		// First raster to export format
		List<Export_MIDI_Event>^ ExportEvents = Raster_Bar_For_Export(bar);

		// Then convert to playback format
		for each (Export_MIDI_Event ExportEvent in ExportEvents)
		{
			Color_To_MIDI_Events(PlaybackEvents, ExportEvent.Color, ExportEvent.TickStart, ExportEvent.TickLength, track_index, midi_channel, octave_note_offset);
		}

		return PlaybackEvents;
	}

	List<Playback_MIDI_Event^>^ MIDI_Event_Raster::Raster_Track_For_Playback(Track^ track, int track_index, uint8_t midi_channel, bool use_anti_flicker)
	{
		List<Playback_MIDI_Event^>^ AllPlaybackEvents = gcnew List<Playback_MIDI_Event^>();

		// Reset anti-flicker state
		this->_Additional_Offset = 0;
		this->_Last_End_Tick = -1;
		this->_Next_Start_Tick = -1;
		this->_Last_Color = Color();

		int OctaveNoteOffset = (track->Octave + OCTAVE_OFFSET) * NOTES_PER_OCTAVE;

		for (int i = 0; i < track->Events->Count; i++)
		{
			BarEvent^ Bar = track->Events[i];

			// Determine next start tick for anti-flicker
			if (i < track->Events->Count - 1) {
				this->_Next_Start_Tick = track->Events[i + 1]->StartTick;
			}
			else {
				this->_Next_Start_Tick = -1;
			}

			List<Playback_MIDI_Event^>^ BarEvents = Raster_Bar_For_Playback(
				Bar,
				track_index,
				midi_channel,
				OctaveNoteOffset,
				use_anti_flicker
			);

			AllPlaybackEvents->AddRange(BarEvents);

			if (BarEvents->Count > 0)
			{
				this->_Last_End_Tick = Bar->StartTick + Bar->Duration;
				this->_Last_Color = Bar->Color;
			}
		}

		return AllPlaybackEvents;
	}

	List<Playback_MIDI_Event^>^ MIDI_Event_Raster::Raster_Timeline_For_Playback(List<Track^>^ tracks, List<int>^ muted_tracks, List<int>^ soloed_tracks, uint8_t global_midi_channel)
	{
		List<Playback_MIDI_Event^>^ AllEvents = gcnew List<Playback_MIDI_Event^>();

		for (int i = 0; i < tracks->Count; i++)
		{
			// Check if track should play based on mute/solo
			if (!Should_Track_Play(i, muted_tracks, soloed_tracks)) {
				continue;
			}

			List<Playback_MIDI_Event^>^ TrackEvents = Raster_Track_For_Playback(tracks[i], i, global_midi_channel, true); // use_anti_flicker
			AllEvents->AddRange(TrackEvents);
		}

		// Sort by timestamp for sequential playback
		AllEvents->Sort(gcnew Comparison<Playback_MIDI_Event^>(&MIDI_Event_Raster::Compare_Events_By_Timestamp));

		return AllEvents;
	}

	List<Playback_MIDI_Event^>^ MIDI_Event_Raster::Get_Timeline_PreRastered_Playback_Events(List<Track^>^ tracks, List<int>^ muted_tracks, List<int>^ soloed_tracks, uint8_t global_midi_channel)
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

	void MIDI_Event_Raster::RasterBarSolid(List<Export_MIDI_Event>^ rastered_events, BarEvent^ bar)
	{
		Export_MIDI_Event Event;
		Event.TickStart = bar->StartTick;
		Event.TickLength = bar->Duration;
		Event.Color = bar->Color;

		rastered_events->Add(Event);
	}

	void MIDI_Event_Raster::RasterBarFade(List<Export_MIDI_Event>^ rastered_events, BarEvent^ bar)
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

			Export_MIDI_Event Event;
			Event.TickStart = BarTickStart;
			Event.TickLength = BarTickDuration;
			Event.Color = BarColor;

			rastered_events->Add(Event);
		}
	}

	void MIDI_Event_Raster::RasterBarStrobe(List<Export_MIDI_Event>^ rastered_events, BarEvent^ bar)
	{
		int Tick_Start = bar->StartTick;
		int Tick_Length = bar->Duration;

		// Calculate number of bars needed
		int NumBars = (int)Math::Ceiling((double)Tick_Length / bar->StrobeInfo->QuantizationTicks) >> 1;

		for (int i = 0; i < NumBars; i++)
		{
			int BarTickStart = Tick_Start + (i * 2 * bar->StrobeInfo->QuantizationTicks);
			int BarTickDuration = bar->StrobeInfo->QuantizationTicks;

			Export_MIDI_Event Event;
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

		// Create Note On events for each color channel
		if (ValueRed > 0)
		{
			Playback_MIDI_Event^ NoteOn = gcnew Playback_MIDI_Event();
			NoteOn->Timestamp_ms = Timestamp_Start_ms;
			NoteOn->Start_Tick = tick_start;
			NoteOn->Timeline_Track_ID = track_index;
			NoteOn->MIDI_Channel = midi_channel;
			NoteOn->MIDI_Command = 0x90;  // Note On
			NoteOn->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Red;
			NoteOn->MIDI_Data2 = ValueRed;
			
			output->Add(NoteOn);


			Playback_MIDI_Event^ NoteOff = gcnew Playback_MIDI_Event();
			NoteOff->Timestamp_ms = Timestamp_End_ms;
			NoteOff->Start_Tick = tick_start + AppliedTickLength;
			NoteOff->Timeline_Track_ID = track_index;
			NoteOff->MIDI_Channel = midi_channel;
			NoteOff->MIDI_Command = 0x80;  // Note Off
			NoteOff->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Red;
			NoteOff->MIDI_Data2 = 0;
			
			output->Add(NoteOff);
		}

		if (ValueGreen > 0)
		{
			Playback_MIDI_Event^ NoteOn = gcnew Playback_MIDI_Event();
			NoteOn->Timestamp_ms = Timestamp_Start_ms;
			NoteOn->Start_Tick = tick_start;
			NoteOn->Timeline_Track_ID = track_index;
			NoteOn->MIDI_Channel = midi_channel;
			NoteOn->MIDI_Command = 0x90;  // Note On
			NoteOn->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Green;
			NoteOn->MIDI_Data2 = ValueGreen;
			
			output->Add(NoteOn);


			Playback_MIDI_Event^ NoteOff = gcnew Playback_MIDI_Event();
			NoteOff->Timestamp_ms = Timestamp_End_ms;
			NoteOff->Start_Tick = tick_start + AppliedTickLength;
			NoteOff->MIDI_Channel = midi_channel;
			NoteOff->MIDI_Command = 0x80;  // Note Off
			NoteOff->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Green;
			NoteOff->MIDI_Data2 = 0;
			NoteOff->Timeline_Track_ID = track_index;
			
			output->Add(NoteOff);
		}

		if (ValueBlue > 0)
		{
			Playback_MIDI_Event^ NoteOn = gcnew Playback_MIDI_Event();
			NoteOn->Timestamp_ms = Timestamp_Start_ms;
			NoteOn->Start_Tick = tick_start;
			NoteOn->Timeline_Track_ID = track_index;
			NoteOn->MIDI_Channel = midi_channel;
			NoteOn->MIDI_Command = 0x90;  // Note On
			NoteOn->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Blue;
			NoteOn->MIDI_Data2 = ValueBlue;
			
			output->Add(NoteOn);


			Playback_MIDI_Event^ NoteOff = gcnew Playback_MIDI_Event();
			NoteOff->Timestamp_ms = Timestamp_End_ms;
			NoteOff->Start_Tick = tick_start + AppliedTickLength;
			NoteOff->Timeline_Track_ID = track_index;
			NoteOff->MIDI_Channel = midi_channel;
			NoteOff->MIDI_Command = 0x80;  // Note Off
			NoteOff->MIDI_Data1 = octave_note_offset + Settings->MIDI_Note_Blue;
			NoteOff->MIDI_Data2 = 0;

			output->Add(NoteOff);
		}
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
