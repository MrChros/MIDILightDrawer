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
		this->name						= trackName;
		this->octave					= octave;
		this->events					= gcnew List<BarEvent^>();
		this->isSelected				= false;
		this->Measures					= gcnew List<TrackMeasure^>();
		this->ShowTablature				= true;
		this->IsDrumTrack				= false;
		this->ShowAsStandardNotation	= true;
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