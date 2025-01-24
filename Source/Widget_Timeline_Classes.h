#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
	//////////////////////////////
	// General Type Definitions //
	//////////////////////////////
	//typedef System::Drawing::Rectangle BarEventBounds;
	
	
	//////////////////////////////
	// Drum Track Visualization //
	//////////////////////////////
	public enum class DrumNotationType {
		FilledDiamond,
		HollowDiamond,
		CircledX,
		AccentedX,    // X with ^ on top
		RegularX,
		NoteEllipse,
		Unknown
	};


	public value struct DrumNoteInfo
	{
		DrumNotationType SymbolType;
		float StringPosition;  // Can be whole numbers for on-line, decimals for between lines
		String^ Description;   // For future reference/documentation
	};


	public ref class DrumNotationMap
	{
	private:
		static Dictionary<int, DrumNoteInfo>^ _Notation_Map	= nullptr;

	public:
		static DrumNoteInfo GetNoteInfo(int noteNumber);
		static Dictionary<int, DrumNoteInfo>^ GetMap();

	private:
		static void InitializeMap();
	};
	

	//////////////////////
	// Tablature String //
	//////////////////////
	public value struct TabStringInfo
	{
		array<float>^ StringYPositions;
		float TotalHeight;
		float VerticalOffset;
		float StringSpacing;
	};

	
	//////////////////////////
	// Forward Declarations //
	//////////////////////////
	ref class Track;
	ref class BarEvent;


	/////////////
	// Measure //
	/////////////
	public ref class Measure
	{
	public:
		static const int TICKS_PER_QUARTER = 960;

		property int		StartTick;
		property int		Length;
		property int		Numerator;
		property int		Denominator;
		property int		Tempo;
		property String^	Marker_Text;

		Measure(int startTick, int num, int denom, int tempo, String^ marker_text);

		virtual String^ ToString() override;
	};


	//////////
	// Note //
	//////////
	public ref class Note {
	public:
		property int String;
		property int Value;		// Fret/pitch
		property bool IsTied;
	};


	//////////
	// Beat //
	//////////
	public ref class Beat {
	public:
		property Track^			Track;
		property int			StartTick;
		property int			Duration;	// In ticks
		property bool			IsDotted;
		property List<Note^>^	Notes;

		Beat();
	};


	//////////////////
	// TrackMeasure //
	//////////////////
	public ref class TrackMeasure : public Measure
	{
	public:
		// Constructor that calls base constructor
		TrackMeasure(int startTick, int num, int denom, String^ marker_text, Track^ track) : Measure(startTick, num, denom, 0, marker_text)
		{
			Track = track;
			Beats = gcnew List<Beat^>();
		}

		// Additional constructor that creates from existing measure
		TrackMeasure(Measure^ measure, Track^ track) : Measure(measure->StartTick, measure->Numerator, measure->Denominator, 0, measure->Marker_Text)
		{
			Track = track;
			Beats = gcnew List<Beat^>();
		}

		property Track^			Track;
		property List<Beat^>^	Beats;

		// Static comparison method for beats
		static int CompareBeatsByTick(Beat^ a, Beat^ b);

		void AddBeat(Beat^ beat);

		Beat^ GetBeatAtTick(int tick);
	};


	///////////
	// Track //
	///////////
	value struct TrackButtonId {
		Track^ Track;
		int ButtonIndex;
	};

	public ref class Track {
	public:
		Track(String^ trackName, int octave);

	private:
		String^ _Name;
		int _Octave;
		List<BarEvent^>^ _Events;
		bool _IsSelected;

	public:
		property String^ Name {
			String^ get() { return _Name; }
			void set(String^ value) { _Name = value; }
		}

		property List<BarEvent^>^ Events {
			List<BarEvent^>^ get() { return _Events; }
		}

		property bool IsSelected {
			bool get() { return _IsSelected; }
			void set(bool value) { _IsSelected = value; }
		}

		property int Octave {
			int get() { return _Octave; }
		}

		property List<TrackMeasure^>^ Measures;

		property int Height;
		property bool ShowTablature;
		property bool IsDrumTrack;
		property bool ShowAsStandardNotation;

		// Add methods for bar management
		void AddBar(int startTick, int length, Color color);
		void AddBar(BarEvent^ bar);
		void RemoveBar(BarEvent^ bar);

		static int CompareBarEvents(BarEvent^ a, BarEvent^ b);
		static Comparison<BarEvent^>^ barComparer = gcnew Comparison<BarEvent^>(&Track::CompareBarEvents);

	
	};


	//////////////
	// BarEvent //
	//////////////
	public ref class BarEvent
	{
	public:
		BarEvent(Track^ track, int start_tick, int duration_in_ticks, Color color);

	private:
		Track^ _Track;
		Track^ _Track_Original;
		int _Start_Tick;
		int _Start_Tick_Original;
		int _Duration_In_Ticks;
		int _Duration_In_Ticks_Original;
		Color _Color;

	public:
		property Track^ ContainingTrack {
			Track^ get() { return _Track; }
			void set(Track^ track) { _Track = track; }
		}

		property int StartTick {
			int get() { return _Start_Tick; }
			void set(int value) { _Start_Tick = value; }
		}

		property int EndTick {
			int get() { return _Start_Tick + _Duration_In_Ticks; }
		}

		property int Duration {
			int get() { return _Duration_In_Ticks; }
			void set(int value) { _Duration_In_Ticks = value; }
		}

		property System::Drawing::Color Color {
			System::Drawing::Color get() { return _Color; }
			void set(System::Drawing::Color color) { _Color = color; }
		}

		property Track^ OriginalContainingTrack {
			Track^ get() { return _Track_Original; }
			void set(Track^ track) { _Track_Original = track; }
		}

		property int OriginalStartTick {
			int get() { return _Start_Tick_Original; }
			void set(int value) { _Start_Tick_Original = value; }
		}

		property int OriginalDuration {
			int get() { return _Duration_In_Ticks_Original; }
			void set(int value) { _Duration_In_Ticks_Original = value; }
		}
	};
}