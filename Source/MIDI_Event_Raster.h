#pragma once

#include "Widget_Timeline.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	public value struct Export_Rastered_Event
	{
		int TickStart;			// Start position in MIDI ticks
		int TickLength;			// Duration in MIDI ticks
		Color Color;			// RGB color to convert to MIDI notes
	};

	// For Real-time Playback (new use case)
	public value struct Playback_Rastered_Event
	{
		uint64_t Timestamp_us;	// Absolute time in microseconds
		uint8_t MIDI_Channel;	// MIDI channel (1-16)
		uint8_t MIDI_Command;	// MIDI command byte (0x90 = Note On, 0x80 = Note Off)
		uint8_t Data1;			// First data byte (note number or CC number)
		uint8_t Data2;			// Second data byte (velocity or CC value)
		int Track_Index;		// Which timeline track this belongs to
		int Original_Tick;		// Original tick position (for debugging/sync)
	};

	// Stores tempo information for tick-to-time conversion
	public value struct Tempo_Change
	{
		int Tick_Position;		// Absolute tick where tempo changes
		uint32_t Tempo_BPM;		// Tempo in beats per minute
		double us_Per_Tick;		// Calculated microseconds per tick at this tempo
	};

	public ref class MIDI_Event_Raster
	{
	private:
		List<Tempo_Change>^ _Tempo_Map;

		int _Last_End_Tick;
		int _Next_Start_Tick;
		Color _Last_Color;
		int _Additional_Offset;         // Toggles 0/1 for anti-flicker
	
	public:
		static const int TICKS_PER_QUARTER	= Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		static const int OCTAVE_OFFSET		= 2;
		static const int NOTES_PER_OCTAVE	= 12;

	public:
		MIDI_Event_Raster();

		void Initialize_Tempo_Map(List<Measure^>^ measures);

		uint64_t Convert_Tick_To_Microseconds(int tick);
		int Convert_Microseconds_To_Tick(uint64_t microseconds);

		uint64_t Convert_Microseconds_To_Samples(uint64_t microseconds, uint32_t sample_rate);
		uint64_t Convert_Samples_To_Microseconds(uint64_t samples, uint32_t sample_rate);

		List<Export_Rastered_Event>^ Raster_Bar_For_Export(BarEvent^ bar);
		List<Export_Rastered_Event>^ Raster_Track_For_Export(Track^ track);

		List<Playback_Rastered_Event>^ Raster_Bar_For_Playback(BarEvent^ bar, int track_index, uint8_t midi_channel, int octave_note_offset, bool use_anti_flicker);
		List<Playback_Rastered_Event>^ Raster_Track_For_Playback(Track^ track, int track_index, uint8_t midi_channel, bool use_anti_flicker);
		List<Playback_Rastered_Event>^ Raster_Timeline_For_Playback(List<Track^>^ tracks, List<int>^ muted_tracks, List<int>^ soloed_tracks, uint8_t global_midi_channel);

	private:
		void RasterBarSolid(List<Export_Rastered_Event>^ rastered_events, BarEvent^ bar);
		void RasterBarFade(List<Export_Rastered_Event>^ rastered_events, BarEvent^ bar);
		void RasterBarStrobe(List<Export_Rastered_Event>^ rastered_events, BarEvent^ bar);

		void Color_To_MIDI_Events(List<Playback_Rastered_Event>^ output, Color color, int tick_start, int tick_length, int track_index, uint8_t midi_channel, int octave_note_offset);
		void Toggle_Additional_Offset();
		bool Should_Track_Play(int track_index, List<int>^ muted_tracks, List<int>^ soloed_tracks);

		Tempo_Change Get_Tempo_At_Tick(int tick);

	private:
		static int Compare_Events_By_Timestamp(Playback_Rastered_Event a, Playback_Rastered_Event b);
	};
}

