#include "Widget_Timeline.h"
#include "Widget_Timeline_Tools.h"

#include "Control_ColorPreset.h"
#include "Widget_Tools_And_Control.h"

namespace MIDILightDrawer
{
	// Widget_Timeline Implementation
	Widget_Timeline::Widget_Timeline(Widget_Tools_And_Control^ toolsAndControl)
	{
		InitializeComponent();

		this->SetStyle(ControlStyles::Selectable, true);
		this->TabStop = true;
		this->Focus();
		this->BackColor = _CurrentTheme.TrackBackground;

		// Optimize for Direct2D
		this->SetStyle(
			ControlStyles::AllPaintingInWmPaint |
			ControlStyles::UserPaint |
			ControlStyles::Opaque, true);  // Add Opaque style

		// Explicitly disable styles that can cause flickering
		this->SetStyle(
			ControlStyles::DoubleBuffer |
			ControlStyles::OptimizedDoubleBuffer |
			ControlStyles::SupportsTransparentBackColor, false);

		this->DoubleBuffered = false;
		

		// Initialize Members
		_CurrentTheme	= Theme_Manager::Get_Instance()->GetTimelineTheme();
		_Tracks			= gcnew List<Track^>();
		_Measures		= gcnew List<Measure^>();
		_Left_Panel		= gcnew Collapsible_Left_Panel();
		_ZoomLevel		= 1.0;
		_ScrollPosition = gcnew Point(0, 0);
		_CommandManager = gcnew TimelineCommandManager(this);

		_TrackBeingResized = nullptr;
		_ResizeHoverTrack = nullptr;
		_ScrollPosition = Point();
		_ResizeStartY = 0;
		_InitialTrackHeight = 0;
		_IsOverPanelResizeHandle = false;
		_IsPanelResizing = false;
		_PanelResizeStartX = 0;
		_InitialPanelWidth = 0;
		
		InitializeToolSystem();
		InitializeContextMenu();

		_D2DRenderer = gcnew Timeline_Direct2DRenderer(_Tracks, _Measures, _Left_Panel, _ZoomLevel, _ScrollPosition);
		_D2DRenderer->SetThemeColors(_CurrentTheme.Background, _CurrentTheme.HeaderBackground, _CurrentTheme.Text, _CurrentTheme.MeasureLine, _CurrentTheme.BeatLine, _CurrentTheme.SubdivisionLine, _CurrentTheme.SelectionHighlight, _CurrentTheme.TrackBackground, _CurrentTheme.TrackBorder);
		_D2DRenderer->SetTimelineAccess(this);
		
		_PerformanceMetrics	= gcnew PerformanceMetrics();
		_Tool_And_Control = toolsAndControl;
	}

	Widget_Timeline::~Widget_Timeline()
	{
		delete _PerformanceMetrics;
	}

	void Widget_Timeline::AddTrack(String^ name, int octave)
	{
		Track^ Trk = gcnew Track(name, octave);
		Trk->Height = Widget_Timeline::DEFAULT_TRACK_HEIGHT;
		_Tracks->Add(Trk);

		UpdateVerticalScrollBarRange();
		Invalidate();
	}

	void Widget_Timeline::AddMeasure(int numerator, int denominator, int tempo)
	{
		AddMeasure(numerator, denominator, tempo, "");
	}

	void Widget_Timeline::AddMeasure(int numerator, int denominator, int tempo, String^ marker_text)
	{
		int startTick = TotalTicks;
		Measure^ newMeasure = gcnew Measure(startTick, numerator, denominator, tempo, marker_text);
		_Measures->Add(newMeasure);

		for each (Track ^ track in _Tracks) {
			track->Measures->Add(gcnew TrackMeasure(newMeasure, track));
		}

		UpdateDrawingForMeasures();
		UpdateScrollBarRange();
		Invalidate();
	}

	Beat^ Widget_Timeline::AddBeat(Track^ track, int measureIndex, int startTick, int durationInTicks, bool is_dotted)
	{
		if (track == nullptr || measureIndex < 0 || measureIndex >= track->Measures->Count)
			return nullptr;

		Beat^ beat = gcnew Beat();
		beat->Track = track;
		beat->StartTick = startTick - INITIAL_TICK_OFFSET;
		beat->Duration = durationInTicks;
		beat->IsDotted = is_dotted;

		TrackMeasure^ measure = track->Measures[measureIndex];
		measure->AddBeat(beat);

		Invalidate();
		return beat;
	}

	void Widget_Timeline::AddNote(Beat^ beat, int stringNumber, int value, bool is_tied)
	{
		if (beat == nullptr)
			return;

		Note^ note = gcnew Note();
		note->String	= stringNumber;
		note->Value		= value;	// Fret/Pitch
		note->IsTied	= is_tied;

		beat->Notes->Add(note);

		Invalidate();
	}

	void Widget_Timeline::AddBarToTrack(Track^ track, int startTick, int length, Color color)
	{
		if (_Tracks->Contains(track)) {
			track->AddBar(startTick, length, color);
			Invalidate();
		}
	}

	void Widget_Timeline::Clear()
	{
		for each (Track ^ track in _Tracks) {
			track->Measures->Clear();
		}

		// Clear all collections
		_Tracks->Clear();
		_Measures->Clear();

		// Reset scroll and zoom
		_ScrollPosition = gcnew Point(0, 0);
		_ZoomLevel = 1.0;

		if (_D2DRenderer != nullptr) {
			_D2DRenderer->SetZoomLevel(_ZoomLevel);
			_D2DRenderer->SetScrollPositionReference(_ScrollPosition);
		}

		// Reset background color
		this->BackColor = _CurrentTheme.Background;

		// Force redraw
		Invalidate();
	}

	void Widget_Timeline::Undo()
	{
		if (_CommandManager->CanUndo) {
			_CommandManager->Undo();
		}
	}

	void Widget_Timeline::Redo()
	{
		if (_CommandManager->CanRedo) {
			_CommandManager->Redo();
		}
	}

	void Widget_Timeline::ZoomIn()
	{
		double newZoom;

		if (_ZoomLevel < 1.0) {
			newZoom = _ZoomLevel * 1.2;
		}
		else {
			newZoom = _ZoomLevel * 1.05;
		}

		SetZoom(newZoom);
	}

	void Widget_Timeline::ZoomOut()
	{
		double newZoom;

		if (_ZoomLevel > 1.0) {
			newZoom = _ZoomLevel / 1.05;
		}
		else {
			newZoom = _ZoomLevel / 1.2;
		}

		SetZoom(newZoom);
	}

	void Widget_Timeline::SetZoom(double newZoom)
	{
		// Clamp zoom level to valid range
		newZoom = Math::Min(Math::Max(newZoom, Timeline_Direct2DRenderer::MIN_ZOOM_LEVEL), Timeline_Direct2DRenderer::MAX_ZOOM_LEVEL);

		// If no change in zoom, exit early
		if (Math::Abs(newZoom - _ZoomLevel) < 0.0001) {
			return;
		}

		// Calculate the center point of the visible area in ticks
		int VisibleWidth = Width - GetLeftPanelAndTrackHeaderWidth();
		int CenterTick = PixelsToTicks(-_ScrollPosition->X + (VisibleWidth / 2));

		// Store old zoom level and apply new zoom
		double OldZoom = _ZoomLevel;
		_ZoomLevel = newZoom;

		if (_D2DRenderer != nullptr) {
			_D2DRenderer->SetZoomLevel(_ZoomLevel);
		}

		// Recalculate scroll position to maintain center
		int NewCenterPixel = TicksToPixels(CenterTick);
		_ScrollPosition->X = -(NewCenterPixel - (VisibleWidth / 2));

		// Ensure proper alignment
		UpdateScrollBounds();
		Invalidate();
	}

	void Widget_Timeline::ScrollTo(Point newPosition)
	{
		_ScrollPosition = newPosition;
		Invalidate();
	}

	void Widget_Timeline::SetTrackHeight(Track^ track, int height)
	{
		if (track == nullptr) return;

		// Calculate minimum height based on whether tablature is shown
		int minHeight = track->ShowTablature ? (int)MIN_TRACK_HEIGHT_WITH_TAB : MINIMUM_TRACK_HEIGHT;

		// Ensure height is within acceptable bounds
		height = Math::Max(minHeight, height);

		track->Height = height;

		int totalHeight = Timeline_Direct2DRenderer::HEADER_HEIGHT;
		for each (Track ^ t in _Tracks) {
			totalHeight += t->Height;
		}

		// Update scrollbars and bounds
		UpdateVerticalScrollBarRange();
		UpdateScrollBounds();

		// Ensure current scroll position is still valid
		int viewportHeight = Height - Timeline_Direct2DRenderer::HEADER_HEIGHT - _HScrollBar->Height;
		if (-_ScrollPosition->Y + viewportHeight > totalHeight)
		{
			_ScrollPosition->Y = Math::Min(0, -(totalHeight - viewportHeight));
			_VScrollBar->Value = -_ScrollPosition->Y;
		}

		Invalidate();
	}

	void Widget_Timeline::SetAllTracksHeight(int height)
	{
		for each (Track ^ track in _Tracks)
		{
			SetTrackHeight(track, height);
		}
	}

	void Widget_Timeline::SetToolSnapping(SnappingType type)
	{
		this->_SnappingType = type;
	}

	void Widget_Timeline::ShowContextMenu(BarEvent^ bar, Point location)
	{
		if (bar == nullptr) {
			return;
		}

		this->_ContextMenuBar = bar;
		this->_ContextMenu->Items->Clear();

		this->CreateContextMenuCommon();

		switch (bar->Type)
		{
			case BarEventType::Solid:	CreateContextMenuSolid();	break;
			case BarEventType::Fade:	CreateContextMenuFade();	break;
			case BarEventType::Strobe:	CreateContextMenuStrobe();	break;
		}

		// Show the context menu
		_ContextMenu->Show(this, location);
	}

	void Widget_Timeline::UpdateLeftPanelEventSelection(List<BarEvent^>^ selectedEvents)
	{
		if (_Left_Panel == nullptr) {
			return;
		}

		_Left_Panel->UpdateSelectedEvents(selectedEvents);

		if(_Left_Panel->IsExpanded) {
			Invalidate();
		}
	}

