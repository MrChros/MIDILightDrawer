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
	
	public value struct Raw_Rasterized_Event
	{
		int TickStart;			// Start position in MIDI ticks
		int TickLength;			// Duration in MIDI ticks
		Color Color;			// RGB color to convert to MIDI notes
	};
	
	public ref struct Export_MIDI_Color_Note
	{
		int Tick_Start;			// Start position in MIDI ticks
		int Tick_Length;		// Duration in MIDI ticks
		int Tick_End;			// End position in MIDI ticks
		Byte Color_Value;
		int Base_Note_In_Octave;
		bool Has_Offset;
		bool Is_Direct_Follower;

		Export_MIDI_Color_Note(int tick_start, int tick_length, Byte color_value, int base_note_in_octave) {
			Tick_Start = tick_start;
			Tick_Length = tick_length;
			Tick_End = tick_start + tick_length;
			Color_Value = color_value;
			Base_Note_In_Octave = base_note_in_octave;
			Has_Offset = false;
			Is_Direct_Follower = false;
		}
	};
	
	public value struct Playback_OnOff_Pair {
		Playback_MIDI_Event^ Note_On;
		Playback_MIDI_Event^ Note_Off;
	};

	public ref struct Export_MIDI_Track
	{
		Track^ Timeline_Track;
		List<Raw_Rasterized_Event>^ Raw_Events;

		List<Export_MIDI_Color_Note^>^ Notes_Red;
		List<Export_MIDI_Color_Note^>^ Notes_Green;
		List<Export_MIDI_Color_Note^>^ Notes_Blue;

		Export_MIDI_Track(Track^ track) {
			Timeline_Track = track;

			Raw_Events	= gcnew List<Raw_Rasterized_Event>();
			
			Notes_Red	= gcnew List<Export_MIDI_Color_Note^>();
			Notes_Green = gcnew List<Export_MIDI_Color_Note^>();
			Notes_Blue	= gcnew List<Export_MIDI_Color_Note^>();
		}
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

		List<Raw_Rasterized_Event>^ Raster_Bar_For_Export(BarEvent^ bar);
		Export_MIDI_Track^ Raster_Track_For_Export(Track^ track);
		List<Export_MIDI_Track^>^ Raster_Timeline_For_Export();

		// Method can probalby be deleted, it is only need to pre-rasterizing events
		List<Playback_MIDI_Event^>^ Raster_Bar_For_Playback(BarEvent^ bar, int track_index, uint8_t midi_channel, int octave_note_offset, bool use_anti_flicker);
		
		List<Playback_MIDI_Event^>^ Raster_Timeline_For_Playback();

		List<Playback_MIDI_Event^>^ Get_Timeline_PreRastered_Playback_Events(List<Track^>^ tracks, List<int>^ muted_tracks, List<int>^ soloed_tracks);

	private:
		void RasterBarSolid(List<Raw_Rasterized_Event>^ rastered_events, BarEvent^ bar);
		void RasterBarFade(List<Raw_Rasterized_Event>^ rastered_events, BarEvent^ bar);
		void RasterBarStrobe(List<Raw_Rasterized_Event>^ rastered_events, BarEvent^ bar);

		// Method can probalby be deleted, it is only need to pre-rasterizing events
		void Color_To_MIDI_Events(List<Playback_MIDI_Event^>^ output, Color color, int tick_start, int tick_length, int track_index, uint8_t midi_channel, int octave_note_offset);
		
		List<Playback_MIDI_Event^>^ Export_Track_To_Playback_Events(Export_MIDI_Track^ export_track);
		Playback_OnOff_Pair Color_Note_To_Playback_Events(Export_MIDI_Color_Note^ note, int note_octave_offset, int track_index);
		void Toggle_Additional_Offset();
		bool Should_Track_Play(int track_index, List<int>^ muted_tracks, List<int>^ soloed_tracks);

	private:
		static int Compare_Events_By_Timestamp(Playback_MIDI_Event^ a, Playback_MIDI_Event^ b);
	};
}

