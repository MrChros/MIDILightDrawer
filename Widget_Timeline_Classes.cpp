#include "Widget_Timeline_Classes.h"

namespace MIDILightDrawer{

	/////////////
	// Measure //
	/////////////
	Measure::Measure(int startTick, int num, int denom, String^ marker_text)
	{
		StartTick = startTick;
		Numerator = num;
		Denominator = denom;
		Marker_Text = marker_text;
		// Calculate length based on time signature
		Length = 4 * TICKS_PER_QUARTER * num / denom;
	}

	String^ Measure::ToString()
	{
		return String::Format("{0}/{1}", Numerator, Denominator);
	}


	//////////
	// Note //
	//////////


	//////////
	// Beat //
	//////////
	Beat::Beat()
	{
		Notes = gcnew List<Note^>();
	}


	//////////////////
	// TrackMeasure //
	//////////////////
	int TrackMeasure::CompareBeatsByTick(Beat^ a, Beat^ b)
	{
		return a->StartTick.CompareTo(b->StartTick);
	}

	void TrackMeasure::AddBeat(Beat^ beat)
	{
		Beats->Add(beat);
		// Sort beats by start time
		Beats->Sort(gcnew Comparison<Beat^>(&TrackMeasure::CompareBeatsByTick));
	}

	Beat^ TrackMeasure::GetBeatAtTick(int tick)
	{
		for each (Beat ^ beat in Beats)
		{
			if (beat->StartTick <= tick && beat->StartTick + beat->Duration > tick)
			{
				return beat;
			}
		}
		return nullptr;
	}


	///////////
	// Track //
	///////////	
	Track::Track(String^ trackName, int octave)
	{
		this->name = trackName;
		this->octave = octave;
		this->events = gcnew List<BarEvent^>();
		this->isSelected = false;
		this->Measures = gcnew List<TrackMeasure^>();
		this->ShowTablature = true;
	}

	void Track::AddBar(int startTick, int length, Color color)
	{
		BarEvent^ newBar = gcnew BarEvent(startTick, length, color);
		events->Add(newBar);

		// Sort using the static comparison delegate
		events->Sort(barComparer);
	}

	void Track::RemoveBar(BarEvent^ bar)
	{
		if (events->Contains(bar)) {
			events->Remove(bar);
		}
	}

	int Track::CompareBarEvents(BarEvent^ a, BarEvent^ b)
	{
		return a->StartTick.CompareTo(b->StartTick);
	}


	//////////////
	// BarEvent //
	//////////////
	BarEvent::BarEvent(int start_tick, int duration_in_ticks, System::Drawing::Color color)
	{
		_Start_Tick			= start_tick;
		_End_Tick			= start_tick + duration_in_ticks;
		_Duration_In_Ticks	= duration_in_ticks;
		_Color				= color;

		OriginalStartTick	= start_tick;
	}
}