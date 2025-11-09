#pragma once

#include "Widget_Timeline.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	// Forward Declaration
	ref class Widget_Timeline;
	ref struct Playback_MIDI_Event;
	
	public ref struct Export_MIDI_Color_Note
	{
		int TickStart;			// Start position in MIDI ticks
		int TickLength;			// Duration in MIDI ticks
		int TickEnd;			// End position in MIDI ticks
		uint8_t Color_Value;
		bool Has_Offset;
		bool Is_Direct_Follower;
	};
	
	public value struct Export_MIDI_Event
	{
		int TickStart;			// Start position in MIDI ticks
		int TickLength;			// Duration in MIDI ticks
		Color Color;			// RGB color to convert to MIDI notes
	};

	public ref struct Export_MIDI_Track
	{
		Track^ Track;
		List<Export_MIDI_Event>^ Events;

		List<Export_MIDI_Color_Note^>^ Notes_Red;
		List<Export_MIDI_Color_Note^>^ Notes_Green;
		List<Export_MIDI_Color_Note^>^ Notes_Blue;
	};

	public ref class MIDI_Event_Raster
	{
	private:
		Widget_Timeline^ _Timeline;

		int _Last_End_Tick;
		int _Next_Start_Tick;
		Color _Last_Color;
		int _Additional_Offset;         // Toggles 0/1 for anti-flicker

	public:
		static const int TICKS_PER_QUARTER	= Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		static const int OCTAVE_OFFSET		= 2;
		static const int NOTES_PER_OCTAVE	= 12;

	public:
		MIDI_Event_Raster(Widget_Timeline^ timeline);

		uint64_t Convert_Microseconds_To_Samples(uint64_t microseconds, uint32_t sample_rate);
		uint64_t Convert_Samples_To_Microseconds(uint64_t samples, uint32_t sample_rate);

		List<Export_MIDI_Event>^ Raster_Bar_For_Export(BarEvent^ bar);
		Export_MIDI_Track^ Raster_Track_For_Export(Track^ track);
		List<Export_MIDI_Track^>^ Raster_Timeline_For_Export();

		List<Playback_MIDI_Event^>^ Raster_Bar_For_Playback(BarEvent^ bar, int track_index, uint8_t midi_channel, int octave_note_offset, bool use_anti_flicker);
		List<Playback_MIDI_Event^>^ Raster_Track_For_Playback(Track^ track, int track_index, uint8_t midi_channel, bool use_anti_flicker);
		List<Playback_MIDI_Event^>^ Raster_Timeline_For_Playback();

		List<Playback_MIDI_Event^>^ Get_Timeline_PreRastered_Playback_Events(List<Track^>^ tracks, List<int>^ muted_tracks, List<int>^ soloed_tracks, uint8_t global_midi_channel);

	private:
		void RasterBarSolid(List<Export_MIDI_Event>^ rastered_events, BarEvent^ bar);
		void RasterBarFade(List<Export_MIDI_Event>^ rastered_events, BarEvent^ bar);
		void RasterBarStrobe(List<Export_MIDI_Event>^ rastered_events, BarEvent^ bar);

		void Color_To_MIDI_Events(List<Playback_MIDI_Event^>^ output, Color color, int tick_start, int tick_length, int track_index, uint8_t midi_channel, int octave_note_offset);
		void Toggle_Additional_Offset();
		bool Should_Track_Play(int track_index, List<int>^ muted_tracks, List<int>^ soloed_tracks);

	private:
		static int Compare_Events_By_Timestamp(Playback_MIDI_Event^ a, Playback_MIDI_Event^ b);
	};
}