	void Widget_Timeline::SetCurrentTool(TimelineToolType tool)
	{
		if (_CurrentToolType != tool)
		{
			// Deactivate current tool if one exists
			if (_CurrentTool != nullptr) {
				_CurrentTool->Deactivate();
			}

			// Set new tool
			_CurrentToolType = tool;
			_CurrentTool = _Tools[tool];

			// Activate new tool
			_CurrentTool->Activate();
			this->Cursor = _CurrentTool->Cursor;

			ToolChanged(this, tool);

			Invalidate();
		}
	}

	PointerTool^ Widget_Timeline::GetPointerTool()
	{ 
		return (PointerTool^)(_Tools[TimelineToolType::Pointer]);
	}

	DrawTool^ Widget_Timeline::GetDrawTool()
	{ 
		return (DrawTool^)(_Tools[TimelineToolType::Draw]); 
	}

	SplitTool^ Widget_Timeline::GetSplitTool()
	{ 
		return (SplitTool^)(_Tools[TimelineToolType::Split]); 
	}

	EraseTool^ Widget_Timeline::GetEraseTool()
	{
		return (EraseTool^)(_Tools[TimelineToolType::Erase]); 
	}
	DurationTool^ Widget_Timeline::GetDurationTool()
	{
		return (DurationTool^)(_Tools[TimelineToolType::Duration]);
	}

	ColorTool^ Widget_Timeline::GetColorTool()
	{ 
		return (ColorTool^)(_Tools[TimelineToolType::Color]); 
	}
	FadeTool^ Widget_Timeline::GetFadeTool()
	{ 
		return (FadeTool^)(_Tools[TimelineToolType::Fade]);
	}

	StrobeTool^ Widget_Timeline::GetStrobeTool()
	{ 
		return (StrobeTool^)(_Tools[TimelineToolType::Strobe]); 
	
	}

	TimelineCommandManager^ Widget_Timeline::CommandManager()
	{
		return _CommandManager;
	}

	TimelineToolType Widget_Timeline::CurrentToolType()
	{
		return _CurrentToolType;
	}

	ITimelineToolAccess^ Widget_Timeline::ToolAccess()
	{
		return safe_cast<ITimelineToolAccess^>(_Tools[_CurrentToolType]);
	}

	TrackButtonId Widget_Timeline::HoverButton()
	{
		return _HoveredButton;
	}

	int Widget_Timeline::SnapTickToGrid(int tick)
	{
		// Get current subdivision level based on zoom
		float SubdivLevel = GetSubdivisionLevel();

		// Calculate snap resolution based on subdivision level
		int SnapResolution = Timeline_Direct2DRenderer::TICKS_PER_QUARTER / (int)SubdivLevel;

		// Calculate the grid point to the left of the given tick
		return (tick / SnapResolution) * SnapResolution;
	}

	int Widget_Timeline::SnapTickBasedOnType(int tick, Point mousePos)
	{
		const int SNAP_THRESHOLD = 480; // Maximum distance in ticks for bar snapping

		switch (_SnappingType)
		{
		case SnappingType::Snap_None:
			return tick;

		case SnappingType::Snap_Grid:
			return SnapTickToGrid(tick);

		case SnappingType::Snap_Events:
		{
			// Get the track under the mouse pointer
			Track^ CurrentTrack = GetTrackAtPoint(mousePos);
			if (CurrentTrack == nullptr) {
				return tick;
			}

			int ClosestEndTick = tick;
			int MinEndDistance = SNAP_THRESHOLD;

			// Only check bars in the current track
			for each (BarEvent^ Bar in CurrentTrack->Events)
			{
				// Only check distance to bar end
				int EndTick = Bar->StartTick + Bar->Duration;
				int EndDistance = Math::Abs(EndTick - tick);
				if (EndDistance < MinEndDistance)
				{
					MinEndDistance = EndDistance;
					ClosestEndTick = EndTick;
				}
			}

			// Return the closest edge tick if within threshold
			if (MinEndDistance < SNAP_THRESHOLD)
			{
				return ClosestEndTick;
			}

			// If no close bars found, return original tick
			return tick;
		}

		case SnappingType::Snap_Tablature:
		{
			// Get the track under the mouse pointer
			Track^ CurrentTrack = GetTrackAtPoint(mousePos);
			if (CurrentTrack == nullptr) {
				return tick;
			}

			// Only proceed if the track has tablature visible
			if (!CurrentTrack->ShowTablature) {
				return tick;
			}

			// Find the nearest left-side beat in the current track
			int NearestLeftBeatTick = -1;  // Initialize to invalid value
			int SmallestDistance = Int32::MaxValue;

			// Get the measure containing this tick
			Measure^ Meas = GetMeasureAtTick(tick);
			if (Meas == nullptr) {
				return tick;
			}

			// Find corresponding track measure
			TrackMeasure^ TrackMeas = nullptr;
			for each (TrackMeasure^ TM in CurrentTrack->Measures)
			{
				if (TM->StartTick == Meas->StartTick)
				{
					TrackMeas = TM;
					break;
				}
			}

			if (TrackMeas == nullptr) {
				return tick;
			}

			// Look for the closest beat that's to the left of our current position
			for each (Beat^ B in TrackMeas->Beats)
			{
				if (B->StartTick <= tick)  // Only consider beats to the left
				{
					int Distance = tick - B->StartTick;
					if (Distance < SmallestDistance)
					{
						SmallestDistance = Distance;
						NearestLeftBeatTick = B->StartTick;
					}
				}
			}

			// If we found a beat to the left, use it, otherwise fallback to grid
			if (NearestLeftBeatTick != -1)
			{
				return NearestLeftBeatTick;
			}

			return tick;
		}

		default:
			return tick;
		}
	}

	void Widget_Timeline::ScrollToMeasure(int measureNumber)
	{
		// Validate measure number
		if (measureNumber < 1 || measureNumber > _Measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + _Measures->Count);
		}

		// Get the start tick of the requested measure
		int TargetTick = GetMeasureStartTick(measureNumber);

		// Convert to pixels
		int TargetPixel = TicksToPixels(TargetTick);

		// Calculate scroll position to center the measure
		int ViewportWidth = Width - GetLeftPanelAndTrackHeaderWidth();
		int NewScrollX = -TargetPixel + (ViewportWidth / 2);

		// Clamp scroll position to valid range
		double VirtualWidth = GetVirtualWidth();
		double MaxScroll = -(VirtualWidth - ViewportWidth);
		NewScrollX = (int)Math::Round(Math::Max(MaxScroll, Math::Min(0.0, (double)NewScrollX)));

		// Update scroll position
		_ScrollPosition->X = NewScrollX;

		// Update scrollbar
		UpdateScrollBarRange();

