#pragma once

#include "Widget_Timeline_Common.h"

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
	// None so far
		
	
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
	public value struct TrackButtonId {
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
	public value struct BarEventBasicInfo
	{
		Track^ Track;
		int StartTick;
		int DurationInTicks;
	};

	public ref class BarEventFadeInfo
	{
	public:
		BarEventFadeInfo(int qunatization_ticks, Color color_start, Color color_end);
		BarEventFadeInfo(int qunatization_ticks, Color color_start, Color color_center, Color color_end);

		property FadeType Type;
		
		property int QuantizationTicks;

		property Color ColorStart;
		property Color ColorEnd;
		property Color ColorCenter;
	};

	public ref class BarEvent
	{
	public:
		BarEvent(Track^ track, int start_tick, int duration_in_ticks, Color color);
		BarEvent(Track^ track, int start_tick, int duration_in_ticks, BarEventFadeInfo^ fade_info);

	private:
		BarEventType _Type;
		
		BarEventBasicInfo _Working;
		BarEventBasicInfo _Original;

		Color _Color;
		BarEventFadeInfo^ _FadeInfo;

	public:
		property BarEventType Type {
			BarEventType get() { return _Type; }
		}

		property Track^ ContainingTrack {
			Track^ get() { return _Working.Track; }
			void set(Track^ track) { _Working.Track = track; }
		}

		property int StartTick {
			int get() { return _Working.StartTick; }
			void set(int value);
		}

		property int EndTick {
			int get() { return _Working.StartTick + _Working.DurationInTicks; }
		}

		property int Duration {
			int get() { return _Working.DurationInTicks; }
			void set(int value);
		}

		property System::Drawing::Color Color {
			System::Drawing::Color get() { return this->_Color; }
			void set(System::Drawing::Color color) { this->_Color = color; }
		}

		property Track^ OriginalContainingTrack {
			Track^ get() { return this->_Original.Track; }
			void set(Track^ track) { this->_Original.Track = track; }
		}

		property int OriginalStartTick {
			int get() { return this->_Original.StartTick; }
			void set(int value) { this->_Original.StartTick = value; }
		}

		property int OriginalDuration {
			int get() { return this->_Original.DurationInTicks; }
			void set(int value) { this->_Original.DurationInTicks = value; }
		}

		property BarEventFadeInfo^ FadeInfo {
			BarEventFadeInfo^ get() { return this->_FadeInfo; }
			void set(BarEventFadeInfo^ fade_info);
		}
	};
}