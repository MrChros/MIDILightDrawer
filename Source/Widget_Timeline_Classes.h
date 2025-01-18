#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace MIDILightDrawer
{
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
	value struct TabStringInfo
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

		property String^ Name {
			String^ get() { return name; }
			void set(String^ value) { name = value; }
		}

		property List<BarEvent^>^ Events {
			List<BarEvent^>^ get() { return events; }
		}

		property bool IsSelected {
			bool get() { return isSelected; }
			void set(bool value) { isSelected = value; }
		}

		property int Octave {
			int get() { return octave; }
		}

		property List<TrackMeasure^>^ Measures;

		property int Height;
		property bool ShowTablature;
		property bool IsDrumTrack;
		property bool ShowAsStandardNotation;

		// Add methods for bar management
		void AddBar(int startTick, int length, Color color);
		void RemoveBar(BarEvent^ bar);

		static int CompareBarEvents(BarEvent^ a, BarEvent^ b);
		static Comparison<BarEvent^>^ barComparer = gcnew Comparison<BarEvent^>(&Track::CompareBarEvents);

	private:
		String^ name;
		int octave;
		List<BarEvent^>^ events;
		bool isSelected;
	};


	//////////////
	// BarEvent //
	//////////////
	public ref class BarEvent
	{
	public:
		BarEvent(int start_tick, int duration_in_ticks, Color color);

	private:
		int _Start_Tick;
		int _End_Tick;
		int _Duration_In_Ticks;
		System::Drawing::Color _Color;

	public:
		property int StartTick {
			int get() { return _Start_Tick; }
			void set(int value) { _Start_Tick = value; }
		}

		property int Duration {
			int get() { return _Duration_In_Ticks; }
			void set(int value) { _Duration_In_Ticks = value; }
		}

		property Color Color {
			System::Drawing::Color get() { return _Color; }
			void set(System::Drawing::Color value) { _Color = value; }
		}

		property int OriginalStartTick;
	};
}