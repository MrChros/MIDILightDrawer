#include "Widget_Timeline_Classes.h"

namespace MIDILightDrawer
{
	//////////////////////////////
	// Drum Track Visualization //
	//////////////////////////////
	DrumNoteInfo DrumNotationMap::GetNoteInfo(int noteNumber)
	{
		if (_Notation_Map == nullptr) {
			InitializeMap();
		}
		
		DrumNoteInfo info;
		if (!_Notation_Map->TryGetValue(noteNumber, info)) {
			// Return default symbol (question mark) for unknown notes
			return DrumNoteInfo{ DrumNotationType::Unknown, 3.0f, "Unknown Note" };
		}
		return info;
	}

	Dictionary<int, DrumNoteInfo>^ DrumNotationMap::GetMap()
	{
		if (_Notation_Map == nullptr) {
			InitializeMap();
		}
		return _Notation_Map;
	}
	
	void DrumNotationMap::InitializeMap()
	{
		_Notation_Map = gcnew Dictionary<int, DrumNoteInfo>();

		// Standard General MIDI Drum Map notation
		_Notation_Map->Add(35, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 4.5f, "Bass Drum 1"		});
		_Notation_Map->Add(36, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 4.5f, "Bass Drum 2"		});
		_Notation_Map->Add(38, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 2.5f, "Snare"				});
		
		_Notation_Map->Add(50, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 1.0f, "High Tom"			});
		_Notation_Map->Add(48, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 1.5f, "Hi-Mid Tom"		});
		_Notation_Map->Add(47, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 2.0f, "Low-Mid Tom"		});
		_Notation_Map->Add(45, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 3.0f, "Low Tom"			});
		_Notation_Map->Add(43, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 3.5f, "High Floor Tom"	});
		_Notation_Map->Add(41, DrumNoteInfo{ DrumNotationType::NoteEllipse	, 4.0f, "Low Floor Tom"		});
		
		_Notation_Map->Add(44, DrumNoteInfo{ DrumNotationType::RegularX		, 4.5f, "Hi-Hat Step"		});
		_Notation_Map->Add(42, DrumNoteInfo{ DrumNotationType::RegularX		, 0.5f, "Closed Hi-Hat"		});
		_Notation_Map->Add(46, DrumNoteInfo{ DrumNotationType::CircledX		, 0.5f, "Open Hi-Hat"		});
			
		_Notation_Map->Add(49, DrumNoteInfo{ DrumNotationType::FilledDiamond, 0.5f, "Crash Cymbal 1"	});
		_Notation_Map->Add(57, DrumNoteInfo{ DrumNotationType::FilledDiamond, 0.5f, "Crash Cymbal 1"	});
		_Notation_Map->Add(51, DrumNoteInfo{ DrumNotationType::HollowDiamond, 0.5f, "Ride Cymbal 1"		});
		_Notation_Map->Add(59, DrumNoteInfo{ DrumNotationType::HollowDiamond, 0.5f, "Ride Cymbal 2"		});
		_Notation_Map->Add(53, DrumNoteInfo{ DrumNotationType::HollowDiamond, 0.5f, "Ride Bell"			});
		_Notation_Map->Add(52, DrumNoteInfo{ DrumNotationType::AccentedX	, 0.5f, "China Cymbal"		});

		_Notation_Map->Add(31, DrumNoteInfo{ DrumNotationType::RegularX		, 2.5f, "Sticks"			});
	}

	
	/////////////
	// Measure //
	/////////////
	Measure::Measure(int startTick, int num, int denom, int tempo, String^ marker_text)
	{
		StartTick = startTick;
		Numerator = num;
		Denominator = denom;
		Tempo = tempo;
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
		this->_Name						= trackName;
		this->_Octave					= octave;
		this->_Events					= gcnew List<BarEvent^>();
		this->_IsSelected				= false;
		this->Measures					= gcnew List<TrackMeasure^>();
		this->ShowTablature				= true;
		this->IsDrumTrack				= false;
		this->ShowAsStandardNotation	= true;
	}

	void Track::AddBar(int startTick, int length, Color color)
	{
		BarEvent^ newBar = gcnew BarEvent(this, startTick, length, color);
		_Events->Add(newBar);

		// Sort using the static comparison delegate
		_Events->Sort(barComparer);
	}

	void Track::AddBar(BarEvent^ bar)
	{
		bar->ContainingTrack = this;

		_Events->Add(bar);
		_Events->Sort(barComparer);
	}

	void Track::RemoveBar(BarEvent^ bar)
	{
		if (_Events->Contains(bar)) {
			_Events->Remove(bar);
		}
	}

	int Track::CompareBarEvents(BarEvent^ a, BarEvent^ b)
	{
		return a->StartTick.CompareTo(b->StartTick);
	}


	//////////////
	// BarEvent //
	//////////////
	BarEvent::BarEvent(Track^ track, int start_tick, int duration_in_ticks, System::Drawing::Color color)
	{
		_Track						= track;
		_Track_Original				= track;
		_Start_Tick					= start_tick;
		_Start_Tick_Original		= start_tick;
		_Duration_In_Ticks			= duration_in_ticks;
		_Duration_In_Ticks_Original	= duration_in_ticks;
		_Color						= color;
	}

	void BarEvent::StartTick::set(int value)
	{
		if(value < 0) {
			value = 0;
		}
		_Start_Tick = value;
	}

	void BarEvent::Duration::set(int value)
	{
		if(value > 0) {
			_Duration_In_Ticks = value;
		}
	}
}