		// Request redraw
		Invalidate();
	}

	int Widget_Timeline::GetMeasureStartTick(int measureNumber)
	{
		if (measureNumber < 1 || measureNumber > _Measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + _Measures->Count);
		}

		int StartTick = 0;
		// Sum up lengths of previous measures
		for (int i = 0; i < measureNumber - 1; i++) {
			StartTick += _Measures[i]->Length;
		}
		return StartTick;
	}

	int Widget_Timeline::GetMeasureLength(int measureNumber) {
		if (measureNumber < 1 || measureNumber > _Measures->Count) {
			throw gcnew ArgumentOutOfRangeException("measureNumber", "Measure number must be between 1 and " + _Measures->Count);
		}

		return _Measures[measureNumber - 1]->Length;
	}

	int Widget_Timeline::GetLeftPanelAndTrackHeaderWidth()
	{
		if (_D2DRenderer != nullptr) {
			return _D2DRenderer->GetLeftPanelAndTrackHeaderWidth();
		}

		return 0;
	}

	void Widget_Timeline::UpdateCursor(System::Windows::Forms::Cursor^ cursor)
	{
		if (this->Cursor != cursor) {
			this->Cursor = cursor;
		}
	}

	bool Widget_Timeline::DoesBarExist(BarEvent^ bar)
	{
		if (bar == nullptr) {
			return false;
		}

		for each (Track^ T in Tracks)
		{
			if (T->Events->Contains(bar)) {
				return true;
			}
		}
		return false;
	}

	int Widget_Timeline::TicksToPixels(int ticks)
	{
		if (this->_D2DRenderer == nullptr) {
			return 0;
		}

		float Pixels = this->_D2DRenderer->TicksToPixels(ticks);
		
		// Protect against overflow
		if (Pixels > Int32::MaxValue) return Int32::MaxValue;
		if (Pixels < Int32::MinValue) return Int32::MinValue;

		return (int)Math::Round(Pixels);
	}

	int Widget_Timeline::PixelsToTicks(int pixels)
	{
		if (this->_D2DRenderer == nullptr) {
			return 0;
		}

		return (int)Math::Round(this->_D2DRenderer->PixelsToTicks(pixels));
	}

	Track^ Widget_Timeline::GetTrackAtPoint(Point p)
	{
		if (_D2DRenderer == nullptr) {
			return nullptr;
		}

		return _D2DRenderer->GetTrackAtPoint(p);
	}

	Rectangle Widget_Timeline::GetTrackBounds(Track^ track)
	{
		if (_D2DRenderer == nullptr) {
			return Rectangle();
		}

		return _D2DRenderer->GetTrackBounds(track);
	}

	Rectangle Widget_Timeline::GetTrackHeaderBounds(Track^ track)
	{
		if (_D2DRenderer == nullptr) {
			return Rectangle();
		}

		return _D2DRenderer->GetTrackHeaderBounds(track);
	}

	Rectangle Widget_Timeline::GetTrackContentBounds(Track^ track)
	{
		if (_D2DRenderer == nullptr) {
			return Rectangle();
		}
		
		return _D2DRenderer->GetTrackContentBounds(track);
	}

	Measure^ Widget_Timeline::GetMeasureAtTick(int tick)
	{
		if (_D2DRenderer == nullptr) {
			return nullptr;
		}
		
		return _D2DRenderer->GetMeasureAtTick(tick);
	}

	BarEvent^ Widget_Timeline::GetBarAtPoint(Point p)
	{
		if (_D2DRenderer == nullptr) {
			return nullptr;
		}

		return _D2DRenderer->GetBarAtPoint(p);
	}

	String^ Widget_Timeline::SaveBarEventsToFile(String^ filePath)
	{
		try
		{
			List<String^>^ lines = gcnew List<String^>();

			// Header with version - updated to v2.0 to indicate support for fade/strobe
			lines->Add("MIDILightDrawer_BarEvents_v2.0");

			// Save pattern information - number of measures
			lines->Add(_Measures->Count.ToString());

			// Save time signatures for all measures
			for each (Measure ^ measure in _Measures) {
				lines->Add(String::Format("{0},{1}",
					measure->Numerator,
					measure->Denominator));
			}

			// Calculate total number of bars across all tracks
			int totalBars = 0;
			for each (Track ^ track in _Tracks) {
				totalBars += track->Events->Count;
			}
			lines->Add(totalBars.ToString());

			// Save each bar's data with track name and type-specific information
			for each (Track ^ track in _Tracks) {
				for each (BarEvent ^ bar in track->Events) {
					String^ baseData = String::Format("{0},{1},{2},{3}",
						bar->StartTick,
						bar->Duration,
						_Tracks->IndexOf(track),
						static_cast<int>(bar->Type)  // Save the bar event type
					);

					// Add type-specific data
					switch (bar->Type) {
					case BarEventType::Solid:
						lines->Add(String::Format("{0},{1},{2},{3},{4},{5}",
							baseData,
							bar->Color.R,
							bar->Color.G,
							bar->Color.B,
							track->Name,
							"SOLID"  // Type identifier for verification
						));
						break;

					case BarEventType::Fade:
						if (bar->FadeInfo != nullptr) {
							if (bar->FadeInfo->Type == FadeType::Two_Colors) {
								lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11}",
									baseData,
									bar->FadeInfo->QuantizationTicks,
									bar->FadeInfo->ColorStart.R,
									bar->FadeInfo->ColorStart.G,
									bar->FadeInfo->ColorStart.B,
									bar->FadeInfo->ColorEnd.R,
									bar->FadeInfo->ColorEnd.G,
									bar->FadeInfo->ColorEnd.B,
									track->Name,
									static_cast<int>(bar->FadeInfo->EaseIn),  // Add easing parameters
									static_cast<int>(bar->FadeInfo->EaseOut),
									"FADE2"  // Two-color fade identifier
								));
							}
							else {  // Three-color fade
								lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14}",
									baseData,
									bar->FadeInfo->QuantizationTicks,
									bar->FadeInfo->ColorStart.R,
									bar->FadeInfo->ColorStart.G,
									bar->FadeInfo->ColorStart.B,
									bar->FadeInfo->ColorCenter.R,
									bar->FadeInfo->ColorCenter.G,
									bar->FadeInfo->ColorCenter.B,
									bar->FadeInfo->ColorEnd.R,
									bar->FadeInfo->ColorEnd.G,
									bar->FadeInfo->ColorEnd.B,
									track->Name,
									static_cast<int>(bar->FadeInfo->EaseIn),  // Add easing parameters
									static_cast<int>(bar->FadeInfo->EaseOut),
									"FADE3"  // Three-color fade identifier
								));
							}
						}
						break;

					case BarEventType::Strobe:
						if (bar->StrobeInfo != nullptr) {
							lines->Add(String::Format("{0},{1},{2},{3},{4},{5},{6}",
								baseData,
								bar->StrobeInfo->QuantizationTicks,
								bar->StrobeInfo->ColorStrobe.R,
								bar->StrobeInfo->ColorStrobe.G,
								bar->StrobeInfo->ColorStrobe.B,
								track->Name,
								"STROBE"  // Strobe identifier
							));
						}
						break;
					}
				}
			}

			System::IO::File::WriteAllLines(filePath, lines->ToArray());
			return String::Empty;
		}
		catch (Exception^ ex)
		{
			return String::Format("Error saving bar events: {0}", ex->Message);
		}
	}

	String^ Widget_Timeline::LoadBarEventsFromFile(String^ filePath)
	{
		try
		{
			array<String^>^ lines = System::IO::File::ReadAllLines(filePath);
			if (lines->Length < 2) return "Invalid file format";

			// Check version
			bool IsLegacyFormat = lines[0]->StartsWith("MIDILightDrawer_BarEvents_v1.0");
			bool IsCurrentFormat = lines[0]->StartsWith("MIDILightDrawer_BarEvents_v2.0");

			if (!IsLegacyFormat && !IsCurrentFormat) {
				return "Invalid or unsupported file format version";
			}

			// Parse number of measures
			int FileMeasureCount;
			if (!Int32::TryParse(lines[1], FileMeasureCount)) {
				return "Invalid measure count";
			}

			// Verify measure count matches
			if (FileMeasureCount != _Measures->Count)
				return String::Format("Pattern mismatch: File has {0} measures, but timeline has {1} measures", FileMeasureCount, _Measures->Count);

			// Verify time signatures for each measure
			for (int i = 0; i < _Measures->Count; i++)
			{
				String^ FileMeasureInfo = lines[2 + i];
				array<String^>^ Parts = FileMeasureInfo->Split(',');

				if (Parts->Length != 2) {
					return String::Format("Invalid time signature format in measure {0}", i + 1);
				}

				int FileNumerator, FileDenominator;
				if (!Int32::TryParse(Parts[0], FileNumerator) || !Int32::TryParse(Parts[1], FileDenominator)) {
					return String::Format("Invalid time signature numbers in measure {0}", i + 1);
				}

				if (FileNumerator != _Measures[i]->Numerator || FileDenominator != _Measures[i]->Denominator) {
					return String::Format("Time signature mismatch at measure {0}: File has {1}/{2}, but timeline has {3}/{4}", i + 1, FileNumerator, FileDenominator, _Measures[i]->Numerator, _Measures[i]->Denominator);
				}
			}

			// Create mapping of track names to indices
			Dictionary<String^, Track^>^ TrackMap = gcnew Dictionary<String^, Track^>();
			for each (Track^ Trk in _Tracks) {
				TrackMap[Trk->Name] = Trk;
			}

			// Clear existing bars from all tracks
			for each (Track^ Trk in _Tracks) {
				Trk->Events->Clear();
			}

			int BarsStartLine = 2 + _Measures->Count;
			int BarCount;

			if (!Int32::TryParse(lines[BarsStartLine], BarCount)) {
				return "Invalid bar count";
			}

			// Load each bar
			for (int i = 0; i < BarCount; i++)
			{
				String^ BarData = lines[BarsStartLine + 1 + i];
				array<String^>^ Parts = BarData->Split(',');

				if (IsLegacyFormat) {
					// Handle v1.0 format (solid bars only)
					if (Parts->Length != 7) {
						continue;
					}

					String^ TrackName = Parts[6];
					if (!TrackMap->ContainsKey(TrackName)) {
						continue;
					}

					Track^ TargetTrack = TrackMap[TrackName];
					int StartTick, Length, R, G, B;

					if (!Int32::TryParse(Parts[0], StartTick) || !Int32::TryParse(Parts[1], Length) || !Int32::TryParse(Parts[3], R) || !Int32::TryParse(Parts[4], G) || !Int32::TryParse(Parts[5], B)) {
						continue;
					}

					TargetTrack->AddBar(StartTick, Length, Color::FromArgb(R, G, B));
				}
				else {
					// Handle v2.0 format with multiple bar types
					if (Parts->Length < 5) {
						continue;
					}

					int StartTick, Length, TrackIndex, EventType;
					if (!Int32::TryParse(Parts[0], StartTick) || !Int32::TryParse(Parts[1], Length) || !Int32::TryParse(Parts[2], TrackIndex) || !Int32::TryParse(Parts[3], EventType)) {
						continue;
					}

					String^ Identifier = Parts[Parts->Length - 1];
					String^ TrackName = Parts[Parts->Length - 2];

					if (!TrackMap->ContainsKey(TrackName)) {
						continue;
					}
					Track^ TargetTrack = TrackMap[TrackName];

					switch (static_cast<BarEventType>(EventType))
					{
						case BarEventType::Solid:
							if (Identifier == "SOLID" && Parts->Length >= 8) {
								int r, g, b;
								if (!Int32::TryParse(Parts[4], r) || !Int32::TryParse(Parts[5], g) || !Int32::TryParse(Parts[6], b)) {
									continue;
								}

								TargetTrack->AddBar(StartTick, Length, Color::FromArgb(r, g, b));
							}
							break;

						case BarEventType::Fade:
							if (Identifier == "FADE2" && Parts->Length >= 11) {

								bool HasEasing = Parts->Length >= 11 && IsCurrentFormat;
								if (Parts->Length >= (HasEasing ? 12 : 10))
								{
									int quantization, r1, g1, b1, r2, g2, b2;
									int easeIn = static_cast<int>(FadeEasing::Linear);
									int easeOut = static_cast<int>(FadeEasing::Linear);

									if (!Int32::TryParse(Parts[4], quantization) ||
										!Int32::TryParse(Parts[5], r1) || !Int32::TryParse(Parts[6], g1) || !Int32::TryParse(Parts[7], b1) ||
										!Int32::TryParse(Parts[8], r2) || !Int32::TryParse(Parts[9], g2) || !Int32::TryParse(Parts[10], b2)) {
										continue;
									}

									if (HasEasing) {
										Int32::TryParse(Parts[9], easeIn);
										Int32::TryParse(Parts[10], easeOut);
									}

									BarEventFadeInfo^ fadeInfo = gcnew BarEventFadeInfo(
										quantization,
										Color::FromArgb(r1, g1, b1),
										Color::FromArgb(r2, g2, b2),
										static_cast<FadeEasing>(easeIn),
										static_cast<FadeEasing>(easeOut)
									);
									TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, fadeInfo));
								}
							}
							else if (Identifier == "FADE3" && Parts->Length >= 14)
							{
								bool HasEasing = Parts->Length >= 14 && IsCurrentFormat;
								if (Parts->Length >= (HasEasing ? 15 : 13)) {
									int quantization, r1, g1, b1, r2, g2, b2, r3, g3, b3;
									int easeIn = static_cast<int>(FadeEasing::Linear);
									int easeOut = static_cast<int>(FadeEasing::Linear);

									if (!Int32::TryParse(Parts[4], quantization) ||
										!Int32::TryParse(Parts[5], r1) || !Int32::TryParse(Parts[6], g1) || !Int32::TryParse(Parts[7], b1) ||
										!Int32::TryParse(Parts[8], r2) || !Int32::TryParse(Parts[9], g2) || !Int32::TryParse(Parts[10], b2) ||
										!Int32::TryParse(Parts[11], r3) || !Int32::TryParse(Parts[12], g3) || !Int32::TryParse(Parts[13], b3)) {
										continue;
									}

									if (HasEasing) {
										Int32::TryParse(Parts[12], easeIn);
										Int32::TryParse(Parts[13], easeOut);
									}

									BarEventFadeInfo^ fadeInfo = gcnew BarEventFadeInfo(
										quantization,
										Color::FromArgb(r1, g1, b1),
										Color::FromArgb(r2, g2, b2),
										Color::FromArgb(r3, g3, b3),
										static_cast<FadeEasing>(easeIn),
										static_cast<FadeEasing>(easeOut)
									);
									TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, fadeInfo));
								}
							}
							break;

						case BarEventType::Strobe:
							if (Identifier == "STROBE" && Parts->Length >= 8) {
								int quantization, r, g, b;
								if (!Int32::TryParse(Parts[4], quantization) || Int32::TryParse(Parts[5], r) || Int32::TryParse(Parts[6], g) || Int32::TryParse(Parts[7], b)) {
									continue;
								}

								BarEventStrobeInfo^ strobeInfo = gcnew BarEventStrobeInfo(
									quantization,
									Color::FromArgb(r, g, b)
								);
								TargetTrack->AddBar(gcnew BarEvent(TargetTrack, StartTick, Length, strobeInfo));
							}
							break;
					}
				}
			}

			// Sort events in each track
			for each (Track^ Trk in _Tracks) {
				Trk->Events->Sort(Track::barComparer);
			}

			this->Invalidate();
			return String::Empty;
		}
		catch (Exception^ ex)
		{
			return String::Format("Error loading bar events: {0}", ex->Message);
		}
	}

	void Widget_Timeline::LogPerformanceMetrics()
	{
		String^ report = _PerformanceMetrics->GetReport();
		Console::WriteLine(report);
		_PerformanceMetrics->Reset();
	}


	///////////////////////
	// Protected Methods //
	///////////////////////
	void Widget_Timeline::OnPaint(PaintEventArgs^ e)
	{
		_PerformanceMetrics->StartFrame();

		if (_D2DRenderer->BeginDraw())
		{
			_D2DRenderer->UpdateLevelOfDetail();

			// Clear the background
			if(this->Measures->Count == 0) {
				_D2DRenderer->DrawWidgetBackground();
			}
				
			_D2DRenderer->DrawTrackBackground();
			_D2DRenderer->DrawTrackContent(_ResizeHoverTrack);
			_D2DRenderer->DrawToolPreview();
			_D2DRenderer->DrawTrackHeaders();
			_D2DRenderer->DrawTrackDividers(_ResizeHoverTrack);
			_D2DRenderer->DrawMeasureNumbers();
			_D2DRenderer->DrawLeftPanel(_IsOverPanelResizeHandle || _IsPanelResizing);

			const float FPSCounter_X_Offset = (float)(Timeline_Direct2DRenderer::PANEL_BUTTON_SIZE + Timeline_Direct2DRenderer::PANEL_BUTTON_MARGIN);
			_D2DRenderer->DrawFPSCounter(FPSCounter_X_Offset, _PerformanceMetrics->GetFPS(), _PerformanceMetrics->LastFrameTime);

			_D2DRenderer->EndDraw();
		}
		
		_PerformanceMetrics->EndFrame();	
	}

	void Widget_Timeline::OnResize(EventArgs^ e)
	{
		UserControl::OnResize(e);

		if (_D2DRenderer != nullptr) {
			_D2DRenderer->Resize(Width, Height);
		}

		UpdateScrollBarRange();
		UpdateVerticalScrollBarRange();
		Invalidate();
	}

	void Widget_Timeline::OnMouseClick(MouseEventArgs^ e)
	{
		if (e->Button == Windows::Forms::MouseButtons::Right) {
			if (_CurrentTool != nullptr) {
				_CurrentTool->OnMouseRightClick(e);
			}
		}

		Control::OnMouseClick(e);
	}

	void Widget_Timeline::OnMouseDown(MouseEventArgs^ e)
	{
		Point MousePos = Point(e->X, e->Y);
		
		if(IsOverPanelResizeHandle(MousePos))
		{
			BeginPanelResize(e->X);
			return;
		}

		if (IsOverPanelToggleButton(MousePos)) {
			ToggleLeftPanel();
			return;
		}

		if (_HoveredButton.Track != nullptr)
		{
			switch (_HoveredButton.ButtonIndex)
			{
				case 0: // Tablature toggle button
					_HoveredButton.Track->ShowTablature = !_HoveredButton.Track->ShowTablature;
					break;
				case 1: // Notation toggle button (only for drum tracks)
					if (_HoveredButton.Track->IsDrumTrack) {
						_HoveredButton.Track->ShowAsStandardNotation = !_HoveredButton.Track->ShowAsStandardNotation;
					}
					break;
			}
			Invalidate();
			return;
		}
		
		Track^ ResizeTrack = nullptr;
		if (IsOverTrackDivider(MousePos, ResizeTrack))
		{
			BeginTrackResize(ResizeTrack, e->Y);
			return;
		}

		// Normal mouse down handling
		this->Focus();
		Control::OnMouseDown(e);

		if (this->_CurrentTool != nullptr && this->_Measures->Count > 0)
		{
			_CurrentTool->OnMouseDown(e);
		}
	}

	void Widget_Timeline::OnMouseMove(MouseEventArgs^ e)
	{
		Point MousePos = Point(e->X, e->Y);
		
		if(_IsOverPanelResizeHandle != IsOverPanelResizeHandle(MousePos)) {
			_IsOverPanelResizeHandle = IsOverPanelResizeHandle(MousePos);
			Invalidate();
		}

		if (_IsPanelResizing)
		{
			UpdatePanelResize(e->X);
			return;
		}

		// Check for hovering over panel resize handle
		if (_IsOverPanelResizeHandle)
		{
			this->Cursor = Cursors::SizeWE;
			return;
		}

		if (IsOverPanelToggleButton(MousePos))
		{
			this->Cursor = Cursors::Hand;
			return;
		}
		
		Track^ HoverTrack = nullptr;
		bool IsOverDivider = IsOverTrackDivider(MousePos, HoverTrack);

		Track^ CurrentTrack = GetTrackAtPoint(MousePos);
		TrackButtonId NewHoveredButton;
		bool IsOverAnyButton = false;

		if (CurrentTrack != nullptr)
		{
			for (int i = 0; i < 2; i++)	
			{ 
				if (IsOverTrackButton(CurrentTrack, i, MousePos))
				{
					NewHoveredButton.Track = CurrentTrack;
					NewHoveredButton.ButtonIndex = i;
					IsOverAnyButton = true;
					break;
				}
			}
		}

		// Update hover states
		if (NewHoveredButton.Track != _HoveredButton.Track || NewHoveredButton.ButtonIndex != _HoveredButton.ButtonIndex) {
			_HoveredButton = NewHoveredButton;
			Invalidate();
		}

		if (IsOverAnyButton) {
			this->Cursor = Cursors::Hand;
			return;
		}

		if (_TrackBeingResized != nullptr)
		{
			// If we're actively resizing, update the track height
			UpdateTrackResize(e->Y);
			return;
		}

		if (IsOverDivider && !_CurrentTool->IsResizing)
		{
			// Just hovering over a divider
			if (_ResizeHoverTrack != HoverTrack)
			{
				_ResizeHoverTrack = HoverTrack;
				Invalidate(); // Redraw to show hover state if needed
			}

			this->Cursor = Cursors::SizeNS;
			return;
		}

		// Not over a divider
		if (_ResizeHoverTrack != nullptr)
		{
			_ResizeHoverTrack = nullptr;
			Invalidate();
		}

		// Handle normal tool behavior
		Control::OnMouseMove(e);

		if (this->_CurrentTool != nullptr && this->_Measures->Count > 0)
		{
			_CurrentTool->OnMouseMove(e);
			// Only update cursor if we're not in a special state
			if (!IsOverAnyButton && !IsOverDivider && _TrackBeingResized == nullptr)
			{
				this->Cursor = _CurrentTool->Cursor;
				return;
			}
		}

		this->Cursor = Cursors::Default;
	}

	void Widget_Timeline::OnMouseUp(MouseEventArgs^ e)
	{
		Control::OnMouseUp(e);

		if (_IsPanelResizing)
		{
			EndPanelResize();
			return;
		}

		if (_TrackBeingResized != nullptr) {
			EndTrackResize();
			return;
		}

		if (this->_CurrentTool != nullptr && this->_Measures->Count > 0)
		{
			_CurrentTool->OnMouseUp(e);
		}
	}

	void Widget_Timeline::OnMouseWheel(MouseEventArgs^ e)
	{
		if (Control::ModifierKeys == Keys::Control)
		{
			double newZoom;
			if (e->Delta > 0) {
				if (_ZoomLevel < 1.0) {
					newZoom = _ZoomLevel * 1.2;
				}
				else {
					newZoom = _ZoomLevel * 1.05;
				}
			}
			else {
				if (_ZoomLevel > 1.0) {
					newZoom = _ZoomLevel / 1.05;
				}
				else {
					newZoom = _ZoomLevel / 1.2;
				}
			}
			SetZoomLevelAtPoint(newZoom, Point(e->X, e->Y));
		}
		else if (Control::ModifierKeys == Keys::Shift) {
			int scrollUnits = e->Delta > 0 ? -1 : 1;
			int newValue = Math::Min(Math::Max(_HScrollBar->Value + scrollUnits,
				_HScrollBar->Minimum),
				_HScrollBar->Maximum - _HScrollBar->LargeChange + 1);
			_HScrollBar->Value = newValue;
			OnScroll(_HScrollBar, gcnew ScrollEventArgs(ScrollEventType::ThumbPosition, newValue));
		}
		else
		{
			int scrollUnits = e->Delta > 0 ? -_VScrollBar->SmallChange : _VScrollBar->SmallChange;
			int newValue = Math::Min(Math::Max(
				_VScrollBar->Value + scrollUnits,
				_VScrollBar->Minimum),
				_VScrollBar->Maximum - _VScrollBar->LargeChange + 1);
			_VScrollBar->Value = newValue;
			OnVerticalScroll(_VScrollBar, gcnew ScrollEventArgs(ScrollEventType::ThumbPosition, newValue));
		}
	}

	void Widget_Timeline::OnKeyDown(KeyEventArgs^ e)
	{
		Control::OnKeyDown(e);
		if (_CurrentTool != nullptr) {
			_CurrentTool->OnKeyDown(e);
		}
	}

	void Widget_Timeline::OnKeyUp(KeyEventArgs^ e)
	{
		Control::OnKeyUp(e);
		if (_CurrentTool != nullptr) {
			_CurrentTool->OnKeyUp(e);
		}
	}

	void Widget_Timeline::OnHandleCreated(EventArgs^ e)
	{
		UserControl::OnHandleCreated(e);

		if (_D2DRenderer != nullptr)
		{
			FormLoading^ Loading = gcnew FormLoading();
			LoadingStatusCallback^ StatusCallback = gcnew LoadingStatusCallback(Loading, &FormLoading::OnLoadingStageChanged);
			LoadingProgressCallback^ ProgressCallback = gcnew LoadingProgressCallback(Loading, &FormLoading::UpdateProgress);

			Loading->Show();

			if (_D2DRenderer->Initialize(this, StatusCallback, ProgressCallback))
			{
				_D2DRenderer->Resize(Width, Height);
			}
		}
	}

	bool Widget_Timeline::ProcessDialogKey(Keys keyData)
	{
		OnKeyDown(gcnew KeyEventArgs(keyData));

		return true;
	}

	bool Widget_Timeline::IsInputKey(Keys keyData)
	{
		return true;  // Tell Windows Forms that all keys are input keys
	}


	/////////////////////
	// Private Methods //
	/////////////////////
	void Widget_Timeline::InitializeComponent()
	{
		// Initialize horizontal scrollbar
		_HScrollBar = gcnew System::Windows::Forms::HScrollBar();
		_HScrollBar->Dock = System::Windows::Forms::DockStyle::Bottom;
		_HScrollBar->Height = System::Windows::Forms::SystemInformation::HorizontalScrollBarHeight;
		_HScrollBar->Scroll += gcnew ScrollEventHandler(this, &Widget_Timeline::OnScroll);

		// Initialize vertical scrollbar
		_VScrollBar = gcnew System::Windows::Forms::VScrollBar();
		_VScrollBar->Dock = System::Windows::Forms::DockStyle::Right;
		_VScrollBar->Width = System::Windows::Forms::SystemInformation::VerticalScrollBarWidth;
		_VScrollBar->Scroll += gcnew ScrollEventHandler(this, &Widget_Timeline::OnVerticalScroll);

		// Add scrollbars to control
		this->Controls->Add(_VScrollBar);
		this->Controls->Add(_HScrollBar);
	}

	void Widget_Timeline::InitializeToolSystem()
	{
		_Tools = gcnew Dictionary<TimelineToolType, TimelineTool^>();

		// Create tools
		_Tools->Add(TimelineToolType::Pointer	, gcnew PointerTool	(this));
		_Tools->Add(TimelineToolType::Draw		, gcnew DrawTool	(this));
		_Tools->Add(TimelineToolType::Split		, gcnew SplitTool	(this));
		_Tools->Add(TimelineToolType::Erase		, gcnew EraseTool	(this));
		_Tools->Add(TimelineToolType::Duration	, gcnew DurationTool(this));
		_Tools->Add(TimelineToolType::Color		, gcnew ColorTool	(this));
		_Tools->Add(TimelineToolType::Fade		, gcnew FadeTool	(this));
		_Tools->Add(TimelineToolType::Strobe	, gcnew StrobeTool	(this));

		// Set default tool
		_CurrentToolType = TimelineToolType::Pointer;
		_CurrentTool = _Tools[_CurrentToolType];
	}

	void Widget_Timeline::InitializeContextMenu()
	{
		_Resources = gcnew System::Resources::ResourceManager("MIDILightDrawer.Icons", System::Reflection::Assembly::GetExecutingAssembly());
		
		_ContextMenu = gcnew System::Windows::Forms::ContextMenuStrip();
		//_ContextMenu->ItemClicked += gcnew ToolStripItemClickedEventHandler(this, &Widget_Timeline::HandleContextMenuClick);
	
		Theme_Manager::Get_Instance()->ApplyThemeToContextMenu(_ContextMenu);
	}

	void Widget_Timeline::CreateContextMenuCommon()
	{
		_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::Copy, true, nullptr));
		_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::Delete, true, nullptr));
		_ContextMenu->Items->Add(ContextMenuStrings::Separator);
	}

	void Widget_Timeline::CreateContextMenuSolid()
	{
		CreateContextMenuSubChangeColor(ContextMenuStrings::ChangeColor);
		_ContextMenu->Items->Add(ContextMenuStrings::Separator);
		_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::GetColor, true, nullptr));
	}

	void Widget_Timeline::CreateContextMenuFade()
	{
		if (_ContextMenuBar->FadeInfo->Type == FadeType::Two_Colors)
		{
			_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::FadeSwitchThree, true, nullptr));
		}
		else if (_ContextMenuBar->FadeInfo->Type == FadeType::Three_Colors)
		{
			_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::FadeSwitchTwo, true, nullptr));
		}
		
		_ContextMenu->Items->Add(ContextMenuStrings::Separator);
		CreateContextMenuSubEasings(ContextMenuStrings::ChangeEasingIn, _ContextMenuBar->FadeInfo->EaseIn);
		CreateContextMenuSubEasings(ContextMenuStrings::ChangeEasingOut, _ContextMenuBar->FadeInfo->EaseOut);

		CreateContextMenuSubChangeQuantization(_ContextMenuBar->FadeInfo->QuantizationTicks);
		_ContextMenu->Items->Add(ContextMenuStrings::Separator);

		CreateContextMenuSubChangeColor(ContextMenuStrings::ChangeColorStart);
		if (_ContextMenuBar->FadeInfo->Type == FadeType::Three_Colors)
		{
			CreateContextMenuSubChangeColor(ContextMenuStrings::ChangeColorCenter);
		}
		CreateContextMenuSubChangeColor(ContextMenuStrings::ChangeColorEnd);

		_ContextMenu->Items->Add(ContextMenuStrings::Separator);

		_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::GetColorStart, true, nullptr));
		if (_ContextMenuBar->FadeInfo->Type == FadeType::Three_Colors)
		{
			_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::GetColorCenter, true, nullptr));
		}
		_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::GetColorEnd, true, nullptr));
	}

	void Widget_Timeline::CreateContextMenuStrobe()
	{
		CreateContextMenuSubChangeQuantization(_ContextMenuBar->StrobeInfo->QuantizationTicks);
		CreateContextMenuSubChangeColor(ContextMenuStrings::ChangeColor);
		_ContextMenu->Items->Add(ContextMenuStrings::Separator);
		_ContextMenu->Items->Add(CreateContextMenuItem(ContextMenuStrings::GetColor, true, nullptr));
	}

	void Widget_Timeline::CreateContextMenuSubChangeColor(String^ menuTitle)
	{
		ToolStripMenuItem^ SubMenuChangeColor = CreateContextMenuItem(menuTitle, false, nullptr);

		Settings^ Settings = Settings::Get_Instance();
		List<Color>^ PresetColors = Settings->ColorPresetsColor;

		SubMenuChangeColor->DropDownItems->Add(CreateContextMenuItem(ContextMenuStrings::CurrentColor, true, Control_ColorPreset::CreateColorBitmap(this->_Tool_And_Control->GetColorPickerSelectedColor(), 20)));

		for (int i = 0;i<PresetColors->Count;i++)
		{
			SubMenuChangeColor->DropDownItems->Add(CreateContextMenuItem(ContextMenuStrings::PresetColor + " " + (i + 1).ToString(), true, Control_ColorPreset::CreateColorBitmap(PresetColors[i], 20)));
		}

		this->_ContextMenu->Items->Add(SubMenuChangeColor);
	}

	void Widget_Timeline::CreateContextMenuSubEasings(String^ text, FadeEasing currentEasing)
	{
		ToolStripMenuItem^ SubMenuChangeEasing = CreateContextMenuItem(text, false, nullptr);

		for each (String^ StringKey in ContextMenuStrings::FadeEasings->Keys)
		{
			ToolStripMenuItem^ ItemEasing = CreateContextMenuItem(StringKey, true, nullptr);

			if (ContextMenuStrings::FadeEasings[StringKey] == currentEasing) {
				ItemEasing->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Tick_White")));
			}

			SubMenuChangeEasing->DropDownItems->Add(ItemEasing);
		}

		this->_ContextMenu->Items->Add(SubMenuChangeEasing);
	}

	void Widget_Timeline::CreateContextMenuSubChangeQuantization(int currentQuantization)
	{
		ToolStripMenuItem^ SubMenuChangeQuantization = CreateContextMenuItem(ContextMenuStrings::ChangeQuantization, false, nullptr);

		for each (String^ StringKey in ContextMenuStrings::QuantizationValues->Keys)
		{
			ToolStripMenuItem^ ItemQuantization = CreateContextMenuItem(StringKey, true, nullptr);;

			if(ContextMenuStrings::QuantizationValues[StringKey] == currentQuantization) {
				ItemQuantization->Image = (cli::safe_cast<System::Drawing::Image^>(_Resources->GetObject(L"Tick_White")));
			} 

			SubMenuChangeQuantization->DropDownItems->Add(ItemQuantization);
		}

		this->_ContextMenu->Items->Add(SubMenuChangeQuantization);
	}

	ToolStripMenuItem^ Widget_Timeline::CreateContextMenuItem(String^ text, bool clickable, Image^ image)
	{
		ToolStripMenuItem^ Item = gcnew ToolStripMenuItem(text, image);

		if (clickable) {
			Item->Click += gcnew System::EventHandler(this, &Widget_Timeline::HandleContextMenuClick);
		}

		return Item;
	}

	void Widget_Timeline::HandleContextMenuClick(System::Object^ sender, EventArgs^ e)
	{
		if (_ContextMenuBar == nullptr) {
			return;
		}

	
		List<BarEvent^>^ TargetBars = gcnew List<BarEvent^>();
		bool IsSelected = _CurrentTool->SelectedBars->Contains(_ContextMenuBar);

		if (IsSelected) {
			TargetBars->AddRange(_CurrentTool->SelectedBars);
		}
		else {
			TargetBars->Add(_ContextMenuBar);
		}


		ToolStripMenuItem^ Sender = safe_cast<ToolStripMenuItem^>(sender);
		String^ ItemText = Sender->Text;

		if (ItemText == ContextMenuStrings::Copy)
		{
			TimelineClipboardManager::Content->Clear();
			int EarliestTick = Int32::MaxValue;

			// Find earliest tick among selected bars
			for each(BarEvent^ Bar in TargetBars) {
				EarliestTick = Math::Min(EarliestTick, Bar->StartTick);
			}

			for each(BarEvent^ Bar in TargetBars) {
				BarEvent^ CopiedBar = TimelineCommandManager::CreateBarCopy(Bar, Bar->StartTick - EarliestTick, false);
				TimelineClipboardManager::Content->Add(CopiedBar);
			}
		}
		else if (ItemText == ContextMenuStrings::Delete)
		{
			if (TargetBars->Count > 1) {
				CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Delete Multiple Bars");
				for each(BarEvent^ Bar in TargetBars) {
					DeleteBarCommand^ Cmd = gcnew DeleteBarCommand(this, Bar->ContainingTrack, Bar);
					CompoundCmd->AddCommand(Cmd);
				}
				CommandManager()->ExecuteCommand(CompoundCmd);
			}
			else {
				DeleteBarCommand^ Cmd = gcnew DeleteBarCommand(this, _ContextMenuBar->ContainingTrack, _ContextMenuBar);
				CommandManager()->ExecuteCommand(Cmd);
			}
		}
		else if (ItemText == ContextMenuStrings::FadeSwitchTwo)
		{
			if (TargetBars->Count > 1) {
				CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change Fade Type for Multiple Bars");

				for each(BarEvent ^ Bar in TargetBars) {
					if (Bar->Type != BarEventType::Fade || Bar->FadeInfo == nullptr) {
						continue;
					}

					ChangeFadeTypeCommand^ Cmd = gcnew ChangeFadeTypeCommand(this, Bar, FadeType::Two_Colors);
					CompoundCmd->AddCommand(Cmd);
				}

				CommandManager()->ExecuteCommand(CompoundCmd);
			}
			else {
				ChangeFadeTypeCommand^ Cmd = gcnew ChangeFadeTypeCommand(this, _ContextMenuBar, FadeType::Two_Colors);
				CommandManager()->ExecuteCommand(Cmd);
			}
		}
		else if (ItemText == ContextMenuStrings::FadeSwitchThree)
		{
			if (TargetBars->Count > 1) {
				CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change Fade Type for Multiple Bars");

				for each(BarEvent ^ Bar in TargetBars) {
					if (Bar->Type != BarEventType::Fade || Bar->FadeInfo == nullptr) {
						continue;
					}

					ChangeFadeTypeCommand^ Cmd = gcnew ChangeFadeTypeCommand(this, Bar, FadeType::Three_Colors);
					CompoundCmd->AddCommand(Cmd);
				}

				CommandManager()->ExecuteCommand(CompoundCmd);
			}
			else {
				ChangeFadeTypeCommand^ Cmd = gcnew ChangeFadeTypeCommand(this, _ContextMenuBar, FadeType::Three_Colors);
				CommandManager()->ExecuteCommand(Cmd);
			}
		}
		else if (ItemText == ContextMenuStrings::GetColor)
		{
			this->_Tool_And_Control->SetColorDirect(_ContextMenuBar->Color);
		}
		else if (ItemText == ContextMenuStrings::GetColorStart)
		{
			if(_ContextMenuBar->FadeInfo != nullptr) {
				this->_Tool_And_Control->SetColorDirect(_ContextMenuBar->FadeInfo->ColorStart);
			}
		}
		else if (ItemText == ContextMenuStrings::GetColorCenter)
		{
			if (_ContextMenuBar->FadeInfo != nullptr && _ContextMenuBar->FadeInfo->Type == FadeType::Three_Colors) {
				this->_Tool_And_Control->SetColorDirect(_ContextMenuBar->FadeInfo->ColorCenter);
			}
		}
		else if (ItemText == ContextMenuStrings::GetColorEnd)
		{
			if (_ContextMenuBar->FadeInfo != nullptr) {
				this->_Tool_And_Control->SetColorDirect(_ContextMenuBar->FadeInfo->ColorStart);
			}
		}
		else {
			// Drop Down Menu clicked
			ToolStripMenuItem^ ParentItem = safe_cast<ToolStripMenuItem^>(Sender->OwnerItem);
			String^ ParentItemText = ParentItem->Text;

			Color NewColor = Color();
			if (ParentItemText == ContextMenuStrings::ChangeColor ||
				ParentItemText == ContextMenuStrings::ChangeColorStart ||
				ParentItemText == ContextMenuStrings::ChangeColorCenter ||
				ParentItemText == ContextMenuStrings::ChangeColorEnd)
			{
				
				if (ItemText->StartsWith(ContextMenuStrings::PresetColor)) {
					int PresetIndex = Int32::Parse(ItemText->Substring(13)) - 1;
					NewColor = Settings::Get_Instance()->ColorPresetsColor[PresetIndex];
				} 
				else if (ItemText->StartsWith(ContextMenuStrings::CurrentColor)) {
					NewColor = this->_Tool_And_Control->GetColorPickerSelectedColor();
				}
			}

			if (ParentItemText == ContextMenuStrings::ChangeColor)
			{
				if (TargetBars->Count > 1) {
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change Color for Multiple Bars");
					
					for each(BarEvent ^ Bar in TargetBars) {
						if (Bar->Type == BarEventType::Fade) {
							continue;
						}
						
						ChangeBarColorCommand^ Cmd = gcnew ChangeBarColorCommand(this, Bar, Bar->Color, NewColor);
						CompoundCmd->AddCommand(Cmd);
					}
					CommandManager()->ExecuteCommand(CompoundCmd);
				}
				else {
					ChangeBarColorCommand^ Cmd = gcnew ChangeBarColorCommand(this, _ContextMenuBar, _ContextMenuBar->Color, NewColor);
					CommandManager()->ExecuteCommand(Cmd);
				}
			}
			
			else if (ParentItemText == ContextMenuStrings::ChangeColorStart)
			{
				ChangeFadeBarColorCommand::ColorType ColorType = ChangeFadeBarColorCommand::ColorType::Start;
				
				if (TargetBars->Count > 1) {
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change Start Color for Fade Bars");

					for each(BarEvent ^ Bar in TargetBars) {
						if (Bar->Type != BarEventType::Fade || Bar->FadeInfo == nullptr) {
							continue;
						}

						Color OldColor = Bar->FadeInfo->ColorStart;

						ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(this, Bar, ColorType, OldColor, NewColor);
						CompoundCmd->AddCommand(Cmd);
					}

					CommandManager()->ExecuteCommand(CompoundCmd);
				}
				else {
					Color OldColor = _ContextMenuBar->FadeInfo->ColorStart;

					ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(this, _ContextMenuBar, ColorType, OldColor, NewColor);
					CommandManager()->ExecuteCommand(Cmd);
				}
			}

			else if (ParentItemText == ContextMenuStrings::ChangeColorCenter)
			{
				ChangeFadeBarColorCommand::ColorType ColorType = ChangeFadeBarColorCommand::ColorType::Center;
				
				if (TargetBars->Count > 1) {
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change Center Color for Fade Bars");

					for each(BarEvent ^ Bar in TargetBars) {
						if (Bar->Type != BarEventType::Fade || Bar->FadeInfo == nullptr) {
							continue;
						}

						Color OldColor = Bar->FadeInfo->ColorCenter;

						ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(this, Bar, ColorType, OldColor, NewColor);
						CompoundCmd->AddCommand(Cmd);
					}

					CommandManager()->ExecuteCommand(CompoundCmd);
				}
				else {
					Color OldColor = _ContextMenuBar->FadeInfo->ColorCenter;

					ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(this, _ContextMenuBar, ColorType, OldColor, NewColor);
					CommandManager()->ExecuteCommand(Cmd);
				}
			}

			else if (ParentItemText == ContextMenuStrings::ChangeColorEnd)
			{
				ChangeFadeBarColorCommand::ColorType ColorType = ChangeFadeBarColorCommand::ColorType::End;
				
				if (TargetBars->Count > 1) {
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change End Color for Fade Bars");

					for each(BarEvent ^ Bar in TargetBars) {
						if (Bar->Type != BarEventType::Fade || Bar->FadeInfo == nullptr) {
							continue;
						}

						Color OldColor = Bar->FadeInfo->ColorEnd;

						ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(this, Bar, ColorType, OldColor, NewColor);
						CompoundCmd->AddCommand(Cmd);
					}

					CommandManager()->ExecuteCommand(CompoundCmd);
				}
				else {
					Color OldColor = _ContextMenuBar->FadeInfo->ColorEnd;

					ChangeFadeBarColorCommand^ Cmd = gcnew ChangeFadeBarColorCommand(this, _ContextMenuBar, ColorType, OldColor, NewColor);
					CommandManager()->ExecuteCommand(Cmd);
				}
			}

			else if (ParentItemText == ContextMenuStrings::ChangeEasingIn)
			{
				FadeEasing NewEasing = ContextMenuStrings::FadeEasings[ItemText];
				
				if (TargetBars->Count > 1) {
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change In Easing for Fade Bars");

					for each (BarEvent ^ Bar in TargetBars) {
						if (Bar->Type != BarEventType::Fade || Bar->FadeInfo == nullptr) {
							continue;
						}

						FadeEasing OldEasing = _ContextMenuBar->FadeInfo->EaseIn;

						ChangeFadeEasingCommand^ Cmd = gcnew ChangeFadeEasingCommand(this, Bar, ChangeFadeEasingCommand::EasingType::InEasing, OldEasing, NewEasing);
						CompoundCmd->AddCommand(Cmd);
					}

					CommandManager()->ExecuteCommand(CompoundCmd);
				}
				else {
					FadeEasing OldEasing = _ContextMenuBar->FadeInfo->EaseIn;

					ChangeFadeEasingCommand^ Cmd = gcnew ChangeFadeEasingCommand(this, _ContextMenuBar, ChangeFadeEasingCommand::EasingType::InEasing, OldEasing, NewEasing);
					CommandManager()->ExecuteCommand(Cmd);
				}
			}

			else if (ParentItemText == ContextMenuStrings::ChangeEasingOut)
			{
				FadeEasing NewEasing = ContextMenuStrings::FadeEasings[ItemText];

				if (TargetBars->Count > 1) {
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("Change Out Easing for Fade Bars");

					for each (BarEvent ^ Bar in TargetBars) {
						if (Bar->Type != BarEventType::Fade || Bar->FadeInfo == nullptr) {
							continue;
						}

						FadeEasing OldEasing = _ContextMenuBar->FadeInfo->EaseOut;

						ChangeFadeEasingCommand^ Cmd = gcnew ChangeFadeEasingCommand(this, Bar, ChangeFadeEasingCommand::EasingType::OutEasing, OldEasing, NewEasing);
						CompoundCmd->AddCommand(Cmd);
					}

					CommandManager()->ExecuteCommand(CompoundCmd);
				}
				else {
					FadeEasing OldEasing = _ContextMenuBar->FadeInfo->EaseOut;

					ChangeFadeEasingCommand^ Cmd = gcnew ChangeFadeEasingCommand(this, _ContextMenuBar, ChangeFadeEasingCommand::EasingType::OutEasing, OldEasing, NewEasing);
					CommandManager()->ExecuteCommand(Cmd);
				}
			}

			else if (ParentItemText == ContextMenuStrings::ChangeQuantization)
			{
				int NewTickQuantization = ContextMenuStrings::QuantizationValues[ItemText];

				if (TargetBars->Count > 1) {
					CompoundCommand^ CompoundCmd = gcnew CompoundCommand("ChangeQuantization for Multiple Bars");

					for each(BarEvent ^ Bar in TargetBars) {
						if (Bar->Type != BarEventType::Fade && Bar->Type != BarEventType::Strobe) {
							continue;
						}

						if (Bar->FadeInfo == nullptr && Bar->StrobeInfo == nullptr) {
							continue;
						}

						if (Bar->Type == BarEventType::Fade) {
							ChangeFadeQuantizationCommand^ Cmd = gcnew ChangeFadeQuantizationCommand(this, Bar, Bar->FadeInfo->QuantizationTicks, NewTickQuantization);
							CompoundCmd->AddCommand(Cmd);
						}
						else if (Bar->Type == BarEventType::Strobe) {
							ChangeStrobeQuantizationCommand^ Cmd = gcnew ChangeStrobeQuantizationCommand(this, Bar, Bar->StrobeInfo->QuantizationTicks, NewTickQuantization);
							CompoundCmd->AddCommand(Cmd);
						}
					}

					CommandManager()->ExecuteCommand(CompoundCmd);
				}
				else {
					if (_ContextMenuBar->Type == BarEventType::Fade) {
						ChangeFadeQuantizationCommand^ Cmd = gcnew ChangeFadeQuantizationCommand(this, _ContextMenuBar, _ContextMenuBar->FadeInfo->QuantizationTicks, NewTickQuantization);
						CommandManager()->ExecuteCommand(Cmd);
					}
					else if (_ContextMenuBar->Type == BarEventType::Strobe) {
						ChangeStrobeQuantizationCommand^ Cmd = gcnew ChangeStrobeQuantizationCommand(this, _ContextMenuBar, _ContextMenuBar->StrobeInfo->QuantizationTicks, NewTickQuantization);
						CommandManager()->ExecuteCommand(Cmd);
					}
				}
			}
		}

		_ContextMenuBar = nullptr; // Clear the reference
	}

	float Widget_Timeline::GetSubdivisionLevel()
	{
		if (_D2DRenderer == nullptr) {
			return 1;
		}
		
		return _D2DRenderer->GetSubdivisionLevel((float)TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER));
	}

	int Widget_Timeline::GetTrackTop(Track^ track)
	{
		if (this->_D2DRenderer == nullptr) {
			return 0;
		}
		
		return this->_D2DRenderer->GetTrackTop(track);
	}

	int Widget_Timeline::GetTotalTracksHeight()
	{
		int totalHeight = 0;
		for each (Track ^ track in _Tracks)
		{
			totalHeight += track->Height;
		}
		return totalHeight;
	}

	void Widget_Timeline::BeginTrackResize(Track^ track, int mouseY)
	{
		_TrackBeingResized	= track;
		_ResizeStartY		= mouseY;
		_InitialTrackHeight	= track->Height;

		// Change cursor to resize cursor
		this->Cursor = Cursors::SizeNS;
	}

	void Widget_Timeline::UpdateTrackResize(int mouseY)
	{
		if (_TrackBeingResized != nullptr)
		{
			int Delta = mouseY - _ResizeStartY;
			int NewHeight = Math::Max(MINIMUM_TRACK_HEIGHT, _InitialTrackHeight + Delta);

			// Store previous total height
			int OldTotalHeight = GetTotalTracksHeight();

			// Update the track height
			SetTrackHeight(_TrackBeingResized, NewHeight);

			// Get new total height
			int NewTotalHeight = GetTotalTracksHeight();

			// If this changes total content height, ensure scroll position is still valid
			if (OldTotalHeight != NewTotalHeight) {
				int ViewportHeight = Height - Timeline_Direct2DRenderer::HEADER_HEIGHT - _HScrollBar->Height;

				// If we're scrolled to bottom, maintain bottom alignment
				if (-_ScrollPosition->Y + ViewportHeight >= OldTotalHeight) {
					_ScrollPosition->Y = Math::Min(0, -(NewTotalHeight - ViewportHeight));
					if (_VScrollBar != nullptr) {
						_VScrollBar->Value = -_ScrollPosition->Y;
					}
				}

				// Update scrollbar range
				UpdateVerticalScrollBarRange();
			}

			Invalidate();
		}
	}

	void Widget_Timeline::EndTrackResize()
	{
		_TrackBeingResized = nullptr;

		// Check if we're still over a divider
		Track^ HoverTrack = nullptr;
		if (IsOverTrackDivider(Point(this->PointToClient(Control::MousePosition)), HoverTrack)) {
			this->Cursor = Cursors::SizeNS;
		}
		else {
			this->Cursor = Cursors::Default;
		}
	}

	void Widget_Timeline::BeginPanelResize(int mouseX)
	{
		_IsPanelResizing = true;
		_PanelResizeStartX = mouseX;
		_InitialPanelWidth = this->_Left_Panel->Width;

		// Change cursor to resize cursor
		this->Cursor = Cursors::SizeWE;
	}

	void Widget_Timeline::UpdatePanelResize(int mouseX)
	{
		if (_IsPanelResizing)
		{
			int DeltaX = mouseX - _PanelResizeStartX;
			int NewWidth = _InitialPanelWidth + DeltaX;

			// Update the panel width
			this->_Left_Panel->Width = NewWidth;
			Invalidate();
		}
	}

	void Widget_Timeline::EndPanelResize()
	{
		_IsPanelResizing = false;

		// Update cursor based on current position
		if (IsOverPanelResizeHandle(this->PointToClient(Control::MousePosition))) {
			this->Cursor = Cursors::SizeWE;
		}
		else {
			this->Cursor = Cursors::Default;
		}
	}

	void Widget_Timeline::ToggleLeftPanel()
	{
		this->_Left_Panel->ToggleExpanded();
		Invalidate();
	}

	bool Widget_Timeline::IsOverTrackDivider(Point mousePoint, Track^% outTrack)
	{
		if (this->_Measures->Count == 0) {
			return false;
		}
		
		// Adjust mouse position for scroll
		int AdjustedY = mousePoint.Y - _ScrollPosition->Y;
		outTrack = nullptr;

		// Start from header height
		int Y = Timeline_Direct2DRenderer::HEADER_HEIGHT;

		// Check each track except the last one (no divider after last track)
		for (int i = 0; i < _Tracks->Count; i++)
		{
			Track^ Trk = _Tracks[i];
			int Height = Trk->Height;
			int DividerY = Y + Height;

			// Check if mouse is within the divider area (using adjusted Y position)
			if (AdjustedY >= DividerY - TRACK_RESIZE_HANDLE_HEIGHT && AdjustedY <= DividerY + TRACK_RESIZE_HANDLE_HEIGHT)
			{
				outTrack = Trk;
				return true;
			}

			Y += Height;
		}

		return false;
	}

	bool Widget_Timeline::IsOverTrackButton(Track^ track, int buttonIndex, Point mousePoint)
	{
		if (track == nullptr || this->_Measures->Count == 0) {
			return false;
		}

		if (buttonIndex == 1) {
			// Notation button only exists for drum tracks
			if (!track->IsDrumTrack) return false;
		}

		Rectangle HeaderBounds = GetTrackHeaderBounds(track);
		HeaderBounds.Y += _ScrollPosition->Y;

		Rectangle ButtonBounds = GetTrackButtonBounds(HeaderBounds, buttonIndex);
		return ButtonBounds.Contains(mousePoint);
	}

	bool Widget_Timeline::IsOverPanelResizeHandle(Point mousePoint)
	{
		if (this->_Left_Panel->IsExpanded == false) {
			return false;
		}

		// Check if point is within resize handle area
		return (mousePoint.X >= this->_Left_Panel->Width - Timeline_Direct2DRenderer::PANEL_RESIZE_HANDLE_WIDTH &&
				mousePoint.X <= this->_Left_Panel->Width + Timeline_Direct2DRenderer::PANEL_RESIZE_HANDLE_WIDTH && 
				mousePoint.Y > Timeline_Direct2DRenderer::HEADER_HEIGHT);
	}

	bool Widget_Timeline::IsOverPanelToggleButton(Point mousePoint)
	{
		if (mousePoint.X >= Timeline_Direct2DRenderer::PANEL_BUTTON_MARGIN &&
			mousePoint.X <= Timeline_Direct2DRenderer::PANEL_BUTTON_MARGIN + Timeline_Direct2DRenderer::PANEL_BUTTON_SIZE &&
			mousePoint.Y >= Timeline_Direct2DRenderer::PANEL_BUTTON_MARGIN &&
			mousePoint.Y <= Timeline_Direct2DRenderer::PANEL_BUTTON_MARGIN + Timeline_Direct2DRenderer::PANEL_BUTTON_SIZE)
		{
			return true;
		}
		
		return false;
	}

	Rectangle Widget_Timeline::GetTrackButtonBounds(Rectangle headerBounds, int buttonIndex)
	{
		return Rectangle(
			headerBounds.Right - (Timeline_Direct2DRenderer::BUTTON_SIZE + Timeline_Direct2DRenderer::BUTTON_MARGIN) * (buttonIndex + 1),
			headerBounds.Y + Timeline_Direct2DRenderer::BUTTON_MARGIN,
			Timeline_Direct2DRenderer::BUTTON_SIZE,
			Timeline_Direct2DRenderer::BUTTON_SIZE
		);
	}

	Rectangle Widget_Timeline::CalculateBarBounds(BarEvent^ bar, Rectangle bounds)
	{
		if (_D2DRenderer == nullptr) {
			return bounds;
		}

		return _D2DRenderer->GetBarBounds(bar, bounds);
	}

	void Widget_Timeline::RecalculateMeasurePositions()
	{
		int currentTick = 0;
		for each (Measure ^ measure in _Measures) {
			measure->StartTick = currentTick;
			currentTick += measure->Length;
		}
	}

	void Widget_Timeline::UpdateDrawingForMeasures()
	{
		RecalculateMeasurePositions();
		// Update any visual elements that depend on measure positions
	}

	double Widget_Timeline::GetRelativePositionInMeasure(int tick)
	{
		Measure^ measure = GetMeasureAtTick(tick);
		if (measure == nullptr) return 0.0;

		int measureStartTick = 0;
		for each (Measure ^ m in _Measures) {
			if (m == measure) break;
			measureStartTick += m->Length;
		}

		return (double)(tick - measureStartTick) / measure->Length;
	}

	double Widget_Timeline::GetVirtualWidth()
	{
		// Calculate total width based on musical content
		double tickScale = 16.0 / Timeline_Direct2DRenderer::TICKS_PER_QUARTER;
		return (double)TotalTicks * tickScale * _ZoomLevel;
	}

	void Widget_Timeline::SetZoomLevelAtPoint(double newZoom, Point referencePoint)
	{
		// Clamp zoom level to valid range
		newZoom = Math::Min(Math::Max(newZoom, Timeline_Direct2DRenderer::MIN_ZOOM_LEVEL), Timeline_Direct2DRenderer::MAX_ZOOM_LEVEL);

		// If no change in zoom, exit early
		if (Math::Abs(newZoom - _ZoomLevel) < 0.0001) {
			return;
		}

		// Get position relative to content area
		Point ContentPos = Point(referencePoint.X - GetLeftPanelAndTrackHeaderWidth(), referencePoint.Y);

		// Find tick at reference point
		int TickAtPosition = PixelsToTicks(ContentPos.X - _ScrollPosition->X);

		// Store old zoom level and apply new zoom
		_ZoomLevel = newZoom;

		if (_D2DRenderer != nullptr) {
			_D2DRenderer->SetZoomLevel(_ZoomLevel);
		}

		// Maintain position of reference point
		int NewPixelPosition = TicksToPixels(TickAtPosition);
		int PositionOffset = referencePoint.X - GetLeftPanelAndTrackHeaderWidth();
		_ScrollPosition->X = -(NewPixelPosition - PositionOffset);

		// Ensure proper alignment
		UpdateScrollBounds();
		Invalidate();
	}

	void Widget_Timeline::UpdateScrollBarRange()
	{
		// Calculate virtual width
		double VirtualWidth = GetVirtualWidth();

		// Calculate viewport width (excluding header)
		int ViewportWidth = Width - GetLeftPanelAndTrackHeaderWidth();

		// Convert to scroll units
		int TotalUnits = GetScrollUnits(VirtualWidth);
		int ViewportUnits = GetScrollUnits(ViewportWidth);

		// Ensure viewportUnits is at least 1
		ViewportUnits = Math::Max(1, ViewportUnits);

		// Update scrollbar
		_HScrollBar->Minimum		= 0;
		_HScrollBar->Maximum		= Math::Max(0, TotalUnits);
		_HScrollBar->LargeChange	= ViewportUnits;
		_HScrollBar->SmallChange	= 1;

		// Ensure current position is valid
		int CurrentUnit = GetScrollUnits(-_ScrollPosition->X);
		_HScrollBar->Value = Math::Min(Math::Max(CurrentUnit, _HScrollBar->Minimum), _HScrollBar->Maximum - _HScrollBar->LargeChange + 1);
	}

	int Widget_Timeline::GetScrollUnits(double width)
	{
		// Convert pixel width to scroll units
		return (int)Math::Ceiling(width / SCROLL_UNIT);
	}

	void Widget_Timeline::UpdateScrollBounds()
	{
		// Calculate total width in pixels
		double VirtualWidth = GetVirtualWidth();
		int ViewportWidth = Width - GetLeftPanelAndTrackHeaderWidth();

		// Calculate maximum scroll position
		double MaxScroll;
		if (VirtualWidth > ViewportWidth) {
			MaxScroll = -(VirtualWidth - ViewportWidth);
		}
		else {
			MaxScroll = 0;
		}

		// Calculate grid alignment based on zoom level
		int GridPixels;
		if (_ZoomLevel > 50.0) {
			GridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 64); // Even finer grid at very high zoom
		}
		else if (_ZoomLevel > 20.0) {
			GridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 32);
		}
		else if (_ZoomLevel > 10.0) {
			GridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 16);
		}
		else if (_ZoomLevel > 5.0) {
			GridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 8);
		}
		else {
			GridPixels = TicksToPixels(Timeline_Direct2DRenderer::TICKS_PER_QUARTER / 4);
		}

		if (GridPixels > 0) {
			// Snap scroll position to grid
			_ScrollPosition->X = (int)Math::Round((double)_ScrollPosition->X / GridPixels) * GridPixels;
		}

		// Clamp scroll position
		_ScrollPosition->X = (int)Math::Round(Math::Max(MaxScroll, Math::Min(0.0, (double)_ScrollPosition->X)));

		// Update scrollbar to reflect new position/range
		UpdateScrollBarRange();
	}
	
	void Widget_Timeline::UpdateVerticalScrollBarRange()
	{
		int TotalHeight = GetTotalTracksHeight();
		// Ensure viewportHeight is at least 0
		int ViewportHeight = Math::Max(0, Height - Timeline_Direct2DRenderer::HEADER_HEIGHT - _HScrollBar->Height);

		// Enable/update scrollbar if content exceeds viewport
		if (TotalHeight > ViewportHeight)
		{
			_VScrollBar->Minimum = 0;
			_VScrollBar->Maximum = TotalHeight + 20;  // Use full total height as Maximum plus a little extra
			// Ensure LargeChange is at least 1
			_VScrollBar->LargeChange = Math::Max(1, ViewportHeight);
			_VScrollBar->SmallChange = Math::Max(1, ViewportHeight / 20); // Adjust scroll speed

			// Ensure value stays within valid range
			int maxValue = Math::Max(0, TotalHeight - ViewportHeight);
			int currentValue = Math::Min(-_ScrollPosition->Y, maxValue);
			_VScrollBar->Value = Math::Max(0, currentValue);
		}
		else
		{
			// Content fits in viewport, but keep scrollbar visible
			_VScrollBar->Minimum = 0;
			_VScrollBar->Maximum = 0;
			_VScrollBar->LargeChange = 1;
			_VScrollBar->Value = 0;
			_ScrollPosition->Y = 0;
		}

		_VScrollBar->Visible = true;
	}
	
	void Widget_Timeline::OnScroll(Object^ sender, ScrollEventArgs^ e)
	{
		// Convert scroll units to pixels
		double NewScrollX = -(double)e->NewValue * SCROLL_UNIT;

		// Update scroll position
		_ScrollPosition->X = (int)Math::Round(NewScrollX);

		// Request redraw
		Invalidate();
	}

	void Widget_Timeline::OnVerticalScroll(Object^ sender, ScrollEventArgs^ e)
	{
		_ScrollPosition->Y = -e->NewValue;
		Invalidate();
	}

	////////////////
	// Properties //
	////////////////
	ThemeColors Widget_Timeline::Theme::get()
	{
		return _CurrentTheme;
	}

	void Widget_Timeline::Theme::set(ThemeColors value)
	{
		_CurrentTheme = value;
	}

	List<Track^>^ Widget_Timeline::Tracks:: get()
	{
		return _Tracks;
	}
	
	List<Measure^>^ Widget_Timeline::Measures::get()
	{
		return _Measures;
	}

	int Widget_Timeline::TotalTicks::get()
	{
		int TotalTicks = 0;

		for each(Measure^ M in _Measures)
		{
			TotalTicks += M->Length;
		}

		return TotalTicks;
	}

	TimelineToolType Widget_Timeline::CurrentTool::get()
	{ 
		return _CurrentToolType; 
	}

	void Widget_Timeline::CurrentTool::set(TimelineToolType value) 
	{ 
		SetCurrentTool(value); 
	}

	Point^ Widget_Timeline::ScrollPosition::get() 
	{ 
		return _ScrollPosition;
	}
}