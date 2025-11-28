#include "Widget_Timeline_Classes.h"
#include "Widget_Timeline.h"

#include "Settings.h"
#include "MIDI_Event_Raster.h"

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
	Measure::Measure(int number, int startTick, double startTime_ms, int num, int denom, int tempo, String^ marker_text)
	{
		Number = number;
		StartTick = startTick;
		Numerator = num;
		Denominator = denom;
		Tempo = tempo;
		StartTime_ms = startTime_ms;
		Marker_Text = marker_text;

		// Calculate length based on time signature
		Length = 4 * TICKS_PER_QUARTER * num / denom;

		Length_ms = (double)4.0 * ((double)num / (double)denom) * ((double)60000.0 / (double)tempo);
		Length_Per_Tick_ms = ((double)60000.0 / (double)tempo) / (double)TICKS_PER_QUARTER;
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
	Track::Track(String^ trackName, int trackIndex, int octave)
	{
		this->_MIDI_Event_Raster = nullptr;

		this->_Name = trackName;
		this->_Index = trackIndex;
		this->_Octave = octave;
		this->_Events = gcnew List<BarEvent^>();
		this->_IsSelected = false;
		this->_IsMuted = false;
		this->_IsSoloed = false;
		this->Measures = gcnew List<TrackMeasure^>();
		this->ShowTablature = true;
		this->IsDrumTrack = false;
		this->ShowAsStandardNotation = true;
	}

	Track::Track(String^ trackName, int trackIndex, int octave, MIDI_Event_Raster^ midi_event_raster)
	{
		this->_MIDI_Event_Raster = midi_event_raster;
		
		this->_Name = trackName;
		this->_Index = trackIndex;
		this->_Octave = octave;
		this->_Events = gcnew List<BarEvent^>();
		this->_Event_Interval_Tree = gcnew Timeline_BarEvent_Interval_Tree();
		this->_IsSelected = false;
		this->_IsMuted = false;
		this->_IsSoloed = false;
		this->Measures = gcnew List<TrackMeasure^>();
		this->ShowTablature = true;
		this->IsDrumTrack = false;
		this->ShowAsStandardNotation = true;
	}

	void Track::AddBar(int startTick, int length, Color color)
	{
		BarEvent^ New_Bar = gcnew BarEvent(this, startTick, length, color);
		_Events->Add(New_Bar);

		// Sort using the static comparison delegate
		_Events->Sort(barComparer);

		_Event_Interval_Tree->Insert(New_Bar);
	}

	void Track::AddBar(BarEvent^ bar)
	{
		if (!bar) {
			return;
		}
		
		bar->ContainingTrack = this;

		_Events->Add(bar);
		_Events->Sort(barComparer);

		_Event_Interval_Tree->Insert(bar);
	}

	void Track::RemoveBar(BarEvent^ bar)
	{
		if (_Events->Contains(bar)) {
			_Events->Remove(bar);

			_Event_Interval_Tree->Remove(bar);
		}
	}

	List<BarEvent^>^ Track::QueryVisibleEvents(int start_tick, int end_tick)
	{
		return _Event_Interval_Tree->QueryRange(start_tick, end_tick);
	}

	List<BarEvent^>^ Track::QueryEventsAtTick(int tick)
	{
		return _Event_Interval_Tree->QueryPoint(tick);
	}

	void Track::InvalidateEventTree()
	{
		_Event_Interval_Tree->MarkDirty();
	}

	void Track::RebuildEventTree()
	{
		_Event_Interval_Tree->Rebuild(_Events);
	}

	void Track::ToggleMute()
	{
		_IsMuted = !_IsMuted;
	}

	void Track::ToggleSolo()
	{
		_IsSoloed = !_IsSoloed;
	}
	
	bool Track::ShouldPlayInPlayback(bool any_track_soloed)
	{
		// If muted, never play
		if (_IsMuted) {
			return false;
		}

		// If any track is soloed, only play if this track is soloed
		if (any_track_soloed) {
			return _IsSoloed;
		}

		// No solos active, play all unmuted tracks
		return true;
	}

	int Track::CompareBarEvents(BarEvent^ a, BarEvent^ b)
	{
		return a->StartTick.CompareTo(b->StartTick);
	}


	//////////////////////
	// BarEventFadeInfo //
	//////////////////////
	BarEventFadeInfo::BarEventFadeInfo(int quantization_ticks, Color color_start, Color color_end, FadeEasing ease_in, FadeEasing ease_out)
	{
		this->Type = FadeType::Two_Colors;
		
		this->QuantizationTicks = quantization_ticks;

		if (this->QuantizationTicks <= 0) {
			this->QuantizationTicks = Widget_Timeline::DEFAULT_FADE_TICK_QUANTIZATION;
		}

		this->ColorStart = color_start;
		this->ColorEnd = color_end;

		this->EaseIn = ease_in;
		this->EaseOut = ease_out;
	}

	BarEventFadeInfo::BarEventFadeInfo(int quantization_ticks, Color color_start, Color color_center, Color color_end, FadeEasing ease_in, FadeEasing ease_out)
	{
		this->Type = FadeType::Three_Colors;

		this->QuantizationTicks = quantization_ticks;

		if (this->QuantizationTicks <= 0) {
			this->QuantizationTicks = Widget_Timeline::DEFAULT_FADE_TICK_QUANTIZATION;
		}

		this->ColorStart = color_start;
		this->ColorEnd = color_end;
		this->ColorCenter = color_center;

		this->EaseIn = ease_in;
		this->EaseOut = ease_out;
	}


	////////////////////////
	// BarEventStrobeInfo //
	////////////////////////
	BarEventStrobeInfo::BarEventStrobeInfo(int quantization_ticks, Color color_strobe)
	{
		this->QuantizationTicks = quantization_ticks;

		if (this->QuantizationTicks <= 0) {
			this->QuantizationTicks = Widget_Timeline::DEFAULT_FADE_TICK_QUANTIZATION;
		}

		this->ColorStrobe = color_strobe;
	}


	//////////////
	// BarEvent //
	//////////////
	BarEvent::BarEvent(Track^ track, int start_tick, int duration_in_ticks, System::Drawing::Color color)
	{
		this->_Type = BarEventType::Solid;
		
		this->_Working.Track = track;
		this->_Working.StartTick = start_tick;
		this->_Working.DurationInTicks = duration_in_ticks;
		
		this->_Original.Track = track;
		this->_Original.StartTick = start_tick;
		this->_Original.DurationInTicks = duration_in_ticks;

		this->_Color = color;
		this->_FadeInfo = nullptr;
		this->_StrobeInfo = nullptr;

		this->_IgnoreForOverlap = false;

		PreRasterMIDIEvents();
	}

	BarEvent::BarEvent(Track^ track, int start_tick, int duration_in_ticks, BarEventFadeInfo^ fade_info)
	{
		this->_Type = BarEventType::Fade;

		this->_Working.Track = track;
		this->_Working.StartTick = start_tick;
		this->_Working.DurationInTicks = duration_in_ticks;

		this->_Original.Track = track;
		this->_Original.StartTick = start_tick;
		this->_Original.DurationInTicks = duration_in_ticks;

		this->_Color = System::Drawing::Color();
		this->_FadeInfo = fade_info;

		if (this->_FadeInfo == nullptr) {
			this->_FadeInfo = gcnew BarEventFadeInfo(Widget_Timeline::DEFAULT_FADE_TICK_QUANTIZATION, System::Drawing::Color::White, System::Drawing::Color::Black, FadeEasing::Linear, FadeEasing::Linear);
		}

		this->_StrobeInfo = nullptr;

		this->_IgnoreForOverlap = false;

		PreRasterMIDIEvents();
	}

	BarEvent::BarEvent(Track^ track, int start_tick, int duration_in_ticks, BarEventStrobeInfo^ strobe_info)
	{
		this->_Type = BarEventType::Strobe;

		this->_Working.Track = track;
		this->_Working.StartTick = start_tick;
		this->_Working.DurationInTicks = duration_in_ticks;

		this->_Original.Track = track;
		this->_Original.StartTick = start_tick;
		this->_Original.DurationInTicks = duration_in_ticks;

		this->_Color = System::Drawing::Color();
		this->_FadeInfo = nullptr;
		this->_StrobeInfo = strobe_info;

		if (this->_StrobeInfo == nullptr) {
			this->_StrobeInfo = gcnew BarEventStrobeInfo(Widget_Timeline::DEFAULT_FADE_TICK_QUANTIZATION, System::Drawing::Color::White);
		}

		this->_IgnoreForOverlap = false;

		PreRasterMIDIEvents();
	}

	void BarEvent::BasicInfoCopyWorkingToOriginal()
	{
		this->_Original.StartTick		= this->_Working.StartTick;
		this->_Original.DurationInTicks = this->_Working.DurationInTicks;
		this->_Original.Track			= this->_Working.Track;
	}

	void BarEvent::BasicInfoCopyOriginalToWorking()
	{
		this->_Working.StartTick		= this->_Original.StartTick;
		this->_Working.DurationInTicks	= this->_Original.DurationInTicks;
		this->_Working.Track			= this->_Original.Track;

		PreRasterMIDIEvents();
	}

	void BarEvent::PreRasterMIDIEvents()
	{
		if (this->ContainingTrack == nullptr || this->ContainingTrack->Event_Raster == nullptr) {
			this->_Playback_Rastered_Events = gcnew List<Playback_MIDI_Event^>;
			return;
		}

		MIDI_Event_Raster^ MIDI_Event_Raster = this->ContainingTrack->Event_Raster;

		int Octave_Note_Offset = (this->ContainingTrack->Octave + MIDI_Event_Raster::OCTAVE_OFFSET) * MIDI_Event_Raster::NOTES_PER_OCTAVE;
		this->_Playback_Rastered_Events = MIDI_Event_Raster->Raster_Bar_For_Playback(this, this->ContainingTrack->Index, Settings::Get_Instance()->Global_MIDI_Output_Channel, Octave_Note_Offset, Settings::Get_Instance()->MIDI_Export_Anti_Flicker);
	}

	void BarEvent::ContainingTrack::set(Track^ track)
	{
		_Working.Track = track;

		PreRasterMIDIEvents();
	}

	void BarEvent::StartTick::set(int value)
	{
		if(value < 0) {
			value = 0;
		}
		_Working.StartTick = value;

		PreRasterMIDIEvents();
	}

	void BarEvent::Duration::set(int value)
	{
		if(value > 0) {
			_Working.DurationInTicks = value;
		}

		PreRasterMIDIEvents();
	}

	System::Drawing::Color BarEvent::Color::get()
	{
		if (this->Type == BarEventType::Strobe && this->StrobeInfo != nullptr) {
			return this->StrobeInfo->ColorStrobe;
		}

		return _Color;
	}

	void BarEvent::Color::set(System::Drawing::Color color)
	{
		if (this->Type == BarEventType::Strobe && this->StrobeInfo != nullptr) {
			this->StrobeInfo->ColorStrobe = color;
		}
		else {
			this->_Color = color;
		}

		PreRasterMIDIEvents();
	}
		

	void BarEvent::FadeInfo::set(BarEventFadeInfo^ fade_info)
	{
		if (fade_info != nullptr)
		{
			this->_FadeInfo = fade_info;

			PreRasterMIDIEvents();
		}
	}

	void BarEvent::StrobeInfo::set(BarEventStrobeInfo^ strobe_info)
	{
		if (strobe_info != nullptr)
		{
			this->_StrobeInfo = strobe_info;

			PreRasterMIDIEvents();
		}
	}


	////////////////////////////
	// Collapsible Left Panel //
	////////////////////////////
	Collapsible_Left_Panel::Collapsible_Left_Panel()
	{
		this->_Expanded = false;
		this->_Width = Timeline_Direct2DRenderer::PANEL_DEFAULT_WIDTH;

		this->_SelectedEvent = nullptr;
		this->_SelectedEvents = gcnew List<BarEvent^>();
		this->_ShowProperties = false;
	}

	void Collapsible_Left_Panel::ToggleExpanded()
	{
		this->_Expanded = !this->_Expanded;
	}

	void Collapsible_Left_Panel::UpdateSelectedEvents(List<BarEvent^>^ events)
	{
		_SelectedEvents = events;

		if (events->Count > 0) {
			SelectedEvent = events[0];
		}
	}

	bool Collapsible_Left_Panel::IsExpanded::get()
	{
		return this->_Expanded;
	}

	void Collapsible_Left_Panel::IsExpanded::set(bool expanded)
	{
		this->_Expanded = expanded;
	}

	int Collapsible_Left_Panel::Width::get()
	{
		return this->_Width;
	}

	void Collapsible_Left_Panel::Width::set(int width)
	{
		this->_Width = width;

		if (this->_Width < Timeline_Direct2DRenderer::PANEL_MIN_WIDTH) {
			this->_Width = Timeline_Direct2DRenderer::PANEL_MIN_WIDTH;
		}
		else if (this->_Width > Timeline_Direct2DRenderer::PANEL_MAX_WIDTH) {
			this->_Width = Timeline_Direct2DRenderer::PANEL_MAX_WIDTH;
		}
	}

	BarEvent^ Collapsible_Left_Panel::SelectedEvent::get()
	{
		return this->_SelectedEvent;
	}

	void Collapsible_Left_Panel::SelectedEvent::set(BarEvent^ event)
	{
		_SelectedEvent = event;
		
		if (event != nullptr) {
			_ShowProperties = true;
		}
	}

	List<BarEvent^>^ Collapsible_Left_Panel::SelectedEvents::get() 
	{ 
		return _SelectedEvents;
	}

	bool Collapsible_Left_Panel::ShowProperties::get()
	{
		return this->_ShowProperties;
	}

	void Collapsible_Left_Panel::ShowProperties::set(bool value)
	{ 
		_ShowProperties = value; 
	}
}